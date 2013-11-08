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


// function to initialise daemons configuration
int initDaemonConfig( struct daemonConfig *newDaemonConf )
{
    // only run through if valid
    if( newDaemonConf )
    {
        // allocate space, set default / error config
        newDaemonConf->interval =  DEF_INTERVAL;
        newDaemonConf->maxlinelength = MAX_LINE_LEN;
        newDaemonConf->maxnumlogfiles = MAX_NUM_FILES;
        newDaemonConf->maxnumpatterns = MAX_NUM_PATTERNS;
        newDaemonConf->logfilesconf = malloc( strlen(LOGS_CONF_FILE)+1 );
        strcpy( newDaemonConf->logfilesconf, LOGS_CONF_FILE );
        newDaemonConf->port =      NETWORK_PORT;
        newDaemonConf->proto = malloc( strlen(DEF_PROTO)+1 );
        strcpy( newDaemonConf->proto, DEF_PROTO );
        newDaemonConf->maxnumclients = MAX_NUM_CLIENTS;
        newDaemonConf->clircvtimout = CLI_RCV_TIMEOUT;
        newDaemonConf->savefile = malloc( strlen(POSITION_FILE)+1 );
        strcpy( newDaemonConf->savefile, POSITION_FILE );
        newDaemonConf->logfile = malloc( strlen(DAEMON_LOGFILE)+1 );
        strcpy( newDaemonConf->logfile, DAEMON_LOGFILE );
        newDaemonConf->loglevel = DEF_LOG_LEVEL;
        newDaemonConf->rad = 0;

        // initialize the mutex of the config
        if( pthread_mutex_init(&newDaemonConf->mutex, NULL) == 0 )
        {
            // return with success
            return EXIT_SUCCESS;
        }
        else
        {
            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        // return with failure
        return EXIT_FAILURE;
    }
}

// function to open and read from daemon config file
int readDaemonConfig( const char *configFilePath, struct daemonConfig *newDaemonConf )
{
    // only run through if valid
    if( newDaemonConf )
    {
        // pointer to config file
        FILE *file;
        // single line of file
        char line[MAX_LINE_LEN];
        // attribute of line
        char *attribute;


        // open daemon config file
        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): trying to open the daemon configuration file\n" );
        if( (file = fopen(configFilePath, "r")) )
        {
            out( MSGNOTICE, newDaemonConf, "readDaemonConfig(): Successfully opened %s\n", configFilePath );
            // read from file
            while( fgets( line, MAX_LINE_LEN, file ) != NULL )
            {
                // first stays the attribute, later the value
                attribute = strtok( line, "\t =\n\r" );
                if( attribute != NULL && attribute[0] != '#' )
                {
                    // change the check interval
                    if( strcmp(attribute, "interval") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the interval parameter\n" );
                        char *value = strtok( NULL, "\t=\n\r" );
                        unsigned long seconds = parseInterval( newDaemonConf, value );
                        if( seconds >= 1 && seconds < ULONG_MAX && newDaemonConf->interval == DEF_INTERVAL )
                        {
                            newDaemonConf->interval = seconds;
                        }
                    }
                    // change the maximum number of logfiles
                    else if( strcmp(attribute, "maxnumlogfiles") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the maxnumlogfiles parameter\n" );
                        unsigned long value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 1 && value < ULONG_MAX )
                        {
                            newDaemonConf->maxnumlogfiles = value;
                        }
                    }
                    // change the maximum line length of logfiles
                    else if( strcmp(attribute, "maxlinelength") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the maxlinelength parameter\n" );
                        unsigned long value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 1 && value < ULONG_MAX )
                        {
                            newDaemonConf->maxlinelength = value;
                        }
                    }
                    // change the maximum number of match patterns
                    else if( strcmp(attribute, "maxnumpatterns") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the maxnumpatterns parameter\n" );
                        unsigned long value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 1 && value < ULONG_MAX )
                        {
                            newDaemonConf->maxnumpatterns = value;
                        }
                    }
                    // change the path to logfiles-configuration
                    else if( strcmp(attribute, "logfilesconf") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the logfilesconf parameter\n" );
                        char *value = strtok( NULL,"\t =\n\r" );
                        if( fileExists(value) == EXIT_SUCCESS && strstr( newDaemonConf->logfilesconf, LOGS_CONF_FILE ) != NULL )
                        {
                            free( newDaemonConf->logfilesconf );
                            newDaemonConf->logfilesconf = malloc( strlen(value)+1 );
                            strcpy( newDaemonConf->logfilesconf, value );
                        }
                    }
                    // change the network port
                    else if( strcmp(attribute, "port") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the port parameter\n" );
                        int value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 1 && value < 65535 )
                        {
                            newDaemonConf->port = value;
                        }
                    }
                    // change the network protocol
                    else if( strcmp(attribute, "proto") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the proto parameter\n" );
                        char *value = strtok( NULL,"\t =\n\r" );
                        if( strcmp(value, "udp") || strcmp(value, "tcp") || strcmp(value, "ssl") )
                        {
                            free( newDaemonConf->proto );
                            newDaemonConf->proto = malloc( strlen(value)+1 );
                            strcpy( newDaemonConf->proto, value );
                        }
                    }
                    // change the maximum number of network clients
                    else if( strcmp(attribute, "maxnumclients") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the maxnumclients parameter\n" );
                        unsigned long value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 1 && value < ULONG_MAX )
                        {
                            newDaemonConf->maxnumclients = value;
                        }
                    }
                    // change the client receive timeout
                    else if( strcmp(attribute, "clircvtimout") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the clircvtimout parameter\n" );
                        unsigned long value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 1 && value < ULONG_MAX )
                        {
                            newDaemonConf->clircvtimout = value;
                        }
                    }
                    // change the path to position savefile
                    else if( strcmp(attribute, "possavefile") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the possavefile parameter\n" );
                        char *value = strtok( NULL,"\t =\n\r" );
                        if( strstr( newDaemonConf->savefile, POSITION_FILE ) != NULL )
                        {
                            free( newDaemonConf->savefile );
                            newDaemonConf->savefile = malloc( strlen(value)+1 );
                            strcpy( newDaemonConf->savefile, value );
                        }
                    }
                    // change the path to daemons logfile
                    else if( strcmp(attribute, "daemonlogfile") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the daemonlogfile parameter\n" );
                        char *value = strtok( NULL,"\t =\n\r" );
                        if( strstr(newDaemonConf->logfile, DAEMON_LOGFILE) != NULL )
                        {
                            free( newDaemonConf->logfile );
                            newDaemonConf->logfile = malloc( strlen(value)+1 );
                            strcpy( newDaemonConf->logfile, value );
                        }
                    }
                    // change the loglevel
                    else if( strcmp(attribute, "loglevel") == 0 )
                    {
                        out( MSGDEBUG, newDaemonConf, "readDaemonConfig(): Got the loglevel parameter\n" );
                        char value = atoi( strtok(NULL,"\t =\n\r") );
                        if( value >= 0 && value <= 7 && newDaemonConf->loglevel == DEF_LOG_LEVEL )
                        {
                            newDaemonConf->loglevel = value;
                        }
                    }
                }
            }
            // close file
            fclose( file );
        }
        else
        {
            out( MSGERROR, newDaemonConf, "readDaemonConfig(): Unable to open %s\n", configFilePath );

            // return with failure
            return EXIT_FAILURE;
        }

        // return with success
        return EXIT_SUCCESS;
    }
    else
    {
        out( MSGCRIT, newDaemonConf, "readDaemonConfig(): The given daemon config structure is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }
}


// function to open and read from logs config file
int readLogfilesConfig( struct daemonConfig *config, struct logfilesConfig *newLogfilesConf )
{
    // only run through if valid
    if( newLogfilesConf )
    {
        // temporary counters
        int akt_file = 0, akt_token = 0, longest = 0;
        // pointer to config file
        FILE *file;
        // single line of file
        char line[config->maxlinelength];
        // attribute and value of line
        char *attribute, *value;


        // temporary allocate space for configurations
        out( MSGINFO, config, "readLogfilesConfig(): Allocating memory for the temporary logfiles config structure\n" );
        struct logfilesConfig *tmpLfsC;
        tmpLfsC = malloc( sizeof(struct logfilesConfig) );
        tmpLfsC->configs = malloc( sizeof(struct logfileConfig[config->maxnumlogfiles]) );
        tmpLfsC->num = 0;
        for( akt_file = 0; akt_file < config->maxnumlogfiles; akt_file++ )
        {
            tmpLfsC->configs[akt_file].path = malloc( sizeof(char[config->maxlinelength]) );
            tmpLfsC->configs[akt_file].name = malloc( sizeof(char[config->maxlinelength]) );
            tmpLfsC->configs[akt_file].offset = 0;
            tmpLfsC->configs[akt_file].save = 0;
            tmpLfsC->configs[akt_file].pos = 0;

            tmpLfsC->configs[akt_file].debug = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numDebug = 0;
            tmpLfsC->configs[akt_file].info = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numInfo = 0;
            tmpLfsC->configs[akt_file].note = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numNote = 0;
            tmpLfsC->configs[akt_file].warn = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numWarn = 0;
            tmpLfsC->configs[akt_file].error = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numError = 0;
            tmpLfsC->configs[akt_file].crit = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numCrit = 0;
            tmpLfsC->configs[akt_file].alert = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numAlert = 0;
            tmpLfsC->configs[akt_file].emerg = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numEmerg = 0;
            tmpLfsC->configs[akt_file].skip = malloc( sizeof(char[config->maxnumpatterns][config->maxlinelength]) );
            tmpLfsC->configs[akt_file].numSkip = 0;

            for( akt_token = 0; akt_token < config->maxnumpatterns; akt_token++ )
            {
                tmpLfsC->configs[akt_file].debug[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].debug[akt_token], "" );
                tmpLfsC->configs[akt_file].info[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].info[akt_token], "" );
                tmpLfsC->configs[akt_file].note[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].note[akt_token], "" );
                tmpLfsC->configs[akt_file].warn[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].warn[akt_token], "" );
                tmpLfsC->configs[akt_file].error[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].error[akt_token], "" );
                tmpLfsC->configs[akt_file].crit[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].crit[akt_token], "" );
                tmpLfsC->configs[akt_file].alert[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].alert[akt_token], "" );
                tmpLfsC->configs[akt_file].emerg[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].emerg[akt_token], "" );
                tmpLfsC->configs[akt_file].skip[akt_token] = malloc( sizeof(char[config->maxlinelength]) );
                strcpy( tmpLfsC->configs[akt_file].skip[akt_token], "" );
            }
        }

        // open logs config file
        if( (file = fopen(config->logfilesconf, "r")) )
        {
            out( MSGNOTICE, config, "readLogfilesConfig(): Successfully opened %s\n", config->logfilesconf );

            // read from file
            while( fgets( line, config->maxlinelength, file ) != NULL )
            {
                // skip all comment lines
                if( line[0] == '#' )
                {
                    out( MSGDEBUG, config, "readLogfilesConfig(): Got a comment line: %s\n", line );
                    continue;
                }

                // check if maximum of logfiles is reached
                if( tmpLfsC->num >= config->maxnumlogfiles )
                {
                    out( MSGWARN, config, "readLogfilesConfig(): Maximum number of logfiles reached, breaking\n" );
                    break;
                }

                // a new logfile configuration starts
                if( line[0] == '/' )
                {
                    out( MSGDEBUG, config, "readLogfilesConfig(): Got a logfile path line: %s\n", line );
                    tmpLfsC->configs[tmpLfsC->num].numDebug = \
                    tmpLfsC->configs[tmpLfsC->num].numInfo = \
                    tmpLfsC->configs[tmpLfsC->num].numNote = \
                    tmpLfsC->configs[tmpLfsC->num].numWarn = \
                    tmpLfsC->configs[tmpLfsC->num].numError = \
                    tmpLfsC->configs[tmpLfsC->num].numCrit = \
                    tmpLfsC->configs[tmpLfsC->num].numAlert = \
                    tmpLfsC->configs[tmpLfsC->num].numEmerg = \
                    tmpLfsC->configs[tmpLfsC->num].numSkip = 0;
                    tmpLfsC->num++;
                    char *path = strtok(line, "\t =\n\r");
                    free( tmpLfsC->configs[tmpLfsC->num-1].path );
                    tmpLfsC->configs[tmpLfsC->num-1].path = malloc( strlen(path)+1 );
                    strcpy( tmpLfsC->configs[tmpLfsC->num-1].path, path );
                    free( tmpLfsC->configs[tmpLfsC->num-1].name );
                    tmpLfsC->configs[tmpLfsC->num-1].name = malloc( strlen(path)+1 );
                    strcpy( tmpLfsC->configs[tmpLfsC->num-1].name, path );
                    continue;
                }

                // options before a given logfilepath will be ignored
                if( tmpLfsC->num > 0 )
                {
                    // first, get the attribute
                    attribute = strtok( line, "\t =\n\r" );
                    if( attribute != NULL )
                    {
                        // check which attribute and then get&set value
                        switch( attribute[0] )
                        {
                            case 'n':   value = strtok( NULL, "\t =\n\r" );
                                        free( tmpLfsC->configs[tmpLfsC->num-1].name );
                                        tmpLfsC->configs[tmpLfsC->num-1].name = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].name, value );
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a name value: %s - %s\n", attribute, value );
                                        break;
                            case 'O':   value = strtok( NULL, "\t =\n\r" );
                                        tmpLfsC->configs[tmpLfsC->num-1].offset = atol( value );
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got an offset value: %s - %s\n", attribute, value );
                                        break;
                            case 'S':   value = strtok( NULL, "\t =\n\r" );
                                        tmpLfsC->configs[tmpLfsC->num-1].save = atoi( value );
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a savepos value: %s - %s\n", attribute, value );
                                        break;
                            case '!':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numSkip >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum skip pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].skip[tmpLfsC->configs[tmpLfsC->num-1].numSkip] );
                                        tmpLfsC->configs[tmpLfsC->num-1].skip[tmpLfsC->configs[tmpLfsC->num-1].numSkip] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].skip[tmpLfsC->configs[tmpLfsC->num-1].numSkip], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numSkip++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a skip value: %s - %s\n", attribute, value );
                                        break;
                            case 'G':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numEmerg >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum emergency pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].emerg[tmpLfsC->configs[tmpLfsC->num-1].numEmerg] );
                                        tmpLfsC->configs[tmpLfsC->num-1].emerg[tmpLfsC->configs[tmpLfsC->num-1].numEmerg] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].emerg[tmpLfsC->configs[tmpLfsC->num-1].numEmerg], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numEmerg++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got an emergence value: %s - %s\n", attribute, value );
                                        break;
                            case 'A':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numAlert >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum alert pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].alert[tmpLfsC->configs[tmpLfsC->num-1].numAlert] );
                                        tmpLfsC->configs[tmpLfsC->num-1].alert[tmpLfsC->configs[tmpLfsC->num-1].numAlert] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].alert[tmpLfsC->configs[tmpLfsC->num-1].numAlert], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numAlert++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got an alert value: %s - %s\n", attribute, value );
                                        break;
                            case 'C':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numCrit >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum critical pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].crit[tmpLfsC->configs[tmpLfsC->num-1].numCrit] );
                                        tmpLfsC->configs[tmpLfsC->num-1].crit[tmpLfsC->configs[tmpLfsC->num-1].numCrit] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].crit[tmpLfsC->configs[tmpLfsC->num-1].numCrit], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numCrit++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a critical value: %s - %s\n", attribute, value );
                                        break;
                            case 'E':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numError >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum error pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].error[tmpLfsC->configs[tmpLfsC->num-1].numError] );
                                        tmpLfsC->configs[tmpLfsC->num-1].error[tmpLfsC->configs[tmpLfsC->num-1].numError] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].error[tmpLfsC->configs[tmpLfsC->num-1].numError], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numError++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got an error value: %s - %s\n", attribute, value );
                                        break;
                            case 'W':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numWarn >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum warning pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].warn[tmpLfsC->configs[tmpLfsC->num-1].numWarn] );
                                        tmpLfsC->configs[tmpLfsC->num-1].warn[tmpLfsC->configs[tmpLfsC->num-1].numWarn] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].warn[tmpLfsC->configs[tmpLfsC->num-1].numWarn], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numWarn++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a warning value: %s - %s\n", attribute, value );
                                        break;
                            case 'N':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numNote >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum notice pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].note[tmpLfsC->configs[tmpLfsC->num-1].numNote] );
                                        tmpLfsC->configs[tmpLfsC->num-1].note[tmpLfsC->configs[tmpLfsC->num-1].numNote] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].note[tmpLfsC->configs[tmpLfsC->num-1].numNote], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numNote++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a notice value: %s - %s\n", attribute, value );
                                        break;
                            case 'I':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numInfo >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum information pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].info[tmpLfsC->configs[tmpLfsC->num-1].numInfo] );
                                        tmpLfsC->configs[tmpLfsC->num-1].info[tmpLfsC->configs[tmpLfsC->num-1].numInfo] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].info[tmpLfsC->configs[tmpLfsC->num-1].numInfo], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numInfo++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got an information value: %s - %s\n", attribute, value );
                                        break;
                            case 'D':   value = strtok( NULL, "\t =\n\r" );
                                        if( tmpLfsC->configs[tmpLfsC->num-1].numDebug >= config->maxnumpatterns )
                                        {
                                            out( MSGWARN, config, "readLogfilesConfig(): Maximum debug pattern reached, discarding >%s<\n", value );
                                            break;
                                        }
                                        if( strlen(value) > longest ) { longest = strlen(value)+1; }
                                        free( tmpLfsC->configs[tmpLfsC->num-1].debug[tmpLfsC->configs[tmpLfsC->num-1].numDebug] );
                                        tmpLfsC->configs[tmpLfsC->num-1].debug[tmpLfsC->configs[tmpLfsC->num-1].numDebug] = malloc( strlen(value)+1 );
                                        strcpy( tmpLfsC->configs[tmpLfsC->num-1].debug[tmpLfsC->configs[tmpLfsC->num-1].numDebug], value );
                                        tmpLfsC->configs[tmpLfsC->num-1].numDebug++;
                                        out( MSGDEBUG, config, "readLogfilesConfig(): Got a debug value: %s - %s\n", attribute, value );
                                        break;
                        }
                    }
                    // if wanted, get the last read position for the logfile
                    if( tmpLfsC->configs[tmpLfsC->num-1].save > 0 && tmpLfsC->configs[tmpLfsC->num-1].pos == 0 )
                    {
                        out( MSGDEBUG, config, "readLogfilesConfig(): Going to read the position of %s\n", tmpLfsC->configs[tmpLfsC->num-1].path );
                        tmpLfsC->configs[tmpLfsC->num-1].pos = readPosition( config, &tmpLfsC->configs[tmpLfsC->num-1] );
                    }
                }
            }
            // close file
            fclose( file );

            // allocate space for configurations and copy from temporary struct, with decreased size
            out( MSGINFO, config, "readLogfilesConfig(): Allocating memory for the final logfiles config structure\n" );
            newLogfilesConf->configs = malloc( sizeof(struct logfileConfig[tmpLfsC->num]) );
            newLogfilesConf->num = tmpLfsC->num;
            for( akt_file = 0; akt_file < tmpLfsC->num; akt_file++ )
            {
                newLogfilesConf->configs[akt_file].path = malloc( strlen(tmpLfsC->configs[akt_file].path)+1 );
                strcpy( newLogfilesConf->configs[akt_file].path, tmpLfsC->configs[akt_file].path );
                newLogfilesConf->configs[akt_file].name = malloc( strlen(tmpLfsC->configs[akt_file].name)+1 );
                strcpy( newLogfilesConf->configs[akt_file].name, tmpLfsC->configs[akt_file].name );
                newLogfilesConf->configs[akt_file].offset = tmpLfsC->configs[akt_file].offset;
                newLogfilesConf->configs[akt_file].save = tmpLfsC->configs[akt_file].save;
                newLogfilesConf->configs[akt_file].pos = tmpLfsC->configs[akt_file].pos;

                // initialize the mutex of the logfile config
                if( pthread_mutex_init(&newLogfilesConf->configs[akt_file].mutex, NULL) == 0 )
                {
                    out( MSGDEBUG, config, "readLogfilesConfig(): Initialised the mutex of the logfile config structure %d\n",akt_file );
                }
                else
                {
                    out( MSGERROR, config, "readLogfilesConfig(): Error initialising the mutex of the base structure\n" );

                    // return with failure
                    return EXIT_FAILURE;
                }

                newLogfilesConf->configs[akt_file].debug = malloc( sizeof(char[tmpLfsC->configs[akt_file].numDebug][longest]) );
                newLogfilesConf->configs[akt_file].numDebug = tmpLfsC->configs[akt_file].numDebug;
                newLogfilesConf->configs[akt_file].info = malloc( sizeof(char[tmpLfsC->configs[akt_file].numInfo][longest]) );
                newLogfilesConf->configs[akt_file].numInfo = tmpLfsC->configs[akt_file].numInfo;
                newLogfilesConf->configs[akt_file].note = malloc( sizeof(char[tmpLfsC->configs[akt_file].numNote][longest]) );
                newLogfilesConf->configs[akt_file].numNote = tmpLfsC->configs[akt_file].numNote;
                newLogfilesConf->configs[akt_file].warn = malloc( sizeof(char[tmpLfsC->configs[akt_file].numWarn][longest]) );
                newLogfilesConf->configs[akt_file].numWarn = tmpLfsC->configs[akt_file].numWarn;
                newLogfilesConf->configs[akt_file].error = malloc( sizeof(char[tmpLfsC->configs[akt_file].numError][longest]) );
                newLogfilesConf->configs[akt_file].numError = tmpLfsC->configs[akt_file].numError;
                newLogfilesConf->configs[akt_file].crit = malloc( sizeof(char[tmpLfsC->configs[akt_file].numCrit][longest]) );
                newLogfilesConf->configs[akt_file].numCrit = tmpLfsC->configs[akt_file].numCrit;
                newLogfilesConf->configs[akt_file].alert = malloc( sizeof(char[tmpLfsC->configs[akt_file].numAlert][longest]) );
                newLogfilesConf->configs[akt_file].numAlert = tmpLfsC->configs[akt_file].numAlert;
                newLogfilesConf->configs[akt_file].emerg = malloc( sizeof(char[tmpLfsC->configs[akt_file].numEmerg][longest]) );
                newLogfilesConf->configs[akt_file].numEmerg = tmpLfsC->configs[akt_file].numEmerg;
                newLogfilesConf->configs[akt_file].skip = malloc( sizeof(char[tmpLfsC->configs[akt_file].numSkip][longest]) );
                newLogfilesConf->configs[akt_file].numSkip = tmpLfsC->configs[akt_file].numSkip;

                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numDebug; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].debug[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].debug[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].debug[akt_token], tmpLfsC->configs[akt_file].debug[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numInfo; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].info[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].info[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].info[akt_token], tmpLfsC->configs[akt_file].info[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numNote; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].note[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].note[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].note[akt_token], tmpLfsC->configs[akt_file].note[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numWarn; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].warn[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].warn[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].warn[akt_token], tmpLfsC->configs[akt_file].warn[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numError; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].error[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].error[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].error[akt_token], tmpLfsC->configs[akt_file].error[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numCrit; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].crit[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].crit[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].crit[akt_token], tmpLfsC->configs[akt_file].crit[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numAlert; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].alert[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].alert[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].alert[akt_token], tmpLfsC->configs[akt_file].alert[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numEmerg; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].emerg[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].emerg[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].emerg[akt_token], tmpLfsC->configs[akt_file].emerg[akt_token] );
                }
                for( akt_token = 0; akt_token < newLogfilesConf->configs[akt_file].numSkip; akt_token++ )
                {
                    newLogfilesConf->configs[akt_file].skip[akt_token] = malloc( strlen(tmpLfsC->configs[akt_file].skip[akt_token])+1 );
                    strcpy( newLogfilesConf->configs[akt_file].skip[akt_token], tmpLfsC->configs[akt_file].skip[akt_token] );
                }
            }

            // free memory of temporary logfile configurations structure
            out( MSGINFO, config, "readLogfilesConfig(): Freeing memory of the temporary logfiles config structure\n" );
            for( akt_file = 0; akt_file < config->maxnumlogfiles; akt_file++ )
            {
                free( tmpLfsC->configs[akt_file].name );
                free( tmpLfsC->configs[akt_file].path );
                for( akt_token = 0; akt_token < config->maxnumpatterns; akt_token++ )
                {
                    free( tmpLfsC->configs[akt_file].debug[akt_token] );
                    free( tmpLfsC->configs[akt_file].info[akt_token] );
                    free( tmpLfsC->configs[akt_file].note[akt_token] );
                    free( tmpLfsC->configs[akt_file].warn[akt_token] );
                    free( tmpLfsC->configs[akt_file].error[akt_token] );
                    free( tmpLfsC->configs[akt_file].crit[akt_token] );
                    free( tmpLfsC->configs[akt_file].alert[akt_token] );
                    free( tmpLfsC->configs[akt_file].emerg[akt_token] );
                    free( tmpLfsC->configs[akt_file].skip[akt_token] );
                }
                free( tmpLfsC->configs[akt_file].debug );
                free( tmpLfsC->configs[akt_file].info );
                free( tmpLfsC->configs[akt_file].note );
                free( tmpLfsC->configs[akt_file].warn );
                free( tmpLfsC->configs[akt_file].error );
                free( tmpLfsC->configs[akt_file].crit );
                free( tmpLfsC->configs[akt_file].alert );
                free( tmpLfsC->configs[akt_file].emerg );
                free( tmpLfsC->configs[akt_file].skip );
            }
            free( tmpLfsC->configs );
            free( tmpLfsC );


            // initialize the mutex of the config
            if( pthread_mutex_init(&newLogfilesConf->mutex, NULL) == 0 )
            {
                // return with success
                return EXIT_SUCCESS;
            }
            else
            {
                // return with failure
                return EXIT_FAILURE;
            }
        }
        else
        {
            out( MSGERROR, config, "readLogfilesConfig(): Unable to open %s\n", config->logfilesconf );

            // return with failure
            return EXIT_FAILURE;
        }
    }
    else
    {
        out( MSGCRIT, config, "readDaemonConfig(): The given daemon config structure is not valid \n" );

        // return with failure
        return EXIT_FAILURE;
    }
}


