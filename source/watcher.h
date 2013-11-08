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

#ifndef _watcher_
#define _watcher_

#include "globals.h"


// structure to give a client thread
typedef struct watcherInfo{
    struct daemonConfig *dconf;         // path to the daemons configuration file
    struct logfileConfig *lfconf;       // the logfile configuration for the watcher
    struct watcherThreads *threads;     // reference to the actual running watchers
    struct messageQueue *msgQueue;      // reference to the message queue
    pthread_mutex_t *mutexW;            // mutex for the watchers logfile
    pthread_mutex_t *mutexL;            // mutex for the logfiles
} watcherInfo;

// structure for watcher thread ids
typedef struct watcherThreads{
    int num;                            // actual number of watcher threads
    pthread_t *threads;                 // references to running watcher threads
    pthread_mutex_t mutex;              // mutex for the watcher threads structure
} watcherThreads;


// function for watcher thread
void* watch( void *param );

// function to start a watcher thread
int createWatcher( struct daemonBase *base );


// function to create a watcher threads structure
int createWatchersStructure( struct daemonConfig *config, struct watcherThreads *newWatcher );

// function to unallocate the memory of a watcher threads structure
void destroyWatchersStructure( struct watcherThreads *watcher );


// function to add a watcher to watcher structure
int addWatcher( struct daemonConfig *config, struct watcherThreads *threads, pthread_t newThread );

// function to remove a watcher from watcher structure
int removeWatcher( struct daemonConfig *config, struct watcherThreads *threads, pthread_t oldThread );


#endif
