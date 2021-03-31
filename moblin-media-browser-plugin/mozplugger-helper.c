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

#define _GNU_SOURCE /* for getsid() */
#include "mozplugger.h"

static Display * display = 0;
static int pipe_fd;
static int flags;
static int repeats;
static char *winname;
static char *command;
static char *file;

static XWindowAttributes wattr;

static struct
{
     int    mapped;
     int    reparented;
     Window window;
     int    borderWidth;
     int    x;
     int    y;
     int    width;
     int    height;
} victimDetails;

static struct 
{
    Window  window;
    int     width;
    int     height;
} parentDetails;

static Atom swallowMutex;
static int  swallowMutexTaken;
static Atom windowOwnerMark;

#define WINDOW (parentDetails.window)

int error_handler(Display *dpy, XErrorEvent *err)
{
#ifdef DEBUG
     char buffer[1024];
     XGetErrorText(dpy, err->error_code, buffer, sizeof(buffer));
     D("!!!ERROR_HANDLER!!!\n");
     D("Error: %s\n", buffer);
#endif
     return 0;
}

/*****************************************************************************
 * Swallow mutex semaphore protection
 *****************************************************************************/
static void initSwallowMutex()
{
    D("Initialising Swallow Mutex\n"); 
    if(!display) 
    {
	fprintf(stderr, "Display not set so cannot initialise semaphore!\n");
        return;
    }

    swallowMutex = XInternAtom(display, "MOZPLUGGER_SWALLOW_MUTEX", 0);
    swallowMutexTaken = 0;

    windowOwnerMark = XInternAtom(display, "MOZPLUGGER_OWNER", 0);
}

uint32 getHostId()
{
    char hostName[128];
    uint32 id;
    int i;
   
    memset(hostName, 0, sizeof(hostName));
    gethostname(hostName, sizeof(hostName)-1);

    D("Host Name = \"%s\"\n", hostName);

    /* OK, Create a 32 bit hash value from the host name....
       use this as a host ID, the possiblity of a collison of hash keys 
       effecting the swallow of victims is so infinitimisally small! */

    id = 0;
    for(i=0; i < (int)(sizeof(hostName)/sizeof(uint32)); i++)
    {
        id = ((id << 5) ^ (id >> 27)) ^ ((uint32 *)hostName)[i];
    }
    return id;
}

static int getSwallowMutexOwner(uint32 * hostId, uint32 * pid)
{   
    unsigned long nitems;
    unsigned long bytes;      
    int fmt;
    Atom type;
    unsigned char * property = NULL;
    int success = 0;

    /* Get hold of the Host & PID that current holds the semaphore for 
       this display! - watch out for the bizarre 64-bit platform behaviour
       where property returned is actually an array of 2 x  64bit ints with
       the top 32bits set to zero! */
    XGetWindowProperty(display, wattr.root, swallowMutex,
                       0, 2, 0, XA_INTEGER,
                       &type, &fmt, &nitems, &bytes,
                       &property);

    if(property)
    {
        /* Just check all is correct! */
	if((type != XA_INTEGER) || (fmt!=32) || (nitems!=2))
        {
	    fprintf(stderr, "XGetWindowProperty returned bad values " 
                            "%ld,%d,%lu,%lu\n", (long) type, fmt, nitems, 
                            bytes);
        }
	else
	{
            *hostId = (uint32)((unsigned long *)property)[0];
            *pid = (uint32)((unsigned long *)property)[1];
            success = 1;
        }
	XFree(property);
    }
    return success;
}

static void setSwallowMutexOwner(uint32 hostId, uint32 pid)
{
    unsigned long temp[2] = {hostId, pid};
    D("Setting swallow mutex owner, hostId = 0x%08X, pid=%d\n", hostId, pid); 

    XChangeProperty(display, wattr.root, swallowMutex,
                        XA_INTEGER, 32, PropModeAppend,
                        (unsigned char*) (&temp), 2);
}

