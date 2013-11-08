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


// function for client socket thread
void* client( void *param )
{
    // cast the given parameter to clientInfo
    struct clientInfo *clientinfo = ((clientInfo*)(param));


    // is the given client info structure valid
    if( clientinfo )
    {
        // receive buffer
        char line[MAX_LINE_LEN];
        int bytes_read = 0, exit = 0;
        // to get from queue
        struct entry *tmpEntry = malloc( sizeof(struct entry) );
        // timeout for receive
        struct timeval tv;
        tv.tv_sec = clientinfo->dconf->clircvtimout;
        tv.tv_usec = 0;
        // for connection status
        struct tcp_info tcpInfo;
        socklen_t tcpInfoLen = sizeof( tcpInfo );


        out( MSGNOTICE, clientinfo->dconf, "client(): %ld Starting thread with socket %d @ (%s:%d)\n", \
             pthread_self(), clientinfo->sdesc, inet_ntoa(clientinfo->addr.sin_addr), ntohs(clientinfo->addr.sin_port) );

        // add the new client to clients structure
        if( addClient(clientinfo->dconf, clientinfo->threads, pthread_self(), clientinfo->sdesc) == EXIT_SUCCESS )
        {
            out( MSGDEBUG, clientinfo->dconf, "client(): %ld Succesfully added client to clients structure\n", pthread_self() );
        }
        else
        {
            out( MSGERROR, clientinfo->dconf, "client(): %ld Failed to add client, stopping it\n", pthread_self() );

            // free memory from watchers argument
            free( clientinfo );
            // free temporary entry
            free( tmpEntry );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }


        // send buffer
        char *bytes;
        if( (bytes = malloc(MAX_BUF_SIZE*sizeof(char))) == NULL )
        {
            out( MSGERROR, clientinfo->dconf, "client(): %ld Error allocating memory for the network buffer\n", pthread_self() );
            exit = 1;
        }

        // set client receive timeout as socket option
        if( setsockopt(clientinfo->sdesc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv) )
        {
            out( MSGERROR, clientinfo->dconf, "client(): $ld Can't set socket options in thread with socket %d, terminating...\n", pthread_self(), clientinfo->sdesc );
            exit = 1;
        }

        // do client stuff
        while( exit != 1 )
        {
            // if there is a new element in the queue...
            if( hasQueueEntries(clientinfo->msgQueue) != EXIT_FAILURE )
            {
                do
                {
                    // lock mutex for ?=??=? of clients structure
                    if( pthread_mutex_lock(clientinfo->mutex) == 0 )
                    {
                        out( MSGDBGEX, clientinfo->dconf, "client(): %ld Locked the mutex of the clients info\n", pthread_self() );
                    }
                    else
                    {
                        out( MSGERROR, clientinfo->dconf, "client(): %ld Error locking the mutex of the clients info\n", pthread_self() );

                        // unlock mutex ?=??=? clients structure
                        if( pthread_mutex_unlock(clientinfo->mutex) == 0 )
                        {
                            out( MSGDBGEX, clientinfo->dconf, "client(): %ld Unlocked the mutex of the clients info\n", pthread_self() );
                        }
                        break;
                    }

                    // the entry gets filled in getFromQueue(), mark as empty
                    tmpEntry->reads = EMPTYELEM;
                    // allocate space for strings
                    if( (tmpEntry->file = malloc(clientinfo->dconf->maxlinelength)) == NULL )
                    {
                        out( MSGERROR, clientinfo->dconf, "client(): %ld Error allocating memory for temp-entries file string\n", pthread_self() );

                        // unlock mutex ?=??=? clients structure
                        if( pthread_mutex_unlock(clientinfo->mutex) == 0 )
                        {
                            out( MSGDBGEX, clientinfo->dconf, "client(): %ld Unlocked the mutex of the clients info\n", pthread_self() );
                        }
                        break;
                    }
                    if( (tmpEntry->msg = malloc(clientinfo->dconf->maxlinelength)) == NULL )
                    {
                        out( MSGERROR, clientinfo->dconf, "client(): %ld Error allocating memory for temp-entries message string\n", pthread_self() );

                        // free temporary allocated memory
                        free( tmpEntry->file );
                        // unlock mutex ?=??=? clients structure
                        if( pthread_mutex_unlock(clientinfo->mutex) == 0 )
                        {
                            out( MSGDBGEX, clientinfo->dconf, "client(): %ld Unlocked the mutex of the clients info\n", pthread_self() );
                        }
                        break;
                    }

                    // ...get it and...
                    if( getFromQueue(clientinfo->dconf, clientinfo->msgQueue, clientinfo->threads, clientinfo->msgQueue->first, pthread_self(), tmpEntry) == EXIT_SUCCESS )
                    {
                        out( MSGDBGEX, clientinfo->dconf, "client(): %ld got an element from queue\n", pthread_self() );

                        // ...convert it and...
                        if( entry2bytes(clientinfo->dconf, *tmpEntry, bytes) == EXIT_SUCCESS )
                        {
//                                // if the socket is still connected
                                if( getsockopt(clientinfo->sdesc, SOL_TCP, TCP_INFO, (void *)&tcpInfo, &tcpInfoLen ) == 0 )
                                {
                                    if( tcpInfo.tcpi_state < TCP_CA_Loss )
                                    {
                                        out( MSGDBGEX, clientinfo->dconf, "client(): %ld going to send the queue element\n", pthread_self() );

                                        // ...send it!
                                        if( send(clientinfo->sdesc, bytes, strlen(bytes), 0) < 0 )
                                        {
                                            out( MSGWARN, clientinfo->dconf, "client(): %ld failed to send the queue element\n", pthread_self() );

                                            // maybe the connection got closed, exit loop
                                            tmpEntry->reads = EMPTYELEM;
                                        }
                                        else
                                        {
                                            out( MSGDBGEX, clientinfo->dconf, "client(): %ld successfully send the queue element\n", pthread_self() );
                                        }
                                    }
                                    else
                                    {
                                        out( MSGERROR, clientinfo->dconf, "client(): %ld Error while checking if connection is alive\n", pthread_self() );

                                        // maybe the connection got closed, exit loop
                                        tmpEntry->reads = EMPTYELEM;
                                    }
                                }
                                else
                                {
                                    out( MSGERROR, clientinfo->dconf, "client(): %ld Error while checking socket options\n", pthread_self() );

                                    // maybe the connection got closed, exit loop
                                    tmpEntry->reads = EMPTYELEM;
                                }
                        }
                        else
                        {
                            out( MSGWARN, clientinfo->dconf, "client(): %ld failed to get a bytebuffer from entry\n", pthread_self() );
                        }
                    }

                    // unlock mutex ?=??=? clients structure
                    if( pthread_mutex_unlock(clientinfo->mutex) == 0 )
                    {
                        out( MSGDBGEX, clientinfo->dconf, "client(): %ld Unlocked the mutex of the clients info\n", pthread_self() );
                    }
                    else
                    {
                        out( MSGERROR, clientinfo->dconf, "client(): %ld Error unlocking the mutex of the clients info\n", pthread_self() );

                        // free temporary allocated memory
                        free( tmpEntry->file );
                        free( tmpEntry->msg );

                        // close thread with failure
                        pthread_exit( (void*)EXIT_FAILURE );
                    }

                    // free temporary allocated memory
                    free( tmpEntry->file );
                    free( tmpEntry->msg );
                }
                while( tmpEntry->reads != EMPTYELEM );
            }


            // wait for incoming packets
            bytes_read = recvfrom( clientinfo->sdesc, line, sizeof(line), 0, (struct sockaddr*)&clientinfo->addr, &clientinfo->slen );
            if( bytes_read > 0 )
            {
//                out( MSGDEBUG, clientinfo->dconf, "client(): %ld is sending to client %s@%d: %s\n", pthread_self(), line, clientinfo->addr, clientinfo->slen ); // TODO oder auch net
                sendto( clientinfo->sdesc, line, bytes_read, 0, (struct sockaddr*)&clientinfo->addr, clientinfo->slen );
            }
            else if( bytes_read < 0 )
            {
                out( MSGINFO, clientinfo->dconf, "client(): %ld RECV-Timeout in thread with socket %d @ (%s:%d): %d\n", \
                     pthread_self(), clientinfo->sdesc, inet_ntoa(clientinfo->addr.sin_addr), ntohs(clientinfo->addr.sin_port), bytes_read );
            }
            else
            {
                out( MSGNOTICE, clientinfo->dconf, "client(): %ld Connection closed by client with socket %d: %d\n", pthread_self(), clientinfo->sdesc, bytes_read );
                exit = 1;
            }
        } //while( exit != 1 )

        // free network byte buffer
        free( bytes );
        // free temporary entry
        free( tmpEntry );

        // remove client from clients structure
        if( removeClient(clientinfo->dconf, clientinfo->threads, pthread_self()) != EXIT_SUCCESS )
        {
            out( MSGERROR, clientinfo->dconf, "client(): %ld Error removing client thread with socket %d from clients structure\n", pthread_self(), clientinfo->sdesc );

            // free memory from clients argument
            free( clientinfo );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }
    }
    else
    {
        out( MSGCRIT, clientinfo->dconf, "client(): %ld The given client info structure is not valid \n", pthread_self() );

        // close thread
        pthread_exit( (void*)EXIT_FAILURE );
    }

    out( MSGINFO, clientinfo->dconf, "client(): %ld Closing client thread with socket %d\n", pthread_self(), clientinfo->sdesc );

    // free memory from clients argument
    free( clientinfo );

    // close thread with success
    pthread_exit( (void*)EXIT_SUCCESS );
}

