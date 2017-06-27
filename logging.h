/* PiTabDaemon - Log File Management */

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

#ifndef __PI_TAB_DAEMON_LOGGING_H__
#define __PI_TAB_DAEMON_LOGGING_H__

extern void WriteToLog( const char *msg );
extern void WriteToLogArgI( const char *msg, int arg );
extern void WriteToLogArgF( const char *msg, double arg );
extern void RotateLogs( void );

#endif