// function to unallocate a daemon configuration
void destroyDaemonConfig( struct daemonConfig *daemonConf )
{
    free( daemonConf->daemonconf );
    free( daemonConf->logfilesconf );
    free( daemonConf->logfile );
    free( daemonConf->savefile );
    free( daemonConf->proto );
    free( daemonConf );
}

// function to unallocate a logfiles configuration
void destroyLogfilesConfig( struct daemonConfig *config, struct logfilesConfig *logfilesConf )
{
    int i, j;
    for( i = 0; i < logfilesConf->num; i++ )
    {
        free( logfilesConf->configs[i].name );
        free( logfilesConf->configs[i].path );
        for( j = 0; j < logfilesConf->configs[i].numDebug; j++ )
        {
            free( logfilesConf->configs[i].debug[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numInfo; j++ )
        {
            free( logfilesConf->configs[i].info[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numNote; j++ )
        {
            free( logfilesConf->configs[i].note[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numWarn; j++ )
        {
            free( logfilesConf->configs[i].warn[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numError; j++ )
        {
            free( logfilesConf->configs[i].error[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numCrit; j++ )
        {
            free( logfilesConf->configs[i].crit[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numAlert; j++ )
        {
            free( logfilesConf->configs[i].alert[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numEmerg; j++ )
        {
            free( logfilesConf->configs[i].emerg[j] );
        }
        for( j = 0; j < logfilesConf->configs[i].numSkip; j++ )
        {
            free( logfilesConf->configs[i].skip[j] );
        }
        free( logfilesConf->configs[i].skip );
        free( logfilesConf->configs[i].debug );
        free( logfilesConf->configs[i].info );
        free( logfilesConf->configs[i].note );
        free( logfilesConf->configs[i].warn );
        free( logfilesConf->configs[i].error );
        free( logfilesConf->configs[i].crit );
        free( logfilesConf->configs[i].alert );
        free( logfilesConf->configs[i].emerg );
        pthread_mutex_destroy( &logfilesConf->configs[i].mutex );
    }
    free( logfilesConf->configs );
    free( logfilesConf );
}


// function to parse the interval parameter and return interval in seconds
unsigned long parseInterval( struct daemonConfig *config, char *intervalPattern )
{
    unsigned long interval = 0;

    switch( intervalPattern[strlen(intervalPattern)-1] )
    {
        case 's':   interval = atol( strtok(intervalPattern,"smhDWMY") );
                    break;
        case 'm':   interval = atol( strtok(intervalPattern,"smhDWMY") ) * 60;
                    break;
        case 'h':   interval = atol( strtok(intervalPattern,"smhDWMY") ) * 3600;
                    break;
        case 'D':   interval = atol( strtok(intervalPattern,"smhDWMY") ) * 86400;
                    break;
        case 'W':   interval = atol( strtok(intervalPattern,"smhDWMY") ) * 604800;
                    break;
        case 'M':   interval = atol( strtok(intervalPattern,"smhDWMY") ) * 2592000;
                    break;
        case 'Y':   interval = atol( strtok(intervalPattern,"smhDWMY") ) * 31536000;
                    break;
        default:    out( MSGERROR, config, "Error parsing interval: %s\n", intervalPattern );
    }

    return interval;
}


// function to print out the daemon configuration
void printDaemonConfig( struct daemonConfig *config )
{
    out( MSGINFO, config, "*****\n" );
    out( MSGINFO, config, "*\n" );
    out( MSGINFO, config, "*  Printing deamon configuration...\n" );
    out( MSGINFO, config, "*  DaemonConf:      %s\n", config->daemonconf );
    out( MSGINFO, config, "*  Interval:        %d\n", config->interval );
    out( MSGINFO, config, "*  MaxNumLogfiles:  %d\n", config->maxnumlogfiles );
    out( MSGINFO, config, "*  MaxLineLength:   %d\n", config->maxlinelength );
    out( MSGINFO, config, "*  MaxNumPatterns:  %d\n", config->maxnumpatterns );
    out( MSGINFO, config, "*  LogFilesConf:    %s\n", config->logfilesconf );
    out( MSGINFO, config, "*  Port:            %d\n", config->port );
    out( MSGINFO, config, "*  Protocol:        %s\n", config->proto );
    out( MSGINFO, config, "*  MaxNumClients:   %d\n", config->maxnumclients );
    out( MSGINFO, config, "*  CliRcvTimOut:    %d\n", config->clircvtimout );
    out( MSGINFO, config, "*  PosSaveFile:     %s\n", config->savefile );
    out( MSGINFO, config, "*  DaemonLogfile:   %s\n", config->logfile );
    out( MSGINFO, config, "*  LogLevel:        %d\n", config->loglevel );
    out( MSGINFO, config, "*  StartAsDaemon:   %d\n", config->rad );
    out( MSGINFO, config, "*\n" );
    out( MSGINFO, config, "*****\n" );
}

// function to print out the logfiles configuration
void printLogfilesConfig( struct daemonConfig *config, struct logfilesConfig logfilesConf )
{
    // counter for actual log file and actual entry
    int akt_file, akt_entry;

    out( MSGINFO, config, "*****\n" );
    out( MSGINFO, config, "*\n" );
    out( MSGINFO, config, "*  Printing logfiles configuration...\n" );
    for( akt_file = 0; akt_file < logfilesConf.num; akt_file++ )
    {
        out( MSGINFO, config, "*  Filepath: %s\n", logfilesConf.configs[akt_file].path );
        out( MSGINFO, config, "*  Filename: %s\n", logfilesConf.configs[akt_file].name );
        out( MSGINFO, config, "*  F-Offset: %d\n", logfilesConf.configs[akt_file].offset );
        out( MSGINFO, config, "*  Save-Pos: %d\n", logfilesConf.configs[akt_file].save );
        out( MSGINFO, config, "*  Position: %d\n", logfilesConf.configs[akt_file].pos );

        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numDebug; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].debug[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Debugpattern %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].debug[akt_entry] );   }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numInfo; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].info[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Infopattern  %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].info[akt_entry] );    }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numNote; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].note[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Notepattern  %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].note[akt_entry] );    }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numWarn; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].warn[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Warnpattern  %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].warn[akt_entry] );    }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numError; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].error[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Errorpattern %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].error[akt_entry] );   }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numCrit; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].crit[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Criticalpattern %d: %s\n", akt_entry, logfilesConf.configs[akt_file].crit[akt_entry] );    }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numAlert; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].alert[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Alertpattern %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].alert[akt_entry] );   }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numEmerg; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].emerg[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Emergencepattern %d: %s\n", akt_entry,logfilesConf.configs[akt_file].emerg[akt_entry] );   }
        }
        for( akt_entry = 0; akt_entry < logfilesConf.configs[akt_file].numSkip; akt_entry++ )
        {
            if( (strlen(logfilesConf.configs[akt_file].skip[akt_entry])) > 0 )
            {   out( MSGINFO, config, "*  Skippattern  %d: %s\n", akt_entry,    logfilesConf.configs[akt_file].skip[akt_entry] );    }
        }
    }
    out( MSGINFO, config, "*\n" );
    out( MSGINFO, config, "*****\n" );
}


