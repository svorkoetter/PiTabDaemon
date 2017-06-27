/* PiTabDaemon - Idle Time Monitoring */

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
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

/* Based on code found at https://superuser.com/questions/638357 */

/* Return user idle time in milliseconds. */
int IdleTime( void )
{
    static Display *display = NULL;
    int event_base, error_base;
    XScreenSaverInfo info;

    if( display == NULL && (display = XOpenDisplay(":0.0")) == NULL )
        return( -1 );

    if( XScreenSaverQueryExtension(display,&event_base,&error_base) ) {
	XScreenSaverQueryInfo(display,DefaultRootWindow(display),&info);
	return( info.idle );
    }

    return( -2 );
}
