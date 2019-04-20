/* PiTabDaemon - Battery Monitoring */

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
#include <stdint.h>

#include "battery.h"
#include "io.h"

/* The battery monitoring input is a single pin, driven by a comparator that
   compares 0.69 times the battery voltage against a triangle wave that
   oscillates between about 1.9 and 3.3V at about 100Hz. The fraction of the
   time that the scaled voltage is higher than the waveform indicates where the
   battery voltage lies in a range of about 2.7 to 4.8V. By keeping a running
   average of the last BATTERY_SAMPLES samples (which are all 0 or 1), we get a
   reasonable estimate of that fraction. */

/* Voltages corresponding to 0% and 100% duty cycle at battery monitor input. */
#define VOLTAGE_AT_0 2.7096
#define VOLTAGE_AT_1 4.8267

/* Points defining a voltage (expressed as raw A/D readings) versus remaining
   energy curve while not charging. */
const double EMPTY = 0.34, EMPTY_ENERGY = 0.00;
const double KNEE1 = 0.44, KNEE1_ENERGY = 0.22;
const double KNEE2 = 0.61, KNEE2_ENERGY = 0.95;
const double FULL = 0.707, FULL_ENERGY =  1.00;

/* Amount to subtract from the raw reading if the charger is connected, as the
   reading is artificially high due to the voltage drop across the battery's
   internal resistance. */
#define CHARGE_DELTA (0.2 / (VOLTAGE_AT_1 - VOLTAGE_AT_0))

static uint8_t batterySamples[BATTERY_SAMPLES];
static unsigned int nextSampleIndex, sampleTotal, chargingSampleTotal;

void InitBattery( void )
{
    for( int i = 0; i < BATTERY_SAMPLES; ++i )
        batterySamples[i] = i & 1;
    nextSampleIndex = 0;
    sampleTotal = BATTERY_SAMPLES / 2;
    chargingSampleTotal = 0;
}

double GetRawBatteryReadings( bool charging, double *rAdj )
{
    /* Remove the sample we're about to throw away from the total. */
    sampleTotal -= batterySamples[nextSampleIndex] & 1;
    chargingSampleTotal -= (batterySamples[nextSampleIndex] & 2) >> 1;

    /* Sample the battery monitor input and add it to the total. */
    if( GetBatterySample() ) {
        batterySamples[nextSampleIndex] = charging ? 3 : 1;
	++sampleTotal;
	if( charging ) ++chargingSampleTotal;
    }
    else
        batterySamples[nextSampleIndex] = 0;

    /* Compute the index of the next sample. */
    nextSampleIndex = (nextSampleIndex + 1) % BATTERY_SAMPLES;

    /* Compute two averages, one corresponding to the actual measured voltage,
       and one adjusted for the charger being connected. */
    double rAct = (double) sampleTotal / BATTERY_SAMPLES;

    double delta = CHARGE_DELTA;
    if( rAct > KNEE2 )
        delta *= (FULL - rAct) / (FULL - KNEE2);
    *rAdj = (sampleTotal - delta * chargingSampleTotal) / BATTERY_SAMPLES;

    /* Return the unadjusted actual reading. */
    return( rAct );
}

double BatteryRawToVoltage( double rAct )
{
    return( rAct * (VOLTAGE_AT_1 - VOLTAGE_AT_0) + VOLTAGE_AT_0 );
}

// TODO - adjust for display brightness and system load (or maybe in io.c)

double BatteryRawToEnergyRemaining( double rAdj )
{
    double e = 0;
    if( rAdj > KNEE2 )
	e = (rAdj - KNEE2) / (FULL - KNEE2) * (FULL_ENERGY - KNEE2_ENERGY) + KNEE2_ENERGY;
    else if( rAdj > KNEE1 )
	e = (rAdj - KNEE1) / (KNEE2 - KNEE1) * (KNEE2_ENERGY - KNEE1_ENERGY) + KNEE1_ENERGY;
    else
	e = (rAdj - EMPTY) / (KNEE1 - EMPTY) * (KNEE1_ENERGY - EMPTY_ENERGY) + EMPTY_ENERGY;
    if( e < 0 ) e = 0; else if( e > 1 ) e = 1;
    return( e * 100.0 );
}