// function for server socket thread
void* serve( void *daemonbase )
{
    // cast the given parameter
    struct daemonBase *base = ((daemonBase*)(daemonbase));

    // is the given daemon base structure valid
    if( base )
    {
        // temporary client descriptor
        int tmpCliDesc;
        // temporary threadt
        pthread_t tmpThread;
        // temporary client argument
        struct clientInfo *info;


        // do serving stuff
        while( 1 )
        {
            // accept incoming client connection...
            tmpCliDesc = accept( base->server->sdesc, (struct sockaddr*)&base->server->addr, &base->server->slen );

            // ...just if the maximum number of clients is not connected already
            if( tmpCliDesc != -1 && base->dconf->maxnumclients > base->clients->num )
            {
                out( MSGNOTICE, base->dconf, "serve(): Connected: %s:%d\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );

                // lock mutex for manipulation of clients structure
                if( pthread_mutex_lock(&base->clients->mutex) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "serve(): Locked the mutex of the clients structure\n" );
                }
                else
                {
                    out( MSGERROR, base->dconf, "serve(): Error locking the mutex of the clients structure \n" );

                    // close server socket
                    close( base->server->sdesc );

                    // close thread with failure
                    pthread_exit( (void*)EXIT_FAILURE );
                }


                out( MSGDEBUG, base->dconf, "serve(): Building the clients info structure for the client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );

                // build a structure with by client needed informations
                if( (info = malloc(sizeof(clientInfo))) == NULL )
                {
                    out( MSGERROR, base->dconf, "serve(): Error allocating memory for temporary client info\n" );

                    // close server socket
                    close( base->server->sdesc );

                    // close thread with failure
                    pthread_exit( (void*)EXIT_FAILURE );
                }
                info->dconf = base->dconf;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added dconf for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );
                info->sdesc = tmpCliDesc;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added sdesc for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );
                info->addr = base->server->addr;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added saddr for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );
                info->slen = base->server->slen;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added slen for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );
                info->threads = base->clients;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added cthreads for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );
                info->msgQueue = base->msgQueue;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added queue for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );
                info->mutex = &base->server->mutex;
                out( MSGDEBUG, base->dconf, "serve(): Build a client info - added mutex for client (%s:%d)\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );


                // create a thread for the new client
                if( pthread_create(&tmpThread, NULL, client, info) != 0 )
                {
                    out( MSGERROR, base->dconf, "serve(): Error creating client thread\n" );
                    pthread_cancel( tmpThread );

                    // unlock mutex of clients structure
                    if( pthread_mutex_unlock(&base->clients->mutex) == 0 )
                    {
                        out( MSGDEBUG, base->dconf, "serve(): Unlocked the mutex of the clients structure\n" );
                    }
                    else
                    {
                        out( MSGERROR, base->dconf, "serve(): Error unlocking the mutex of the clients structure \n" );

                        // close server socket
                        close( base->server->sdesc );

                        // close thread with failure
                        pthread_exit( (void*)EXIT_FAILURE );
                    }
                }
                else
                {
                    out( MSGNOTICE, base->dconf, "serve(): Created a client thread for %d @ %s:%d\n", tmpCliDesc, inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );

                    // disassociate client from parent/server-thread, num is here +1
                    if( pthread_detach(tmpThread) == 0 )
                    {
                        out( MSGDEBUG, base->dconf, "serve(): Detached the client thread from parent\n" );
                    }
                    else
                    {
                        out( MSGERROR, base->dconf, "serve(): Error detaching the watcher thread from parent\n" );
                    }

                    // unlock mutex of clients structure
                    if( pthread_mutex_unlock(&base->clients->mutex) == 0 )
                    {
                        out( MSGDEBUG, base->dconf, "serve(): Unlocked the mutex of the clients structure\n" );
                    }
                    else
                    {
                        out( MSGERROR, base->dconf, "serve(): Error unlocking the mutex of the clients structure \n" );

                        // close server socket
                        close( base->server->sdesc );

                        // close thread with failure
                        pthread_exit( (void*)EXIT_FAILURE );
                    }
                }
            }
            else
            {
                out( MSGNOTICE, base->dconf, "serve(): Connection rejected %s:%d - maximum connections reached\n", inet_ntoa(base->server->addr.sin_addr), ntohs(base->server->addr.sin_port) );

                // close "rejected" socket
                close( tmpCliDesc );
            }
        }

        out( MSGNOTICE, base->dconf, "serve(): Closing server thread %ld with socket %d\n", pthread_self(), base->server->sdesc );


        // lock mutex for manipulation of base structure
        if( pthread_mutex_lock(&base->mutex) == 0 )
        {
            out( MSGDEBUG, base->dconf, "serve(): Locked the mutex of the base structure\n" );
        }
        else
        {
            out( MSGERROR, base->dconf, "serve(): Error locking the mutex of the base structure \n" );

            // close server socket
            close( base->server->sdesc );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }

        // close server socket
        out( MSGDEBUG, base->dconf, "serve(): Closing the server socket\n" );
        close( base->server->sdesc );

        // unlock mutex of base structure
        if( pthread_mutex_unlock(&base->mutex) == 0 )
        {
            out( MSGDEBUG, base->dconf, "serve(): Unlocked the mutex of the base structure\n" );

            // close thread with success
            pthread_exit( (void*)EXIT_SUCCESS );
        }
        else
        {
            out( MSGERROR, base->dconf, "serve(): Error unlocking the mutex of the base structure \n" );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }
    }
    else
    {
        out( MSGCRIT, base->dconf, "serve(): The given base structure is not valid \n" );

        // close thread with failure
        pthread_exit( (void*)EXIT_FAILURE );
    }
}

