/*****************************************************************************
 * Authors: Louis Bavoil <bavoil@cs.utah.edu>
 *          Peter Leese <peter@leese.net>
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

#define THIS ((data_t *)(instance->pdata))

#ifndef __GNUC__
#define __inline
#endif

/*****************************************************************************
 * Type declarations
 *****************************************************************************/
typedef struct argument
{
     char *name;
     char *value;
} argument_t;

typedef struct data
{
     Display *display;
     char *displayname;
     NPWindow windata;
     pid_t pid;
     int fd;
     int repeats;
     int cmd_flags;
     const char *command;
     const char *winname;
     int mode_flags;
     char *mimetype;
     char *href;
     char *mms;
     char autostart;
     char autostartNotSeen;
     int num_arguments;
     struct argument *args;
} data_t;

typedef struct mimetype
{
     const char * type;
} mimetype_t;

typedef struct command
{
     int flags;
     const char * cmd;
     const char * winname;
     const char * fmatchStr;
} command_t;

typedef struct handle
{
     int num_types;
     int num_cmds;
     mimetype_t types[MAX_NUM_TYPES_PER_HANDLER];
     command_t cmds[MAX_NUM_CMDS_PER_HANDLER];
} handler_t;

typedef struct
{
     const char *name;
     int value;
} flag_t;

/*****************************************************************************
 * Global variables
 *****************************************************************************/
static char *config_fname;
static char *helper_fname;
static char *controller_fname;
static int num_handlers;
static handler_t handlers[MAX_NUM_HANDLERS];
static int browserApiMajorVer;
static int browserApiMinorVer;

/*****************************************************************************
 * Wrapper for fork() which handles some magic needed to prevent netscape
 * from freaking out. It also prevents interference from signals.
 * Also it closes all those file descriptors inherited from the parent 
 * (except STDIN, STDOUT & STDERR (0,1,2))
 *
 *****************************************************************************/
static pid_t my_fork(NPP instance, int pipeFd)
{
     pid_t pid;
     sigset_t set;
     sigset_t oset;
     int maxFds;

     /* Mask all the signals to avoid being interrupted by a signal */
     sigfillset(&set);
     sigprocmask(SIG_SETMASK, &set, &oset);

#if 0      /* If sysconf not support do this? */
     {
          struct rlimit rl;
          if (getrlimit(RLIMIT_NOFILE, &fdl) < 0)
          {
               maxFds = 256;
          }
          else if (rl.rlim_max == RLIM_INFINITY)
          {
               maxFds = 1024;
          }
          else
          {
              maxFds = rl.rlim_max;
          }
     }
#else
     maxFds = sysconf(_SC_OPEN_MAX);
#endif

     D(">>>>>>>>Forking<<<<<<<<,\n");

     if ((pid = fork()) == -1)
     {
	  return -1;
     }

     if (pid == 0)
     {
          int i;
          int signum;

	  alarm(0);

	  if (!(THIS->cmd_flags & H_DAEMON))
	       setsid();

	  for (signum=0; signum < NSIG; signum++)
	       signal(signum, SIG_DFL);

          /* Close all those File descriptors inherited from the
             parent (except STDIN, STDOUT, STDERR and pipeFd. */
#ifdef DEBUG
	  close_debug();
#endif
          D("Closing up to %i Fds\n", maxFds);
          for (i = 3; i < pipeFd; i++)
          {
               close(i);
          }
          for (i = pipeFd+1; i < maxFds; i++)
          {
               close(i);
          }

#ifdef DEBUG
	  D("Child re-opened ndebug\n");
#endif
     }
     else
     {
	  D("Child running with pid=%d\n", pid);
     }

     /* Restore the signal mask */
     sigprocmask(SIG_SETMASK, &oset, &set);

     return pid;
}

/*****************************************************************************
 * Wrapper for putenv().
 *****************************************************************************/
static void my_putenv(char *buffer, int *offset, const char *var, 
                                                 const char *value)
{
     int l = strlen(var) + strlen(value) + 2;
     if (*offset + l >= ENV_BUFFER_SIZE)
     {
	  D("Buffer overflow in putenv(%s=%s)\n", var, value);
	  return;
     }
     
     snprintf(buffer+*offset, l, "%s=%s", var, value);
     putenv(buffer+*offset);
     *offset += l;
}

/*****************************************************************************
 * Wrapper for execlp() that calls the helper.
 *
 * WARNING: This function runs in the daughter process so one must assume the
 * daughter uses a copy (including) heap memory of the parent's memory space
 * i.e. any write to memory here does not affect the parent memory space. 
 * Since Linux uses copy-on-write, best leave memory read-only and once execlp
 * is called, all daughter memory is wiped anyway (except the stack).
 *****************************************************************************/
