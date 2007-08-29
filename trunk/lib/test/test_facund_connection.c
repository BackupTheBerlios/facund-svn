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

#include <facund_connection.h>

#include <sys/stat.h>

#include <string.h>

/*
 * Tests for a facund_connection server
 */
START_TEST(facund_connection_server_null)
{
	/* Check a NULL argument returns NULL */
	fail_unless(facund_connect_server(NULL) == NULL);
}
END_TEST

START_TEST(facund_connection_server_good)
{
	struct facund_conn *conn;
	struct stat sb;

	fail_unless((conn = facund_connect_server("/tmp/facund_test")) != NULL,
	    NULL);
	fail_unless(lstat("/tmp/facund_test", &sb) == 0, NULL);
	fail_unless(S_ISSOCK(sb.st_mode), NULL);
	facund_cleanup(conn);

	/* Check the socket is cleaned up */
	fail_unless(lstat("/tmp/facund_test", &sb) == -1, NULL);
}
END_TEST

/*
 * Tests for a facund_connection client
 */
START_TEST(facund_connection_client_null)
{
	/* Check a NULL argument returns NULL */
	fail_unless(facund_connect_client(NULL) == NULL);
}
END_TEST

START_TEST(facund_connection_client_nonexistant)
{
	/* Check a non-existant socket returns NULL */
	fail_unless(facund_connect_client("/nonexistant") == NULL, NULL);
}
END_TEST

/*
 * Tests for both working together
 */
START_TEST(facund_connection_cs_connects)
{
	char buf[5];

	struct facund_conn *conn_s, *conn_c;
	fail_unless((conn_s = facund_connect_server("/tmp/facund_test"))
	    != NULL, NULL);
	fail_unless((conn_c = facund_connect_client("/tmp/facund_test"))
	    != NULL, NULL);
	fail_unless(facund_accept(conn_s) == 0, NULL);

	/* Test sending Client -> Server works */
	fail_unless(facund_send(conn_c, "test", 4) == 4, NULL);
	fail_unless(facund_recv(conn_s, buf, 4) == 4, NULL);
	buf[4] = '\0';
	fail_unless(strcmp(buf, "test") == 0, NULL);

	/* Test sending Server -> Client works */
	fail_unless(facund_send(conn_s, "mesg", 4) == 4, NULL);
	fail_unless(facund_recv(conn_c, buf, 4) == 4, NULL);
	buf[4] = '\0';
	fail_unless(strcmp(buf, "mesg") == 0, NULL);

	facund_close(conn_c);
	facund_cleanup(conn_s);
}
END_TEST

Suite *
facund_connection_suite()
{
	Suite *s;
	TCase *tc;

	s = suite_create("facund_connection");

	tc = tcase_create("server");
	tcase_add_test(tc, facund_connection_server_null);
	tcase_add_test(tc, facund_connection_server_good);
	suite_add_tcase(s, tc);

	tc = tcase_create("client");
	tcase_add_test(tc, facund_connection_client_null);
	tcase_add_test(tc, facund_connection_client_nonexistant);
	suite_add_tcase(s, tc);

	tc = tcase_create("client server");
	tcase_add_test(tc, facund_connection_cs_connects);
	suite_add_tcase(s, tc);

	return s;
}

