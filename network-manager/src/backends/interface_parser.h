/* NetworkManager -- Network link manager
 *
 * Tom Parker <palfrey@tevp.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2004 Tom Parker
 */


#ifndef _INTERFACE_PARSER_H
#define _INTERFACE_PARSER_H

#define INTERFACES "/etc/network/interfaces"

typedef struct _if_data
{
	char *key;
	char *data;
	struct _if_data *next;
} if_data;

typedef struct _if_block
{
	char *type;
	char *name;
	if_data *info;
	struct _if_block *next;
	struct _if_block *nextsame;
} if_block;

void ifparser_init(void);
void ifparser_destroy(void);

if_block *ifparser_getif(const char* iface);
if_block *ifparser_getmapping(const char* iface);
if_block *ifparser_getfirst(void);
const char *ifparser_getkey(if_block* iface, const char *key);

if_block *add_block(const char *type, const char* name);
void add_data(const char *key,const char *data);
void _destroy_data(if_data *ifd);
void _destroy_block(if_block* ifb);
#endif
	