static void run(NPP instance, const char *file, int fd)
{
     char buffer[ENV_BUFFER_SIZE];
     char foo[SMALL_BUFFER_SIZE];
     int offset = 0;
     int i;
     int flags = THIS->cmd_flags;
     int autostart = THIS->autostart;

     /* Specially linking objects to external app must use controls */
     if (THIS->mode_flags & H_LINKS)
	  flags |= H_CONTROLS;

     /* If no autostart seen and using controls, dont autostart by default */
     if ((flags & H_CONTROLS) && (THIS->autostartNotSeen))
	  autostart = 0;

     snprintf(buffer, sizeof(buffer), "%d,%d,%d,%lu,%d,%d,%d,%d",
	      flags,
	      THIS->repeats,
	      fd,
	      (unsigned long int) THIS->windata.window,
	      (int) THIS->windata.x,
	      (int) THIS->windata.y,
	      (int) THIS->windata.width,
	      (int) THIS->windata.height);

     D("Executing helper: %s %s %s %s %s %s\n",
       helper_fname,
       buffer,
       file,
       THIS->displayname,
       THIS->command,
       THIS->mimetype);

     offset = strlen(buffer)+1;

     snprintf(foo, sizeof(foo), "%lu", (long unsigned)THIS->windata.window);
     my_putenv(buffer, &offset, "window", foo);

     snprintf(foo, sizeof(foo), "0x%lx", (long unsigned)THIS->windata.window);
     my_putenv(buffer, &offset, "hexwindow", foo);

     snprintf(foo, sizeof(foo), "%ld", (long)THIS->repeats);
     my_putenv(buffer, &offset, "repeats", foo);

     snprintf(foo, sizeof(foo), "%ld", (long)THIS->windata.width);
     my_putenv(buffer, &offset, "width", foo);

     snprintf(foo, sizeof(foo), "%ld", (long)THIS->windata.height);
     my_putenv(buffer, &offset, "height", foo);

     my_putenv(buffer, &offset, "mimetype", THIS->mimetype);
     
     my_putenv(buffer, &offset, "file", file);

     my_putenv(buffer, &offset, "autostart", autostart ? "1" : "0");
      
     if (THIS->winname)
	  my_putenv(buffer, &offset, "winname", THIS->winname);
     
     if (THIS->displayname)
	  my_putenv(buffer, &offset, "DISPLAY", THIS->displayname);

     for (i = 0; i < THIS->num_arguments; i++)
	  if (THIS->args[i].value)
	       my_putenv(buffer, &offset, THIS->args[i].name, THIS->args[i].value);

     if(flags & H_CONTROLS)
     {
          execlp(controller_fname, helper_fname, buffer, THIS->command, NULL);
     }
     else
     {
          execlp(helper_fname, helper_fname, buffer, THIS->command, NULL);
     }
     D("EXECLP FAILED!\n");
     
     _exit(EX_UNAVAILABLE);
}

/*****************************************************************************
 * Check if 'file' is somewhere to be found in the user PATH.
 *****************************************************************************/
static int inpath(char *path, const char *file)
{
     static struct stat filestat;
     static char buf[1024];
     int i;
     int count = 1;

     for (i = strlen(path)-1; i > 0; i--)
     {
	  if (path[i] == ':')
	  {
	       path[i] = 0;
	       count++;
	  }
     }

     for (i = 0; i < count; i++)
     {
	  snprintf(buf, sizeof(buf), "%s/%s", path, file);
	  D("stat(%s)\n", buf);
	  if (!stat(buf, &filestat))
	  {
	       D("yes\n");
	       return 1;
	  }
	  D("no\n");
	  path += (strlen(path) + 1);
     }

     return 0;
}

typedef struct 
{
    char name[SMALL_BUFFER_SIZE];
    short exists;
} cacheEntry_t;

/*****************************************************************************
 * Check if 'file' exists. Uses a cache of previous finds to avoid performing
 * a stat call on the same file over and over again.
 *****************************************************************************/
static int find(const char *file)
{
     static cacheEntry_t cache[FIND_CACHE_SIZE];
     static int cacheSize = 0;
     static int cacheHead = 0;

     struct stat filestat;
     int i;
     int exists;

     D("find(%s)\n", file);

     for(i = 0; i < cacheSize; i++)
     {
         if( strcmp(cache[i].name, file) == 0)
         {
             const int exists = cache[i].exists;
             D("find cache hit exists = %s\n", exists ? "yes" : "no");
             return exists;
         }
     }

     if (file[0] == '/')
     {
	  exists = !stat(file, &filestat);
     }
     else
     {
          char *path;
          char *env_path;

          /* Get the environment variable PATH */
          if (!(env_path = getenv("PATH")))
          {
	       D("No PATH !\n");
	       exists = 0;
          }
          else
          {
               /* Duplicate to avoid corrupting Mozilla's PATH */
               path = strdup(env_path);
               exists = inpath(path, file);
               free(path);
          }
     }

     strncpy(cache[cacheHead].name, file, SMALL_BUFFER_SIZE);
     cache[cacheHead].name[SMALL_BUFFER_SIZE-1] = '\0';
     cache[cacheHead].exists = exists;

     cacheHead++;
     if(cacheHead > cacheSize)
     {
          cacheSize = cacheHead;
     }
     if(cacheHead >= FIND_CACHE_SIZE)      /* Wrap around the head */
     {
          cacheHead = 0;
     }
     return exists;
}

/*****************************************************************************
 * Delete a mime handler if no command has been found for it.
 *****************************************************************************/
static void filter_previous_handler(void)
{
     handler_t *h;
     if (num_handlers >= 1)
     {
	  h = &handlers[num_handlers-1];
	  if (h->num_cmds == 0)
	  {
	       D("Empty handler: '%s'.\n", h->types[0].type);
	       h->num_types = 0;
	       num_handlers--;
	  }
     }
}

static char staticPool[MAX_STATIC_MEMORY_POOL];
static int  staticPoolIdx = 0;

/*****************************************************************************
 * Allocate some memory from the static pool. We use a static pool for
 * the database because Mozilla can unload the plugin after use and this
 * would lead to memory leaks if we use the heap.
 *****************************************************************************/
static void * allocStaticMem(int size)
{
     void * retVal; 
     const int newIdx = staticPoolIdx + size;

     if(newIdx > MAX_STATIC_MEMORY_POOL)
     {
	  fprintf(stderr, "MozPlugger: Too many entries in mozpluggerc\n");
	  exit(1);
     }

     retVal = &staticPool[staticPoolIdx];
     staticPoolIdx = newIdx;
     return retVal;
}

/*****************************************************************************
 * Given a pointer to a string in temporary memory, return the same string
 * but this time stored in permanent (i.e. static) memory. Will only be deleted
 * when the plugin is unloaded by Mozilla.
 *****************************************************************************/
static const char * makeStrStatic(const char * str, int len)
{
    /* plus one for string terminator */
    char * const buf = allocStaticMem(len + 1);

    strncpy(buf, str, len);
    buf[len] ='\0';
    return (const char *) buf;
}


/*****************************************************************************
 * Match a word to a flag that takes a parameter e.g. swallow(name).
 *****************************************************************************/
