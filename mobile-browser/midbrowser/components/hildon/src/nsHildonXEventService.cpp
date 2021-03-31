/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Feed Content Sniffer.
 *
 * The Initial Developer of the Original Code is Canonical Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alexander Sack <asac@jwsdot.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>

#include "prlog.h"

#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIFrame.h"

#include "nsHildonXEventService.h"

typedef enum {
  _HILDON_EVENT_NONE,
  _HILDON_EVENT_MENU,
  _COUNT_HILDON_EVENTS
} nsHildonXEventService__HildonEvent;

NS_IMPL_ISUPPORTS1(nsHildonXEventService, nsIHildonXEventService)

typedef struct _Listener {
  nsCOMPtr<nsIHildonXEventListener> listener;
  int mask;
  Window root;
} Listener;

static PRLogModuleInfo *gHildonXEventServiceLog = PR_NewLogModule("hildon");

static Window gCurrentActive = None;

static int
xclient_message_type_check                      (XClientMessageEvent *cm,
                                                 const gchar *name)
{
  return cm->message_type == XInternAtom(GDK_DISPLAY(), name, FALSE);
}


static GdkWindow*
nsHildonXEventService__peek_window_from_dom_element(nsIDOMElement *domElement)
{
  nsCOMPtr<nsIDOMNode> elementNode = do_QueryInterface(domElement);
  nsCOMPtr<nsIContent> elementContent = do_QueryInterface(elementNode);
  nsCOMPtr<nsIDOMDocument> ownerDocument;
  elementNode->GetOwnerDocument(getter_AddRefs(ownerDocument));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(ownerDocument);
  nsIPresShell *presShell = doc->GetPrimaryShell();

  if (!presShell)
    return 0;

  nsIFrame *frame = nsnull;

  nsCOMPtr<nsIContent> peekContent = elementContent;

  while(!frame && elementContent)
  {
    frame = presShell -> GetPrimaryFrameFor(peekContent);
    peekContent = peekContent->GetParent();
  }

  NS_ASSERTION(frame, "No primary frame for peeked xul element.");

  nsIWidget *boxObjectWindowWidget = frame->GetWindow();
  GdkWindow* gdkWindow = (GdkWindow*) boxObjectWindowWidget->GetNativeData(NS_NATIVE_WINDOW);
  return gdkWindow;
}


static GdkWindow*
nsHildonXEventService__peek_root_window_from_dom_element(nsIDOMElement *domElement)
{
  GdkWindow* gdkWindow = nsHildonXEventService__peek_window_from_dom_element(domElement);

  if(!gdkWindow)
    return NULL;

  GdkWindow* rootWindow = gdk_window_get_toplevel(gdkWindow);
  
  return rootWindow;
}

static Window
nsHildonXEventService__get_active_window                 (void)
{
  Atom realtype;
  gint xerror;
  int format;
  int status;
  Window ret;
  unsigned long n;
  unsigned long extra;
    union
    {
      Window *win;
      unsigned char *char_pointer;
    } win;
    Atom active_app_atom =
      XInternAtom (GDK_DISPLAY (), "_MB_CURRENT_APP_WINDOW", False);

    win.win = NULL;

    gdk_error_trap_push ();
    status = XGetWindowProperty (GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                                 active_app_atom, 0L, 16L,
                                 0, XA_WINDOW, &realtype, &format,
                                 &n, &extra, &win.char_pointer);
    xerror = gdk_error_trap_pop ();

    if (xerror || !(status == Success && realtype == XA_WINDOW && format == 32
                    && n == 1 && win.win != NULL))
    {
      if (win.win != NULL)
        XFree (win.char_pointer);
      return None;
    }

    ret = win.win[0];

    if (win.win != NULL)
      XFree(win.char_pointer);

    return ret;
}