// function to open a socket connection
int createServerSocket( struct daemonBase *base )
{
    // is the given daemon base structure valid
    if( base->server )
    {
        // lock mutex for manipulation of base structure
        if( pthread_mutex_lock(&base->mutex) == 0 )
        {
            out( MSGDEBUG, base->dconf, "createServerSocket(): Locked the mutex of the base structure\n" );
        }
        else
        {
            out( MSGERROR, base->dconf, "createServerSocket(): Error locking the mutex of the base structure \n" );
            // return with failure
            return EXIT_FAILURE;
        }

        // initialize mutex of server structure
        if( pthread_mutex_init(&base->server->mutex, NULL) == 0 )
        {
            out( MSGDEBUG, base->dconf, "createServerSocket(): Initialised the mutexof the server structure\n" );
        }
        else
        {
            out( MSGERROR, base->dconf, "createServerSocket(): Error initialising the mutex of the server structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        if( strstr(base->dconf->proto, "tcp") )
        {
            out( MSGDEBUG, base->dconf, "createServerSocket(): Protocol is TCP (%s)...\n", base->dconf->proto );
            // create a tcp server socket
            if( (base->server->sdesc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
            {
                out( MSGERROR, base->dconf, "createServerSocket(): Failed to create tcp server socket\n" );

                // unlock mutex of base structure
                if( pthread_mutex_unlock(&base->mutex) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
                }
                else
                {
                    out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
                }

                // return with failure
                return EXIT_FAILURE;
            }
        }
//         else if( strstr(base->dconf->proto, "udp") )
//         {
//             out( MSGDEBUG, base->dconf, "createServerSocket(): Protocol is UDP (%s)...\n", base->dconf->proto );
//             // create a udp server socket
//             if( (base->server->sdesc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
//             {
//                 out( MSGERROR, base->dconf, "createServerSocket(): Failed to create udp server socket\n" );
// 
//                 // unlock mutex of base structure
//                 if( pthread_mutex_unlock(&base->mutex) == 0 )
//                 {
//                     out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
//                 }
//                 else
//                 {
//                     out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
//                 }
// 
//                 // return with failure
//                 return EXIT_FAILURE;
//             }
//         }
//         else if( strstr(base->dconf->proto, "ssl") )
//         {
//             out( MSGDEBUG, base->dconf, "createServerSocket(): Protocol is SSL (%s)...\n", base->dconf->proto );
//             // create a ssl server socket
// //            if( (base->server->sdesc = socket(PF_INET, SOCK_SSL, IPPROTO_SSL)) < 0 )
//             {
//                 out( MSGERROR, base->dconf, "createServerSocket(): Failed to create ssl server socket\n" );
//                 out( MSGERROR, base->dconf, "createServerSocket(): At the moment no ssl server socket option is available!!!\n" );
// 
//                 // unlock mutex of base structure
//                 if( pthread_mutex_unlock(&base->mutex) == 0 )
//                 {
//                     out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
//                 }
//                 else
//                 {
//                     out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
//                 }
// 
//                 // return with failure
//                 return EXIT_FAILURE;
//             }
//         }
        else
	{
	      out( MSGERROR, base->dconf, "createServerSocket(): Failed to create >%s< server socket\n", base->dconf->proto );
	      out( MSGERROR, base->dconf, "createServerSocket(): At the moment only tcp server socket option is available!!!\n" );

	      // unlock mutex of base structure
	      if( pthread_mutex_unlock(&base->mutex) == 0 )
	      {
		  out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
	      }
	      else
	      {
		  out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
	      }

	      // return with failure
	      return EXIT_FAILURE;
        }


        // configure server socket
        out( MSGDEBUG, base->dconf, "createServerSocket(): Configuring the servers %s socket\n", base->dconf->proto );
        base->server->addr.sin_family = AF_INET;
        base->server->addr.sin_addr.s_addr = INADDR_ANY;
        base->server->addr.sin_port = htons( base->dconf->port );
        base->server->slen = sizeof(base->server->addr);


        // bind socket to address
        if( bind(base->server->sdesc, (struct sockaddr*)&base->server->addr, base->server->slen) != 0 )
        {
            out( MSGERROR, base->dconf, "createServerSocket(): Failed to bind server socket to port %d\n", ntohs(base->server->addr.sin_port) );

            // unlock mutex of base structure
            if( pthread_mutex_unlock(&base->mutex) == 0 )
            {
                out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
            }
            else
            {
                out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
            }
            // return with failure
            return EXIT_FAILURE;
        }
        else
        {
            // if bind is successfull, listen on socket
            if( listen(base->server->sdesc, MAX_NUM_CLIENTS) != 0 )
            {
                out( MSGERROR, base->dconf, "createServerSocket(): Failed listening on socket %d\n", base->server->sdesc );

                // unlock mutex of base structure
                if( pthread_mutex_unlock(&base->mutex) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
                }
                else
                {
                    out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
                }
                // return with failure
                return EXIT_FAILURE;
            }
            else
            {
                // if listen is successfull, start server thread
                if( pthread_create(&base->server->thread, NULL, serve, base) != 0 )
                {
                    out( MSGERROR, base->dconf, "createServerSocket(): Failed to create server thread %d\n", base->server->thread );

                    // unlock mutex of base structure
                    if( pthread_mutex_unlock(&base->mutex) == 0 )
                    {
                        out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );
                    }
                    else
                    {
                        out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );
                    }
                    // return with failure
                    return EXIT_FAILURE;
                }
                else
                {
                    out( MSGNOTICE, base->dconf, "createServerSocket(): Started server thread; listening on %s port %d @ %x\n", \
                         base->dconf->proto, ntohs(base->server->addr.sin_port), base->server->addr.sin_addr.s_addr );

                    // disassociate server thread from parent/main-process
                    if( pthread_detach(base->server->thread) == 0 )
                    {
                        out( MSGDEBUG, base->dconf, "createServerSocket(): Detached the server listener thread from parent\n" );
                    }
                    else
                    {
                        out( MSGERROR, base->dconf, "createServerSocket(): Error detaching the watcher thread from parent\n" );
                    }

                    // unlock mutex of base structure
                    if( pthread_mutex_unlock(&base->mutex) == 0 )
                    {
                        out( MSGDEBUG, base->dconf, "createServerSocket(): Unlocked the mutex of the base structure\n" );

                        // return with success
                        return EXIT_SUCCESS;
                    }
                    else
                    {
                        out( MSGERROR, base->dconf, "createServerSocket(): Error unlocking the mutex of the base structure \n" );

                        // return with failure
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }
    else
    {
        out( MSGCRIT, base->dconf, "createServerSocket(): The given base/server structure is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }
}


// function to create a client threads structure
int createClientsStructure( struct daemonConfig *config, struct clientThreads *newClients )
{
    // is the given client threads structure valid?
    if( newClients )
    {
        out( MSGINFO, config, "createClientsStructure(): Creating a new client threads structure\n" );

        // allocate space for threads
        newClients->sdescs = malloc( sizeof(int[1]) );
        newClients->threads = malloc( sizeof(pthread_t[1]) );
        newClients->num = 0;


        // initialize mutex of clients structure
        if( pthread_mutex_init(&newClients->mutex, NULL) == 0 )
        {
            out( MSGDEBUG, config, "createClientsStructure(): Initialised the mutexof the clients structure\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "createClientsStructure(): Error initialising the mutex of the clients structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "createClientsStructure(): The given client threads structure is not valid\n" );

        // return with failure
        return EXIT_FAILURE;
    }
}

// function to unallocate the memory of a client threads structure
void destroyClientsStructure( struct clientThreads *clients )
{
    int i;
    for( i = 0; i < clients->num; i++ )
    {
        pthread_cancel( clients->threads[i] );
        close( clients->sdescs[i] );
    }
    pthread_mutex_destroy( &clients->mutex );
    free( clients->sdescs );
    free( clients->threads );
    free( clients );
}


// function to add a client to clients structure
int addClient( struct daemonConfig *config, struct clientThreads *threads, pthread_t newThread, int newSdesc )
{
    // only to a valid client threads structure
    if( threads )
    {
        // temporary counter
        int i;

        // lock mutex for manipulation of clients structure
        if( pthread_mutex_lock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "addClient(): Locked the mutex of the clients structure\n" );
        }
        else
        {
            out( MSGERROR, config, "addClient(): Error locking the mutex of the clients structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }


        out( MSGINFO, config, "addClient(): Going to add a new client as %d. to clients structure with %d\n", threads->num+1, newSdesc );

        // temporary structure for threads
        out( MSGDEBUG, config, "addClient(): Instantiating and allocating a temporary clients structure\n" );
        struct clientThreads tmpThreads;
        if( (tmpThreads.sdescs = malloc(sizeof(int[threads->num+1]))) == NULL )
        {
            out( MSGERROR, config, "addClient(): Error allocating memory for temporary client descriptors\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (tmpThreads.threads = malloc(sizeof(pthread_t[threads->num+1]))) == NULL )
        {
            out( MSGERROR, config, "addClient(): Error allocating memory for temporary client threadts\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy all threads to temporary structure
        out( MSGDEBUG, config, "addClient(): Going to copy the %d entries to temp\n", threads->num );
        for( i = 0; i < threads->num; i++ )
        {
            out( MSGDEBUG, config, "addClient(): Copying number %d of the clients structure\n", i+1 );
            tmpThreads.sdescs[i] = threads->sdescs[i];
            tmpThreads.threads[i] = threads->threads[i];
        }


        // memory management for enlarging client threads structure
        out( MSGDEBUG, config, "addClient(): Enlarging the size of the clients structure to %d\n", threads->num+1 );
        free( threads->sdescs );
        free( threads->threads );
        if( (threads->sdescs = malloc(sizeof(int[threads->num+1]))) == NULL )
        {
            out( MSGERROR, config, "addClient(): Error allocating memory for enlarged client descriptors\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (threads->threads = malloc(sizeof(pthread_t[threads->num+1]))) == NULL )
        {
            out( MSGERROR, config, "addClient(): Error allocating memory for enlarged client threadts\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy back all thread infos from temp
        out( MSGDEBUG, config, "addClient(): Going to copy %d back from temp to clients structure\n", threads->num );
        for( i = 0; i < threads->num; i++ )
        {
            out( MSGDEBUG, config, "addClient(): Copying thread and socket of client %d\n", i+1 );
            threads->threads[i] = tmpThreads.threads[i];
            threads->sdescs[i] = tmpThreads.sdescs[i];
        }

        // free temporary allocated
        free( tmpThreads.sdescs );
        free( tmpThreads.threads );


        // add the new thread to watcher structure
        threads->threads[threads->num] = newThread;
        threads->sdescs[threads->num] = newSdesc;
        out( MSGDEBUG, config, "addClient(): Added client thread %ld with socket %d as %d\n", newThread, newSdesc, threads->num+1 );

        // one thread was added so increment
        threads->num++;
        out( MSGDEBUG, config, "addClient(): Incrementing the number of connected clients to %d\n", threads->num );


        // unlock mutex of clients structure
        if( pthread_mutex_unlock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "addClient(): Unlocked the mutex of the clients structure\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "addClient(): Error locking the mutex of the clients structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "addClient(): The given client threads structure is not valid\n" );

        // return with failure
        return EXIT_FAILURE;
    }
}

// function to remove a client from clients structure
int removeClient( struct daemonConfig *config, struct clientThreads *threads, pthread_t oldThread )
{
    // only from a valid client threads structure
    if( threads )
    {
        // temporary counter
        int i, num, akt_thread = 0;
        // temporary socket descriptor
        int oldSdesc;


        // lock mutex for manipulation of clients structure
        if( pthread_mutex_lock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeClient(): Locked the mutex of the clients structure\n" );
        }
        else
        {
            out( MSGERROR, config, "removeClient(): Error locking the mutex of the clients structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }


        out( MSGINFO, config, "removeClient(): Going to remove a client from clients structure\n" );

        // don't let the arrays be initialised with < 0
        if( threads->num > 1 )
        {
            num = threads->num-1;
        }
        else
        {
            num = 1;
        }

        out( MSGDEBUG, config, "removeClient(): Instantiating and allocating a temporary, smaller clients structure\n" );

        // temporary structure for threads
        struct clientThreads tmpThreads;
        if( (tmpThreads.sdescs = malloc(sizeof(int[num]))) == NULL )
        {
            out( MSGERROR, config, "removeClient(): Error allocating memory for temporary client descriptors\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (tmpThreads.threads = malloc(sizeof(pthread_t[num]))) == NULL )
        {
            out( MSGERROR, config, "removeClient(): Error allocating memory for temporary client threadts\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy all threads except the one to remove
        out( MSGDEBUG, config, "removeClient(): Going to copy the old entries to temp\n" );
        for( i = 0; i < threads->num; i++ )
        {
            if( threads->threads[i] != oldThread )
            {
                out( MSGDEBUG, config, "removeClient(): Copying number %d of the clients structure\n", i+1 );
                tmpThreads.sdescs[akt_thread] = threads->sdescs[i];
                tmpThreads.threads[akt_thread] = threads->threads[i];
                akt_thread++;
            }
            else
            {
                oldSdesc = threads->sdescs[i];
            }
        }


        // memory management for redurcing client threads structure
        out( MSGDEBUG, config, "removeClient(): Reducing the size of the clients structure to %d\n", num );
        free( threads->sdescs );
        free( threads->threads );
        if( (threads->sdescs = malloc(sizeof(int[num]))) == NULL )
        {
            out( MSGERROR, config, "removeClient(): Error allocating memory for reduced client descriptors\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (threads->threads = malloc(sizeof(pthread_t[num]))) == NULL )
        {
            out( MSGERROR, config, "removeClient(): Error allocating memory for reduced client threadts\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy back all remaining client thread infos
        out( MSGDEBUG, config, "removeClient(): Going to copy back from temp to clients structure\n" );
        for( i = 0; i < threads->num-1; i++ )
        {
            out( MSGDEBUG, config, "removeClient(): Copying thread and socket of client %d\n", i+1 );
            threads->sdescs[i] = tmpThreads.sdescs[i];
            threads->threads[i] = tmpThreads.threads[i];
        }

        // free temporary allocated
        free( tmpThreads.sdescs );
        free( tmpThreads.threads );


        // one thread will be stopped so descrement
        threads->num--;
        out( MSGDEBUG, config, "removeClient(): Decremented the number of connected clients to %d\n", threads->num );


        // unlock mutex of clients structure
        if( pthread_mutex_unlock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeClient(): Unlocked the mutex of the clients structure\n" );
        }
        else
        {
            out( MSGERROR, config, "removeClient(): Error unlocking the mutex of the clients structure \n" );
        }


        // close client socket
        out( MSGDEBUG, config, "removeClient(): Closing the clients socket: %ld\n", oldThread );
        close( oldSdesc );

        // return with success
        return EXIT_SUCCESS;
    }
    else
    {
        out( MSGCRIT, config, "removeClient(): The given client threads structure is not valid\n" );

        // return with failure
        return EXIT_FAILURE;
    }
}


// fcuntion to convert queue element to byte stream
int entry2bytes( struct daemonConfig *config, struct entry element, char *bytes )
{
    // is the bytes buffer valid?
    if( bytes )
    {
        out( MSGINFO, config, "entry2bytes(): Converting queue element to byte buffer\n" );
        //add packet separator
        bytes[0] = '|';
        bytes[1] = '!';
        bytes[2] = '|';
        // add 48 to send the type number as ascii
        bytes[3] = element.type + 48;
        bytes[4] = '\0';
        // add a separator
        strcat( bytes, "|:|" );
        // add the filename/specified name
        strcat( bytes, element.file );
        // add a separator
        strcat( bytes, "|:|" );
        // add the message
        strcat( bytes, element.msg );
        // add packet separator
        strcat( bytes, "|!|" );

        // return with success
        return EXIT_SUCCESS;
    }
    else
    {
        out( MSGCRIT, config, "entry2bytes(): The given byte buffer is not valid\n" );
        // return with failure
        return EXIT_FAILURE;
    }
}
