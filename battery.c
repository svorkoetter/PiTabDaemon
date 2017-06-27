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

#include "battery.h"

/* Voltages corresponding to 0% and 100% duty cycle of battery monitor input. */
#define VOLTAGE_AT_0 2.7096
#define VOLTAGE_AT_1 4.8267

/* Table mapping battery monitor raw values to percent of energy remaining.
   This was determined empirically by letting the tablet run down from a full
   charge while logging the raw readings, fitting a 9th degree polynomial to
   the data points, and then sampling that at 100 evenly spaced time intervals.
   Display brightness was set to a raw value of 23 (brightness level 3), and
   the battery was a 6200mAh 9x60x90mm lithium ion polymer cell. */
const float ENERGY_CURVE[101] = {
    0.37246, 0.37741, 0.38235, 0.38730, 0.39230, 0.39737, 0.40251, 0.40772,
    0.41300, 0.41831, 0.42366, 0.42901, 0.43435, 0.43965, 0.44488, 0.45002,
    0.45506, 0.45996, 0.46471, 0.46930, 0.47371, 0.47793, 0.48196, 0.48577,
    0.48938, 0.49278, 0.49598, 0.49898, 0.50178, 0.50439, 0.50683, 0.50911,
    0.51124, 0.51323, 0.51511, 0.51689, 0.51858, 0.52021, 0.52178, 0.52332,
    0.52485, 0.52637, 0.52791, 0.52948, 0.53108, 0.53274, 0.53446, 0.53625,
    0.53811, 0.54006, 0.54209, 0.54421, 0.54641, 0.54870, 0.55106, 0.55350,
    0.55601, 0.55858, 0.56120, 0.56386, 0.56654, 0.56924, 0.57193, 0.57462,
    0.57728, 0.57989, 0.58245, 0.58494, 0.58734, 0.58965, 0.59185, 0.59394,
    0.59590, 0.59773, 0.59943, 0.60100, 0.60243, 0.60374, 0.60493, 0.60601,
    0.60701, 0.60793, 0.60882, 0.60969, 0.61059, 0.61155, 0.61262, 0.61386,
    0.61531, 0.61706, 0.61916, 0.62169, 0.62475, 0.62842, 0.63280, 0.63800,
    0.64414, 0.65134, 0.65973, 0.66945, 0.68064
};

// TODO - adjust for display brightness (or maybe in io.c)
// TODO - add state of charge estimation during charging

double BatteryRawToVoltage( double raw )
{
    return( raw * (VOLTAGE_AT_1 - VOLTAGE_AT_0) + VOLTAGE_AT_0 );
}

double BatteryRawToEnergyRemaining( double raw )
{
    /* Calculate the index of the right end of the range containing raw. */
    int lo = 0, hi = 101;
    while( lo < hi ) {
	int mid = (lo + hi) / 2;
	if( ENERGY_CURVE[mid] < raw )
	    lo = mid + 1;
	else
	    hi = mid;
    }
    /* Raw is lower than 0% charged, so return 0% remaining. */
    if( lo == 0 )
        return( 0.0 );
    /* Raw is higher than 100% charged, so return 100% remaining. */
    if( lo > 100 )
        return( 100.0 );
    /* Raw falls within a range of points. Interpolate to find percentage. */
    return( lo - 1 + (raw - ENERGY_CURVE[lo-1])
                   / (ENERGY_CURVE[lo] - ENERGY_CURVE[lo-1]) );
}