// function to save the reading position of a logfile
int savePosition( struct daemonConfig *config, struct logfileConfig *fileConf )
{
    // buffer for a single line of a file
    char *line = malloc( sizeof(char[config->maxlinelength]) );
    // entry for file in position conf found
    char found = 0;
    // array for position entries
    struct logPosition positions[config->maxnumlogfiles];
    // counter for actual log file
    int akt_logfile = 0, i = 0;


    // open file for reading
    FILE * posFile = fopen( config->savefile, "r" );

    if( posFile == NULL )
    {
        out( MSGERROR, config, "savePosition(): Unable to open %s for reading\n", config->savefile );
    }
    else
    {
        out( MSGDEBUG, config, "savePosition(): Going to find the last read position of %s\n", fileConf->path );

        // read from logfile...
        while( fgets( line, config->maxlinelength, posFile ) != NULL )
        {
            // ...and see if an entry for the file exists
            if( strstr(line, fileConf->path) != NULL )                                                    //TODO
            {
                out( MSGDEBUG, config, "savePosition(): Found a position entry for %s\n", fileConf->path );
                found = 1;
            }
            // copy all old entries
            if( strstr(line,"/") != NULL )
            {
                out( MSGDEBUG, config, "savePosition(): Copying the last read position of %s\n", line );
                positions[akt_logfile].path = malloc( strlen(line)+1 );
                strcpy( positions[akt_logfile].path, strtok(line,"\t\n\r") );
            }
            else
            {
//                out( MSGDEBUG, config, "savePosition(): The last read position of %s is @ %d\n", positions[akt_logfile].path, atoi(line) );
                positions[akt_logfile++].position = atol( line );
            }
        }

        // free memory of line buffer
        free( line );

        // close position save file
        fclose( posFile );
    }

    // if an old entry for the file exists...
    if( found )
    {
        // open the positions file in write mode
        posFile = fopen( config->savefile, "w" );

        if( posFile == NULL )
        {
            out( MSGERROR, config, "savePosition(): Unable to open %s for writing\n", config->savefile );

            // return with failure
            return EXIT_FAILURE ;
        }
        else
        {
            out( MSGNOTICE, config, "savePosition(): Saving position of %s (%s) @ %ld\n", fileConf->name, fileConf->path, fileConf->pos );

            // ...then we will write the positions file again
            for( i = 0; i < akt_logfile; i++ )
            {
                // actualise old entry with new position-value or insert the old ones
                if( strstr(positions[i].path,fileConf->path) != NULL )
                {
                    fprintf( posFile, "%s\n%ld\n", strtok(positions[i].path,"\t\n\r"), fileConf->pos );
                }
                else
                {
                    fprintf( posFile, "%s\n%ld\n", strtok(positions[i].path,"\t\n\r"), positions[i].position );
                }
            }
            // close position save file
            fclose( posFile );
        }
    }
    else // if no entry for the file exists...
    {
        // open the positions file in append mode
        posFile = fopen( config->savefile, "a+" );

        if( posFile == NULL )
        {
            out( MSGERROR, config, "savePosition(): Unable to open %s for appending\n", config->savefile );

            // return with failure
            return EXIT_FAILURE ;
        }
        else
        {
            out( MSGNOTICE, config, "savePosition(): Saving position of %s (%s) @ %ld, first time\n", fileConf->name, fileConf->path, fileConf->pos );

            // ...then we will append a new entry
            fprintf( posFile, "%s\n%ld\n", strtok(fileConf->path,"\t\n\r"), fileConf->pos );

            // close position save file
            fclose( posFile );
        }
    }

    // free temporary allocated memory
    for( i = 0; i < akt_logfile; i++ )
    {
        if( positions[i].path )
        {
            free( positions[i].path );
        }
    }

    // return with successfull
    return EXIT_SUCCESS;
}

