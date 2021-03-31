/*
 *
 * Copyright 2008 Canonical USA Inc.
 *
 * Contributors:
 * Bill Filler <bill.filler@canonical.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "mobile-basic-home-plugin.h"

void
on_active_window_changed (WnckScreen *screen,
                          WnckWindow *prev_window,
                          plugin_context_t *plugin_context);
void
on_window_opened (WnckScreen *screen, WnckWindow *window, plugin_context_t *plugin_context);
extern void
init_window_helper (plugin_context_t *priv);

extern void hide_banner();
static int desktop_pid = 0;

static void activate_window(WnckWindow *wnck_window) {
  GTimeVal now;
  g_get_current_time(&now);
  wnck_window_activate (wnck_window, now.tv_sec + now.tv_usec);
}

void
on_active_window_changed (WnckScreen *screen, 
                          WnckWindow *prev_window,
                          plugin_context_t *plugin_context)
{
  WnckWindow *window;
  WnckWindowType type;
  guint pid = 0;
  window = wnck_screen_get_active_window (screen);

  if (!WNCK_IS_WINDOW (window))
  {
    return;
  }

  pid = wnck_window_get_pid (window);
  type = wnck_window_get_window_type (window);

  g_print("window activated type=%i pid=%i name=%s\n", type, pid, wnck_window_get_name (window));

  // ignore if desktop window
  if (pid == desktop_pid) {
    return;
  }

  switch (type)
  {
    case WNCK_WINDOW_NORMAL:
    case WNCK_WINDOW_DIALOG:
      hide_banner();
      return;
    default:
      return;
   }
}

void
on_window_opened (WnckScreen *screen, WnckWindow *window, plugin_context_t *plugin_context)
{
  WnckWindowType type;
  int pid;

  type = wnck_window_get_window_type (window);
  pid = wnck_window_get_pid (window);
  g_print("window open type=%i pid=%i name=%s\n", type, pid , wnck_window_get_name (window));

  if (pid == desktop_pid) {
    return;
  }

  // activate normal windows and dialogs so that they have focus by default
  // and we get the active-window-changed signal so we know to tear down 
  // banner
  switch (type)
  {
    case WNCK_WINDOW_DESKTOP:
      if (desktop_pid == 0) {
        g_print("settings desktop_pid=%i\n", pid);
        desktop_pid = pid;
      }
      return;
    case WNCK_WINDOW_NORMAL:
    case WNCK_WINDOW_DIALOG:
      activate_window (window);
      break;
    default:
      return;
   }
}

extern void
init_window_helper (plugin_context_t *priv)
{

  WnckScreen *screen = wnck_screen_get_default ();

  g_signal_connect (screen, "window-opened",
                    G_CALLBACK (on_window_opened), (gpointer)priv);
  g_signal_connect (screen, "active-window-changed",
                    G_CALLBACK (on_active_window_changed), (gpointer)priv);
}

