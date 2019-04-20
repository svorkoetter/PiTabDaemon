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

#include "battery.h"
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

/* Battery Monitoring Input */

#define GPIO_BATT_MON RPI_BPLUS_GPIO_J8_38

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

    /* Initialize battery monitoring port. */
    initPort(GPIO_BATT_MON);

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

bool GetBatterySample( void )
{
    return( bcm2835_gpio_lev(GPIO_BATT_MON) != 0 );
}