// function to read the last position of a logfile
long readPosition( struct daemonConfig *config, struct logfileConfig *fileConf )
{
    // single line of file
    char line[config->maxlinelength];
    // position value
    long position = 0;


    // open file for reading
    FILE * posFile = fopen( config->savefile, "r" );

    if( posFile == NULL )
    {
        out( MSGERROR, config, "readPosition(): Unable to open %s\n", config->savefile );

        // return zero as start position
        return 0;
    }
    else
    {
        out( MSGDEBUG, config, "readPosition(): Going to get the last read position of %s\n", fileConf->path );

        // read from position save file...
        while( fgets( line, config->maxlinelength, posFile ) != NULL )
        {
            // ...and see if an entry for the file exists
            if( strstr(line,fileConf->path) != NULL )
            {
                out( MSGDEBUG, config, "readPosition(): Found an entry for %s\n", fileConf->path );
                break;
            }
        }

        // get the value/position
        if( fgets(line, config->maxlinelength, posFile) != NULL )
        {
            position = atol( line );
            out( MSGNOTICE, config, "readPosition(): Read position of %s (%s) @ %ld\n", fileConf->name, fileConf->path, position );
        }

        // close the file
        fclose( posFile );
    }

    // return with the last position or 0
    if( position > 0 && position < LONG_MAX )
    {
        // return the position
        return position;
    }
    else
    {
        // return zero as start position
        return 0;
    }
}
