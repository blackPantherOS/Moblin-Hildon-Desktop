/*****************************************************************************
 * Author:  Fredrik Hübinette <hubbe@hubbe.net>
 *          Peter Leese <peter@leese.net>
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

#include <unistd.h>
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

#include "mozplugger.h"

static Display *dpy=0;
static Window topLevel;
static int buttonsize=10;

#define STATE_PLAY 1
#define STATE_PAUSE 2
#define STATE_STOP 3

static int state=STATE_STOP;
static pid_t pid=-1;

static int repeats = 0;
static int repeatsResetVal;

static int pipe_fd;
static int flags;

static GC gc_white;
static GC gc_black;
static GC gc_gray;

static int bx;
static int by;

static int windowed=0;

XPoint coord(int x, int y)
{
     XPoint ret;
     ret.x=(x+1) * buttonsize / 16 + bx;
     ret.y=y * buttonsize / 16 + by;
     return ret;
}

void redraw()
{
     static int old_buttonsize = -1;
     XWindowAttributes attr;
     XPoint points[6];

     if(windowed && state == STATE_PLAY) return;

     XGetWindowAttributes(dpy, topLevel, &attr);
     buttonsize = attr.width/3;
     if(attr.height < buttonsize) buttonsize=attr.height;

     if(old_buttonsize != buttonsize)
     {
	  old_buttonsize=buttonsize;
	  XFillRectangle(dpy, topLevel, gc_white,
			 0,0,(unsigned)attr.width, (unsigned)attr.height);
     }
    

     bx=(attr.width - buttonsize*3)/2;
     by=(attr.height - buttonsize)/2;

     /* play */
     points[0]=coord(4,2);
     points[1]=coord(5,2);
     points[2]=coord(10,7);
     points[3]=coord(10,8);
     points[4]=coord(5,13);
     points[5]=coord(4,13);

     XFillPolygon(dpy,
		  topLevel,
		  state==STATE_PLAY ? gc_gray : gc_black,
		  points, 6,
		  Convex, CoordModeOrigin);

     /* pause */
     XFillRectangle(dpy, topLevel, state==STATE_PAUSE ? gc_gray : gc_black,
		    bx + buttonsize + buttonsize*3/16,
		    by + buttonsize * 2/16,
		    (unsigned) (buttonsize * 3/16),
		    (unsigned) (buttonsize * 11/16));

     XFillRectangle(dpy, topLevel, state==STATE_PAUSE ? gc_gray : gc_black,
		    bx + buttonsize + buttonsize*9/16,
		    by + buttonsize * 2/16,
		    (unsigned) (buttonsize * 3/16),
		    (unsigned) (buttonsize * 11/16));

     /* stop */
     XFillRectangle(dpy, topLevel, state==STATE_STOP ? gc_gray : gc_black,
		    bx+buttonsize*2 + buttonsize*3/16,
		    by+buttonsize * 3/16,
		    (unsigned) (buttonsize * 9/16),
		    (unsigned) (buttonsize * 9/16));
}

int igetenv(char *var, int def)
{
     char *tmp=getenv(var);
     if(!tmp) return def;
     return atoi(tmp);
}

void my_play(char * command)
{
     if(state != STATE_STOP)
     {
	  if(!kill(-pid, SIGCONT))
	  {
	       state=STATE_PLAY;
	       return;
	  }
     }

     pid=fork();
     if(pid == -1)
     {
	  state=STATE_STOP;
	  return;
     }
     if(pid == 0)
     {
	  char *cmd[4];
          /* Child process, so as usually need to cleanup any inherited
           * file descriptors */
          if(dpy)
          {
               close(ConnectionNumber(dpy));
          }

          /* Redirect stdout & stderr to /dev/null */
          if(flags & H_NOISY)
          {
	        const int ofd = open("/dev/null", O_RDONLY);
	        if(ofd)
                {
	             dup2(ofd, 1);
	             dup2(ofd, 2);
	             close(ofd);
                }
          }

	  setpgid(pid, 0);

	  cmd[0]="/bin/sh";
	  cmd[1]="-c";
	  cmd[2]=command;
	  cmd[3]=0;
	  execvp(cmd[0], cmd);
	  exit(EX_UNAVAILABLE);
     }
     else
     {
	  state=STATE_PLAY;
	  if(!repeats) 
              repeats = repeatsResetVal;
	  if((repeats > 0) && (repeats != MAXINT)) 
              repeats--;
     }
}

void my_pause(char * command)
{
     if(state != STATE_STOP)
     {
	  if(!kill(-pid, SIGSTOP))
	       state = STATE_PAUSE;
	  else
	       state = STATE_STOP;

	  return;
     }
}

void low_die()
{
     if(pid > 0) my_kill(-pid);
     _exit(0);
}

