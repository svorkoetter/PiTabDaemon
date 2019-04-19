/* PiTabDaemon

   Monitors battery, power switch, push buttons, and dashboard requests
   for Stefan Vorkoetter's Raspberry Pi tablet. */

/* Copyright (c) 2017 by Stefan Vorkoetter

   This file is part of PiTabDaemon.

   PiTabDaemon is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   PiTabDaemon is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PiTabDaemon. If not, see <http://www.gnu.org/licenses/>. */

#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <errno.h>
#include <math.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "battery.h"
#include "display.h"
#include "idle.h"
#include "io.h"
#include "logging.h"

/* RAM disk file used by the daemon to send status to the dashboard. */
#define DAT_FILE	"/ram/pitabd.dat"

/* Disk file where daemon's process ID is recorded so it can be killed. */
#define PID_FILE	"/var/run/pitabd.pid"

/* RAM disk file used by the dashboard to send settings to the daemon. On
   shutdown it is saved to the real disk (SD card) so the settings persist. */
#define CMD_FILE	"/ram/pitabd.cmd"
#define CMD_SAVE_FILE	"/var/tmp/pitabd.cmd"

/* Time in ms that LBO must persist before a forced shutdown. */
#define LBO_TO_SHUTDOWN	60000

/* Idle time in ms before display is dimmed, and then additional time until
   the backlight is turned off completely. */
#define IDLE_TO_DIM	120000
#define DIM_TO_DARK	180000
#define IDLE_RECOVERY	500

/* Command line options (in the form expected by getopt). */
#define OPTIONS		"bkn"

static void usage( void )
{
    /* Print usage information and exit. */
    fprintf(stderr,"usage: pitabd [-%s]\n",OPTIONS);
    fprintf(stderr,"-b\tlog detailed battery usage\n");
    fprintf(stderr,"-k\tkill running pitabd and then exit\n");
    fprintf(stderr,"-n\tdo not become a daemon, remain in foreground\n");
    exit(1);
}

