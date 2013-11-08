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


// function for watcher thread
void* watch( void *param )
{
    // cast the given parameter to watcherInfo
    struct watcherInfo *watcherinfo = ((watcherInfo*)(param));


    // is the given watcher info structure valid
    if( watcherinfo )
    {
        // pointer to logfile
        FILE *logfile;
        // single line of file
        char line[MAX_LINE_LEN];
        // offset in lines
        int offset = 0;


        // lock mutex of watchers logfile
        if( pthread_mutex_lock(watcherinfo->mutexW) == 0 )
        {
            out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld Locked the mutex of the watchers logfile\n", pthread_self() );
        }
        else
        {
            out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error locking the mutex of the watchers logfile\n", pthread_self() );

            // free memory from watchers argument
            free( watcherinfo );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }

        out( MSGNOTICE, watcherinfo->dconf, "watch(): %ld is watching at %s in %s\n", pthread_self(), watcherinfo->lfconf->name, watcherinfo->lfconf->path );

        // add the new watcher to watcher structure
        if( addWatcher(watcherinfo->dconf, watcherinfo->threads, pthread_self()) == EXIT_SUCCESS )
        {
            out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld Succesfully added watcher to watcher structure\n", pthread_self() );
        }
        else
        {
            out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error adding watcher to watcher structure\n", pthread_self() );

            // unlock mutex of watchers logfile
            if( pthread_mutex_unlock(watcherinfo->mutexW) == 0 )
            {
                out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld Unlocked the mutex of the watchers logfile\n", pthread_self() );
            }
            else
            {
                out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error unlocking the mutex of the watchers logfile\n", pthread_self() );
            }
            // free memory from watchers argument
            free( watcherinfo );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }


        // open logfile for reading
        if( (logfile = fopen(watcherinfo->lfconf->path, "r")) != NULL )
        {
            // seek to last reading position, if wanted
            if( watcherinfo->lfconf->pos > 0 )
            {
                out( MSGINFO, watcherinfo->dconf, "watch(): %ld The start position of file %s (%s) is %d\n", pthread_self(), watcherinfo->lfconf->name, watcherinfo->lfconf->path, watcherinfo->lfconf->pos );
                fseek( logfile, watcherinfo->lfconf->pos, SEEK_SET );
            }

            // read from logfile
            while( fgets( line, MAX_LINE_LEN, logfile ) != NULL )
            {
                int akt_token = 0, skip = 0;

                // skip offset lines
                if( offset < watcherinfo->lfconf->offset )
                {
                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld Skipping offset %d in %s\n", pthread_self(), offset, watcherinfo->lfconf->name );
                    offset++;
                    continue;
                }

                // check line for every skip pattern given via config
                for( akt_token = 0; akt_token < watcherinfo->lfconf->numSkip; akt_token++ )
                {
                    // don't use empty tokens
                    if( strlen(watcherinfo->lfconf->skip[akt_token]) > 0 )
                    {
                        // if skip pattern matches, skip line
                        if( strstr(line, watcherinfo->lfconf->skip[akt_token]) != NULL )
                        {
                            out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found SKIP %s in: %s", pthread_self(), watcherinfo->lfconf->skip[akt_token], line );
                            skip = 1;
                            break;
                        }
                    }
                }
                if( skip == 0 )
                {
                    // won't check empty lines
                    if( strlen(line) > 0 )
                    {
                        // if not skipped, check line for every other pattern given via config
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numEmerg; akt_token++ )
                        {
                            // don't use empty tokens
                            if( strlen(watcherinfo->lfconf->emerg[akt_token]) > 0 )
                            {
                                // if emerg pattern matches, put into message queue as emergence
                                if( strstr(line, watcherinfo->lfconf->emerg[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found EMERGENCE %s in: %s", pthread_self(), watcherinfo->lfconf->emerg[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGEMERG, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numAlert; akt_token++ )
                        {
                            // don't use empty tokens
                            if( strlen(watcherinfo->lfconf->alert[akt_token]) > 0 )
                            {
                                // if alert pattern matches, put into message queue as alert
                                if( strstr(line, watcherinfo->lfconf->alert[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found ALERT %s in: %s", pthread_self(), watcherinfo->lfconf->alert[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGALERT, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numCrit; akt_token++ )
                        {
                            // don't use empty tokens
                            if( strlen(watcherinfo->lfconf->crit[akt_token]) > 0 )
                            {
                                // if crit pattern matches, put into message queue as critical
                                if( strstr(line, watcherinfo->lfconf->crit[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found CRITICAL %s in: %s", pthread_self(), watcherinfo->lfconf->crit[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGCRIT, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numError; akt_token++ )
                        {
                            // don't use empty tokens
                            if( strlen(watcherinfo->lfconf->error[akt_token]) > 0 )
                            {
                                // if error pattern matches, put into message queue as error
                                if( strstr(line, watcherinfo->lfconf->error[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found ERROR %s in: %s", pthread_self(), watcherinfo->lfconf->error[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGERROR, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                            // don't use empty tokens
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numWarn; akt_token++ )
                        {
                            if( strlen(watcherinfo->lfconf->warn[akt_token]) > 0 )
                            {
                                // if warning pattern matches, put into message queue as warning
                                if( strstr(line, watcherinfo->lfconf->warn[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found WARN %s in: %s", pthread_self(), watcherinfo->lfconf->warn[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGWARN, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                            // don't use empty tokens
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numNote; akt_token++ )
                        {
                            if( strlen(watcherinfo->lfconf->note[akt_token]) > 0 )
                            {
                                // if note pattern matches, put into message queue as notice
                                if( strstr(line, watcherinfo->lfconf->note[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found NOTICE %s in: %s", pthread_self(), watcherinfo->lfconf->note[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGNOTICE, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                            // don't use empty tokens
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numInfo; akt_token++ )
                        {
                            if( strlen(watcherinfo->lfconf->info[akt_token]) > 0 )
                            {
                                // if info pattern matches, put into message queue as info
                                if( strstr(line, watcherinfo->lfconf->info[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found INFO %s in: %s", pthread_self(), watcherinfo->lfconf->info[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGINFO, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                        }
                        if( skip == 1 ) { continue; }
                        for( akt_token = 0; akt_token < watcherinfo->lfconf->numDebug; akt_token++ )
                        {
                            // don't use empty tokens
                            if( strlen(watcherinfo->lfconf->debug[akt_token]) > 0 )
                            {
                                // if debug pattern matches, put into message queue as debug
                                if( strstr(line, watcherinfo->lfconf->debug[akt_token]) != NULL )
                                {
                                    out( MSGDBGEX, watcherinfo->dconf, "watch(): %ld found DEBUG %s in: %s", pthread_self(), watcherinfo->lfconf->debug[akt_token], line );
                                    addToQueue( watcherinfo->dconf, watcherinfo->msgQueue, MSGDEBUG, watcherinfo->lfconf->name, line );
                                    skip = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // save actual position of logfile
            watcherinfo->lfconf->pos = ftell( logfile );

            // lock mutex of the logfiles
            if( pthread_mutex_lock(watcherinfo->mutexL) == 0 )
            {
                out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld Locked the mutex of the logfiles\n", pthread_self() );

                // if wanted save the position permanent
                if( watcherinfo->lfconf->save > 0 )
                {
                    out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld wants to save position of %s @ %d\n", pthread_self(), watcherinfo->lfconf->name, watcherinfo->lfconf->pos );
                    savePosition( watcherinfo->dconf, watcherinfo->lfconf );
                }
                else
                {
                    out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld memorised new position of %s @ %d\n", pthread_self(), watcherinfo->lfconf->name, watcherinfo->lfconf->pos );
                }

                // unlock mutex of the logfiles
                if( pthread_mutex_unlock(watcherinfo->mutexL) == 0 )
                {
                    out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld Unlocked the mutex of the logfiles\n", pthread_self() );
                }
                else
                {
                    out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error unlocking the mutex of the logfiles\n", pthread_self() );

                }
            }
            else
            {
                out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error locking the mutex of the logfiles\n", pthread_self() );
            }


            // close logfile
            if( fclose(logfile) == 0 )
            {
                out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld closed file %s\n", pthread_self(), watcherinfo->lfconf->path );
            }
            else
            {
                out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error closing file %s\n", pthread_self(), watcherinfo->lfconf->path );
            }


            // unlock mutex of watchers logfile
            if( pthread_mutex_unlock(watcherinfo->mutexW) == 0 )
            {
                out( MSGDEBUG, watcherinfo->dconf, "watch(): %ld Unlocked the mutex of the watchers logfile\n", pthread_self() );
            }
            else
            {
                out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error unlocking the mutex of the watchers logfile\n", pthread_self() );

                // free memory from watchers argument
                free( watcherinfo );

                // close thread with failure
                pthread_exit( (void*)EXIT_FAILURE );
            }
        }
        else
        {
            out( MSGERROR, watcherinfo->dconf, "watch(): %ld Unable to open %s\n", pthread_self(), watcherinfo->lfconf->path );
        }


        // remove watcher from watcher structure
        if( removeWatcher(watcherinfo->dconf, watcherinfo->threads, pthread_self()) != EXIT_SUCCESS )
        {
            out( MSGERROR, watcherinfo->dconf, "watch(): %ld Error removing watcher thread from watcher structure\n", pthread_self() );

            // free memory from watchers argument
            free( watcherinfo );

            // close thread with failure
            pthread_exit( (void*)EXIT_FAILURE );
        }
    }
    else
    {
        out( MSGCRIT, watcherinfo->dconf, "watch(): %ld The given watcher info structure is not valid \n", pthread_self() );

        // close thread with failure
        pthread_exit( (void*)EXIT_FAILURE );
    }

    out( MSGINFO, watcherinfo->dconf, "watch(): %ld Closing watcher thread (%s)\n", pthread_self(), watcherinfo->lfconf->name );

    // free memory from watchers argument
    free( watcherinfo );

    // close thread with success
    pthread_exit( (void*)EXIT_SUCCESS );
}

// function to start a watcher thread
int createWatcher( struct daemonBase *base )
{
    // is the given daemon base structure valid
    if( base )
    {
        // temporary counter
        int i = 0;
        // temporary threadt
        pthread_t tmpThread;
        // temporary watcher argument
        struct watcherInfo *info;


        // start logfile watcher threads
        for( i = 0; i < base->lfconf->num; i++ )
        {
            // lock mutex for manipulation of watcher structure
            if( pthread_mutex_lock(&base->watcher->mutex) == 0 )
            {
                out( MSGDEBUG, base->dconf, "createWatcher(): Locked the mutex of the watcher structure\n" );
            }
            else
            {
                out( MSGERROR, base->dconf, "createWatcher(): Error locking the mutex of the watcher structure \n" );

                // return with failure
                return EXIT_FAILURE;
            }


            // build a structure with watcher needed informations
            out( MSGDEBUG, base->dconf, "createWatcher(): Building a watcher info structure for the watcher(%s)\n", base->lfconf->configs[i].name );
            if( (info = malloc(sizeof(watcherInfo))) == NULL )
            {
                out( MSGERROR, base->dconf, "createWatcher(): Error allocating memory for temporary watcher info\n" );

                // unlock mutex of watcher structure
                if( pthread_mutex_unlock(&base->watcher->mutex) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "createWatcher(): Unlocked the mutex of the watcher structure\n" );
                }
                // return with failure
                return EXIT_FAILURE;
            }
            info->dconf = base->dconf;
            out( MSGDEBUG, base->dconf, "createWatcher(): Build a watcher info - added dconf for watcher (%s)\n", base->lfconf->configs[i].name );
            info->lfconf = &base->lfconf->configs[i];
            out( MSGDEBUG, base->dconf, "createWatcher(): Build a watcher info - added lfconf for watcher (%s)\n", base->lfconf->configs[i].name );
            info->threads = base->watcher;
            out( MSGDEBUG, base->dconf, "createWatcher(): Build a watcher info - added wthreads for watcher (%s)\n", base->lfconf->configs[i].name );
            info->msgQueue = base->msgQueue;
            out( MSGDEBUG, base->dconf, "createWatcher(): Build a watcher info - added queue for watcher (%s)\n", base->lfconf->configs[i].name );
            info->mutexW = &base->lfconf->configs[i].mutex;
            info->mutexL = &base->lfconf->mutex;
            out( MSGDEBUG, base->dconf, "createWatcher(): Build a watcher info - added mutexe for watcher (%s)\n", base->lfconf->configs[i].name );


            // start the thread with watcher-function and infos
            if( pthread_create(&tmpThread, NULL, watch, info) != 0 )
            {
                out( MSGERROR, base->dconf, "createWatcher(): Error creating thread %d for %s\n", i+1, base->lfconf->configs[i].name );
                pthread_cancel( tmpThread );

                // unlock mutex of watcher structure
                if( pthread_mutex_unlock(&base->watcher->mutex) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "createWatcher(): Unlocked the mutex of the watcher structure\n" );
                }
                else
                {
                    out( MSGERROR, base->dconf, "createWatcher(): Error unlocking the mutex of the watcher structure \n" );

                    // return with failure
                    return EXIT_FAILURE;
                }
            }
            else
            {
                out( MSGNOTICE, base->dconf, "createWatcher(): Created a watcher thread %d for %s\n", i+1, base->lfconf->configs[i].path );

                // disassociate client from parent/server-thread, num is here +1
                if( pthread_detach(tmpThread) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "createWatcher(): Detached the watcher thread from parent\n" );
                }
                else
                {
                    out( MSGERROR, base->dconf, "createWatcher(): Error detaching the watcher thread from parent\n" );
                }

                // unlock mutex of watcher structure
                if( pthread_mutex_unlock(&base->watcher->mutex) == 0 )
                {
                    out( MSGDEBUG, base->dconf, "createWatcher(): Unlocked the mutex of the watcher structure\n" );
                }
                else
                {
                    out( MSGERROR, base->dconf, "createWatcher(): Error unlocking the mutex of the watcher structure \n" );

                    // return with failure
                    return EXIT_FAILURE;
                }
            }
        }
    }
    else
    {
        out( MSGCRIT, base->dconf, "createWatcher(): The given base structure is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }
    // return with success
    return EXIT_SUCCESS;
}

// function to create a watcher threads structure
int createWatchersStructure( struct daemonConfig *config, struct watcherThreads *newWatcher )
{
    // is the given watcher threads structure valid
    if( newWatcher )
    {
        out( MSGINFO, config, "createWatchersStructure(): Creating a new watcher threads structure\n" );

        // allocate space for threads
        newWatcher->threads = malloc( sizeof(pthread_t[1]) );
        newWatcher->num = 0;


        // initialize mutex of watcher structure
        if( pthread_mutex_init(&newWatcher->mutex, NULL) == 0 )
        {
            out( MSGDEBUG, config, "createWatchersStructure(): Initialised the mutexof the watcher structure\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "createWatchersStructure(): Error initialising the mutex of the watcher structure\n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "createWatchersStructure(): The given watcher threads structure is not valid\n" );

        // return with failure
        return EXIT_FAILURE;
    }
}

// function to unallocate the memory of a watcher threads structure
void destroyWatchersStructure( struct watcherThreads *watcher )
{
    int i;
    for( i = 0; i < watcher->num; i++ )
    {
        pthread_cancel( watcher->threads[i] );
    }
    pthread_mutex_destroy( &watcher->mutex );
    free( watcher->threads );
    free( watcher );
}


// function to add a watcher from clients structure
int addWatcher( struct daemonConfig *config, struct watcherThreads *threads, pthread_t newThread )
{
    // only from a valid watcher threads structure
    if( threads )
    {
        // temporary counter
        int i;


        // lock mutex for manipulation of watcher structure
        if( pthread_mutex_lock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "addWatcher(): Locked the mutex of the watcher structure\n" );
        }
        else
        {
            out( MSGERROR, config, "addWatcher(): Error locking the mutex of the watcher structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }


        out( MSGINFO, config, "addWatcher(): Going to add a watcher as %d. to watcher structure\n", threads->num+1 );

        // temporary structure for threads
        out( MSGDEBUG, config, "addWatcher(): Instantiating and allocating a temporary watcher structure\n" );
        struct watcherThreads tmpThreads;
        if( (tmpThreads.threads = malloc(sizeof(pthread_t[threads->num+1]))) == NULL )
        {
            out( MSGERROR, config, "addWatcher(): Error allocating memory for temporary watcher descriptors\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy all threads to temporary structure
        out( MSGDEBUG, config, "addWatcher(): Going to copy the %d old entries to temp\n", threads->num );
        for( i = 0; i < threads->num; i++ )
        {
            out( MSGDEBUG, config, "addWatcher(): Copying number %d of the watcher structure\n", i+1 );
            tmpThreads.threads[i] = threads->threads[i];
        }


        // memory management for enlarging watcher threads structure
        out( MSGDEBUG, config, "addWatcher(): Enlarging the size of the watcher structure to %d\n", threads->num+1 );
        free( threads->threads );
        if( (threads->threads = malloc(sizeof(pthread_t[threads->num+1]))) == NULL )
        {
            out( MSGERROR, config, "addWatcher(): Error allocating memory for enlarged watcher threadts\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy back all remaining watcher thread infos
        out( MSGDEBUG, config, "addWatcher(): Going to copy %d back from temp to watcher structure\n", threads->num );
        for( i = 0; i < threads->num; i++ )
        {
            out( MSGDEBUG, config, "addWatcher(): Copying threadt of watcher %d\n", i+1 );
            threads->threads[i] = tmpThreads.threads[i];
        }

        // free temporary allocated
        free( tmpThreads.threads );


        // add the new thread to watcher structure
        threads->threads[threads->num] = newThread;
        out( MSGDEBUG, config, "addWatcher(): Added watcher thread %ld as %d\n", newThread, threads->num+1 );

        // one thread was added so increment
        threads->num++;
        out( MSGDEBUG, config, "addWatcher(): Incremented the number of active watcher to %d\n", threads->num );


        // unlock mutex of watcher structure
        if( pthread_mutex_unlock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "addWatcher(): Unlocked the mutex of the watcher structure\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "addWatcher(): Error unlocking the mutex of the watcher structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "addWatcher(): The given watcher threads structure is not valid\n" );

        // return with failure
        return EXIT_FAILURE;
    }
}

// function to remove a watcher from watcher structure
int removeWatcher( struct daemonConfig *config, struct watcherThreads *threads, pthread_t oldThread )
{
    // only from a valid watcher threads structure
    if( threads )
    {
        // temporary counter
        int i, num, akt_thread = 0;


        // lock mutex for manipulation of watcher structure
        if( pthread_mutex_lock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeWatcher(): Locked the mutex of the watcher structure\n" );
        }
        else
        {
            out( MSGERROR, config, "removeWatcher(): Error locking the mutex of the watcher structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }


        out( MSGINFO, config, "removeWatcher(): Going to remove a watcher from watcher structure\n" );

        // temporary structure for threads
        if( threads->num > 1 )
        {
            num = threads->num-1;
        }
        else
        {
            num = 1;
        }
        out( MSGDEBUG, config, "removeWatcher(): Instantiating and allocating a temporary, smaller watcher structure\n" );
        struct watcherThreads tmpThreads;
        if( (tmpThreads.threads = malloc(sizeof(pthread_t[num]))) == NULL )
        {
            out( MSGERROR, config, "removeWatcher(): Error allocating memory for temporary watcher descriptors\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy all threads except the one to remove
        out( MSGDEBUG, config, "removeWatcher(): Going to copy the %d old entries to temp\n", threads->num );
        for( i = 0; i < threads->num; i++ )
        {
            if( !pthread_equal(threads->threads[i], oldThread) )
            {
                out( MSGDEBUG, config, "removeWatcher(): Copying number %d of the watcher structure\n", i+1 );
                tmpThreads.threads[akt_thread] = threads->threads[i];
                akt_thread++;
            }
        }


        // memory management for reducing watcher threads structure
        out( MSGDEBUG, config, "removeWatcher(): Reducing the size of the watcher structure to %d\n", num );
        free( threads->threads );
        if( (threads->threads = malloc(sizeof(pthread_t[num]))) == NULL )
        {
            out( MSGERROR, config, "removeWatcher(): Error allocating memory for reduced watcher threadts\n" );

            // return with failure
            return EXIT_FAILURE;
        }


        // copy back all remaining watcher thread infos
        out( MSGDEBUG, config, "removeWatcher(): Going to copy %d back from temp to watcher structure\n", threads->num );
        for( i = 0; i < threads->num-1; i++ )
        {
            out( MSGDEBUG, config, "removeWatcher(): Copying the pthreadt of watcher %d\n", i+1 );
            threads->threads[i] = tmpThreads.threads[i];
        }

        // free temporary allocated
        free( tmpThreads.threads );


        // one thread will be stopped so descrement
        threads->num--;
        out( MSGDEBUG, config, "removeWatcher(): Decremented the number of active watcher to %d\n", threads->num );


        // unlock mutex of watcher structure
        if( pthread_mutex_unlock(&threads->mutex) == 0 )
        {
            out( MSGDEBUG, config, "removeWatcher(): Unlocked the mutex of the watcher structure\n" );

            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            out( MSGERROR, config, "removeWatcher(): Error unlocking the mutex of the watcher structure \n" );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "removeWatcher(): The given watcher threads structure is not valid\n" );

        // return with failure
        return EXIT_FAILURE;
    }
}
