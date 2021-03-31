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


#include "interface_parser.h"
#include "NetworkManagerUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nm-utils.h"

if_block* first;
if_block* last;

if_data* last_data; 

if_block *add_block(const char *type, const char* name)
{
	if_block *ret = (if_block*)calloc(1,sizeof(struct _if_block));
	ret->name = g_strdup(name);
	ret->type = g_strdup(type);
	if (first == NULL)
		first = last = ret;
	else
	{
		last->next = ret;
		last = ret;
	}
	last_data = NULL;
	//printf("added block '%s' with type '%s'\n",name,type);
	return ret;
}

void add_data(const char *key,const char *data)
{
	if_data *ret;

	// Check if there is a block where we can attach our data
	if (first == NULL)
		return;
			
	ret = (if_data*) calloc(1,sizeof(struct _if_data));
	ret->key = g_strdup(key);
	ret->data = g_strdup(data);
	
	if (last->info == NULL)
	{
		last->info = ret;
		last_data = ret;
	}
	else
	{
		last_data->next = ret;
		last_data = last_data->next;
	}
	//printf("added data '%s' with key '%s'\n",data,key);
}

// define what we consider a whitespace
#define WS " \t"

void ifparser_init(void)
{
	FILE *inp = fopen(INTERFACES,"r");
	int ret = 0;
	int pos;
	int len;
	char *line;
	char *key;
	char *data;
	char rline[255];

	if (inp == NULL)
	{
		nm_warning ("Error: Can't open %s\n",INTERFACES);
		return;
	}
	first = last = NULL;
	while(1)
	{
		line = NULL;
		ret = fscanf(inp,"%255[^\n]\n",rline);
		if (ret == EOF)
			break;
		// If the line did not match, skip it
		if (ret == 0) {
			fgets(rline, 255, inp);
			continue;
		}
		
		line = rline;
		while(line[0] == ' ')
			line++;
		if (line[0] == '#' || line[0] == '\0')
			continue;
		
		len = strlen(line);
		pos = 0;
		while (!strchr(WS, line[pos]) && pos < len) pos++;

		// terminate key string and skip further whitespaces
		line[pos++] = '\0';
		while (strchr(WS, line[pos]) && pos < len) pos++;

		if (pos >= len)
		{
			nm_warning ("Error: Can't parse line '%s'\n", line);
			continue;
		}
		key = &line[0];
		data = &line[pos];

		// There are four different stanzas:
		// iface, mapping, auto and allow-*. Create a block for each of them.
		if (strcmp(key, "iface") == 0)
		{
			char *key2;
			if_block *old, *new;

			while (!strchr(WS, line[pos]) && pos < len ) pos++;

			// terminate first data string and skip further whitespaces
			line[pos++] = '\0';
			while (strchr(WS, line[pos]) && pos < len) pos++;
			if (pos >= len)
			{
				nm_warning ("Error: Can't parse iface line '%s'\n", data);
				continue;
			}
			key2 = &line[pos];

			// check if we already have an instance of iface foo.
			old = ifparser_getif(data);

			new = add_block(key, data);

			// if we do have an instance of iface foo, make sure the new
			// is linked from the old one.

			if ((old) && (old != new))
			{
				while (old->nextsame != 0)
				{
					old=old->nextsame;
				}
				old->nextsame=new;
			}		

			if (pos < len)
			{
				
				while (!strchr(WS, line[pos]) && pos < len ) pos++;

				// terminate key2 string and skip further whitespaces
				line[pos++] = '\0';
				while (strchr(WS, line[pos]) && pos < len) pos++;
				if (pos >= len)
				{
					nm_warning ("Error: Can't parse inet line '%s'\n", key2);
					continue;
				}
				data = &line[pos];

				add_data(key2, data);
			}
		}
		else if (strcmp(key, "auto") == 0)
			add_block(key, data);
		else if (strcmp(key, "mapping") == 0)
			add_block(key, data);
		else if (strncmp(key, "allow-", 6) == 0)
			add_block(key, data);
		else
			add_data(key, data);
		
		//printf("line: '%s' ret=%d\n",rline,ret);
	}
	fclose(inp);
}	
	
void _destroy_data(if_data *ifd)
{
	if (ifd == NULL)
		return;
	_destroy_data(ifd->next);
	free(ifd->key);
	free(ifd->data);
	free(ifd);
	return;
}

void _destroy_block(if_block* ifb)
{
	if (ifb == NULL)
		return;
	_destroy_block(ifb->next);
	_destroy_data(ifb->info);
	free(ifb->name);
	free(ifb->type);
	free(ifb);
	return;
}

void ifparser_destroy(void) 
{
	_destroy_block(first);
	first = last = NULL;
}

if_block *ifparser_getfirst(void)
{
	return first;
}

if_block *ifparser_getif(const char* iface)
{
	if_block *curr = first;
	while(curr!=NULL)
	{
		if (strcmp(curr->type,"iface")==0 && strcmp(curr->name,iface)==0)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

if_block *ifparser_getmapping(const char* iface)
{
	if_block *curr = first;
	while(curr!=NULL)
	{
		if (strcmp(curr->type,"mapping")==0 && strcmp(curr->name,iface)==0)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

const char *ifparser_getkey(if_block* iface, const char *key)
{
	if_data *curr = iface->info;
	while(curr!=NULL)
	{
		if (strcmp(curr->key,key)==0)
			return curr->data;
		curr = curr->next;
	}
	return NULL;
}
