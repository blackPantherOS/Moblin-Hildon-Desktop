/*
 * Copyright (C) 2007 Intel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njp@o-hand.com>
 *
 */
#include <string.h>

#include "launcher-util.h"

/* Convert command line to argv array, stripping % conversions on the way */
#define MAX_ARGS 255

/*
 * Convert a desktop file exec string to an array of strings suitable for
 * passing to gdk_spawn_on_screen. Code from libtaku (matchbox-desktop-2).
 */
gchar**
launcher_util_exec_to_argv (const gchar *exec)
{
  const char *p;
  char *buf, *bufp, **argv;
  int nargs;
  gboolean escape, single_quote, double_quote;
  
  argv = g_new (char *, MAX_ARGS + 1);
  buf = g_alloca (strlen (exec) + 1);
  bufp = buf;
  nargs = 0;
  escape = single_quote = double_quote = FALSE;
  
  for (p = exec; *p; p++) {
    if (escape) {
      *bufp++ = *p;
      
      escape = FALSE;
    } else {
      switch (*p) {
      case '\\':
        escape = TRUE;

        break;
      case '%':
        /* Strip '%' conversions */
        if (p[1] && p[1] == '%')
          *bufp++ = *p;
        
        p++;

        break;
      case '\'':
        if (double_quote)
          *bufp++ = *p;
        else
          single_quote = !single_quote;
        
        break;
      case '\"':
        if (single_quote)
          *bufp++ = *p;
        else
          double_quote = !double_quote;
        
        break;
      case ' ':
        if (single_quote || double_quote)
          *bufp++ = *p;
        else {
          *bufp = 0;
          
          if (nargs < MAX_ARGS)
            argv[nargs++] = g_strdup (buf);
          
          bufp = buf;
        }
        
        break;
      default:
        *bufp++ = *p;
        break;
      }
    }
  }
  
  if (bufp != buf) {
    *bufp = 0;
    
    if (nargs < MAX_ARGS)
      argv[nargs++] = g_strdup (buf);
  }
  
  argv[nargs] = NULL;
  
  return argv;
}