static void takeSwallowMutex()
{
    int countDown;
    uint32 ourPid = (uint32)getpid();
    uint32 ourHostId = getHostId();
    uint32 otherPid;
    uint32 otherHostId;
    uint32 prevOtherPid = 0;
 
    if((display == 0) || (swallowMutex == 0))
	return;

    D("Attempting to take Swallow Mutex\n"); 
    /* Try up tp forty times ( 40 * 250ms = 10 seconds) to take 
       the semaphore... */

    countDown = 40;
    while(1)
    {

        /* While someone owns the semaphore */
        while(getSwallowMutexOwner(&otherHostId, &otherPid))
        {
            if( otherHostId == ourHostId)    
            {
                if(otherPid == ourPid)     
                {
 	            /* Great we have either successfully taken the semaphore
                       OR (unlikely) we previously had the semaphore! Exit
                       the function...*/
	            swallowMutexTaken = 1;
                    D("Taken Swallow Mutex\n"); 
		    return;
                }

     	    	D("Semaphore currently taken by pid=%ld\n", otherPid);
 		/* Check that the process that has the semaphore exists...
        	   Bit of a hack, I cant find a function to directly check
                   if process exists. */
            	if( (getsid(otherPid) < 0 ) && (errno == ESRCH))
	        {
	    	    D("Strange other Pid(%lu) cannot be found\n", otherPid);
                    XDeleteProperty(display, wattr.root, swallowMutex);
                    break;    /* OK force early exit of inner while loop */
                }
	    }

            /* Check if the owner of semaphore hasn't recently changed if it
               has restart timer */
            if(prevOtherPid != otherPid)
            {
   	        D("Looks like semaphore's owner has changed pid=%ld\n", 
                     otherPid);
	        countDown = 40;
                prevOtherPid = otherPid;
            }
            
            /* Do one step of the timer... */
	    countDown--;
            if(countDown <= 0)
            {
	    	D("Waited long enough for Pid(%lu)\n", otherPid);
                XDeleteProperty(display, wattr.root, swallowMutex);
		break;
            }
	
            usleep(250000);        /* 250ms */
 	}  
        /* else no one has semaphore, timeout, or owner is dead -
           Set us as the owner, but we need to check if we weren't 
           beaten to it so once more around the loop */
        setSwallowMutexOwner(ourHostId, ourPid);
    }
}


static void giveSwallowMutex()
{
    if((display == 0) || (swallowMutex == 0) || (swallowMutexTaken ==0))
	return;

    D("Giving Swallow Mutex\n"); 
    XDeleteProperty(display, wattr.root, swallowMutex);
    swallowMutexTaken = 0;
}

/*****************************************************************************
 * Mark the victim window with a property that indicates that this instance
 * of mozplugger-helper has made a claim to this victim. This is used to 
 * guard against two instances of mozplugger-helper claiming the same victim.
 *****************************************************************************/
static int chkAndMarkVictimWindow(Window w)
{
     unsigned long nitems;
     unsigned long bytes;      
     int fmt;
     Atom type;
     unsigned char * property = NULL;
     unsigned long temp[2];
     uint32 ourPid;
     uint32 ourHostId;
     int gotIt = 0;

     /* See if some other instance of Mozplugger has already 'marked' this
      * window */
     XGetWindowProperty(display, w, windowOwnerMark,
                        0, 2, 0, XA_INTEGER,
                        &type, &fmt, &nitems, &bytes,
                        &property);

     if(property)
     {
          /* Just check all is correct! */
	  if((type != XA_INTEGER) || (fmt!=32) || (nitems!=2))
          {
	       D("XGetWindowProperty(windowOwnerMark) returned bad values " 
                                   "%ld,%d,%lu,%lu\n", (long) type, fmt, nitems, 
                                   bytes);
          }
  	  XFree(property);
          return 0;          /* Looks like someone else has marked this window */
     }

     /* OK lets claim it for ourselves... */
     ourPid = (uint32)getpid();
     ourHostId = getHostId();

     temp[0] = ourHostId;
     temp[1] = ourPid;

     XChangeProperty(display, w, windowOwnerMark,
                     XA_INTEGER, 32, PropModeAppend,
                     (unsigned char*) (&temp), 2);

     /* See if we got it */
     XGetWindowProperty(display, w, windowOwnerMark,
                        0, 2, 0, XA_INTEGER,
                        &type, &fmt, &nitems, &bytes,
                        &property);

     if(property)
     {

          /* Just check all is correct! */
          if((type != XA_INTEGER) || (fmt!=32) || (nitems!=2))
          {
               fprintf(stderr, "XGetWindowProperty returned bad values " 
                                   "%ld,%d,%lu,%lu\n", (long) type, fmt, nitems, 
                                   bytes);
          }
          else if( ( ((unsigned long *)property)[1] == ourPid)
                  && ( ((unsigned long *)property)[0] == ourHostId) )
          {
               gotIt = 1;
          }
  	  XFree(property);
     }
     else         
     {
         D("Strange, couldn't set windowOwnerMark\n");
         gotIt = 1;
     }

     return gotIt;
}

