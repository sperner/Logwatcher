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

#ifndef _net_
#define _net_

#include "globals.h"


// structure to give a client thread
typedef struct clientInfo{
    struct daemonConfig *dconf;         // path to the daemons configuration file
    int sdesc;                          // the clients own socket descriptor
    struct sockaddr_in addr;            // the clients socket address configuration
    socklen_t slen;                     // the clients length of sockaddr_in
    pthread_t thread;                   // the clients own thread reference
    struct clientThreads *threads;      // reference to the actual running clients
    struct messageQueue *msgQueue;      // reference to the message queue
    pthread_mutex_t *mutex;             // mutex for the client/server
} clientInfo;

// structure for client thread ids
typedef struct clientThreads{
    int num;                            // actual number of client threads
    int *sdescs;                        // reference to running client socket descriptors
    pthread_t *threads;                 // reference to running client threads
    pthread_mutex_t mutex;              // mutex for the client threads structure
} clientThreads;

// structure for socket connection
typedef struct serverSocket{
    int sdesc;                          // the servers own socket descriptor
    pthread_t thread;                   // the servers own thread reference
    struct sockaddr_in addr;            // the servers socket address configuration
    socklen_t slen;                     // the servers length of sockaddr_in
    pthread_mutex_t mutex;              // mutex for the server/client
} serverSocket;


// function for client socket thread
void* client( void *clientinfo );

// function for server socket thread
void* serve( void *daemonbase );

// function to open a socket connection
int createServerSocket( struct daemonBase *base );


// function to create a client threads structure
int createClientsStructure( struct daemonConfig *config, struct clientThreads *newClients );

// function to unallocate the memory of a client threads structure
void destroyClientsStructure( struct clientThreads *clients );


// function to add a client to clients structure
int addClient( struct daemonConfig *config, struct clientThreads *threads, pthread_t newThread, int newSdesc );

// function to remove a client from clients structure
int removeClient( struct daemonConfig *config, struct clientThreads *threads, pthread_t oldThread );


// fcuntion to convert queue element to byte stream
int entry2bytes( struct daemonConfig *config, struct entry element, char *bytes );


#endif
