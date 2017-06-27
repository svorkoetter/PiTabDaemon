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

#ifndef __PI_TAB_DAEMON_IO_H__
#define __PI_TAB_DAEMON_IO_H__

/* Inputs that can be checked by GetInput. */
#define SWITCH_ON 0
#define BUTTON_1  1
#define BUTTON_2  2
#define BUTTON_3  3
#define LOW_BATT  4
#define CHARGING  5
#define CHARGED   6

extern bool InitGPIO( void );
extern int GetInput( int inputNum );

/* Number of binary samples used to compute battery reading. */
#define BATTERY_SAMPLES 16384

extern double GetBatteryRaw( void );

#endif