/*****************************************************************************
 * Window resizing
 *****************************************************************************/

static int xaspect;
static int yaspect;

static int gcd(int a, int b)
{
     if (a < b) return gcd(b,a);
     if (b == 0) return a;
     return gcd(b, a % b);
}

static int set_aspect(int x, int y)
{
     int ox = xaspect;
     int oy = yaspect;
     int d = gcd(x, y);
     xaspect = x / d;
     yaspect = y / d;
     D("xaspect=%d yaspect=%d\n", xaspect, yaspect);
     return (ox != xaspect || oy != yaspect);
}

static void adjust_window_size(void)
{
     int x = 0;
     int y = 0;
     int w = parentDetails.width;
     int h = parentDetails.height;

     int tmpw, tmph;

     if (!victimDetails.window) return;
     
     if (!victimDetails.mapped || !victimDetails.reparented)
     {
          D("Victim not ready to be resized\n");
          return;
     }

     if (flags & H_FILL)
     {
	  D("Resizing window %x with FILL\n", victimDetails.window);
     }
     else if (flags & H_MAXASPECT)
     {
	  if (xaspect && yaspect)
	  {
	       D("Resizing window %x with MAXASPECT\n", victimDetails.window);
	       tmph = h / yaspect;
	       tmpw = w / xaspect;
	       if (tmpw < tmph) tmph = tmpw;
	       tmpw = tmph * xaspect;
	       tmph = tmph * yaspect;
	       
	       x = (w - tmpw) / 2;
	       y = (h - tmph) / 2;
	       
	       w = tmpw;
	       h = tmph;
	  }
	  else
	  {
	       D("Not resizing window\n");
	       return;
	  }
     }
  
     /* Compensate for the Victims border width (usually set to zero anyway) */
     w -= 2 * victimDetails.borderWidth;
     h -= 2 * victimDetails.borderWidth;

     D("New size: %dx%d+%d+%d\n", w, h, x, y);

     if((victimDetails.x == x) && (victimDetails.y == y)
        && (victimDetails.width == w) && (victimDetails.height == h))
     {
          XEvent event;
          D("No change in window size so sending ConfigureNotify instead\n");
          /* According to X11 ICCCM specification, a compliant  window 
           * manager, (or proxy in this case) should sent a  ConfigureNotify
           * event to the client even if no size change has occurred!
           * The code below is taken and adapted from the
           * TWM window manager and fixed movdev bug #18298. */
          event.type = ConfigureNotify;
          event.xconfigure.display = display;
          event.xconfigure.event = victimDetails.window;
          event.xconfigure.window = victimDetails.window;
          event.xconfigure.x = x;
          event.xconfigure.y = y;
          event.xconfigure.width = w;
          event.xconfigure.height = h;
          event.xconfigure.border_width = victimDetails.borderWidth;
          event.xconfigure.above = Above;
          event.xconfigure.override_redirect = False;

          XSendEvent(display, victimDetails.window, False, StructureNotifyMask,
                  &event);
     }
     else
     {
          XMoveResizeWindow(display, victimDetails.window, 
                                     x, y, (unsigned)w, (unsigned)h);
          victimDetails.x = x;
          victimDetails.y = y;
          victimDetails.width = w;
          victimDetails.height = h;
    }
}