static char *get_parameter(char *x, const char *flag, const char **c)
{
     char *end;
     int len;
 
     /* skip spaces or tabs between flag name and parameter */
     while ((*x == ' ' || *x == '\t')) 
         x++;

     if (*x != '(')
     {
	  fprintf(stderr, 
                  "MozPlugger: Warning: Expected '(' after '%s'\n", flag);
	  return NULL;
     }
     x++;
     end = strchr(x,')');
     if (end)
     {
	  len = end - x;
	  *c = makeStrStatic(x, len);
	  x = end+1;
     }
     return x;
}

/*****************************************************************************
 * Match two words.
 *****************************************************************************/
__inline
static int match_word(const char *line, const char *kw)
{
     return !strncasecmp(line, kw, strlen(kw)) && !isalnum(line[strlen(kw)]);
}

/*****************************************************************************
 * Scan a line for all the possible flags.
 *****************************************************************************/
static int parse_flags(char **x, command_t *commandp)
{
     static flag_t flags[] = {
	  { "repeat", 		H_REPEATCOUNT 	},
	  { "loop", 		H_LOOP 		},
	  { "stream", 		H_STREAM 	},
	  { "ignore_errors",	H_IGNORE_ERRORS },
	  { "exits", 		H_DAEMON 	},
	  { "nokill", 		H_DAEMON 	},
	  { "maxaspect",	H_MAXASPECT 	},
	  { "fill",		H_FILL 		},
	  { "noisy",		H_NOISY 	},
	  { "embed",            H_EMBED         },
	  { "noembed",          H_NOEMBED       },
          { "links",            H_LINKS         },
	  { "hidden",           H_HIDDEN        },
          { "controls",         H_CONTROLS      },
          { "swallow",          H_SWALLOW       },
          { "fmatch",           H_FMATCH        },
	  { NULL, 		0 		}
     };
     flag_t *f;

     for (f = flags; f->name; f++)
     {
	  if (match_word(*x, f->name))
	  {
	       *x += strlen(f->name);
	       commandp->flags |= f->value;

               /* Get parameter for thoses commands with parameters */
               if(f->value & H_SWALLOW)
               {
	            if ((*x = get_parameter(*x, f->name, &commandp->winname)))
                        return 1;
               }
               else if(f->value & H_FMATCH)
               {
	            if ((*x = get_parameter(*x, f->name, &commandp->fmatchStr)))
                        return 1;
               }
               else
	            return 1;
	  }
     }
     return 0;
}

/*****************************************************************************
 * Read the configuration file into memory.
 *****************************************************************************/
