/*
Just-In-Time Logwatcher with a joint venture to the desktopmanagementsystem
Copyright (C) 2013 Sven Sperner - Bachelor Thesis, FH Frankfurt/Main

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "globals.h"


// function to check if a file exists
int fileExists( const char *filePath )
{
    // temporary pointer to file
    FILE *file;

    // try to open file
    if( (file = fopen(filePath, "r")) )
    {
        fclose( file );
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

// function for output ( terminal / logfile )
void out( char level, struct daemonConfig *config, const char *msg, ... )
{
    // only if loglevel is smaller than configured
    if( level <= config->loglevel )
    {
        // get arguments out of "msg"
        va_list args;
        va_start( args, msg );

        // get the actual time
        char *datetime;
        if( (datetime = malloc(sizeof(char[TIMELENGTH]))) == NULL )
        {
            fprintf( stderr, "out(): Unable to allocate date/time string\n" );
            return;
        }
        if( getTime(datetime) != EXIT_SUCCESS )
        {
            fprintf( stderr, "out(): Unable to get actual date/time\n" );
            return;
        }

        // lock mutex for
        if( pthread_mutex_lock(&config->mutex) == 0 )
        {
            // if run as daemon, log to a file
            if( config->rad )
            {
                // open the logfile in append mode
                char *mode="a+";
                FILE *logfile = fopen( config->logfile, mode );
                if( logfile == NULL )
                {
                    // can't open logfile -> try to log to /tmp
                    fprintf( stderr, "out(): %s Unable to open %s\n", datetime, config->logfile );
                    logfile = fopen( TMP_LOGFILE, mode );
                    if( logfile != NULL )
                    {
                        fprintf( logfile, datetime, 0 );
                        vfprintf( logfile, msg, args );
                        fclose( logfile );
                    }
                }
                else // log to configured logfile
                {
                    fprintf( logfile, datetime, 0 );
                    vfprintf( logfile, msg, args );
                    fclose( logfile );
                }
            }
            else // no daemon -> print to standard out
            {
                fprintf( stdout, datetime, 0 );
                vfprintf( stdout, msg, args );
            }
        }

        // unlock mutex of message queue
        pthread_mutex_unlock(&config->mutex);

        // free the in getTime() allocated memory
        free( datetime );

        // close the argument list
        va_end( args );
    }
}

int getTime( char *strTime )
{
    // get time in seconds since 01.01.1970-00:00:00
    time_t aktTime = time( NULL );

    // convert to a usable date-time structure
    struct tm *dt = gmtime( &aktTime );

    struct timeval *tv = malloc( sizeof(struct timeval) );
    struct timezone *tz = malloc( sizeof(struct timezone) );
    if( gettimeofday( tv, tz ) != 0 )
    {
        printf("gtod-err\n");
    }

    // the tm_year counts from 1900 so it must be added, tm_mon begins with 0 so 1 must be added
    if( sprintf(strTime, "%04d-%02d-%02dT%02d:%02d:%02dZ %ld ", dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec, tv->tv_usec) > 0 )
    {
        free( tv );
        free( tz );
        // return with success
        return EXIT_SUCCESS;
    }
    else
    {
        free( tv );
        free( tz );
        // return with failure
        return EXIT_FAILURE;
    }
}