static GdkFilterReturn
nsHildonXEventService__root_window_event_filter         (GdkXEvent *xevent,
                                                         GdkEvent *event,
                                                         gpointer data)
{
  nsHildonXEventService *self = (nsHildonXEventService*) data;

  g_assert(self);

  XAnyEvent *eventti = (XAnyEvent*) xevent;
  Atom active_app_atom =
    XInternAtom (GDK_DISPLAY (), "_MB_CURRENT_APP_WINDOW", False);

  Window active_window = nsHildonXEventService__get_active_window();

  if (eventti->type == PropertyNotify)
  {
    XPropertyEvent *pevent = (XPropertyEvent*) xevent;

    PR_LOG(gHildonXEventServiceLog,
           PR_LOG_DEBUG,
           ("  X Property Event Atom %d (A=%d)", pevent->atom , active_app_atom));

    if (pevent->atom == active_app_atom)
    {
      Window oldActive = gCurrentActive;
      gCurrentActive = active_window;

      if(oldActive != active_window)
      {
        PR_LOG(gHildonXEventServiceLog,
               PR_LOG_NOTICE,
               ("Active Window Changed: %d --> %d", oldActive, gCurrentActive));
      }
    }
  }

  return GDK_FILTER_CONTINUE;
}


static GdkFilterReturn
nsHildonXEventService__window_event_filter         (GdkXEvent *xevent,
                                                    GdkEvent *event,
                                                    gpointer data)
{
  nsHildonXEventService *self = (nsHildonXEventService*) data;
  g_assert(self);
  nsHildonXEventService__HildonEvent hildonevent = _HILDON_EVENT_NONE;

  XAnyEvent *eventti = (XAnyEvent*) xevent;
  Atom grab_transfer_atom =
    XInternAtom (GDK_DISPLAY (), "_MB_GRAB_TRANSFER", False);

  Window active_window = nsHildonXEventService__get_active_window();

  if (eventti->type == ClientMessage)
  {
    PR_LOG(gHildonXEventServiceLog,
           PR_LOG_DEBUG,
           ("  XEvent ClientMessage - event received."));

    XClientMessageEvent *cm = (XClientMessageEvent*) eventti;

    if (xclient_message_type_check (cm, "_MB_GRAB_TRANSFER"))
    {
      hildonevent = _HILDON_EVENT_MENU;
    }
  }
  else if (eventti->type == KeyRelease)
  {
    PR_LOG(gHildonXEventServiceLog,
           PR_LOG_DEBUG,
           ("  XEvent KeyRelease - event received."));

    XKeyReleasedEvent *keyevent = (XKeyReleasedEvent*) eventti;  
    KeySym keysym = XKeycodeToKeysym(GDK_DISPLAY (), keyevent->keycode, 0);

    switch (keysym) {
    case XK_F4:
      PR_LOG(gHildonXEventServiceLog,
             PR_LOG_DEBUG,
             ("  XEvent KeyRelease - hildon key released (keysym/keycode): %d/%d.", keysym, keyevent->keycode));
      hildonevent = _HILDON_EVENT_MENU;
      break;
    default:
      PR_LOG(gHildonXEventServiceLog,
             PR_LOG_DEBUG,
             ("  XEvent KeyRelease - none-hildon key released (keysym/keycode): %d/%d.", keysym, keyevent->keycode));
      break;
    }
  }

  switch (hildonevent) {
  case _HILDON_EVENT_MENU:
    {
      int c,i,count=0;
      c=self->mEventListeners->Count();
      for(i=0; i < c; i++)
      {
        Listener *listener = (Listener*) self->mEventListeners->ElementAt(i);
        if(listener->root == active_window || listener->root == eventti->window)
        {
          listener->listener->HildonEvent(1, nsnull);
          count++;
        }
      }
      PR_LOG(gHildonXEventServiceLog,
             PR_LOG_DEBUG,
             ("  XEvent - hildon event dispatched to %d (of %d) listeners.", count, c));
    }
    return GDK_FILTER_REMOVE;
  default:
    PR_LOG(gHildonXEventServiceLog,
           PR_LOG_DEBUG,
           ("  XEvent - not a hildon event."));
    break;
  }

  return GDK_FILTER_CONTINUE;
}