static void read_config(FILE *f)
{
     handler_t *handler = NULL;
     command_t *cmd = NULL;
     mimetype_t *type = NULL;
     char buffer[LARGE_BUFFER_SIZE];
     char file[SMALL_BUFFER_SIZE];
     int have_commands = 1;

     D("read_config\n");

     while (fgets(buffer, sizeof(buffer), f))
     {
	  D("::: %s", buffer);

	  if (buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\0')
	       continue;

	  if (buffer[strlen(buffer)-1] == '\n')
	       buffer[strlen(buffer)-1] = 0;

	  if (!isspace(buffer[0]))
	  {
	       /* Mime type */

	       if (have_commands)
	       {
		    D("-------------------------------------------\n");
		    D("New handler\n");
		    D("-------------------------------------------\n");

		    filter_previous_handler();

		    handler = &handlers[num_handlers++];

		    if (!(num_handlers < MAX_NUM_HANDLERS))
		    {
			 fprintf(stderr, "MozPlugger: Error: Too many handlers (%d)\n", num_handlers);
			 exit(1);
		    }

		    have_commands = 0;
	       }

	       D("New mime type\n");

	       type = &handler->types[handler->num_types++];

	       if (!(handler->num_types < MAX_NUM_TYPES_PER_HANDLER))
	       {
		    fprintf(stderr, "MozPlugger: Error: Too many types (%d) for handler %d (%s)\n",
			    handler->num_types, num_handlers, handler->types[0].type);
		    exit(1);
	       }
	       type->type = makeStrStatic(buffer, strlen(buffer));
	  }
	  else
	  {
	       /* Command */

	       char *x = buffer + 1;
	       while (isspace(*x)) x++;
	       if (!*x)
	       {
		    D("Empty command.\n");
		    have_commands++;
		    continue;
	       }

	       D("New command\n");

	       have_commands++;

	       cmd = &handler->cmds[handler->num_cmds];
	       memset(cmd, 0, sizeof(command_t));

	       D("Parsing %s\n", x);

	       while (*x != ':' && *x)
	       {
		    if (*x == ',' || *x == ' ' || *x == '\t')
		    {
			 x++;
		    }
		    else if (!parse_flags(&x, cmd))
		    {
			 fprintf(stderr, "MozPlugger: Warning: Unknown directive: %s\n", x);
			 x += strlen(x);
		    }
	       }
	       if (*x == ':')
	       {
		    x++;
		    while (isspace(*x)) x++;

		    if (sscanf(x, "if %"SMALL_BUFFER_SIZE_STR"s", file) != 1
			&& sscanf(x, "%"SMALL_BUFFER_SIZE_STR"s", file) != 1)
			 continue;

		    if (!find(file))
			 continue;

		    cmd->cmd = makeStrStatic(x, strlen(x));
	       }
	       else
	       {
		    D("No column? (%s)\n", x);
	       }

	       handler->num_cmds++;

	       if (!(handler->num_cmds < MAX_NUM_CMDS_PER_HANDLER))
	       {
		    fprintf(stderr, "MozPlugger: Error: Too many commands (%d) for handler %d (%s)\n",
			    handler->num_cmds, num_handlers, handler->types[0].type);
		    exit(1);
	       }
	  }
     }

     filter_previous_handler();

     D("Num handlers: %d\n", num_handlers);
}

/*****************************************************************************
 * Find the config file or the helper file in function of the function cb.
 *****************************************************************************/
static int find_helper_file(const char *basename, 
                            int (*cb)(const char *,void *data), 
                            void *data)
{
     static char fname[LARGE_BUFFER_SIZE];
     char *tmp;

     D("find_helper_file '%s'\n", basename);

     if ((tmp = getenv("HOME")))
     {
	  snprintf(fname, sizeof(fname), "%s/.netscape/%s", tmp, basename);
	  if (cb(fname,data)) return 1;

	  snprintf(fname, sizeof(fname), "%s/.mozilla/%s", tmp, basename);
	  if (cb(fname,data)) return 1;

	  snprintf(fname, sizeof(fname), "%s/.opera/%s", tmp, basename);
	  if (cb(fname,data)) return 1;
     }

     if ((tmp = getenv("MOZILLA_HOME")))
     {
	  snprintf(fname, sizeof(fname), "%s/%s", tmp, basename);
	  if (cb(fname, data)) return 1;
     }

     if ((tmp = getenv("OPERA_DIR")))
     {
	  snprintf(fname, sizeof(fname), "%s/%s", tmp, basename);
	  if (cb(fname, data)) return 1;
     }

     snprintf(fname, sizeof(fname), "/etc/%s", basename);
     if (cb(fname, data)) return 1;

     snprintf(fname, sizeof(fname), "/usr/etc/%s", basename);
     if (cb(fname, data)) return 1;

     snprintf(fname, sizeof(fname), "/usr/local/mozilla/%s", basename);
     if (cb(fname, data)) return 1;

     snprintf(fname, sizeof(fname), "/usr/local/netscape/%s", basename);
     if (cb(fname, data)) return 1;

     if (cb(basename, data)) return 1;
  
     return 0;
}

static int read_config_cb(const char *fname, void *data)
{
     int m4out[2];
     pid_t pid;
     int fd;
     FILE *fp;

     D("READ_CONFIG(%s)\n", fname);

     fd = open(fname, O_RDONLY);
     if (fd < 0) return 0;

     if (pipe(m4out) < 0)
     {
	  perror("pipe");
	  return 0;
     }

     if ((pid = fork()) == -1)
	  return 0;

     if (!pid)
     {
	  close(m4out[0]);

	  dup2(m4out[1], 1);
	  close(m4out[1]);

	  dup2(fd, 0);
	  close(fd);

	  execlp("m4", "m4", NULL);
	  fprintf(stderr, "MozPlugger: Error: Failed to execute m4.\n");
	  exit(1);
     }
     else
     {
	  close(m4out[1]);
	  close(fd);
	  
	  fp = fdopen(m4out[0], "r");
	  if (!fp) return 0;
	  read_config(fp);
	  fclose(fp);
	  
	  waitpid(pid, 0, 0);
	  config_fname = strdup(fname);
     }
     return 1;
}

static int find_plugger_helper_cb(const char *fname, void *data)
{
     struct stat buf;
     if (stat(fname, &buf)) return 0;
     helper_fname = strdup(fname);
     return 1;
}

static int find_plugger_controller_cb(const char *fname, void *data)
{
     struct stat buf;
     if (stat(fname, &buf)) return 0;
     controller_fname = strdup(fname);
     return 1;
}

/*****************************************************************************
 * Find configuration file and read it into memory.
 *****************************************************************************/
static void do_read_config(void)
{
     if (num_handlers > 0) return;

     D("do_read_config\n");
  
     if (!find_helper_file("mid-mozpluggerrc", read_config_cb, 0))
     {
	  fprintf(stderr, "MozPlugger: Warning: Unable to find the mid-mozpluggerrc file.\n");
	  return;
     }

     if (!find_helper_file("mozplugger-helper", find_plugger_helper_cb, 0))
     {
	  if (find("mozplugger-helper")) {
	       helper_fname = "mozplugger-helper";
	  } else { 
	       fprintf(stderr, "MozPlugger: Warning: Unable to find mozplugger-helper.\n");
	       return;
	  }
     }

     if (!find_helper_file("mozplugger-controller", find_plugger_controller_cb, 0))
     {
	  if (find("mozplugger-controller")) {
	       controller_fname = "mozplugger-controller";
	  } else {
	       fprintf(stderr, "MozPlugger: Warning: Unable to find mozplugger-controller.\n");
	       return;
	  }
     }

     D("do_read_config done\n");
}

/*****************************************************************************
 * Since href's are passed to an app as an argument, just check for ways that 
 * a shell can be tricked into executing a command.
 *****************************************************************************/
static int safeURL(const char* url)
{
     int  i = 0; 
     int  len = strlen(url);
    
     if (url[0] == '/')
	  return 0;

     for (i = 0; i < len; i++)
     {
	  if (url[i] == '`' || url[i] == ';')
	  {
	       /* Somebody's trying to do something naughty. */
	       return 0;
	  }
     }
     return 1;
}

/*****************************************************************************
 * See if the URL matches out match criteria.
 *****************************************************************************/
__inline
static int match_url(const char * matchStr, const char * url)
{
     int matchStrLen;
     const char * end;

     switch (matchStr[0])
     {
     case '*':
          /* Does the URL start with the match String */
          matchStr++;     /* Step over the asterisk */
	  return (strncasecmp(matchStr, url, strlen(matchStr)) == 0);

     case '%':
          /* Does the URL end with the match String */
          matchStr++;     /* Step over the percent sign */

          /* Need to find the end of the url, before any 
           * extra params i.e'?=xxx' or '#yyy' */
          end = strchr(url, '?');
          if(end == NULL)
          {
               end = strchr(url, '#');
               if(end == NULL)
               {
                    end = &url[strlen(url)];
               }
          }
          matchStrLen = strlen(matchStr);
          if(end - matchStrLen < url)
          {
               return 0;
          }
	  return (strncasecmp(matchStr, end-matchStrLen, matchStrLen) == 0);
 
     default:
          /* Is the match string anywhere in the URL */
	  return (strstr(url, matchStr) != NULL);
     }
}

/*****************************************************************************
 * Go through the commands in the config file and find one that fits our needs.
 *****************************************************************************/
__inline
static int match_command(NPP instance, int streaming, 
                         command_t *c, const char * url)
{
#define MODE_MASK (H_NOEMBED | H_EMBED | H_LINKS)

     D("Checking command: %s\n", c->cmd);

     /* If command is specific to a particular mode... */
     if (c->flags & MODE_MASK)
     {
          /* Check it matches the current mode... */
          if ( (THIS->mode_flags & MODE_MASK) != (c->flags & MODE_MASK) )
          {
	       D("Flag mismatch: mode different %x != %x\n",
                     THIS->mode_flags & MODE_MASK,  c->flags & MODE_MASK);
	       return 0;
          }
     }

     if ((c->flags & H_LOOP) && (THIS->repeats != MAXINT))
     {
	  D("Flag mismatch: loop\n");
	  return 0;
     }
     if ((!!streaming) != (!!(c->flags & H_STREAM)))
     {
	  D("Flag mismatch: stream\n");
	  return 0;
     }

     if(c->fmatchStr)
     {
          if(!match_url(c->fmatchStr, url))
          {
               D("fmatch mismatch: url '%s' doesnt have '%s'\n", 
                                              url, c->fmatchStr);
               return 0;
          }
     }
     D("Match found!\n");
     return 1;
}

/*****************************************************************************
 * See if mimetype matches.
 *****************************************************************************/
__inline
static int match_mime_type(NPP instance, mimetype_t *m)
{
     char mimetype[SMALL_BUFFER_SIZE];
     sscanf(m->type, "%"SMALL_BUFFER_SIZE_STR"[^:]", mimetype);
     sscanf(mimetype, "%s", mimetype);

     D("Checking '%s' ?= '%s'\n", mimetype, THIS->mimetype);
     if ((strcasecmp(mimetype, THIS->mimetype) != 0) 
             && (strcmp(mimetype,"*") != 0))
     {
	  D("Not same.\n");
	  return 0;
     }

     D("Same.\n");
     return 1;
}

/*****************************************************************************
 * See if handler matches.
 *****************************************************************************/
__inline
static int match_handler(handler_t *h, NPP instance, 
                         int streaming, const char *url)
{
     mimetype_t *m;
     command_t *c;
     int mid, cid;

     D("-------------------------------------------\n");
     D("Commands for this handle at (%p):\n", h->cmds);

     for (mid = 0; mid < h->num_types; mid++)
     {
	  m = &h->types[mid];
	  if (match_mime_type(instance, m))
	  {
	       for (cid = 0; cid < h->num_cmds; cid++)
	       {
		    c = &h->cmds[cid];
		    if (match_command(instance, streaming, c, url))
		    {
			 THIS->cmd_flags = c->flags;
			 THIS->command = c->cmd;
			 THIS->winname = c->winname;
			 return 1;
		    }
	       }
	  }
     }
     return 0;
}

/*****************************************************************************
 * Find the appropriate command
 *****************************************************************************/
static int find_command(NPP instance, int streaming, const char * url)
{
     int hid;

     D("find_command...\n");

     do_read_config();

     for (hid = 0; hid < num_handlers; hid++)
     {
	  if (match_handler(&handlers[hid], instance, streaming, url))
	  {
	       D("Command found.\n");
	       return 1;
	  }
     }

     D("No command found.\n");
     return 0;
}

/*****************************************************************************
 * Construct a MIME Description string for netscape so that mozilla shall know
 * when to call us back.
 *****************************************************************************/
char *NPP_GetMIMEDescription(void)
{
     char *x,*y;
     handler_t *h;
     mimetype_t *m;
     int size_required;
     int hid, mid;

     D("GetMIMEDescription\n");

     do_read_config();

     size_required = 0;

     for (hid = 0; hid < num_handlers; hid++)
     {
	  h = &handlers[hid];
	  for (mid = 0; mid < h->num_types; mid++)
	  {
	       m = &h->types[mid];
	       size_required += strlen(m->type)+1;
	  }
     }

     D("Size required=%d\n", size_required);

     if (!(x = (char *)malloc(size_required+1)))
	  return 0;

     D("Malloc did not fail\n");

     y = x;

     for (hid = 0; hid < num_handlers; hid++)
     {
	  h = &handlers[hid];
	  for (mid = 0; mid < h->num_types; mid++)
	  {
	       m = &h->types[mid];
	       memcpy(y,m->type,strlen(m->type));
	       y+=strlen(m->type);
	       *(y++)=';';
	  }
     }
     if (x != y) y--;
     *(y++) = 0;

     D("Getmimedescription done: %s\n", x);

     return x;
}

/*****************************************************************************
 * Let Mozilla know things about mozplugger.
 *****************************************************************************/
NPError NPP_GetValue(void *instance, NPPVariable variable, void *value)
{
     static char desc_buffer[8192];
     NPError err = NPERR_NO_ERROR;

     D("Getvalue %d\n", variable);
  
     switch (variable)
     {
     case NPPVpluginNameString:
	  D("GET Plugin name\n");
	  *((char **)value) = "MozPlugger "
	       VERSION
	       " handles QuickTime and Windows Media Player Plugin"
	       ;
	  break;

     case NPPVpluginDescriptionString:
	  D("GET Plugin description\n");
	  snprintf(desc_buffer, sizeof(desc_buffer),
		   "MozPlugger version "
		   VERSION
		   ", written by Fredrik H&uuml;binette, Louis Bavoil and Peter Leese.<br>"
		   "For documentation on how to configure mozplugger, "
		   "check the man page. (type <tt>man&nbsp;mozplugger</tt>)"
		   " <table> "
		   " <tr><td>Configuration file:</td><td>%s</td></tr> "
		   " <tr><td>Helper binary:</td><td>%s</td></tr> "
		   " <tr><td>Controller binary:</td><td>%s</td></tr> "
#ifdef DEBUG
		   " <tr><td>Debug file:</td><td><a href=\"file:///home/user/tmp/mozdebug\">$HOME/tmp/mozdebug</a></td></tr> "
#endif
		   " </table> "
		   "<br clear=all>",
		   config_fname ? config_fname : "Not found!",
		   helper_fname ? helper_fname : "Not found!",
		   controller_fname ? controller_fname : "Not found!");
	  *((char **)value) = desc_buffer;
	  break;

     default:
	  err = NPERR_GENERIC_ERROR;
     }
     return err;
}

/*****************************************************************************
 * Convert a string to an integer.
 * The string can be true, false, yes or no.
 *****************************************************************************/
static int my_atoi(const char *s, int my_true, int my_false)
{
     switch (s[0])
     {
     case 't': case 'T': case 'y': case 'Y':
	  return my_true;
     case 'f': case 'F': case 'n': case 'N':
	  return my_false;
     case '0': case '1': case '2': case '3': case '4':
     case '5': case '6': case '7': case '8': case '9':
	  return atoi(s);
     }
     return -1;
}

/*****************************************************************************
 * Initialize another instance of mozplugger. It is important to know
 * that there might be several instances going at one time.
 *****************************************************************************/
NPError NPP_New(NPMIMEType pluginType,
		NPP instance,
		uint16 mode,
		int16 argc,
		char* argn[],
		char* argv[],
		NPSavedData* saved)
{
     int e;

     int src_idx = -1;
     int href_idx = -1;
     int data_idx = -1;
     int alt_idx = -1;
     int autostart_idx = -1;
     int autohref_idx = -1;
     int target_idx = -1;

     char *url = NULL;

     D("NEW (%s) - instance=%p\n", pluginType, instance);

     if (!instance)
     {
	  return NPERR_INVALID_INSTANCE_ERROR;
     }

     if (!pluginType)
     {
	  return NPERR_INVALID_INSTANCE_ERROR;
     }

     instance->pdata = NPN_MemAlloc(sizeof(data_t));
     if (instance->pdata == NULL) 
          return NPERR_OUT_OF_MEMORY_ERROR;


     memset((void *)THIS, 0, sizeof(data_t));

     THIS->windata.window = 0;
     THIS->display = NULL;
     THIS->pid = -1;
     THIS->fd = -1;
     THIS->repeats = 1;
     THIS->autostart = 1;
     THIS->autostartNotSeen = 1;
     if(mode == NP_EMBED)
     {
        THIS->mode_flags = H_EMBED;
     }
     else
     {
        THIS->mode_flags = H_NOEMBED;
     }

     if (!(THIS->mimetype = strdup(pluginType)))
	  return NPERR_OUT_OF_MEMORY_ERROR;

     THIS->num_arguments = argc;
     if (!(THIS->args = (argument_t *)NPN_MemAlloc(
                                          (uint32)(sizeof(argument_t) * argc))))
	  return NPERR_OUT_OF_MEMORY_ERROR;
     
     for (e = 0; e < argc; e++)
     {
	  if (strcasecmp("loop", argn[e]) == 0)
	  {
	       THIS->repeats = my_atoi(argv[e], MAXINT, 1);
	  }
          /* realplayer also uses numloop tag */
          /* windows media player uses playcount */
          else if((strcasecmp("numloop", argn[e]) == 0) ||
                  (strcasecmp("playcount", argn[e]) == 0))
          {
	       THIS->repeats = atoi(argv[e]);
          }
	  else if((strcasecmp("autostart", argn[e]) == 0) ||
	          (strcasecmp("autoplay", argn[e]) == 0))
	  {
               autostart_idx = e;
	  }
	  /* get the index of the src attribute if this is a 'embed' tag */
	  else if (strcasecmp("src", argn[e]) == 0)
	  {
	       src_idx = e;
	  }
	  /* get the index of the data attribute if this is a 'object' tag */
          else if (strcasecmp("data", argn[e]) == 0)
          {
               data_idx = e;
          }
          /* Special case for quicktime. If there's an href or qtsrc attribute,
           * remember it for now */
          else if (((strcasecmp("href", argn[e]) == 0) ||
	            (strcasecmp("qtsrc", argn[e]) == 0)) &&
	            href_idx == -1)
          {
               href_idx = e;
          }
          else if (((strcasecmp("filename", argn[e]) == 0) ||
	            (strcasecmp("url", argn[e]) == 0) ||
	            (strcasecmp("location", argn[e]) == 0)) &&
                    alt_idx == -1)
          {
               alt_idx = e;
          }
          /* Special case for quicktime. If there's an autohref or target
           * attributes remember them for now */
          else if (strcasecmp("target", argn[e]) == 0)
          {
               target_idx = e;
          }
	  else if(strcasecmp("autohref", argn[e]) == 0)
	  {
               autohref_idx = e;
	  }

	  /* copy the flag to put it into the environment later */
	  D("VAR_%s=%s\n", argn[e], argv[e]);
          {
               const int len = strlen(argn[e]) + 5;

    	       if (!(THIS->args[e].name = (char *)malloc(len)))
	             return NPERR_OUT_OF_MEMORY_ERROR;
	       snprintf(THIS->args[e].name, len, "VAR_%s", argn[e]);
 	       THIS->args[e].value = argv[e] ? strdup(argv[e]) : NULL;
          }
     }

     if (src_idx >= 0)
     {
          url = THIS->args[src_idx].value;
          /* Special case for quicktime. If there's an href or qtsrc 
           * attribute, we want that instead of src but we HAVE to
           * have a src first. */
          if (href_idx >= 0)
          {
	       D("Special case QT detected\n");
	       THIS->href = THIS->args[href_idx].value;

               autostart_idx = autohref_idx;

               if(target_idx >= 0)
               {
                   /* One of those clickable Quicktime linking objects! */
                   THIS->mode_flags &= ~(H_EMBED | H_NOEMBED);
                   THIS->mode_flags |= H_LINKS;
               }

          }
     }
     else if (data_idx >= 0)
     {
          D("Looks like an object tag with data attribute\n");
          url = THIS->args[data_idx].value;
     }
     else if (alt_idx >= 0)
     {
          D("Fall-back use alternative tags\n");
          url = THIS->args[alt_idx].value;
     }

     /* Do the autostart check here, AFTER we have processed the QT special
      * case which can change the autostart attribute */
     if(autostart_idx > 0)
     {
	  THIS->autostart = !!my_atoi(argv[autostart_idx], 1, 0);
	  THIS->autostartNotSeen = 0;
     }

     if (url)
     {
          /* Mozilla does not support the following protocols directly and
           * so it never calls NPP_NewStream for these protocols */
	  if(   (strncmp(url, "mms://", 6) == 0)        
             || (strncmp(url, "mmsu://", 7) == 0)    /* MMS over UDP */
             || (strncmp(url, "mmst://", 7) == 0)    /* MMS over TCP */
             || (strncmp(url, "rtsp://", 7) == 0)
             || (strncmp(url, "rtspu://", 8) == 0)   /* RTSP over UDP */
             || (strncmp(url, "rtspt://", 8) == 0))  /* RTSP over TCP */
	  {
	       D("Detected MMS -> url=%s\n", url);
	       THIS->mms = url;
	  }
          else 
          {
               /* For protocols that Mozilla does support, sometimes
                * the browser will call NPP_NewStream straight away, some
                * times it wont (depends on the nature of the tag). So that
                * it works in all cases call NPP_GetURL, this may result
                * in NPP_NewStream() being called twice (i.e. if this is an
                * embed tag with src attribute or object tag with data
                * attribute) */
               if (mode == NP_EMBED)
               {
                    const NPError retVal = NPN_GetURL(instance, url, 0);
                    if(retVal != NPERR_NO_ERROR)
                    {
                         D("NPN_GetURL(%s) failed with %i\n", url, retVal);

	                 fprintf(stderr, "MozPlugger: Warning: Couldn't get"
                                 "%s\n", url);
                    }
               }
          }
     }

     D("New finished\n");

     return NPERR_NO_ERROR;
}

/*****************************************************************************
 * Free data, kill processes, it is time for this instance to die.
 *****************************************************************************/
NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
     int e;

     D("Destroy() - instance=%p\n", instance);

     if (!instance)
	  return NPERR_INVALID_INSTANCE_ERROR;

     if (THIS)
     {
	  /* Kill the mozplugger-helper process and his sons */
 	  if (THIS->pid > 0)
	       my_kill(-THIS->pid);

	  if (THIS->fd > 0)
	       close(THIS->fd);

	  for (e = 0; e < THIS->num_arguments; e++)
	  {
	       free((char *)THIS->args[e].name);
	       free((char *)THIS->args[e].value);
	  }

	  NPN_MemFree((char *)THIS->args);

	  free(THIS->mimetype);
	  THIS->href = NULL;
	  THIS->mms = NULL;

	  NPN_MemFree(instance->pdata);
	  instance->pdata = NULL;
     }

     D("Destroy finished\n");
  
     return NPERR_NO_ERROR;
}

