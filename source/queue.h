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

#ifndef _queue_
#define _queue_

#include "globals.h"

#define EMPTYELEM -123          // indicator for an empty queue element


// structure for message queue entries
typedef struct entry{
    char *file;                         // the given name of the logfile (or path)
    char type;                          // the message type of the message
    char *msg;                          // the message itself
    int reads;                          // number of reads by clients of the entry
    pthread_t *threads;                 // thread ids of clients which read the entry
    struct entry *next;                 // reference to the next entry in queue
    struct entry *prev;                 // reference to the previous entry in queue
} entry;

// structure of the message queue
typedef struct messageQueue {
    struct entry *first;                // reference to the first entry in queue
    struct entry *last;                 // reference to the last entry in queue
    int num;                            // actual number of entries in queue
    pthread_mutex_t mutex;              // mutex for the messsage queue
} messageQueue;


// function to create a message queue
int createQueue( struct daemonConfig *config, struct messageQueue *newQueue );

// function to unallocate a message queue
void destroyQueue( struct messageQueue *msgList );

// function for checking if an entry is available
int hasQueueEntries( struct messageQueue *msgList );


// function to add an entry to message queue
int addToQueue( struct daemonConfig *config, struct messageQueue *msgList, char type, char *file, char *msg );

// function to get an entry from message queue
int getFromQueue( struct daemonConfig *config, struct messageQueue *msgList, struct clientThreads *threads, struct entry *startEntry, pthread_t thread, struct entry *retEntry );

// function to remove an entry from message queue
int removeFromQueue( struct daemonConfig *config, struct messageQueue *msgList, struct clientThreads *threads, struct entry *element );


// function to print out one queue entry
void printQueueElement( struct daemonConfig *config, struct entry *element);

// function to print out the full queue content
void printQueue( struct daemonConfig *config, struct messageQueue *msgList );


#endif
