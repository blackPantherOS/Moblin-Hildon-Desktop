/* NetworkManager -- Forget about your network
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

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum TestResult
{
	TEST_FAIL = 0,
	TEST_SUCCEED
} TestResult;

void test_result (const char *progname,
                  const char *test,
                  TestResult result,
                  const char *format,
                  ...);

#endif	/* TEST_COMMON_H */