/*****************************************************************************
 * Check that no child is already running before forking one.
 *****************************************************************************/
static void new_child(NPP instance, const char* fname)
{
     int pipe[2];

     D("NEW_CHILD(%s)\n", fname);

     if (!instance || !fname)
	  return;

     if (THIS->pid != -1)
	  return;

     /* Guard against spawning helper if no command! */
     if(THIS->command == 0)
         return;

     if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipe) < 0)
     {
	  NPN_Status(instance, "MozPlugger: Failed to create a pipe!");
	  return;
     }

     if ((THIS->pid = my_fork(instance, pipe[1])) == -1)
     {
	  NPN_Status(instance, "MozPlugger: Failed to fork helper!");
	  return;
     }
     
     if (!THIS->pid)
     {
	  close(pipe[0]);

	  D("CHILD RUNNING run() [2]\n");
	  run(instance, fname, pipe[1]);

	  _exit(EX_UNAVAILABLE);
     }
     else
     {
	  THIS->fd = pipe[0];
	  close(pipe[1]);
     }
}

/*****************************************************************************
 * Open a new stream.
 * Each instance can only handle one stream at a time.
 *****************************************************************************/
NPError NPP_NewStream(NPP instance,
		      NPMIMEType type,
		      NPStream *stream, 
		      NPBool seekable,
		      uint16 *stype)
{
     char * savedMimetype = NULL;

     D("NewStream() - instance=%p\n", instance);

     if (instance == NULL)
	  return NPERR_INVALID_INSTANCE_ERROR;

     if (THIS->pid != -1)
     {
          D("NewStream() exiting process already running\n");
	  return NPERR_INVALID_INSTANCE_ERROR;
     }

     /* This is a stupid special case and should be coded into
      * mozpluggerc instead... */
     if (!strncasecmp("image/", type, 6) ||
	 !strncasecmp("x-image/", type, 6))
	  THIS->repeats = 1;

     /*  Replace the stream's URL with the URL in THIS->href if it
      *  exists.  Since we're going to be replacing <embed>'s src with
      *  href, we want to make sure that the URL is not going to try
      *  anything dirty. */
     if (THIS->href != NULL && safeURL(THIS->href))
     {
	  D("Replacing SRC with HREF... \n");
	  stream->url = THIS->href;
     }

     D("Mime type %s\n", type);
     D("Url is %s (seekable=%d)\n", stream->url, seekable);

     /* Ocassionally the MIME type here is different to that passed to the
      * NEW function - this is because of either badly configure web server
      * who's HTTP response content-type does not match the mimetype in the
      * preceding embebbed, object or link tag. Or badly constructed embedded
      * tag or ambiguity in the file extension to mime type mapping. Lets
      * first assume the HTTP response was correct and if not fall back to
      * the original tag in the mime type. */
     if(strcmp(type, THIS->mimetype) != 0)
     {
         D("Mismatching mimetype reported, originally was \'%s\' now '\%s' "
           "for url %s\n", THIS->mimetype, type, stream->url);
         savedMimetype = THIS->mimetype;
         THIS->mimetype = strdup(type);
     }

     /* Find a suitable command, fall back to savedMimeType if defined */
     while(!find_command(instance, 1, stream->url) 
                                 && !find_command(instance, 0, stream->url))
     {
          if(savedMimetype)       /* Lets try the saved one */
          {
              free(THIS->mimetype);
              THIS->mimetype = savedMimetype;
              savedMimetype = NULL;
          }
          else 
          {
	      NPN_Status(instance, 
                         "MozPlugger: No appropriate application found.");
	      return NPERR_GENERIC_ERROR;
          }
     }
     free(savedMimetype);     /* discard the saved Mime type */


     if ((THIS->cmd_flags & H_STREAM)
	 && strncasecmp(stream->url, "file:", 5)
	 && strncasecmp(stream->url, "imap:", 5)
	 && strncasecmp(stream->url, "mailbox:", 8))
     {
	  *stype = NP_NORMAL;
	  new_child(instance, stream->url);
     }
     else
     {
	  *stype = NP_ASFILEONLY;
     }

     return NPERR_NO_ERROR;
}

