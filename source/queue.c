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


// function to create a message queue
int createQueue( struct daemonConfig *config,  messageQueue *newQueue )
{
    // is the given message queue structure valid
    if( newQueue )
    {
        out( MSGINFO, config, "createQueue(): Creating a new message queue\n" );

        // in the queue can't be a first or a last entry
        newQueue->first = NULL;
        newQueue->last = NULL;
        newQueue->num = 0;

        // initialize mutex of queue
        if( pthread_mutex_init(&newQueue->mutex, NULL) == 0 )
        {
            out( MSGDEBUG, config, "createQueue(): Initialised the mutexof the message queue\n" );
        }
        else
        {
            out( MSGERROR, config, "createQueue(): Error initialising the mutex of the message queue\n" );

            // return with failure
            return EXIT_FAILURE;
        }

        // initialized the message queue
        out( MSGDEBUG, config, "createQueue(): Successfully created a new message queue\n" );
    }
    else
    {
        out( MSGCRIT, config, "createQueue(): The given message queue structure is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }

    // return with success
    return EXIT_SUCCESS;
}

// function to unallocate a message queue
void destroyQueue( struct messageQueue *msgList )
{
    // is a valid queue with at least one element given?
    if( msgList->first )
    {
        // get the first entry in queue
        struct entry *tmpEntry = msgList->first;
        struct entry *freeEntry;

        // for every entry in the queue...
        do{
            if( tmpEntry )
            {
                freeEntry = tmpEntry;
                free( tmpEntry->file );
                free( tmpEntry->msg );
                free( tmpEntry->threads );
                tmpEntry = tmpEntry->next;
                free( freeEntry );
            }
        }while( tmpEntry != NULL );

        free( tmpEntry );
    }

    // delete the mutex of the queue
    pthread_mutex_destroy( &msgList->mutex );

    // delete the message queue
    free( msgList );
}

// function for checking if an element is available
int hasQueueEntries( struct messageQueue *msgList )
{
    // lock mutex for manipulation of message queue
    pthread_mutex_lock( &msgList->mutex );

    // if the first element points to NULL there is no entry available
    if( msgList->first != NULL )
    {
        // unlock mutex of message queue
        pthread_mutex_unlock( &msgList->mutex );

        // queue has at least one entry, exit with success
        return EXIT_SUCCESS;
    }
    else
    {
        // unlock mutex of message queue
        pthread_mutex_unlock( &msgList->mutex );

        // queue has no entries, exit with failure
        return EXIT_FAILURE;
    }
}


// function to add an entry to a message queue
int addToQueue( struct daemonConfig *config,  struct messageQueue *msgList, char type, char *file, char *msg )
{
    // only a valid entry should be added to a valid queue
    if( msgList && file && msg )
    {
        out( MSGINFO, config, "addToQueue(): Trying to add an entry to message queue with %d entries\n", msgList->num );


        // temporary element for inserting
        struct entry *element;
        if( (element = malloc(sizeof(struct entry))) == NULL )
        {
            out( MSGERROR, config, "addToQueue(): Error allocating memory for the temporary element\n" );

            // return with failure
            return EXIT_FAILURE;
        }

        // allocate and copy file string
        if( (element->file = malloc(strlen(file)+1)) == NULL )
        {
            out( MSGERROR, config, "addToQueue(): Error allocating memory for the filename of the element\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (strcpy(element->file, file)) == NULL )
        {
            out( MSGERROR, config, "addToQueue(): Error copying the filename to the new element\n" );

            // return with failure
            return EXIT_FAILURE;
        }

        // allocate and copy message string
        if( (element->msg = malloc(strlen(msg)+1)) == NULL )
        {
            out( MSGERROR, config, "addToQueue(): Error allocating memory for the mesage of the element\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        if( (strcpy(element->msg, msg)) == NULL )
        {
            out( MSGERROR, config, "addToQueue(): Error copying the message to the new element\n" );

            // return with failure
            return EXIT_FAILURE;
        }

        // set the type of the new entry
        element->type = type;

        // allocate space for readers array
        if( (element->threads = malloc(sizeof(pthread_t[config->maxnumclients]))) == NULL )
        {
            out( MSGERROR, config, "addToQueue(): Error allocating memory for the readers array\n" );

            // return with failure
            return EXIT_FAILURE;
        }
        else
        {
            int i;
            for( i = 0; i < config->maxnumclients; i++ )
            {
                element->threads[i] = 0;
            }
        }

        // the entry can't be read from any thread yet
        element->reads = 0;
        // there is no next element for the entry, be sure it is set to NULL
        element->next = NULL;

        out( MSGDBGEX, config, "addToQueue(): The temporary element has been packed successfully\n" );


        // lock mutex for manipulation of message queue
        if( pthread_mutex_lock(&msgList->mutex) == 0 )
        {
            out( MSGDEBUG, config, "addToQueue(): Locked the mutex of the message queue\n" );
        }
        else
        {
            out( MSGERROR, config, "addToQueue(): Error locking the mutex of the message queue \n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // append to message queue
        if( msgList->first == NULL && msgList->last == NULL )
        {
            out(MSGDEBUG, config,  "addToQueue(): Adding entry no %d to message queue, as first\n", msgList->num+1 );

            // there is no previous element
            element->prev = NULL;
            // insert the entry as first
            msgList->first = element;
            // insert the entry also as last
            msgList->last = element;
        }
        else
        {
            out(MSGDEBUG, config,  "addToQueue(): Adding entry no %d to message queue, as last\n", msgList->num+1 );

            // the last entry had a reference to next = NULL
            msgList->last->next = element;
            // the previous entry of element is the actual last in queue
            element->prev = msgList->last;
            // add the new element as last
            msgList->last = element;
        }

        // the queue got a new entry so increment the counter
        msgList->num++;

        // an entry was added, so increment element number
        out( MSGDEBUG, config, "addToQueue(): Inkrementing the number of messages in queue to %d\n", msgList->num );


        // unlock mutex of message queue
        if( pthread_mutex_unlock(&msgList->mutex) == 0 )
        {
            out( MSGDEBUG, config, "addToQueue(): Unlocked the mutex of the message queue\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "addToQueue(): Error locking the mutex of the message queue\n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "addToQueue(): The given message queue or the entry is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }
}

// function to get an entry from message queue
int getFromQueue( struct daemonConfig *config,  struct messageQueue *msgList, struct clientThreads *threads, struct entry *startEntry, pthread_t thread, struct entry *retEntry )
{
    // only a valid entry from a valid queue and a valid threads structure
    if( msgList && startEntry && threads && retEntry )
    {
        // mark as empty until it gets filled
        retEntry->reads = EMPTYELEM;

        // counter
        int akt_thread = 0;


        out( MSGINFO, config, "getFromQueue(): %ld Trying to get an element from message queue\n", thread );

        // if the thread already got it, get the next element
        for( akt_thread = 0; akt_thread < config->maxnumclients; akt_thread++ )
        {
            if( pthread_equal(startEntry->threads[akt_thread], thread) )
            {

                // if there is a next entry in queue...
                //if( startEntry->next->prev )
                {
                    out( MSGDEBUG, config, "getFromQueue(): %ld The client already got the element, so returning with next\n", thread );

                    // ...return with the next element
                    return getFromQueue( config, msgList, threads, startEntry->next, thread, retEntry );
                }
            }
        }


        // lock mutex for manipulation of message queue
        if( pthread_mutex_lock(&msgList->mutex) == 0 )
        {
            out( MSGDEBUG, config, "getFromQueue(): %ld Locked the mutex of the message queue\n", thread );

            // lock mutex for manipulation of clients structure
            if( pthread_mutex_lock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Locked the mutex of the clients structure\n", thread );
            }
            else
            {
                out( MSGERROR, config, "getFromQueue(): %ld Error locking the mutex of the clients structure \n", thread );

                // unlock mutex of message queue
                if( pthread_mutex_unlock(&msgList->mutex) == 0 )
                {
                    out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the message queue\n", thread );
                }
                // return with failure
                return EXIT_FAILURE;
            }
        }
        else
        {
            out( MSGERROR, config, "getFromQueue(): %ld Error locking the mutex of the message queue\n", thread );

            // return with failure
            return EXIT_FAILURE;
        }


        // set the thread as "has read" in the entry
        startEntry->threads[startEntry->reads] = thread;
        startEntry->reads++;

        // if the calling thread din't got the entry the entry already, copy to give him
        //retEntry = *startEntry;
        retEntry->type = startEntry->type;
        retEntry->reads = startEntry->reads;
        free( retEntry->file );
        if( (retEntry->file = malloc(strlen(startEntry->file)+1)) == NULL )
        {
            out( MSGERROR, config, "getFromQueue(): %ld Error allocating memory for logfiles config structure\n", thread );

            // unlock mutex of message queue
            if( pthread_mutex_unlock(&msgList->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the message queue\n", thread );
            }
            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the clients structure\n", thread );
            }
            // return with failure
            return EXIT_FAILURE;
        }
        if( (strcpy(retEntry->file, startEntry->file)) == NULL )
        {
            out( MSGERROR, config, "getFromQueue(): %ld Error copying the filename to the new element\n", thread );

            // unlock mutex of message queue
            if( pthread_mutex_unlock(&msgList->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the message queue\n", thread );
            }
            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the clients structure\n", thread );
            }
            // return with failure
            return EXIT_FAILURE;
        }
        free( retEntry->msg );
        if( (retEntry->msg = malloc(strlen(startEntry->msg)+1)) == NULL )
        {
            out( MSGERROR, config, "getFromQueue(): %ld Error allocating memory for logfiles config structure\n", thread );

            // unlock mutex of message queue
            if( pthread_mutex_unlock(&msgList->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the message queue\n", thread );
            }
            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the clients structure\n", thread );
            }
            // return with failure
            return EXIT_FAILURE;
        }
        if( (strcpy(retEntry->msg, startEntry->msg)) == NULL )
        {
            out( MSGERROR, config, "getFromQueue(): %ld Error copying the message to the new element\n", thread );

            // unlock mutex of message queue
            if( pthread_mutex_unlock(&msgList->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the message queue\n", thread );
            }
            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the clients structure\n", thread );
            }
            // return with failure
            return EXIT_FAILURE;
        }
/*
        for( akt_thread = 0; akt_thread < config.maxnumclients; akt_thread++ )
        {
            retEntry->threads[akt_thread] = startEntry->threads[akt_thread];
        }
*/
        out( MSGDBGEX, config, "getFromQueue(): %ld Put thread in readers array and copied the element for return\n", thread );


        // unlock mutex of message queue
        if( pthread_mutex_unlock(&msgList->mutex) == 0 )
        {
            out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the message queue\n", thread );

            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the clients structure\n", thread );

                // try to remove the "returned" entry
                removeFromQueue( config, msgList, threads, startEntry );
            }
            else
            {
                out( MSGERROR, config, "getFromQueue(): %ld Error unlocking the mutex of the clients structure \n", thread );

                // return with failure
                return EXIT_FAILURE;
            }
        }
        else
        {
            out( MSGERROR, config, "getFromQueue(): %ld Error unlocking the mutex of the message queue\n", thread );

            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "getFromQueue(): %ld Unlocked the mutex of the clients structure\n", thread );
            }
            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {   // got invalid attribute(s)
        out( MSGCRIT, config, "getFromQueue(): %ld One of the given parameter is not valid \n", thread );

        // return with failure
        return EXIT_FAILURE;
    }

    // return with success
    return EXIT_SUCCESS;
}

// function to remove an entry from message queue
int removeFromQueue( struct daemonConfig *config,  struct messageQueue *msgList, struct clientThreads *threads, struct entry *element )
{
    // only a valid entry from a valid queue and a valid threads structure
    if( msgList && element && threads )
    {
        // marker if an element is requested twice or more
        char already = 0;
        // counter
        int akt_thread = 0, akt_registered = 0;


        // lock mutex for manipulation of message queue
        if( pthread_mutex_lock(&msgList->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeFromQueue(): Locked the mutex of the message queue\n" );

            // lock mutex for manipulation of clients structure
            if( pthread_mutex_lock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "removeFromQueue(): Locked the mutex of the clients structure\n" );
            }
            else
            {
                out( MSGERROR, config, "removeFromQueue(): Error locking the mutex of the clients structure \n" );

                // unlock mutex of message queue
                if( pthread_mutex_unlock(&msgList->mutex) == 0 )
                {
                    out( MSGDEBUG, config, "removeFromQueue(): Unlocked the mutex of the message queue\n" );

                }
                // return with failure
                return EXIT_FAILURE;
            }
        }
        else
        {
            out( MSGERROR, config, "removeFromQueue(): Error locking the mutex of the message queue\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        out( MSGINFO, config, "removeFromQueue(): Trying to remove an element from message queue, read %d of %d\n", element->reads, threads->num );

        // check if every thread got it
        for( akt_thread = 0; akt_thread < config->maxnumclients; akt_thread++ )
        {
            for( akt_registered = 0; akt_registered < threads->num; akt_registered++ )
            {
                if( element->threads[akt_thread] != 0 )
                {
                    if( pthread_equal(element->threads[akt_thread], threads->threads[akt_registered]) )
                    {
                        out( MSGDEBUG, config, "removeFromQueue(): Thread with socket %ld = %ld got it already, [%d=%d]\n", element->threads[akt_thread], threads->threads[akt_registered], akt_thread, akt_registered );
                        already++;
                    }
                }
            }
        }

        // if every thread already got it, remove from queue
        if( already >= threads->num )
        {
            out( MSGDEBUG, config, "removeFromQueue(): Every thread already got it (%d of %d), removing...\n",already, threads->num );

            if( element == msgList->first )
            {
                out( MSGDEBUG, config, "removeFromQueue(): Element is first...\n" );

                if( element->next )
                {
                    out( MSGDEBUG, config, "removeFromQueue(): First element has next...\n" );

                    // the second entry will be first, can't have prev
                    element->next->prev = NULL;
                    msgList->first->next->prev = NULL;
                    msgList->first = msgList->first->next;
                }
                else
                {
                    out( MSGDEBUG, config, "removeFromQueue(): First element has NOT next...\n" );

                    // element is the only element, queue must point to NULLs
                    msgList->first = NULL;
                    msgList->last = NULL;
                }
            }
            else
            {
                if( element == msgList->last )
                {
                    out( MSGDEBUG, config, "removeFromQueue(): Element is last...\n" );

                    // second last must point to NULL as next
                    msgList->last->prev->next = NULL;
                    // second last gets new last
                    msgList->last = msgList->last->prev;
                }
                else
                {
                    out( MSGDEBUG, config, "removeFromQueue(): Element is NOT first/last...\n" );

                    // the entry before must point to next as next
                    element->prev->next = element->next;
                    // the entry after must point to prev as prev
                    element->next->prev = element->prev;
                }
            }

            // remove from memory
            free( element->file );
            free( element->msg );
            free( element->threads );
            free( element );

            // an element was removed, so decrement element number
            msgList->num--;

            out( MSGDEBUG, config, "removeFromQueue(): Removed the entry, new number of entries in queue: %d\n", msgList->num );
        }
        else
        {
            out( MSGDEBUG, config, "removeFromQueue(): NOT every thread already got it, keeping the entry\n" );
        }


        // unlock mutex of message queue
        if( pthread_mutex_unlock(&msgList->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeFromQueue(): Unlocked the mutex of the message queue\n" );

        }
        else
        {
            out( MSGERROR, config, "removeFromQueue(): Error unlocking the mutex of the message queue\n" );

            // unlock mutex of clients structure
            if( pthread_mutex_unlock(&threads->mutex) == 0 )
            {
                out( MSGDEBUG, config, "removeFromQueue(): Unlocked the mutex of the clients structure\n" );
            }
            // return with failure
            return EXIT_FAILURE;
        }
        // unlock mutex of clients structure
        if( pthread_mutex_unlock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeFromQueue(): Unlocked the mutex of the clients structure\n" );
        }
        else
        {
            out( MSGERROR, config, "removeFromQueue(): Error unlocking the mutex of the clients structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "removeFromQueue(): One of the given parameter is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }

    // return with success
    return EXIT_SUCCESS;
}


// function to print out one queue element
void printQueueElement( struct daemonConfig *config, struct entry *element)
{
    // only a given element can be printed
    if( element )
    {
        out( MSGINFO, config, "+++++\n" );
        out( MSGINFO, config, "+\n" );
        out( MSGINFO, config, "+  Printing an entry...\n" );
        out( MSGINFO, config, "+  Reads: %d\n", element->reads );
        out( MSGINFO, config, "+  Type: %d\n", element->type );
        out( MSGINFO, config, "+  File: %s\n", element->file );
        out( MSGINFO, config, "+  Msg:  %s\n", strtok(element->msg,"\t\n\r") );
        out( MSGINFO, config, "+\n" );
        out( MSGINFO, config, "+++++\n" );
    }
}

// function to print out the full queue content
void printQueue( struct daemonConfig *config, struct messageQueue *msgList )
{
    // lock mutex of message queue
    pthread_mutex_lock( &msgList->mutex );

    // is a valid queue with at least one element given?
    if( msgList->first )
    {
        // get the first entry in queue
        struct entry *tmpEntry = msgList->first;

        out( MSGINFO, config, "+++++\n" );
        out( MSGINFO, config, "+\n" );
        out( MSGINFO, config, "+  Printing message queue with %d entries...\n", msgList->num );

        // for every entry in the queue...
        do{
            if( tmpEntry )
            {
                out( MSGINFO, config, "+  Reads: %d  Type: %d  File: %s  Msg: %s\n", tmpEntry->reads, tmpEntry->type, tmpEntry->file, strtok(tmpEntry->msg,"\t\n\r") );
            }
        }while( (tmpEntry = tmpEntry->next) != NULL );

        out( MSGINFO, config, "+\n" );
        out( MSGINFO, config, "+++++\n" );
    }

    // unlock mutex of message queue
    pthread_mutex_unlock( &msgList->mutex );
}
