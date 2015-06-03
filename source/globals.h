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

#ifndef _globals_
#define _globals_

#include <arpa/inet.h>
#include <getopt.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "base.h"
#include "conf.h"
#include "queue.h"
#include "watcher.h"
#include "net.h"

// initial and fallback parameters
#define MAX_LINE_LEN    8192    // max line length of a logfile to parse
#define MAX_NUM_FILES    100    // max number of logfiles to be opened
#define MAX_NUM_PATTERNS  10    // max number of keywords for file parsing
#define MAX_NUM_CLIENTS   10    // max number of possible network connections
#define CLI_RCV_TIMEOUT    2    // client receive timeout in seconds
#define MAX_BUF_SIZE    1400    // max buffer size for network packets
#define NETWORK_PORT      26    // default network port for listening
#define DEF_PROTO       "tcp"   // default network protocol
#define DEF_INTERVAL       3    // default interval for parsing logfiles
#define DEF_LOG_LEVEL      3    // default log level (unused level)

// syslog message severity levels as described in rfc 5424
#define MSGEMERG    0           // severity level (G) emergency: system is unusable
#define MSGALERT    1           // severity level (A) alert: action must be taken immediately
#define MSGCRIT     2           // severity level (C) critical: critical conditions
#define MSGERROR    3           // severity level (E) error: error conditions
#define MSGWARN     4           // severity level (W) warning: warning conditions
#define MSGNOTICE   5           // severity level (N) notice: normal but significant condition
#define MSGINFO     6           // severity level (I) information: informational messages
#define MSGDEBUG    7           // severity level (D) debug: debug-level messages
#define MSGDBGEX    8           // output only level  dbgex: debug-extended

#define TIMELENGTH 30           // length of a rfc conform date-time string


// function to check if a file exists
int fileExists( const char *filePath );

// function for output ( terminal / logfile )
void out( char level, struct daemonConfig *config, const char *msg, ... );

// function to get a timestamp as in RFC3339/5424
int getTime( char *strTime );


#endif
