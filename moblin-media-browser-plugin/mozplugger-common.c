/*****************************************************************************
 * Authors: Louis Bavoil <bavoil@cs.utah.edu>
 *          Fredrik Hübinette <hubbe@hubbe.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include "mozplugger.h"

/*****************************************************************************
 * Debug function
 *****************************************************************************/
#ifdef DEBUG

#define DEBUG_FILENAME "mozdebug"

static FILE *debug_output;

static FILE *getout()
{
     char debug_filename[128];
     char *tmpdir;

     if (debug_output)
	  return debug_output;

     if (!(tmpdir = getenv("TMPDIR")))
     {
         /* If (as on default Fedora 7 install) the environment variable 
          * TMPDIR is not defined, just assume tmp */
         tmpdir = "tmp";
     }

     snprintf(debug_filename, sizeof(debug_filename),
	      "%s/%s", tmpdir, DEBUG_FILENAME);
     
     if (!(debug_output = fopen(debug_filename,"a+")))
	  return NULL;
     fprintf(debug_output, "------------\n");
     return debug_output;
}

void close_debug()
{
     FILE *f;
     if ((f = getout()))
	  fclose(f);
     debug_output = NULL;
}

void D(char *fmt, ...)
{
     FILE *f;
     char buffer[9999];
     va_list ap;

     va_start(ap,fmt);
     vsnprintf(buffer,sizeof(buffer),fmt,ap);
     va_end(ap);

     if ((f = getout()))
     {
	  fprintf(f,"PID%4d: %s",(int) getpid(), buffer);
	  fflush(f);
     }
}

#else

void D(char *fmt, ...) { }

#endif

/*****************************************************************************
 * Adaptive kill()
 *****************************************************************************/
void my_kill(pid_t pid)
{
     int status;

     D("Killing PID %d with SIGTERM\n", pid);
     if (!kill(pid, SIGTERM))
     {
	  usleep(KILL_TIMEOUT_USEC);
	  D("Killing PID %d with SIGTERM\n", pid);
	  if (!kill(pid, SIGTERM))
	  {
	       usleep(KILL_TIMEOUT_USEC);
   	       D("Killing PID %d with SIGTERM\n", pid);
	       if (!kill(pid, SIGTERM))
	       {
		    D("Killing PID %d with SIGKILL\n", pid);
		    kill(pid, SIGKILL);
	       }
	  }
     }

     D("Waiting for sons\n");
     while (waitpid(-1, &status, WNOHANG) > 0);
}
