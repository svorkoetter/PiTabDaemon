/* PiTabDaemon - Display Brightness Management */

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

#include <stdio.h>

#include "display.h"

/* These levels were chosen to result in a doubling of LED current over a
   range of about 4mA to 500mA, and then tweaking them until the brightness
   changes appeared to be uniform. */
static const int LEVELS[] = { 0, 10, 14, 23, 42, 72, 115, 172, 212 };
static const int NUM_LEVELS = sizeof(LEVELS) / sizeof(int);
static const int DEFAULT_INDEX = 4;

static int nextLevelIndex, currentLevel, targetLevel, rememberLevel;

/* Initialize brightness as specified, or about 1/4 of maximum (about 3/4
   perceptually) by default if specified index is 0 or out of range. */
void InitBrightness( int initialIndex )
{
    nextLevelIndex = 0 < initialIndex && initialIndex < NUM_LEVELS
    		   ? initialIndex : DEFAULT_INDEX;
    NextBrightness();
    currentLevel = targetLevel - 1;
    NudgeBrightness();
}

/* Set the target brightness to the next value in the table. */
void NextBrightness( void )
{
    /* Set target brightness from table and compute index for next time. */
    targetLevel = rememberLevel = LEVELS[nextLevelIndex];
    nextLevelIndex = (nextLevelIndex + 1) % NUM_LEVELS;
}

/* Set the target brightness to the maximum brighness value in the table. */
void MaxBrightness( void )
{
    targetLevel = rememberLevel = LEVELS[NUM_LEVELS-1];
    nextLevelIndex = 0;
}

/* Return the current brightness index. */
int GetBrightnessIndex( void )
{
    return( (nextLevelIndex + NUM_LEVELS - 1) % NUM_LEVELS );
}

/* Nudge the display brightness towards the target brightness by 5% of the
   current brightness. */
void NudgeBrightness( void )
{
    if( currentLevel != targetLevel ) {
	if( currentLevel == 0 )
	    currentLevel = LEVELS[1];
        else if( currentLevel < targetLevel ) {
	    if( currentLevel < 20 )
	        ++currentLevel;
	    else
		currentLevel = (currentLevel * 21) / 20;
	    if( currentLevel > targetLevel )
	        currentLevel = targetLevel;
	}
	else {
	    /* Darken faster than we brighten so that full-on to full-off
	       doesn't take so long. */
	    currentLevel = (currentLevel * 10) / 11;
	    if( currentLevel < targetLevel )
	        currentLevel = targetLevel;
	}
	FILE *fp = fopen("/sys/class/backlight/rpi_backlight/brightness","w");
	if( fp != NULL ) {
	    fprintf(fp,"%d\n",currentLevel);
	    fclose(fp);
	}
    }
}

/* Temporarily dim the display (unless it's already off) to half the current
   perceived brightness. */
void DimDisplay( void )
{
    if( nextLevelIndex != 1 ) {
	if( nextLevelIndex == 0 )
	    targetLevel = LEVELS[NUM_LEVELS/2];
	else {
	    targetLevel = LEVELS[(nextLevelIndex-1)/2];
	    if( targetLevel < LEVELS[1] ) targetLevel = LEVELS[1];
	}
    }
}

/* Temporarily turn off the backlight. */
void DarkenDisplay( void )
{
    targetLevel = LEVELS[0];
}

/* Restore the display to the level it was at before we dimmed it. */
void RestoreDisplay( void )
{
    targetLevel = rememberLevel;
}