/*****************************************************************************
 * Called after NPP_NewStream if *stype = NP_ASFILEONLY.
 *****************************************************************************/
void NPP_StreamAsFile(NPP instance,
		      NPStream *stream,
		      const char* fname)
{
     D("StreamAsFile() - instance=%p\n", instance);
     new_child(instance, fname);
}

/*****************************************************************************
 * The browser should have resized the window for us, but this function was
 * added because of a bug in Mozilla 1.7 (see Mozdev bug #7734) and
 * https://bugzilla.mozilla.org/show_bug.cgi?id=201158
 *
 * Bug was fixed in Mozilla CVS repositary in version 1.115 of
 * ns4xPluginInstance.cpp (13 Nov 2003), at the time version was 0.13.
 * version 0.14 happened on 14th July 2004
 ****************************************************************************/
void resize_window(NPP instance)
{
     XSetWindowAttributes attrib;

     /* Mozilla work around not require for versions > 0.13 */
     if((browserApiMajorVer > 0) || (browserApiMinorVer > 13))
         return;

     attrib.override_redirect = True;
     XChangeWindowAttributes(THIS->display, (Window)THIS->windata.window,
			     (unsigned long) CWOverrideRedirect, &attrib);
     
     D("Bug #7734 work around - resizing WIN 0x%x to %dx%d!?\n", 
            THIS->windata.window,
            THIS->windata.width, THIS->windata.height);

     XResizeWindow(THIS->display, (Window)THIS->windata.window,
		   (unsigned) THIS->windata.width, 
                   (unsigned) THIS->windata.height);
}

