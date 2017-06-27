/* PiTabDaemon - Button and Monitoring I/O */

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

#include <stdbool.h>
#include <bcm2835.h>
#include <math.h>

#include "io.h"

/* Debounced Inputs

   The ports for monitoring the power switch, user buttons, and status from the
   PowerBoost 1000C are all treated as buttons, and are debounced to avoid
   spurious triggering of actions. The debouncing is accomplished by shifting
   the (possibly inverted) raw state of an input into the low order bit of a
   word, and when enough consecutive ones are seen, the input becomes active.
   Likewise, when same number of consecutive zeroes are seen, the input becomes
   inactive. The degree of debouncing can be specified separately for each
   input. Since the inputs are scanned by the main program about once per
   millisecond, each bit in the mask corresponds to 1ms of debouncing. */

#define NUM_INPUTS 7

struct PinInfo {
    uint8_t gpioPin;	    /* BCM2835 GPIO logical pin number. */
    uint32_t invertBit;	    /* Set to 1 if input is active low. */
    uint32_t debounceMask;  /* A mask of all zeroes followed by all ones. */
};

const static struct PinInfo PIN_INFO[NUM_INPUTS] = {
    { RPI_BPLUS_GPIO_J8_40, 0, 0x0000FFFF },	/* Power Switch (1=on, 0=off) */
    { RPI_BPLUS_GPIO_J8_33, 1, 0x0000000F },	/* User Button 1 */
    { RPI_BPLUS_GPIO_J8_35, 1, 0x0000000F },	/* User Button 2 */
    { RPI_BPLUS_GPIO_J8_37, 1, 0x0000000F },	/* User Button 3 */
    { RPI_BPLUS_GPIO_J8_36, 1, 0x0000000F },	/* Low Battery Warning */
    { RPI_BPLUS_GPIO_J8_31, 1, 0x0000000F },	/* Charging in Progress */
    { RPI_BPLUS_GPIO_J8_29, 1, 0x0000000F }	/* Charge Completed */
};

/* The scan map records the actual state of each input over the last 32 scans,
   and the logical state of each input after debouncing. */

struct ScanMap {
    uint32_t raw;
    bool debounced;
};

static struct ScanMap scanMap[NUM_INPUTS];

/* Battery Monitoring

   The battery monitoring input is a single pin, driven by a comparator that
   compares 0.69 times the battery voltage against a triangle wave that
   oscillates between about 1.9 and 3.3V at about 100Hz. The fraction of the
   time that the scaled voltage is higher than the waveform indicates where the
   battery voltage lies in a range of about 2.7 to 4.8V. By keeping a running
   average of the last BATTERY_SAMPLES samples (which are all 0 or 1), we get a
   reasonable estimate of that fraction. */

#define GPIO_BATT_MON RPI_BPLUS_GPIO_J8_38
#define VOLTAGE_AT_0 2.7096
#define VOLTAGE_AT_1 4.8267

static uint8_t batterySamples[BATTERY_SAMPLES];
static unsigned int nextSampleIndex, sampleTotal;
static double lastBattery;

/* ----------------------------- Initialization ----------------------------- */

static void initPort( uint8_t pin )
{
    bcm2835_gpio_fsel(pin,BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(pin,BCM2835_GPIO_PUD_UP);
}

bool InitGPIO( void )
{
    /* Initialize BCM2835 GPIO library. */
    bcm2835_set_debug(0);
    if( !bcm2835_init() )
        return( false );

    /* Initialize debounced inputs. */
    for( int i = 0; i < NUM_INPUTS; ++i ) {
	initPort(PIN_INFO[i].gpioPin);
        scanMap[i].raw = 0x00000000;
	scanMap[i].debounced = false;
    }

    /* Initialize battery monitoring to 50%. */
    initPort(GPIO_BATT_MON);
    for( int i = 0; i < BATTERY_SAMPLES; ++i )
        batterySamples[i] = i & 1;
    nextSampleIndex = 0;
    sampleTotal = BATTERY_SAMPLES / 2;
    lastBattery = (VOLTAGE_AT_0 + VOLTAGE_AT_1) / 2;

    return( true );
}

/* ---------------------------- Debounced Input ----------------------------- */

/* Read the specified input (0..NUM_INPUTS-1) and debounce it, returning 1
   if it just became active, -1 if it just became inactive, or 0 if nothing has
   changed. */

int GetInput( int inputNum )
{
    /* Ensure that the input number is in range. */
    if( inputNum < 0 || inputNum >= NUM_INPUTS )
        return( 0 );

    struct ScanMap *state = &scanMap[inputNum];
    const struct PinInfo *input = &PIN_INFO[inputNum];

    /* Read the input pin and shift its value (possibly inverted) into the raw
       buffer for this pin. */
    state->raw = (state->raw << 1)
	       | (bcm2835_gpio_lev(input->gpioPin) ^ input->invertBit);

    /* If the input has been active (e.g. button press) for long enough (and
       wasn't already), record that it became active and return 1. */
    if( !state->debounced
     && (state->raw & input->debounceMask) == input->debounceMask )
    {
	state->debounced = true;
	return( 1 );
    }

    /* If the input has been inactive (e.g. button release) for long enough
       (and wasn't already), record that it became inactive and return -1. */
    if( state->debounced && (state->raw & input->debounceMask) == 0 ) {
	state->debounced = false;
        return( -1 );
    }

    /* Nothing changed. */
    return( 0 );
}

/* --------------------------- Battery Monitoring --------------------------- */

/* Sample the battery monitoring input, update a circular buffer of samples,
   and return the average of all the samples in the buffer. */

double GetBatteryRaw( void )
{
    /* Remove the sample we're about to throw away from the total. */
    sampleTotal -= batterySamples[nextSampleIndex];

    /* Sample the battery monitor input and add it to the total. */
    sampleTotal += batterySamples[nextSampleIndex]
		 = bcm2835_gpio_lev(GPIO_BATT_MON);

    /* Compute the index of the next sample. */
    nextSampleIndex = (nextSampleIndex + 1) % BATTERY_SAMPLES;

    /* Return the average of all the samples. */
    return( (double) sampleTotal / BATTERY_SAMPLES );
}