/*****************************************************************************
 * Window reparenting
 *****************************************************************************/

static void change_leader()
{
     XWMHints *leader_change;
     if ((leader_change = XGetWMHints(display, victimDetails.window)))
     {
	  leader_change->flags = (leader_change->flags | WindowGroupHint);
	  leader_change->window_group = wattr.root;
	  XSetWMHints(display,victimDetails.window,leader_change);
	  XFree(leader_change);
     }
}

static int reparent_window(void)
{
     if (!victimDetails.window) return 0;
     if (!WINDOW) return 0;

     XSelectInput(display, victimDetails.window, StructureNotifyMask); 
     XSync(display, False);

     D("Changing leader of window %x\n", victimDetails.window);
     change_leader();
     
     D("Reparenting window %x into %x\n", victimDetails.window, WINDOW);
     XReparentWindow(display, victimDetails.window, WINDOW, 0, 0);

     D("reparent_window() done\n");

     return 1;
}

static int is_viewable(Display *display, Window w)
{
     XWindowAttributes a;

     if (!XGetWindowAttributes(display, w, &a))
     {
          D("Could not get Window Attributes\n");
	  return 0;
     }
     if (a.map_state != IsViewable)
     {
	  D("Invisible window\n");
	  return 0;
     }
     return 1;
}
/******************************************************************************
 * Traditionally strcmp returns -1, 0 and +1 depending if name comes before
 * or after windowname, this function also returns +1 if error
 *****************************************************************************/
static int my_strcmp(char *windowname, char *name)
{
     if (!name) return 1;
     if (!windowname) return 1;

     switch (name[0])
     {
     case '=':
	  return strcmp(name+1, windowname);
     case '~':
	  return strcasecmp(name+1, windowname);
     case '*':
	  return strncasecmp(name+1, windowname, strlen(name)-1);
     default:
          /* Return 0 for success so need to invert logic as strstr
             returns pointer to the match or NULL if no match */
	  return !strstr(windowname, name);
     }
}

static char check_window_name(Window w, char *name)
{
     char *windowname;
     XClassHint windowclass;
     char match;

     D("XFetchName\n");
     if (XFetchName(display, w, &windowname))
     {
	  D("Winrecur, checking window NAME %x (%s != %s)\n",
	    w, windowname, name);
	  match = (my_strcmp(windowname, name) == 0);
	  XFree(windowname);
	  if (match) {
	       return 1;
	  }
     }
     
     D("XGetClassHint\n");
     if (XGetClassHint(display, w, &windowclass))
     {
	  D("Winrecur, checking window CLASS %x (%s != %s)\n",
	    w, windowclass.res_name, name);
	  match = (my_strcmp(windowclass.res_name, name) == 0);
	  XFree(windowclass.res_class);
	  XFree(windowclass.res_name);
          return match;
     }
     return 0;
}

#define MAX_IGNORED 3000
static unsigned int ignore_window_count = 0;
static Window ignored[MAX_IGNORED];

/*****************************************************************************
 * init_winrecur
 *
 * This function should be called before forking the application. It runs 
 * through all windows on the screen to a depth of 'depth' and records their
 * identify in the ignored list. 
 *****************************************************************************/
static void init_winrecur(Window from, int depth, char * name)
{
     Window root, parent;
     Window *children;
     unsigned int e;
     unsigned int num_children;

     D("init_winrecur: storing family tree of window %x (depth=%d)\n", 
                                                          from, depth);
     
     if (check_window_name(from, name))
     {
          if (ignore_window_count < MAX_IGNORED) 
          {
               ignored[ignore_window_count++] = from;
          }
          else
          {
               D("Woo too many windows!\n");
          }
          return;
     }

     if (depth > 0)
     {
          if (!XQueryTree(display, from, &root, &parent, 
                                         &children, &num_children))
          {
	       D("init_winrecur: Querytree failed!!!\n");
	       return;
          }

	  D("init_winrecur: Num children = %d\n", num_children);
	  for (e = 0; e < num_children; e++)
	  {
	       init_winrecur(children[e], depth-1, name);
	  }
	  XFree(children);
     }
}