void my_stop(char * command)
{
     if(state == STATE_PAUSE) my_play(command);
     if(state == STATE_PLAY)
     {
	  if(pid > 0) my_kill(-pid);
	  state=STATE_STOP;
	  repeats=0;
     }
}

int die(Display *dpy, XErrorEvent *ev) 
{ 
     low_die(); 
     return 0; 
}

int die2(Display *dpy) 
{ 
     low_die(); 
     return 0; 
}

void sigdie(int sig) 
{ 
     low_die(); 
}

Bool AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg)
{
     return True;
}


/******************************************************************************
 * Given initial width & height of the window containing the buttons 
 * limit the width and aspect ratio and place in the center of the 
 * window (calculate x & y)
 ******************************************************************************/
static void normalise_window_coords(unsigned * width, unsigned * height,
                                    int * x, int * y)
{
    unsigned w = *width;
    unsigned h = *height;

    if(flags & H_FILL)        /* Dont bother if user wants to fill the window */
    {
        *x = *y = 0;
        return;
    }

    if((w > 300) & !(flags & H_MAXASPECT))
    {
        w = 300;
    }

    /* Keep the controls as close to 3:1 ratio as possible */
    if(h > w/3)
    {
        h = w/3;
    }

    *x = (int) (*width - w)/2;
    *y = (int) (*height - h)/2;

    *height = h;
    *width = w;
}

/******************************************************************************
 * Check to see if new window size information has arrived from plugin and if
 * so resize the controls accordingly
 ******************************************************************************/
static void check_pipe_fd_events(void)
{
     NPWindow wintmp;
     int n;
     int x, y;
     unsigned w, h;

     D("Got pipe_fd data, pipe_fd=%d\n", pipe_fd);
  
     n = read(pipe_fd, ((char *)& wintmp),  sizeof(wintmp));
     if (n < 0)
     {
	  if (errno == EINTR)
               return;
	  exit(EX_UNAVAILABLE);
     }
  
     if (n == 0)
     {
	  exit(EX_UNAVAILABLE);
     }
  
     if (n != sizeof(wintmp))
     {
	  return;
     }

     /* Adjust the width & height to compensate for the window border of 
      * 1 pixel */
     w = (unsigned) wintmp.width - 2;
     h = (unsigned) wintmp.height - 2;

     normalise_window_coords(&w, &h, &x, &y);

     D("Controller window: x=%i, y=%i, w=%u, h=%u\n", x, y, w, h);

     XMoveResizeWindow(dpy, topLevel, x, y, w, h);
}


/******************************************************************************
 * mozplugger-controller takes two arguments. The first is a shell command line
 * to run when the 'play' button is pressed (or if autostart is set). The second
 * parameter is optional and is the ID of the parent window into which to draw
 * the controls.
 *
 * If the command line contains the string "$window", then no controls are 
 * drawn.
 *
 ******************************************************************************/
