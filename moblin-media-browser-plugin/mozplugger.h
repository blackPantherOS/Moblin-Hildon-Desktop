#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sysexits.h>
#include <signal.h>
#include <stdarg.h>
#include <npapi.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sys/socket.h>

/*****************************************************************************
 * Defines
 *****************************************************************************/

#ifndef MAXINT
#define MAXINT 0x7fffffff
#endif

#define MAX(X,Y) ((X)>(Y)?(X):(Y))

/* Timeout for select() */
#define SELECT_TIMEOUT_SEC  0
#define SELECT_TIMEOUT_USEC 100*1000

/* Time to wait for a process to exit */
#define KILL_TIMEOUT_USEC 100000

/* Each handler has a list of mime types, and a list of commands. */
#define MAX_NUM_HANDLERS 64
#define MAX_NUM_TYPES_PER_HANDLER 32
#define MAX_NUM_CMDS_PER_HANDLER 32

#define MAX_STATIC_MEMORY_POOL   65536

/* Maximum size of the buffer used for environment variables. */
#define ENV_BUFFER_SIZE 16348

#define LARGE_BUFFER_SIZE 16384
#define LARGE_BUFFER_SIZE_STR "16384"

#define SMALL_BUFFER_SIZE 128
#define SMALL_BUFFER_SIZE_STR "128"
#define FIND_CACHE_SIZE   10

/* Flags */
#define H_LOOP 		0x1
#define H_DAEMON 	0x2
#define H_STREAM 	0x4
#define H_NOISY 	0x8
#define H_REPEATCOUNT 	0x10
#define H_EMBED         0x20
#define H_NOEMBED       0x40
#define H_IGNORE_ERRORS 0x80
#define H_SWALLOW 	0x100
#define H_MAXASPECT 	0x200
#define H_FILL 		0x400
#define H_HIDDEN        0x800
#define H_CONTROLS 	0x1000
#define H_LINKS         0x2000
#define H_FMATCH        0x4000

/*****************************************************************************
 * mozplugger-common.c functions
 *****************************************************************************/
#ifdef DEBUG
void close_debug();
#endif
void D(char *fmt, ...);
void my_kill(pid_t pid);