/*****************************************************************************
 * The browser calls NPP_SetWindow after creating the instance to allow drawing
 * to begin. Subsequent calls to NPP_SetWindow indicate changes in size or 
 * position. If the window handle is set to null, the window is destroyed. In
 * this case, the plug-in must not perform any additional graphics operations
 * on the window and should free any associated resources.
  ***************************************************************************/
NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
     D("SetWindow() - instance=%p\n", instance);

     if (!instance)
	  return NPERR_INVALID_INSTANCE_ERROR;

     if (!window)
	  return NPERR_NO_ERROR;
 
     /* TODO - Should really pass to helper / controller the fact that the
      * window has disappeared! - need to check the consequences first
      * before deleting these 2 lines */
     if (!window->window)
	  return NPERR_NO_ERROR;

     if (!window->ws_info)
	  return NPERR_NO_ERROR;
     
     THIS->display = ((NPSetWindowCallbackStruct *)window->ws_info)->display;
     THIS->displayname = XDisplayName(DisplayString(THIS->display));
     THIS->windata = *window;

     if (THIS->mms)
     {
          if(THIS->command == 0)
          {
               /* Can only use streaming commands, as Mozilla cannot handle
                * these types (mms) of urls */
               if (!find_command(instance, 1, THIS->mms))
               {
	           THIS->mms = NULL;
	           NPN_Status(instance, 
                          "MozPlugger: No appropriate application found.");
 	           return NPERR_GENERIC_ERROR;
               }
          }
	  new_child(instance, THIS->mms);
	  THIS->mms = NULL;
	  return NPERR_NO_ERROR;
     }

     if (THIS->fd != -1)
     {
	  D("Writing WIN 0x%x to fd %d\n", window->window, THIS->fd);
	  write(THIS->fd, (char *)window, sizeof(*window));
     }

     resize_window(instance);

     /* In case Mozilla would call NPP_SetWindow() in a loop. */
     usleep(4000);
     return NPERR_NO_ERROR;
}

