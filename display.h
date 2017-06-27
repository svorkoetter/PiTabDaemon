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

#ifndef __PI_TAB_DAEMON_DISPLAY_H__
#define __PI_TAB_DAEMON_DISPLAY_H__

extern void InitBrightness( int initialIndex );
extern void NextBrightness( void );
extern void MaxBrightness( void );
extern void NudgeBrightness( void );
extern int GetBrightnessIndex( void );

extern void DimDisplay( void );
extern void DarkenDisplay( void );
extern void RestoreDisplay( void );

#endif
