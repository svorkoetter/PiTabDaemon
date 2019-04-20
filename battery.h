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

#ifndef __PI_TAB_DAEMON_BATTERY_H__
#define __PI_TAB_DAEMON_BATTERY_H__

/* Number of binary samples used to compute battery reading. */
#define BATTERY_SAMPLES 16384

extern void InitBattery( void );

/* Sample the battery monitoring input, update a circular buffer of samples,
   and return the average of all the samples (each of which is 0 or 1) in the
   buffer. Also return a separate sample adjusted for the charger having been
   connected when the samples were taken. */
extern double GetRawBatteryReadings( bool charging, double *rAdj );

/* Convert a raw battery reading to a voltage. */
extern double BatteryRawToVoltage( double rAct );

/* Convert an adjusted raw battery reading to an estimate of the energy
   remaining in the battery. */
extern double BatteryRawToEnergyRemaining( double rAdj );

#endif