nsHildonXEventService::nsHildonXEventService()
{
  mEventListeners = new nsVoidArray();
  PR_LOG(gHildonXEventServiceLog, PR_LOG_DEBUG, ("nsHildonXEventService::ctor - start ..."));

  GdkWindow* win = gdk_get_default_root_window();

  gdk_window_set_events (gdk_get_default_root_window (),
                        (GdkEventMask) (gdk_window_get_events (gdk_get_default_root_window ())
                                        | GDK_ALL_EVENTS_MASK));
  gdk_window_add_filter (win,
                         nsHildonXEventService__root_window_event_filter, this );

  gdk_display_add_client_message_filter(gdk_drawable_get_display(win),
                                        gdk_atom_intern_static_string("_MB_GRAB_TRANSFER"),
                                        nsHildonXEventService__window_event_filter,
                                        this);
  gdk_display_add_client_message_filter(gdk_drawable_get_display(win),
                                        gdk_atom_intern_static_string("_NET_WM_CONTEXT_CUSTOM"),
                                        nsHildonXEventService__window_event_filter,
                                        this);
  PR_LOG(gHildonXEventServiceLog, PR_LOG_DEBUG, ("nsHildonXEventService::ctor - ... end."));
}

nsHildonXEventService::~nsHildonXEventService()
{
}

nsresult
nsHildonXEventService::Init()
{
  return NS_OK;
}


NS_IMETHODIMP
nsHildonXEventService::AddEventListener(PRInt32 eventMask,
					nsIHildonXEventListener *listener,
                                        nsIDOMElement *domElement,
					nsISupports *data)
{
  Atom *old_atoms, *new_atoms;
  int atom_count = 0;

  NS_ASSERTION(domElement, "nsHildonXEventService::AddEventListener - domElement must not be null.");

  GdkWindow *gdkWindow =
    nsHildonXEventService__peek_window_from_dom_element(domElement);

  if(!gdkWindow)
  {
    PR_LOG(gHildonXEventServiceLog, PR_LOG_DEBUG, ("nsHildonXEventService::AddEventListener - no window found for dom element."));
    return NS_ERROR_FAILURE;
  }

  GdkWindow *topGdkWindow = gdkWindow;
  while(topGdkWindow && gdk_window_get_window_type(topGdkWindow) != GDK_WINDOW_TOPLEVEL)
  {
    topGdkWindow = gdk_window_get_parent(topGdkWindow);
  }
  PR_LogPrint("GdkWindow (Element): %d (Toplevel): %d", gdkWindow, topGdkWindow);

  gdk_window_add_filter (topGdkWindow, nsHildonXEventService__window_event_filter, this);

  Window window = GDK_WINDOW_XID(topGdkWindow);
  Display *display = GDK_WINDOW_XDISPLAY(topGdkWindow);

  XGetWMProtocols (display, window, &old_atoms, &atom_count);
  new_atoms = g_new (Atom, atom_count + 1);

  memcpy (new_atoms, old_atoms, sizeof(Atom) * atom_count);

  PR_LogPrint("Win atom count: %d", atom_count);

  new_atoms[atom_count++] =
    XInternAtom (display, "_NET_WM_CONTEXT_CUSTOM", False);

  XSetWMProtocols (display, window,  new_atoms, atom_count);

  XFree(old_atoms);
  g_free(new_atoms);

  PR_LogPrint("Display: %d", display);

  Listener *eventListener = new Listener();

  eventListener->mask = eventMask;
  eventListener->listener = listener;
  eventListener->root = window;
  mEventListeners->AppendElement(eventListener);

  PR_LogPrint("Root Window found: %d", window);
  return NS_OK;
}


NS_IMETHODIMP
nsHildonXEventService::GetElementsRootWindowOffsets (nsIDOMElement *element,
                                                     PRInt32 *xOffset,
                                                     PRInt32 *yOffset)
{
  GdkWindow *gdk_window = nsHildonXEventService__peek_root_window_from_dom_element(element);
  gdk_window_get_origin (gdk_window, xOffset, yOffset);

  PR_LOG(gHildonXEventServiceLog,
         PR_LOG_DEBUG,
         ("  XEvent Window - X/Y:%d/%d", *xOffset, *yOffset));
  return NS_OK;
}
