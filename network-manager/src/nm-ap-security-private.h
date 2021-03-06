/* NetworkManager -- Network link manager
 *
 * Dan Williams <dcbw@redhat.com>
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
 * (C) Copyright 2005 Red Hat, Inc.
 */

#ifndef NM_AP_SECURITY_PRIVATE_H
#define NM_AP_SECURITY_PRIVATE_H

#include "nm-ap-security.h"

void nm_ap_security_set_we_cipher (NMAPSecurity *self, int we_cipher);

void nm_ap_security_set_key (NMAPSecurity *self, const char *key, int key_len);

void nm_ap_security_set_description (NMAPSecurity *self, const char *desc);

void nm_ap_security_copy_properties (NMAPSecurity *self, NMAPSecurity *dst);

#endif	/* NM_AP_SECURITY_PRIVATE_H */