int main(int argc, char **argv)
{
     int old_state;
     unsigned int width = 60;       /* Set defaults, may be changed later */
     unsigned int height = 20;
     Window parentWid;
     unsigned long temp = 0;
     int x, y ;
     char * command;

     XColor colour;
     XClassHint classhint;
     XSetWindowAttributes attr;
     XSizeHints wmHints;

     D("Controller started.....\n");

     if (argc < 2)
     {
	  fprintf(stderr,"MozPlugger version " VERSION 
                         " controller application.\n"
		         "Please see 'man mozplugger' for details.\n");
	  exit(1);
     }

     sscanf(argv[1],"%d,%d,%d,%lu,%d,%d,%d,%d",
	    &flags,
	    &repeatsResetVal,
	    &pipe_fd,
	    &temp,
	    (int *)&x,
	    (int *)&y,
	    (int *)&width,
	    (int *)&height);

     parentWid = (Window)temp;
     command = argv[2];

     D("CONTROLLER: %s %s %s %s\n",
       argv[0],
       argv[1],
       getenv("file"),
       command);

     /* The application will use the $repeat variable */
     if (flags & H_REPEATCOUNT)
     {   
	  repeatsResetVal = 1;
     }

     if(!(dpy = XOpenDisplay(getenv("DISPLAY"))))
     {
	  fprintf(stderr,"%s: unable to open display %s\n",
		  argv[0], XDisplayName(getenv("DISPLAY")));
	  return False;
     }

     /* If the command contains a reference to window it is likely that that
      * application also wants to draw into the same window as the controls
      * make a note of this for later */
     if(strstr(command,"$window") || strstr(command,"$hexwindow"))
     {
	  windowed=1;
     }

     /* Adjust the width & height to compensate for the window border of 
      * 1 pixel */
     width -= 2;
     height -= 2;

     /* x, y co-ords of the parent is of no interest, we need to know
      * the x, y relative to the parent. */
     normalise_window_coords(&width, &height, &x, &y);
     
     D("Controller window: x=%i, y=%i, w=%u, h=%u\n", x, y, width, height);

     wmHints.x=0;
     wmHints.y=0;
     wmHints.min_width = 48;
     wmHints.min_height = 16;
     wmHints.base_width = 60;
     wmHints.base_height = 20;
     wmHints.flags=PPosition | USPosition | PMinSize;

     attr.border_pixel = 0; 
     attr.background_pixel = WhitePixelOfScreen(DefaultScreenOfDisplay(dpy));
     attr.event_mask = ExposureMask;
     if(!windowed) attr.event_mask |= ButtonPressMask;
     attr.override_redirect=0;

     topLevel = XCreateWindow(dpy,
			      parentWid,
			      x, y,
			      width, height, 1, CopyFromParent,
			      InputOutput, (Visual *) CopyFromParent,
			      (unsigned long)(CWBorderPixel|
			       CWEventMask|
			       CWOverrideRedirect|
			       CWBackPixel),
			      &attr);

     classhint.res_name = "mozplugger-controller";
     classhint.res_class = "mozplugger-controller";
     XSetClassHint(dpy, topLevel, &classhint);
     XStoreName(dpy, topLevel, "mozplugger-controller");

     gc_black=XCreateGC(dpy,topLevel,0,0);
     XSetForeground(dpy,gc_black,BlackPixel(dpy,DefaultScreen(dpy)));

     gc_white=XCreateGC(dpy,topLevel,0,0);
     XSetForeground(dpy,gc_white,WhitePixel(dpy,DefaultScreen(dpy)));

     gc_gray=XCreateGC(dpy,topLevel,0,0);
     colour.red=0x0000;
     colour.green=0xa000;
     colour.blue=0x0000;
     colour.pixel=0;
     colour.flags=0;

     XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &colour);
     XSetForeground(dpy,gc_gray,colour.pixel);

     XSetWMNormalHints(dpy, topLevel, &wmHints);

     /* Map the window, if the parent has asked for redirect this does nothing
      * (i.e. if swallow has been used in mozplugger.c) */
     XMapWindow(dpy, topLevel);

     XSetIOErrorHandler(die2);
     XSetErrorHandler(die);

     old_state=state;

  
     {
	  static char buffer[128];
	  char *tmp;
          int left;
	  snprintf(buffer,sizeof(buffer), "window=%ld",(long)topLevel);
	  putenv(buffer);
	  tmp=buffer+strlen(buffer)+1;
          left = sizeof(buffer) - (tmp - buffer);
	  snprintf(tmp, left, "hexwindow=0x%lx",(unsigned long)topLevel);
	  putenv(tmp);
     }

     signal(SIGHUP, sigdie);
     signal(SIGINT, sigdie);
     signal(SIGTERM, sigdie);


     if(igetenv("autostart",1))
	  my_play(command);

     while(1)
     {
	  struct timeval tv;
	  fd_set fds;
	  XEvent ev;
          int maxFd;

	  if(state != old_state)
	  {
	       redraw();
	       old_state=state;
	  }

	  FD_ZERO(&fds);
	  FD_SET(ConnectionNumber(dpy),&fds);
	  FD_SET(pipe_fd,&fds);

          maxFd = ConnectionNumber(dpy);
          maxFd = MAX(maxFd, pipe_fd);

	  tv.tv_sec = 0;
	  tv.tv_usec = 1000000/5; /* Check if the subprocess died 5 times / sec */

	  if( select(maxFd + 1, &fds, NULL, NULL, &tv) > 0)
          {
	       if (FD_ISSET(pipe_fd, &fds))
               {    
	            check_pipe_fd_events();
               }
          }

	  if(pid != -1)
	  {
	       int status;
	       if(waitpid(pid, &status, WNOHANG) > 0)
	       {
		    pid=-1;
		    if(state == STATE_PLAY)
		    {
			 state = STATE_STOP;
			 if(repeats > 0) my_play(command);
		    }
	       }
	  }

	  while(XCheckIfEvent(dpy, &ev, AllXEventsPredicate, NULL))
	  {
	       switch(ev.type)
	       {
	       case ButtonPress:
		    if(ev.xbutton.button == 1)
		    {
			 int button = (ev.xbutton.x - bx) / buttonsize;
			 switch(button)
			 {
			 case 0: /* play */ my_play(command); break;
			 case 1: /* pause*/ my_pause(command);break;
			 case 2: /* stop */ my_stop(command); break;
			 }
		    }
		    break;
	  
	       case Expose:
		    if(ev.xexpose.count) break;
	       case ResizeRequest:
	       case MapNotify:
		    redraw();
		    break;
	  
#ifdef DEBUG
	       default:
		    fprintf(stderr,"Unknown event %d\n",ev.type);
#endif
	       }
	  }
     }
}