/*****************************************************************************
 * find_winrecur
 *
 * This function should be called after forking the application and tries to
 * see if this window (or its child) are the ones to swallow. 
 *****************************************************************************/
static Window find_winrecur(Window from, int depth, char *name)
{
     Window root, parent;
     Window *children;
     unsigned int e;
     unsigned int num_children;

     D("find_winrecur, checking window %x (depth=%d)\n",from,depth);
     
     for (e=0; e < ignore_window_count; e++) 
     {
	  if (ignored[e] == from)
          {
               /* This window is OLD, ignore it (and its children) */
	       return (Window)0;
          }
     }

     /* This window doesn't match our 'HIDDEN' criteria, ignore it
      * (and its children) */
     if (!(flags & H_HIDDEN) && !is_viewable(display, from))
	  return (Window)0;
  
     if (check_window_name(from, name))
     {
          /* See if some instance of mozplugger got the window before us, if
           * not mark it as ours */
          if(chkAndMarkVictimWindow(from))
          {
	        return from;
          }
     }

     if (depth > 0)
     {
          /* OK if we get here, lets check out the children just in case the
           * application has decided to name a child not the parent?
           * Question? Does this ever happen? */

          if (!XQueryTree(display, from, &root, &parent, 
                                         &children, &num_children))
          {
	        D("find_winrecur: Querytree failed!!!\n");
	        return (Window)0;
          }

	  D("find_winrecur: Num children = %d\n", num_children);
	  for (e = 0; e < num_children; e++)
	  {
	       Window win = find_winrecur(children[e], depth-1, name);
	       if (win)
	       {
		    XFree(children);
		    return (Window)win;
	       }
	  }
          XFree(children);
     }
     return (Window)0;
}

void save_initial_windows()
{
     ignore_window_count = 0;
     init_winrecur(wattr.root, 3, winname);
}

static int setup_display()
{
     char *displayname = getenv("DISPLAY");
     D("setup_display(%s)\n", displayname);

     XSetErrorHandler(error_handler);

     if (!(display = XOpenDisplay(displayname)))
	  return 1;

     if (!XGetWindowAttributes(display, WINDOW, &wattr))
          return 1;

     D("display=%x\n", display);
     D("WINDOW=%x\n", WINDOW);
     D("rootwin=%x\n", wattr.root);
     
     D("setup_display() done\n");

     return 0;
}

/*****************************************************************************
 * Wrapper for execlp() that calls the application.
 *
 * WARNING: This function runs in the daughter process so one must assume the
 * daughter uses a copy (including) heap memory of the parent's memory space
 * i.e. any write to memory here does not affect the parent memory space. 
 * Since Linux uses copy-on-write, best leave memory read-only and once execlp
 * is called, all daughter memory is wiped anyway (except the stack).
 *****************************************************************************/
static void run_app(char **argv)
{
     /* Close all File descriptors inherited from the parent,
      * that's the XServer connection FD and pipe_fd */
     if(display)
     {
          close(ConnectionNumber(display));
     }
     close(pipe_fd);

     /* Redirect stdout & stderr to /dev/null */
     if(flags & (H_NOISY | H_DAEMON))
     {
	  const int ofd = open("/dev/null", O_RDONLY);

	  D("Redirecting stdout and stderr\n");

	  if(ofd == -1)
          {
	       exit(EX_UNAVAILABLE);
          }

	  dup2(ofd, 1);
	  dup2(ofd, 2);
	  close(ofd);
     }

     /* For a DAEMON we must also redirect STDIN to /dev/null
      * otherwise we can get some weird behaviour especially with realplayer
      * (I suspect it uses stdin to communicate with its peers) */
     if(flags & H_DAEMON)
     {
          const int ifd = open("/dev/null", O_WRONLY);

          D("Redirecting stdin also\n");

          if(ifd == -1)
          {
               exit(EX_UNAVAILABLE);
          }
          dup2(ifd, 0);
          close(ifd);
     }
#if DEBUG
     close_debug();
#endif
     execvp(argv[0], argv);
     D("Execvp failed. (errno=%d)\n", errno);
     exit(EX_UNAVAILABLE);
}


