/*
 * Copyright (C) 2007 Andrew Turner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "test.h"

/* Tests for a facund_connection server */
START_TEST(pkg_freebsd_connection_server_null)
{
	/* Check a NULL argument returns NULL */
	fail_unless(facund_connect_server(NULL) == NULL);
}
END_TEST

/* Tests for a facund_connection client */
START_TEST(pkg_freebsd_connection_client_null)
{
	/* Check a NULL argument returns NULL */
	fail_unless(facund_connect_client(NULL) == NULL);
}
END_TEST

START_TEST(pkg_freebsd_connection_client_nonexistant)
{
	/* Check a non-existant socket returns NULL */
	fail_unless(facund_connect_server("/nonexistant") == NULL);
}
END_TEST

Suite *
facund_connection_suite()
{
	Suite *s;
	TCase *tc;

	s = suite_create("facund_connection");

	tc = tcase_create("server");
	tcase_add_test(tc, pkg_freebsd_connection_server_null);
	suite_add_tcase(s, tc);

	tc = tcase_create("server");
	tcase_add_test(tc, pkg_freebsd_connection_client_null);
	tcase_add_test(tc, pkg_freebsd_connection_client_nonexistant);
	suite_add_tcase(s, tc);

	return s;
}

