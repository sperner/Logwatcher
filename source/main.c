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

// to allow network traffic through iptables
// iptables -I INPUT -p tcp --dport [PORT-NO] -j ACCEPT


#include "globals.h"


char breaker = 0;
char readConf = 0;
//char chgTime = 0;

void breakMain( int signalType )
{
    if( signalType != SIGINT )
    {
        return;
    }
    else
    {
        breaker = 1;
    }
}

void readConfig( int signalType )
{
    if( signalType != SIGUSR1 )
    {
        return;
    }
    else
    {
        readConf = 1;
    }
}
/*
void chgTimestamp( int signalType )
{
    if( signalType != SIGUSR2 )
    {
        return;
    }
    else
    {
        chgTime = 1;
    }
}
*/


int main( int argc, char **argv, char **envp )
{
    // create temporary daemon configuration for out()              // need to free anywhere or use as default for base?
    struct daemonConfig *config = malloc( sizeof(struct daemonConfig) );
    if( initDaemonConfig( config ) != EXIT_SUCCESS )
    {
        out( MSGEMERG, config, "main(): Error initialising daemon config structure\n" );
        return EXIT_FAILURE;
    }


    // get commandline arguments
    struct option longopts[] = {
    { "conf",	    required_argument,	NULL,	'c' },
    { "lconf",		required_argument,	NULL,	'l' },
    { "logfile",	required_argument,	NULL,	'o' },
    { "loglevel",	required_argument,	NULL,	'v' },
    { "interval",	required_argument,	NULL,	'i' },
    { "tmpfile",	required_argument,	NULL,	't' },
    { "daemon", 	no_argument,	    NULL,	'd' },
    { "help",		no_argument,		NULL,	'h' },
    { 0, 0, 0, 0 }
    };
    int option = 0;
    while( ( option = getopt_long(argc, argv, "c:l:o:v:i:t:dh", longopts, NULL)) != -1 )
    {
        switch( option )
        {
            case 'c':	config->daemonconf = malloc( strlen(optarg)+1 );
                        strcpy( config->daemonconf, optarg );
                        break;
            case 'l':	config->logfilesconf = malloc( strlen(optarg)+1 );
                        strcpy( config->logfilesconf, optarg );
                        break;
            case 'o':	config->logfile = malloc( strlen(optarg)+1 );
                        strcpy( config->logfile, optarg );
                        break;
            case 'v':	config->loglevel = atoi( optarg );
                        break;
            case 'i':	config->interval = atoi( optarg );
                        break;
            case 't':	config->savefile = malloc( strlen(optarg)+1 );
                        strcpy( config->savefile, optarg );
                        break;
            case 'd':	config->rad = 1;
                        break;
            case 'h':	usage( config );
                        break;
            default:
                        break;
        }
    }

    // if requested turn the process into a daemon
    if( config->rad )
    {
        if( runAsDaemon(config) != EXIT_SUCCESS )
        {
            out( MSGEMERG, config, "main(): Error trying to run as daemon\n" );
            return EXIT_FAILURE;
        }
    }

    // create the base structure
    struct daemonBase *dBase = malloc( sizeof(struct daemonBase) );
    if( createBase(config, dBase) != EXIT_SUCCESS )
    {
        out( MSGEMERG, config, "main(): Error creating base structure\n" );
        return EXIT_FAILURE;
    }

    // read and set configuration of daemon
    if( readDaemonConfig(config->daemonconf, dBase->dconf) != EXIT_SUCCESS )
    {
        out( MSGEMERG, config, "main(): Error reading daemon configuration\n" );
        return EXIT_FAILURE;
    }
    printDaemonConfig( dBase->dconf );

    // read and set configuration of logfiles
    if( readLogfilesConfig(dBase->dconf, dBase->lfconf) != EXIT_SUCCESS )
    {
        out( MSGEMERG, dBase->dconf, "main(): Error reading logfiles configuration\n" );
        return EXIT_FAILURE;
    }
    printLogfilesConfig( dBase->dconf, *dBase->lfconf );

    // create network client structure
    if( createClientsStructure(dBase->dconf, dBase->clients) != EXIT_SUCCESS )
    {
        out( MSGEMERG, dBase->dconf, "main(): Error creating network clients structure\n" );
        return EXIT_FAILURE;
    }

    // create logfile watcher structure
    if( createWatchersStructure(dBase->dconf, dBase->watcher) != EXIT_SUCCESS )
    {
        out( MSGEMERG, dBase->dconf, "main(): Error creating logfile watcher structure\n" );
        return EXIT_FAILURE;
    }

    // create message queue
    if( createQueue(dBase->dconf, dBase->msgQueue) != EXIT_SUCCESS )
    {
        out( MSGEMERG, dBase->dconf, "main(): Error creating message queue\n" );
        return EXIT_FAILURE;
    }
/*
    // main-loop
    int i = 1;
    if( i ) // just for testing purpose
    {
        while( createServerSocket(dBase) != EXIT_SUCCESS )
        {
            out( MSGERROR, dBase->dconf, "main(): Couldn't create server socket (%d), trying again...\n", i );
            sleep( 10 );
        }
*/
        if( createServerSocket(dBase) != EXIT_SUCCESS )
        {
            out( MSGERROR, dBase->dconf, "main(): Couldn't create server socket, quitting\n" );
            return EXIT_SUCCESS;
        }

        signal( SIGINT, breakMain );
        signal( SIGUSR1, readConfig );
//        signal( SIGUSR2, chgTimestamp );

        while( !breaker )
        {

            if( createWatcher(dBase) != EXIT_SUCCESS )
            {
                out( MSGERROR, dBase->dconf, "main(): Error creating logfile watcher\n" );
                //return EXIT_FAILURE;
            }
            else
            {
                out( MSGNOTICE, dBase->dconf, "main(): Started %d watcher threads\n", dBase->watcher->num );
            }

            out( MSGNOTICE, dBase->dconf, "main(): The message queue now has %d entries\n", dBase->msgQueue->num );

            while( dBase->watcher->num > 0 ) { sleep(1); }

            if( readConf )
            {
                initDaemonConfig( dBase->dconf );
                readDaemonConfig( dBase->dconf->daemonconf, dBase->dconf );
                printDaemonConfig( dBase->dconf );
                readLogfilesConfig( dBase->dconf, dBase->lfconf );
                printLogfilesConfig( dBase->dconf, *dBase->lfconf );
                readConf = 0;
            }
/*
            if( chgTime )
            {
                chgTime = 0;
            }
*/

            sleep( dBase->dconf->interval );

//            printQueue( *dBase->dconf, dBase->msgQueue );
        }//while( !breaker )
/*
    }
    else // just for testing purpose
    {
        struct entry *myEntry = malloc( sizeof(struct entry) );
        char *message = malloc( sizeof(char[11]) );
        for( i = 0 ; i < 10 ; i++ )
        {
            sprintf( message, "%d%d%d%d%d",i,i,i,i,i );
            addToQueue( dBase->dconf, dBase->msgQueue, MSGDEBUG, "testfile", message );
        }
//        printQueueElement( dBase->dconf, dBase->msgQueue->first );
//        printQueue( dBase->dconf, dBase->msgQueue );
//        printQueueElement( dBase->dconf, dBase->msgQueue->last );
        pthread_t threadT = 999;
        addClient( dBase->dconf, dBase->clients, threadT, 5 );
        for( i = 0 ; i < 10 ; i++ )
        {
//            printQueueElement( dBase->dconf, dBase->msgQueue->first );
            printQueue( dBase->dconf, dBase->msgQueue );
//            printQueueElement( *dBase->dconf, dBase->msgQueue->last );
            if( getFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->first, threadT, myEntry) != EXIT_SUCCESS )
//            if( getFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->last, threadT, myEntry) != EXIT_SUCCESS )
//            if( getFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->first->next, threadT, myEntry) != EXIT_SUCCESS )
//            if( getFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->last->prev, threadT, myEntry) != EXIT_SUCCESS )
                out( MSGINFO, dBase->dconf, "main(): Error getting element from queue!\n" );
//            if( removeFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->first, threadT) != EXIT_SUCCESS )
//            if( removeFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->last) != EXIT_SUCCESS )
//            if( removeFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->first->next) != EXIT_SUCCESS )
//            if( removeFromQueue(dBase->dconf, dBase->msgQueue, dBase->clients ,dBase->msgQueue->last->prev) != EXIT_SUCCESS )
//                out( MSGINFO, dBase->dconf, "main(): Error removing element from queue!\n" );
            if( myEntry->reads != EMPTYELEM )
                out( MSGINFO, dBase->dconf, "main(): Got from Queue: |%d| %s: %s\n", myEntry->reads, myEntry->file, myEntry->msg );
            else
                out( MSGINFO, dBase->dconf, "main(): Got an empty element from Queue!\n" );
      }
//        printQueue( *dBase->dconf, dBase->msgQueue );
    }
*/
    // tidy up
    out( MSGNOTICE, dBase->dconf, "main(): Cancelling threads & freeing memory...\n" );

    pthread_cancel( dBase->server->thread );
    close( dBase->server->sdesc );
    free( dBase->server );

    destroyWatchersStructure( dBase->watcher );
    destroyClientsStructure( dBase->clients );

    destroyQueue( dBase->msgQueue );

    destroyLogfilesConfig( dBase->dconf, dBase->lfconf );
    destroyDaemonConfig( dBase->dconf );

    pthread_mutex_destroy( &dBase->mutex );
    free( dBase );

    // exit
    return EXIT_SUCCESS;
}