/*****************************************************************************
 * Looks for the window and reparents it when found.
 *
 *****************************************************************************/
static void find_victim(Window window)
{
     if (!victimDetails.window)
     {
	  D("Looking for victim... (%s)\n", winname);
	  
	  victimDetails.window = find_winrecur(window, 2, winname);
          
	  if (victimDetails.window)
	  {
	       XWindowAttributes ca;
	       XGetWindowAttributes(display, victimDetails.window, &ca);

   	       D("Found it!! Victim=%x\n", victimDetails.window);
               victimDetails.borderWidth = ca.border_width;
               victimDetails.x = ca.x;
               victimDetails.y = ca.y;
               victimDetails.width = ca.width;
               victimDetails.height = ca.height;

	       /* We don't need to listen for new windows anymore */
	       XSelectInput(display, wattr.root, 0);
	       giveSwallowMutex();
 	
	       if (flags & H_MAXASPECT)
	       {
		    set_aspect(ca.width, ca.height);
	       }
	       reparent_window();
	  }
     }
}

static Bool AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg)
{
     return True;
}

static void check_x_events(void)
{
     XEvent ev;

     while (XCheckIfEvent(display, &ev, AllXEventsPredicate, NULL))
     {
	  if (ev.xany.window == wattr.root)
	  {
	       switch (ev.type)
	       {
	       case CreateNotify:
		    D("***CreateNotify for root\n");
		    find_victim(ev.xcreatewindow.window);
		    break;

	       case MapNotify:
		    D("***MapNotify for root: %x\n", ev.xmap.window);
		    find_victim(ev.xmap.window);
		    break;

	       case ConfigureNotify:
		    D("***ConfigureNotify for root: %x\n", ev.xconfigure.window);
		    find_victim(ev.xconfigure.window);
		    break;

               default: 
                   /* 21 = ReparentNotify */
	           D("!!Got unhandled event for root->%d\n", ev.type);
                   break;
	       }
	  }
	  else if (victimDetails.window 
                               && ev.xany.window == victimDetails.window)
	  {
	       switch (ev.type)
	       {
	       case UnmapNotify:
		    D("UNMAPNOTIFY for victim\n");
		    victimDetails.mapped = 0;
		    break;
		    
	       case MapNotify:
		    D("MAPNOTIFY for victim\n");
		    victimDetails.mapped = 1;
		    adjust_window_size();
		    break;
	    
	       case ReparentNotify:
		    if (ev.xreparent.parent == WINDOW)
		    {
			 D("REPARENT NOTIFY on victim to the right window\n");
			 adjust_window_size();
			 victimDetails.reparented = 1;
		    } else {
			 D("REPARENT NOTIFY on victim to some other window!\n");
			 reparent_window();
			 victimDetails.reparented = 0;
		    }
		    break;

	       case ConfigureNotify:
		    D("CONFIGURE NOTIFY for victim\n");
		    D("- x=%d, y=%d, w=%d, h=%d, border_width=%d\n",
                          ev.xconfigure.x, ev.xconfigure.y, 
                          ev.xconfigure.width, ev.xconfigure.height, 
                          ev.xconfigure.border_width);
		    break;

               case ClientMessage:
		    D("CLIENT MESSAGE for victim\n");
                    /* I see this with evince! perhaps it needs to be handled? */
                    break;

               default: 
                   /* 22 = ConfigureNotify, 33 = ClientMessage */
	           D("!!Got unhandled event for victim->%d\n", ev.type);
                   break;
	       }
	  }
	  else if (WINDOW && ev.xany.window == WINDOW)
	  {
               unsigned long mask;
	       switch (ev.type)
	       {
	       case ConfigureRequest:
                    mask = ev.xconfigurerequest.value_mask;
		    D("ConfigureRequest to WINDOW mask=0x%x\n", mask);
                    if(ev.xconfigurerequest.window != victimDetails.window)
                    {
                         D("- Strange Configure Request not for victim\n");
                         break;
                    }
                    if(mask & (CWHeight | CWWidth))
                    {
                         D(" - request to set width & height\n");
		         set_aspect(ev.xconfigurerequest.width, 
                                     ev.xconfigurerequest.height);
		         adjust_window_size();
                    }
                    if(mask & (CWBorderWidth))
                    {
                         const int w =  ev.xconfigurerequest.border_width; 
                         D("- request to set border width=%d\n", w);
                         if(victimDetails.borderWidth != w)
                         {
                              victimDetails.borderWidth = w;
                              adjust_window_size();
                         }
                         XSetWindowBorderWidth(display, victimDetails.window, 
                                                        (unsigned) w);
                    }
		    break;

               default: 
	           D("!!Got unhandled event for WINDOW->%d\n", ev.type);
                   break;
	       }
	  }
          else
          {
	       D("!!Got unhandled event for uknown->%d\n", ev.type);
          }
     }

     /* For Window Maker */
     if (victimDetails.reparented && !victimDetails.mapped) 
     {
	  D("XMapWindow\n");
	  XMapWindow(display, victimDetails.window);
     }
}

