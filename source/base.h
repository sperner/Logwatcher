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

#ifndef _base_
#define _base_

#include "globals.h"


// structure for servers base variables
typedef struct daemonBase{
    struct daemonConfig *dconf;         // reference to the daemons configuration
    struct logfilesConfig *lfconf;      // reference to the logfiles configuration
    struct serverSocket *server;        // reference to the server socket
    struct clientThreads *clients;      // reference to the client threads
    struct watcherThreads *watcher;     // reference to the watcher threads
    struct messageQueue *msgQueue;      // reference to the message queue
    pthread_mutex_t mutex;              // mutex of the daemon base
} daemonBase;


// function to create a base structure
int createBase( struct daemonConfig *config, struct daemonBase *newBase );


// function to run logwatcher as a daemon process
int runAsDaemon( struct daemonConfig *config );


// function to print the command line usage
void usage( struct daemonConfig *config );


#endif
