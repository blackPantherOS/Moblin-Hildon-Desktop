/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __EMPATHY_TIME_H__
#define __EMPATHY_TIME_H__

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#include <time.h>

#include <glib.h>

G_BEGIN_DECLS

#define EMPATHY_TIME_FORMAT_DISPLAY_SHORT "%H:%M"
#define EMPATHY_TIME_FORMAT_DISPLAY_LONG  "%a %d %b %Y"

/* Note: Always in UTC. */
typedef long EmpathyTime;

EmpathyTime  empathy_time_get_current     (void);
time_t      empathy_time_get_local_time  (struct tm   *tm);
EmpathyTime  empathy_time_parse           (const gchar *str);
EmpathyTime  empathy_time_parse_format    (const gchar *str,
					 const gchar *format);
gchar      *empathy_time_to_string_utc   (EmpathyTime   t,
					 const gchar *format);
gchar      *empathy_time_to_string_local (EmpathyTime   t,
					 const gchar *format);

G_END_DECLS

#endif /* __EMPATHY_TIME_H__ */

