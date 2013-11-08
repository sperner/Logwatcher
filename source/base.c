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


// function to create a base structure
int createBase( struct daemonConfig *config, struct daemonBase *newBase )
{
    // are given daemon config & base structure valid
    if( config && newBase )
    {
        out( MSGINFO, config, "createBase(): Creating a new daemon base structure\n" );

        // copy the given daemon config to the base
        newBase->dconf = config;

        // allocate space for base structure entries
        if( (newBase->lfconf = malloc(sizeof(struct logfilesConfig))) == NULL )
        {
            out( MSGERROR, config, "createBase(): Error allocating memory for logfiles config structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (newBase->server = malloc(sizeof(struct serverSocket))) == NULL )
        {
            out( MSGERROR, config, "createBase(): Error allocating memory for server structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (newBase->clients = malloc(sizeof(struct clientThreads))) == NULL )
        {
            out( MSGERROR, config, "createBase(): Error allocating memory for client threads structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (newBase->watcher = malloc(sizeof(struct watcherThreads))) == NULL )
        {
            out( MSGERROR, config, "createBase(): Error allocating memory for watcher threads structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (newBase->msgQueue = malloc(sizeof(struct messageQueue))) == NULL )
        {
            out( MSGERROR, config, "createBase(): Error allocating memory for message queue structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }

        // initialize the mutex of the base
        if( pthread_mutex_init(&newBase->mutex, NULL) == 0 )
        {
            out( MSGDEBUG, config, "createBase(): Initialised the mutex of the base structure\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "createBase(): Error initialising the mutex of the base structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "createBase(): The given daemon base structure is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }
}

// function to run logwatcher as a daemon process
int runAsDaemon( struct daemonConfig *config )
{
    int openDesc;

    out( MSGNOTICE, config, "runAsDaemon(): Trying to run logwatcher as a daemon\n" );

    // create a new process and kill the parent to run in background
    switch( fork() )
    {
        case -1:    out( MSGERROR, config, "runAsDaemon(): Error creating a new process, first time\n" );
                    exit( EXIT_FAILURE );
                    break;
        case 0:     out( MSGINFO, config, "runAsDaemon(): The child is up and running, first time\n" );
                    break;
        default:    out( MSGINFO, config, "runAsDaemon(): Going to terminate the parent, first time\n" );
                    exit( EXIT_SUCCESS );
                    break;
    }

    // create a new session, get leader and be free from terminal
    if( setsid() < 0 )
    {
        out( MSGERROR, config, "runAsDaemon(): Error creating a new session\n" );
        exit( EXIT_FAILURE );
    }
    else
    {
        out( MSGDEBUG, config, "runAsDaemon(): Created a new session and got leader\n" );
    }

    // for next fork we need to ignore the hang-up
    if( signal(SIGHUP | SIGINT | SIGWINCH, SIG_IGN) != SIG_ERR )
    {
        out( MSGDEBUG, config, "runAsDaemon(): Connected signals to be irgnored for fork() \n" );
    }

    // create a new process and kill the parent (former child)
    // to be sure the process can't get a control terminal
    switch( fork() )
    {
        case -1:    out( MSGERROR, config, "runAsDaemon(): Error creating a new process, second time\n" );
                    exit( EXIT_FAILURE );
                    break;
        case 0:     out( MSGINFO, config, "runAsDaemon(): The child is up and running, second time\n" );
                    break;
        default:    out( MSGINFO, config, "runAsDaemon(): Going to terminate the parent, second time\n" );
                    exit( EXIT_SUCCESS );
                    break;
    }

    // change the workdir to root
    if( chdir("/") == 0 )
    {
        out( MSGDEBUG, config, "runAsDaemon(): Changed working directory to root (/)\n" );
    }

    // change the filemode creation mask
    if( umask(0) >= 0 )
    {
        out( MSGDEBUG, config, "runAsDaemon(): Changed umask to zero (0)\n" );
    }

    // close all possibly open file descriptors
    out( MSGDEBUG, config, "runAsDaemon(): Closing all possibly open file descriptors\n" );
    for( openDesc = sysconf(_SC_OPEN_MAX) ; openDesc > 0 ; openDesc-- )
    {
        close( openDesc );
    }

    out( MSGNOTICE, config, "runAsDaemon(): Succesfully running as system daemon now\n" );

    // return with success
    return EXIT_SUCCESS;
}

// function to print the command line usage
void usage( struct daemonConfig *config )
{
    printf( "Usage: logwatcher -[clovitdh]\n" );
    printf( "\n" );
    printf( "Options:\n" );
    printf( "  -c (--conf)    \tpath to logwatcher conf\n" );
    printf( "  -l (--lconf)   \tpath to logfiles conf\n" );
    printf( "  -o (--logfile) \tpath to logwatchers logfile\n" );
    printf( "  -v (--loglevel)\toutput- / log-level\n" );
    printf( "  -i (--interval)\tparsing interval\n" );
    printf( "  -t (--tmpfile) \tpath to logwatchers possavefile\n" );
    printf( "  -d (--daemon)  \tstart logwatcher as daemon\n" );
    printf( "  -h (--help)    \tthis help\n" );
    printf( "\n" );
    exit( EXIT_SUCCESS );
}