int main( int argc, char **argv )
{
    /* Process command line options. */
    bool optLogBattery = false, optKillOnly = false, optDaemonize = true;
    int c;
    while( (c = getopt(argc,argv,OPTIONS)) != -1 ) {
	switch( c ) {
	case 'b':
	    optLogBattery = true;
	    break;
	case 'k':
	    optKillOnly = true;
	    break;
	case 'n':
	    optDaemonize = false;
	    break;
	default:
	    usage();
	}
    }
    if( optind < argc )
        usage();

    /* If there's an existing instance running, terminate it. */
    FILE *fp = fopen(PID_FILE,"r");
    if( fp != NULL ) {
	pid_t pid;
	if( fscanf(fp,"%d",&pid) == 1 )
	    kill(pid,SIGINT);
        fclose(fp);
	unlink(PID_FILE);
	WriteToLogArgI("killed %d",pid);
    }
    
    /* Nothing left to do if -k was specified. */
    if( optKillOnly )
        return( 0 );

    /* Initialize GPIO ports. */
    if( !InitGPIO() ) {
	fprintf(stderr,"pitabd: failed to initialize GPIO\n");
	return( 1 );
    }

    /* Do what it takes to become a daemon. */
    if( optDaemonize && daemon(0,0) != 0 ) {
	perror("pitabd: failed to become a daemon");
	return( 1 );
    }

    /* Record our process id so we know which process to kill if reinvoked. */
    if( (fp = fopen(PID_FILE,"w")) == NULL ) {
        fprintf(stderr,"pitabd: unable to record process ID\n");
	return( 1 );
    }
    fprintf(fp,"%d\n",getpid());
    fclose(fp);

    /* Rotate the log files and start a fresh one. */
    RotateLogs();
    WriteToLogArgI("starting with pid=%d",getpid());

    /* Copy the saved command file to the RAM disk if it's not already there,
       so the dashboard can find its settings. */
    int brightnessIndex = -1;
    if( (fp = fopen(CMD_FILE,"r")) != NULL )
        fclose(fp);
    else if( (fp = fopen(CMD_SAVE_FILE,"r")) != NULL ) {
	FILE *fp2 = fopen(CMD_FILE,"w");
	if( fp2 != NULL ) {
	    while( (c = fgetc(fp)) != -1 ) {
		if( 'A' <= c && c <= 'I' ) {
		    /* If we find a letter from A to I, it's the saved
		       brightness, and we can stop copying the file, since
		       the dashboard doesn't need any more information. */
		    brightnessIndex = c - 'A';
		    break;
		}
	        fputc(c,fp2);
	    }
	    fclose(fp2);
	}
	fclose(fp);
	chmod(CMD_FILE,0666);
    }

    /* Set initial display brightness, but never to zero, to avoid scares. */
    InitBrightness(brightnessIndex + !brightnessIndex);

    /* Keep track of how long we've had a consistent low battery warning and
       shut down when it's been long enough. */
    int cyclesSinceLBO = 0;

    /* Initialize previous state of each monitored quantity. */
    bool charging = false, completed = false;
    double lastVoltage = -1, lastEnergy = -1;

    /* Variables to keep track of idle time while minimizing X11 calls to
       check the idle time. */
    enum { ACTIVE = 0, DIM, DARK } displayState = ACTIVE;
    int nextIdleCheck = IDLE_TO_DIM;

    /* Cycle number after which a button press is considered a long press. */
    int button1LongPress = 0, button2LongPress = 0, button3LongPress = 0;

    /* Current state of USB/Ethernet/Bluetooth, Wi-Fi, and idle dimming. */
    bool usbOn = true, wifiOn = true, allowDim = true;
    
    /* Loop forever, keeping track of how many cycles have taken place. */
    for( int cycle = 0;; ++cycle ) {

	/* Shut down if the power switch is turned off. */
	if( GetInput(SWITCH_ON) == -1 ) {
	    WriteToLog("shutdown initiated");
	    break;
	}

	/* Button 1 brings either the on-screen keyboard (short press) or the
	   dashboard (long press) to the front. */
	bool endIdle = false;
	c = GetInput(BUTTON_1);
	if( c == 1 ) {
	    button1LongPress = cycle + 500;
	    endIdle = true;
	}
	else if( c == -1 ) {
	    /* Ensure the application isn't in fullscreen mode, otherwise
	       nothing can be displayed on top of it. */
	    system("wmctrl -r :ACTIVE: -b remove,fullscreen");
	    if( cycle > button1LongPress )
		system("wmctrl -a \"%\"");
	    else
		system("wmctrl -a \"xvkbd\"");
	}

	/* Button 2 cycles through the preprogrammed brightness levels (short
	   press) or jumps directly to maximum brightness (long press). */
	c = GetInput(BUTTON_2);
	if( c == 1 ) {
	    button2LongPress = cycle + 500;
	    endIdle = true;
	}
	else if( c == -1 ) {
	    if( cycle > button2LongPress )
	        MaxBrightness();
	    else
		NextBrightness();
	}

	/* Button 3 toggles maximized (short press) or fullscreen (long press)
	   mode on the foreground application. */
	c = GetInput(BUTTON_3);
	if( c == 1 ) {
	    button3LongPress = cycle + 500;
	    endIdle = true;
	}
	else if( c == -1 ) {
	    if( cycle > button3LongPress )
		system("wmctrl -r :ACTIVE: -b toggle,fullscreen");
	    else {
		/* Remove fullscreen before toggling maximization, or nothing
		   will happen. */
		system("wmctrl -r :ACTIVE: -b remove,fullscreen");
		system("wmctrl -r :ACTIVE: -b toggle,maximized_vert,maximized_horz");
	    }
	}

	/* If the display is currently dimmed or blank, pressing any button
	   will restore it. Button presses also reset the idle timer. */
	if( endIdle ) {
	    if( displayState != ACTIVE ) {
		RestoreDisplay();
		displayState = ACTIVE;
	    }
	    nextIdleCheck = cycle + IDLE_TO_DIM;
	}
	
	/* Look for commands from the dashboard every 5 seconds. */
	if( cycle % 5000 == 0 && (fp = fopen(CMD_FILE,"r")) != NULL ) {

	    /* Read the RAM disk command file that the dashboard writes to. */
	    int wantDim = 1, wantUSB = 1, wantWifi = 1;
	    int nScanned = fscanf(fp,"%d %d %d",&wantDim,&wantUSB,&wantWifi);
	    fclose(fp);

	    /* Act on the commands only if the read was successful. */
	    if( nScanned == 3 ) {

		/* Remember whether we want to allow dimming or not. */
		allowDim = wantDim;
		
		/* Turn USB, including wired Ethernet and Bluetooth, on or off.
		   Bluetooth is included only because we replaced the flakey
		   built-in one with a hard-wired USB dongle. */
		if( usbOn && !wantUSB ) {
		    /* Turn off USB, wired Ethernet, and Bluetooth. */
		    if( (fp = fopen("/sys/devices/platform/soc/3f980000.usb/buspower","w")) != NULL ) {
			fprintf(fp,"0\n");
			fclose(fp);
			/* Workaround for bug that lxpanel goes to 100% CPU,
			   because the USB sound card goes away. Doesn't happen
			   if the default audio is the built-in audio. */
			system("/usr/bin/lxpanelctl restart");
			WriteToLog("disabled USB and Bluetooth");
			usbOn = false;
		    }
		}
		else if( !usbOn && wantUSB ) {
		    /* Turn on USB, wired Ethernet, and Bluetooth. */
		    if( (fp = fopen("/sys/devices/platform/soc/3f980000.usb/buspower","w")) != NULL ) {
			fprintf(fp,"1\n");
			fclose(fp);
			WriteToLog("enabled USB and Bluetooth");
			usbOn = true;
		    }
		}

		/* Turn Wi-Fi on or off. */
		// TODO - make these two separate options
		// TODO - make sure the wifi is actually turned off
		//        iwconfig wlan0 txpower off
		// To turn it back on, do this twice:
		//        iwconfig wlan0 txpower auto
		if( wifiOn && !wantWifi  ) {
		    /* Turn off Wi-Fi. */
		    system("/sbin/iwconfig wlan0 txpower off");
		    WriteToLog("disabled wifi");
		    wifiOn = false;
		}
		else if( !wifiOn && wantWifi ) {
		    /* Turn on Wi-Fi. */
		    system("/sbin/iwconfig wlan0 txpower auto");
		    /* Yes, we have to do this twice. Do we need a delay? */
		    system("/sbin/iwconfig wlan0 txpower auto");
		    WriteToLog("enabled wifi");
		    wifiOn = true;
		}
	    }
	}

	/* Monitor changes to the two charging LEDs (charging and completed).
	   If either one is lit, then the charger must be connected. */
	bool pluggedIn = charging || completed;
	bool changed = false;

	/* Check status of charging LED. */
	c = GetInput(CHARGING);
	if( c == 1 ) {
	    charging = true;
	    changed = true;
	}
	else if( c == -1 ) {
	    charging = false;
	    changed = true;
	}

	/* Check and record status of charge-completed LED. */
	c = GetInput(CHARGED);
	if( c == 1 ) {
	    WriteToLog("charging completed");
	    completed = true;
	    changed = true;
	}
	else if( c == -1 ) {
	    completed = false;
	    changed = true;
	}

	/* Record changes in charger-connected status. */
	if( pluggedIn && !(charging || completed) ) {
	    WriteToLog("charger disconnected");
	    /* Ensure display doesn't dim immediately after unplugging. */
	    nextIdleCheck = cycle + IDLE_TO_DIM;
	    pluggedIn = false;
	}
	else if( !pluggedIn && (charging || completed) ) {
	    WriteToLog("charger connected");
	    pluggedIn = true;
	}

	/* Read battery state. This will be inaccurate until BATTERY_SAMPLES
	   cycles have been completed. */
	double r = GetBatteryRaw();
	double v = round(BatteryRawToVoltage(r) * 100.0) / 100.0;
	double e = round(BatteryRawToEnergyRemaining(r,pluggedIn));

	/* Don't do anything that relies on battery readings until the battery
	   monitor has collected enough samples for an accurate reading. */
	if( cycle >= BATTERY_SAMPLES ) {

	    /* If the rounded voltage has increased while charging, decreased
	       while discharging, or changed by more than 10mV, update it. */
	    if( charging && v > lastVoltage || !charging && v < lastVoltage
	     || fabs(v - lastVoltage) > 0.0101 )
	    {
		if( optLogBattery )
		    WriteToLogArgF("battery voltage %1.2fV",v);
		lastVoltage = v;
		changed = true;
	    }

	    /* If the rounded energy remaining has increased while charging,
	       decreased while discharging, or changed by more than 1%, update
	       it. */
	    if( charging && e > lastEnergy || !charging && e < lastEnergy
	     || fabs(e - lastEnergy) > 1.01 )
	    {
		if( optLogBattery )
		    WriteToLogArgF("energy remaining %1.0f%%",e);
		lastEnergy = e;
		changed = true;
	    }

	    /* Log the raw battery reading once per minute when stable. */
	    if( optLogBattery && cycle % 60000 == BATTERY_SAMPLES )
	        WriteToLogArgF("raw battery %1.3f",r);
	}

	/* After two minutes of inactivity while running on batteries, dim the
	   screen. After three additional minutes, turn off the backlight. This
	   can be overridden by a no-dim command from the dashboard. */
	if( cycle > nextIdleCheck && (displayState != ACTIVE || !pluggedIn) ) {
	    int i = IdleTime();
	    switch( displayState ) {
	    case ACTIVE:
		if( i > IDLE_TO_DIM && allowDim ) {
		    DimDisplay();
		    displayState = DIM;
		    nextIdleCheck = cycle + IDLE_RECOVERY;
		}
		else
		    nextIdleCheck = cycle + IDLE_TO_DIM - i;
		break;
	    case DIM:
	        if( i < IDLE_TO_DIM || !allowDim || pluggedIn ) {
		    RestoreDisplay();
		    displayState = ACTIVE;
		    nextIdleCheck = cycle + IDLE_TO_DIM - i;
		}
		else if( i > IDLE_TO_DIM + DIM_TO_DARK && allowDim ) {
		    DarkenDisplay();
		    /* Bring dashboard to front so there's somewhere safe to
		       tap. */
		    system("wmctrl -r :ACTIVE: -b remove,fullscreen");
		    system("wmctrl -a \"%\"");
		    displayState = DARK;
		    nextIdleCheck = cycle + IDLE_RECOVERY;
		}
		else
		    nextIdleCheck = cycle + IDLE_RECOVERY;
		break;
	    case DARK:
	        if( i < IDLE_TO_DIM || !allowDim || pluggedIn ) {
		    RestoreDisplay();
		    displayState = ACTIVE;
		    nextIdleCheck = cycle + IDLE_TO_DIM - i;
		}
		else
		    nextIdleCheck = cycle + IDLE_RECOVERY;
		break;
	    }
	    if( !allowDim )
		nextIdleCheck = cycle + IDLE_TO_DIM;
	}

	/* When the low battery input becomes active, start a counter. If it
	   ever becomes inactive, stop and reset the counter. If the counter
	   reaches the specified limit with a consistent low battery signal,
	   shut down the system. */
	int lbo = GetInput(LOW_BATT);
	if( lbo == 1 )
	    cyclesSinceLBO = 1;
	else if( lbo == -1 )
	    cyclesSinceLBO = 0;
	else if( cyclesSinceLBO > 0 && ++cyclesSinceLBO >= LBO_TO_SHUTDOWN ) {
	    WriteToLogArgF("low battery at %1.2fV",v);
	    break;
	}

	/* Move the display brightness towards the desired brightness by about
	   5% every 16 milliseconds (off to full in about 1 second). */
	if( cycle % 16 == 0 )
	    NudgeBrightness();

	/* If anything changed that we want to tell the user about, update the
	   RAM disk file monitored by the dashboard. */
	if( changed && (fp = fopen(DAT_FILE,"w")) != NULL ) {
	    fprintf(fp,"%4.2f %2.0f %1d %1d\n",v,e,charging,completed);
	    fclose(fp);
	}

	/* Sleep for approximately 1ms between scans (sleep time is tweaked
	   slightly to allow for overhead). */
	usleep(927);
    }

    /* Copy the command file back to its persistent location for next time. */
    if( (fp = fopen(CMD_FILE,"r")) != NULL ) {
	FILE *fp2 = fopen(CMD_SAVE_FILE,"w");
	if( fp2 != NULL ) {
	    while( (c = fgetc(fp)) != -1 )
	        fputc(c,fp2);
	    /* Save current display brightness too. */
	    fprintf(fp2,"%c\n",'A'+GetBrightnessIndex());
	    fprintf(fp2,"Do not edit this file!\n");
	    fclose(fp2);
	}
	fclose(fp);
    }

    /* Perform an orderly shutdown. */
    unlink(PID_FILE);
    // system("/usr/bin/aplay /usr/local/share/pitabd/shutdown.wav");
    system("/sbin/shutdown now");

    return( 0 );
}
