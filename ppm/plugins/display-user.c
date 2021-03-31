/*
 * This file is part of the Linux Power Policy Manager
 *
 * Copyright © 2007 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU General Public License version 2 (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of the above.  If you wish to
 * allow the use of your version of this file only under the terms of the GPL
 * and not to allow others to use your version of this file under the MIT
 * license, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the GPL.  If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under either the GPL or the MIT license.
 *
 * Authors:
 * 	Tariq Shureih  <tariq.shureih@intel.com>
 * 	Arjan van de Ven <arjan@linux.intel.com>
 * 	Mohamed Abbas <mohamed.abbas@intel.com>
 * 	Sarah Sharp <sarah.a.sharp@intel.com>
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <asm/types.h>
#include <X11/Xfuncs.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/dpmsstr.h>

#include "linuxppm.h"


static Display *display = NULL;



void set_brightness(char *option)
{
	/* do nothing the ppmd should handle this one */
}


void set_dpms(char *option)
{

	if (display == NULL) {
		PRINTF("Error we can not get display\n");
		return;
	}

	if ( (strcmp(option, "on")) == 0) {
		DPMSEnable(display);
		DPMSForceLevel(display, DPMSModeOn);
		XSync(display, FALSE);
	}
	if ( (strcmp(option, "standby")) == 0) {
		DPMSEnable(display);
		DPMSForceLevel(display, DPMSModeStandby);
		XSync(display, FALSE);
	}
	if ( (strcmp(option, "suspend")) == 0) {
		DPMSEnable(display);
		DPMSForceLevel(display, DPMSModeSuspend);
		XSync(display, FALSE);
	}
	if ( (strcmp(option, "off")) == 0) {
		DPMSEnable(display);
		DPMSForceLevel(display, DPMSModeOff);

	}
}


static int handle_xerror(Display *disp, XErrorEvent *error_event)
{
	return 0;
}

void set_screensaver(char *option)
{
	/* if the option is "user" then we don't do anything */
	if ( (strcmp(option, "user")) == 0)
		return;

	if (display == NULL) {
		PRINTF("Error we can not get display\n");
		return;
	}
	int timeout, interval, prefer_blank, allow_exp;

	/* First we get the current values */
	XGetScreenSaver(display, &timeout, &interval, &prefer_blank, &allow_exp);

	PRINTF("Current Screensaver values: timeout: %d, interval: %d, prefer_blank: %d, allow_exposure: %d\n",
			timeout, interval, prefer_blank, allow_exp);

	if ((strcmp(option, "activate")) == 0) {
		XActivateScreenSaver(display);
		return;
	}

	/* disbling screensaver can be string "off" or "0" */
	if (((strcmp(option, "off")) == 0)  ||
	    ((strcmp(option, "0")) == 0)){
		XSetScreenSaver(display, 0, interval, prefer_blank, allow_exp);
		return;
	}

	/* anyother value passed in as string */
	timeout = atoi(option);

	if (!timeout) {
		PRINTF("Screen Saver Timeout Invalid?\n");
		XSetScreenSaver(display, -1, interval, prefer_blank, allow_exp);
		return;
	} else {
		XSetScreenSaver(display, timeout, interval, prefer_blank, allow_exp);
		return;
	}
}

static void display_message(char *class, char *command, char *option)
{
	if (!class || !command) {
		PRINTF("Malformed Messages, aborting\n");
		return; /* malformed message */
	}

	if (strcmp(class, "display"))
		return; /* not for us */

	display = XOpenDisplay(NULL);

	if (display)
		XSetErrorHandler (handle_xerror);

	PRINTF("Display command is %s\n", command);
	/* if command is dpms we can do dpms that's special */
	if (strcmp(command, "dpms")== 0 && option)
		set_dpms(option);

	/* if command is brightness we can do basic brightness controles */
	if (strcmp(command, "brightness")== 0 && option)
		set_brightness(option);

	/* if command is screensaver we manipulate the screensaver */
	if (strcmp(command, "screensaver")== 0 && option)
		set_screensaver(option);

	if (display)
		XCloseDisplay(display);
	display = NULL;
}


void start_plugin(void)
{
	register_plugin("display", display_message);
}

