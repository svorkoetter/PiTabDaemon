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

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "logging.h"

#define LOG_FILE "/var/log/pitabd.log"

void WriteToLog( const char *msg )
{
    time_t t;
    static struct tm *tm;
    static char s[100];

    time(&t);
    tm = localtime(&t);
    strftime(s,sizeof(s),"%Y-%m-%d %H:%M:%S",tm);

    FILE *fp = fopen(LOG_FILE,"a");
    if( fp != NULL ) {
	fprintf(fp,"%s %s\n",s,msg);
        fclose(fp);
    }
}

void WriteToLogArgI( const char *msg, int arg )
{
    char buf[100];
    sprintf(buf,msg,arg);
    WriteToLog(buf);
}

void WriteToLogArgF( const char *msg, double arg )
{
    char buf[100];
    sprintf(buf,msg,arg);
    WriteToLog(buf);
}

void RotateLogs( void )
{
    unlink(LOG_FILE ".9");
    for( int i = 8; i >= 1; --i ) {
        char from[80], to[80];
	sprintf(from, LOG_FILE ".%d", i);
	sprintf(to, LOG_FILE ".%d", i+1);
	rename(from,to);
    }
    rename(LOG_FILE, LOG_FILE ".1");
}
