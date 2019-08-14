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
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

/* Return user idle time in milliseconds. */
int IdleTime( void )
{
    /* Check X11 idle time. Based on code found at
       https://superuser.com/questions/638357 */
    static Display *display = NULL;
    int event_base, error_base, idle;
    XScreenSaverInfo info;

    if( display == NULL && (display = XOpenDisplay(":0.0")) == NULL )
        return( -1 );

    if( !XScreenSaverQueryExtension(display,&event_base,&error_base) )
	return( -2 );

    XScreenSaverQueryInfo(display,DefaultRootWindow(display),&info);
    idle = info.idle;

    /* Check console idle time. */
    time_t now = time(NULL);
    char tty[10];
    struct stat st;

    for( int i = 1; i <= 6 && idle > 0; ++i ) {
	sprintf(tty,"/dev/tty%1.1d",i);
	if( stat(tty,&st) == 0 ) {
	    int ms = (now - st.st_mtime) * 1000;
	    if( ms < idle )
		idle = ms;
	}
    }

    return( idle );
}