/*****************************************************************************
 * Empty functions.
 *****************************************************************************/
NPError NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
     D("DestroyStream() - instance=%p\n", instance);
     return NPERR_NO_ERROR;
}

/*****************************************************************************
 * The browser calls this function only once; when the plug-in is loaded, 
 * before the first instance is created. NPP_Initialize tells the plug-in that
 * the browser has loaded it.
 *****************************************************************************/
NPError NPP_Initialize(void)
{
     int pluginApiMajorVer;
     int pluginApiMinorVer;

     NPN_Version(&pluginApiMajorVer,  &pluginApiMinorVer,
                 &browserApiMajorVer, &browserApiMinorVer);

     D("Initialize - API versions plugin=%d.%d Browser=%d.%d\n", 
             pluginApiMajorVer, pluginApiMinorVer,
             browserApiMajorVer, browserApiMinorVer);

     do_read_config();

#ifdef DEBUG
     {
         const int free = MAX_STATIC_MEMORY_POOL - staticPoolIdx;
         D("Static Pool used=%i, free=%i\n", staticPoolIdx, free);
     }
#endif

     return NPERR_NO_ERROR;
}

jref NPP_GetJavaClass()
{
     D("GetJavaClass\n");
     return NULL;
}

void NPP_Shutdown(void)
{
     D("Shutdown\n");
}

void NPP_Print(NPP instance, NPPrint* printInfo)
{
     D("Print() - instance=%p\n", instance);
}

int32 NPP_Write(NPP instance,
		NPStream *stream,
		int32 offset,
		int32 len,
		void *buf)
{
     D("Write(%d) - instance=%p\n", len, instance);
     return len;
}

int32 NPP_WriteReady(NPP instance, NPStream *stream)
{
    D("WriteReady() - instance=%p\n", instance);
    if (instance)
    {
	 /* Tell the browser that it can finish with the stream
	    (actually we just wanted the name of the stream!)
	    And not the stream data. */
        NPN_DestroyStream(instance, stream, NPRES_DONE);
    }
    return 0;
}