static void check_pipe_fd_events(void)
{
     static NPWindow wintmp;
     int n;
  
     Window oldwindow = WINDOW;
     D("Got pipe_fd data, old parent=%x pipe_fd=%d\n", WINDOW, pipe_fd);
  
     n = read(pipe_fd, ((char *)& wintmp),  sizeof(wintmp));
     if (n < 0)
     {
	  if (errno == EINTR) return;
	  D("Winddata read error, exiting\n");
          giveSwallowMutex();
	  exit(EX_UNAVAILABLE);
     }
  
     if (n == 0)
     {
	  D("Winddata EOF, exiting\n");
          giveSwallowMutex();
	  exit(EX_UNAVAILABLE);
     }
  
     if (n != sizeof(wintmp))
	  return;
  
     parentDetails.window = (Window) wintmp.window;
     parentDetails.width = (int) wintmp.width;
     parentDetails.height = (int) wintmp.height;

     D("Got pipe_fd data, new parent=%x\n", WINDOW);
  
     if (WINDOW && WINDOW != oldwindow && display)
     {
          victimDetails.reparented = 0;
	  XSelectInput(display, oldwindow, 0);
	  XSelectInput(display, WINDOW, SubstructureRedirectMask);
	  XSync(display, False);
          reparent_window();
     }
  
     /* The window has been resized. */
     adjust_window_size();
}

static void check_all_events()
{
     struct timeval tv;
     int maxfd;
     fd_set fds;

     FD_ZERO(&fds);
     FD_SET(ConnectionNumber(display), &fds);
     FD_SET(pipe_fd, &fds);

     maxfd = MAX(ConnectionNumber(display), pipe_fd);
     
     tv.tv_sec = SELECT_TIMEOUT_SEC;
     tv.tv_usec = SELECT_TIMEOUT_USEC;
     
#if 0
     D("SELECT IN\n");
#endif

     if (select(maxfd + 1, &fds, NULL, NULL, &tv) > 0)
     {
	  D("SELECT OUT\n");
	  if (FD_ISSET(pipe_fd, &fds))
	       check_pipe_fd_events();
     }

     if (display)
     {
	  check_x_events();
     }
}

