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

#include "battery.h"

double BatteryRawToVoltage( double raw )
{
    /* Voltages corresponding to 0% and 100% duty cycle of battery monitor
       input. */
    const double VOLTAGE_AT_0 = 2.7096, VOLTAGE_AT_1 = 4.8267;
    return( raw * (VOLTAGE_AT_1 - VOLTAGE_AT_0) + VOLTAGE_AT_0 );
}

// TODO - adjust for display brightness and system load (or maybe in io.c)

double BatteryRawToEnergyRemaining( double raw, bool charger )
{
    //const double EMPTY = 0.34, KNEE = 0.44, KNEE_ENERGY = 0.22, FULL = 0.61;
    const double EMPTY = 0.34, EMPTY_ENERGY = 0.00;
    const double KNEE1 = 0.44, KNEE1_ENERGY = 0.22;
    const double KNEE2 = 0.61, KNEE2_ENERGY = 0.95;
    const double FULL = 0.707, FULL_ENERGY =  1.00;
    if( charger )
        raw -= 0.0945;
    double e = 0;
    if( raw > KNEE2 )
	e = (raw - KNEE2) / (FULL - KNEE2) * (FULL_ENERGY - KNEE2_ENERGY) + KNEE2_ENERGY;
    else if( raw > KNEE1 )
	e = (raw - KNEE1) / (KNEE2 - KNEE1) * (KNEE2_ENERGY - KNEE1_ENERGY) + KNEE1_ENERGY;
    else
	e = (raw - EMPTY) / (KNEE1 - EMPTY) * (KNEE1_ENERGY - EMPTY_ENERGY) + EMPTY_ENERGY;
    if( e < 0 ) e = 0; else if( e > 1 ) e = 1;
    return( e * 100.0 );
}
