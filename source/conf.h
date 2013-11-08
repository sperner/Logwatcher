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

#ifndef _conf_
#define _conf_

#include "globals.h"

#define DAEMON_CONF_FILE    "/etc/logwatcher.conf"              // configuration file for daemon process
#define LOGS_CONF_FILE      "/etc/logwatcher_files.conf"        // configuration file for monitored logfiles
#define DAEMON_LOGFILE      "/var/log/logwatcher.log"           // output logfile of the daemon process
#define TMP_LOGFILE         "/tmp/logwatcher.log"               // if run as daemon and logfile not ready
#define POSITION_FILE       "/var/cache/logwatcher/positions"   // to save last read position of logfile


// structure for daemon config
typedef struct daemonConfig{
    unsigned long interval;         // interval between each parsing of logfiles
    unsigned long maxnumlogfiles;   // maximum number of logfiles to be monitored
    unsigned long maxlinelength;    // maximum length of a line
    unsigned long maxnumpatterns;   // maximum number of patterns per patterntype
    char *daemonconf;               // path to the daemons configuration file
    char *logfilesconf;             // path to the logfiles configuration file
    unsigned int port;              // network port for listening of incoming connections
    char *proto;                    // used network protocol
    unsigned long maxnumclients;    // maximum number of client connections
    unsigned long clircvtimout;     // client receive timeout in seconds
    char *savefile;                 // path to the last read positions file
    char *logfile;                  // path to the daemons logfile
    char loglevel;                  // loglevel for output to stdout/logfile
    char rad;                       // run-as-daemon (run logwatcher in daemon mode)
    pthread_mutex_t mutex;          // mutex for out()
} daemonConfig;

// structure for a logfile config
typedef struct logfileConfig{
    char *path;                     // path to the logfile being monitored
    char *name;                     // custom name of the logfile
    unsigned long offset;           // skip first number of lines
    char save;                      // save the last read position to file?
    unsigned long pos;              // last read position of logfile
    char **skip;                    // skip every skip-pattern matching line
    unsigned long numSkip;          // actual number of skip patterns
    char **debug;                   // severity level debug: debug-level messages
    unsigned long numDebug;         // actual number of debug patterns
    char **info;                    // severity level information: informational messages
    unsigned long numInfo;          // actual number of information patterns
    char **note;                    // severity level notice: normal but significant condition
    unsigned long numNote;          // actual number of noice patterns
    char **warn;                    // severity level warning: warning conditions
    unsigned long numWarn;          // actual number of warning patterns
    char **error;                   // severity level error: error conditions
    unsigned long numError;         // actual number of error patterns
    char **crit;                    // severity level critical: critical conditions
    unsigned long numCrit;          // actual number of critical patterns
    char **alert;                   // severity level alert: action must be taken immediately
    unsigned long numAlert;         // actual number of alert patterns
    char **emerg;                   // severity level emergency: system is unusable
    unsigned long numEmerg;         // actual number of emergency patterns
    pthread_mutex_t mutex;          // mutex for watcher/logfileconfig
} logfileConfig;

// structure for all log configs
typedef struct logfilesConfig{
    int num;                        // actual number of logfiles being monitored
    struct logfileConfig *configs;  // array of logfile configurations
    pthread_mutex_t mutex;          // mutex for watcher/logfileconfig
} logfilesConfig;

// structure for reading positions
typedef struct logPosition{
    char *path;                     // full path to the logfile
    unsigned long position;         // last red position of the logfile
} logPosition;


// function to initialise daemons configuration
int initDaemonConfig( struct daemonConfig *newDaemonConf );


// function to open and read from daemon config file
int readDaemonConfig( const char *configFilePath, struct daemonConfig *newDaemonConf );

// function to open and read from logfiles config file
int readLogfilesConfig( struct daemonConfig *config, struct logfilesConfig *newLogfilesConf );


// function to unallocate the memory of a daemon configuration
void destroyDaemonConfig( struct daemonConfig *daemonConf );

// function to unallocate the memory of a logfiles configuration
void destroyLogfilesConfig( struct daemonConfig *config, struct logfilesConfig *logfilesConf );


// function to parse the interval parameter and return interval in seconds
unsigned long parseInterval( struct daemonConfig *config, char *intervalPattern );


// function to print out the daemon configuration
void printDaemonConfig( struct daemonConfig *config );

// function to print out the logfiles configuration
void printLogfilesConfig( struct daemonConfig *config, struct logfilesConfig logfilesConf );


// function to save the reading position of a logfile
int savePosition( struct daemonConfig *config, struct logfileConfig *fileConf );

// function to read the last position of a logfile
long readPosition( struct daemonConfig *config, struct logfileConfig *fileConf );


#endif