static void handle_app(pid_t pid)
{
     int status;

     if (flags & H_SWALLOW)
     {
          /* Whilst waiting for the Application to complete, check X events */
	  while (!waitpid(pid, &status, WNOHANG))
	  {
	       check_all_events();
	  }
          /* Make sure the semaphore has been released */
          giveSwallowMutex();
     }
     else if(flags & H_DAEMON)
     {
          /* If Daemon, then it is not supposed to exit, so we exit instead */
          exit(0);
     }
     else
     {
          /* Just wait for the Application to complete dont check X events */
	  waitpid(pid, &status, 0);
     }

     /* If Application completed is a bad way, then lets give up now */
     if (!WIFEXITED(status))
     {
	  D("Process dumped core or something...\n");
	  exit(EX_UNAVAILABLE);
     }

     if (WEXITSTATUS(status) && !(flags & H_IGNORE_ERRORS))
     {
	  D("Process exited with error code: %d\n", WEXITSTATUS(status));
	  exit(WEXITSTATUS(status));
     }

     D("Exited OK!\n");
}

/******************************************************************************
 * main() normally called from the child process started by mozplugger.so 
 * 
 *****************************************************************************/
int main(int argc, char **argv)
{
     unsigned long temp = 0;
     int x, y;
     D("Helper started.....\n");

     if (argc < 2)
     {
	  fprintf(stderr,"MozPlugger version " VERSION " helper application.\n"
		  "Please see 'man mozplugger' for details.\n");
	  exit(1);
     }

     memset(&victimDetails, 0, sizeof(victimDetails));
     memset(&parentDetails, 0, sizeof(parentDetails));

     sscanf(argv[1],"%d,%d,%d,%lu,%d,%d,%d,%d",
	    &flags,
	    &repeats,
	    &pipe_fd,
	    &temp,
	    &x,
	    &y,
	    &parentDetails.width,
	    &parentDetails.height);

     parentDetails.window = (Window)temp;

     command = argv[2];
     winname = getenv("winname");
     file = getenv("file");

     D("HELPER: %s %s %s %s\n",
       argv[0],
       argv[1],
       file,
       command);

     if (repeats < 1) repeats = 1;

     while (repeats > 0)
     {
	  char *argv[10];
	  int loops = 1;
	  pid_t pid;

	  /* This application will use the $repeat variable */
	  if (flags & H_REPEATCOUNT)
	       loops = repeats;

	  /* Expecting the application to loop */
	  if (flags & H_LOOP)
	       loops = MAXINT;

	  /* Create command line */
	  argv[0] = "/bin/sh";
	  argv[1] = "-c";
	  argv[2] = command;
	  argv[3] = NULL;
	  
	  if (setup_display())
	  {
	       D("setup_display() failed\n");
	       flags &=~ H_SWALLOW;
	  }

	  if (flags & H_SWALLOW)
	  {
	       /* If we are swallowing a victim window we need to guard
		  against more than one instance of helper running in
		  parallel, this is done using a global mutex semaphore. */
	       initSwallowMutex();

	       /* Belt & braces, make sure we give semaphore on error. */
	       signal(SIGTERM, giveSwallowMutex);

	       takeSwallowMutex(); 

	       /* Has to be done before forking */
	       save_initial_windows();
	       XSelectInput(display, WINDOW, SubstructureRedirectMask);
	       XSelectInput(display, wattr.root, SubstructureNotifyMask);
	       XSync(display, False);
	  }

	  if ((pid = fork()) == -1)
          {
               giveSwallowMutex();  
	       exit(EX_UNAVAILABLE);
          }
     
	  if (pid == 0)
	  {
               /* Child process */

#if 0 /* Peter Leese - I'm sure this is not needed, especially if FD is closed 
         in run_app() */
               XSelectInput(display, WINDOW, 0);
               XSelectInput(display, wattr.root, 0);
#endif
	       D("Running %s\n", command);
	       run_app(argv);
	  }
	  else
	  {
	       D("Waiting for pid=%d\n", pid);
	       handle_app(pid);
	       D("Wait done (repeats=%d, loops=%d)\n", repeats, loops);
	       if (repeats < MAXINT)
		    repeats -= loops;
	  }
          /* On each repeat we re-open the XServer connection, so need to
           * close it at end of loop - TODO perhaps we should keep in open
           * for all repeats? */
          XCloseDisplay(display);
     }

     exit(0);
}
