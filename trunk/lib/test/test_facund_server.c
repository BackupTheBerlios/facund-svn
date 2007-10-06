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
#include <facund_object.h>
#include <facund_response.h>

#include <stdlib.h>
#include <string.h>

#define facund_server_pre(server_var, client_var) \
do { \
	/* Make sure the socket is removed */ \
	system("rm -f /tmp/facund_test"); \
	fail_unless((server_var = facund_connect_server("/tmp/facund_test")) \
	    != NULL, NULL); \
	fail_unless((client_var = facund_connect_client("/tmp/facund_test")) \
	    != NULL, NULL); \
	fail_unless(facund_server_start(server_var, 0) == 0, NULL); \
} while(0)

#define facund_server_check_data(connection, data) \
do { \
	char readbuf[1024]; \
	fail_unless(sizeof readbuf > strlen(data), NULL); \
	memset(readbuf, 0, sizeof readbuf); \
	facund_recv(connection, readbuf, strlen(data)); \
	fail_unless(strcmp(readbuf, data) == 0, \
	    "\nExpected: %s\n     Got: %s", data, readbuf); \
} while (0)

#define facund_server_test_empty_data(type) \
do { \
	struct facund_conn *conn_s, *conn_c; \
	const char msg[] = "<facund-client version=\"0\">" \
	    "<call name=\"foo\" id=\"1\">" \
	    "<data type=\""type"\"></data>" \
	    "</call>" \
	    "</facund-client>"; \
	/* Connect */ \
	facund_server_pre(conn_s, conn_c); \
	/* Check we connected correctly */ \
	facund_server_check_data(conn_c, "<facund-server version=\"0\">"); \
	/* Add a call handler and attempt to call it */ \
	facund_dummy_cb_called = 0; \
	fail_unless(facund_server_add_call("foo", facund_dummy_cb) == 0,NULL); \
	fail_unless(facund_send(conn_c, msg, sizeof msg) == sizeof msg, NULL); \
	/* Parse the call */ \
	fail_unless(facund_server_get_request(conn_s) == 0, NULL); \
	/* Check it failed */ \
	facund_server_check_data(conn_c, \
	    "<response id=\"1\" code=\"400\" message=\"Missing value\">" \
	    "</response>"); \
	fail_unless(facund_dummy_cb_called == 0, NULL); \
	fail_unless(facund_dummy_cb_obj_value == 0, NULL); \
	fail_unless(facund_server_finish(conn_s) == 0, NULL); \
	/* Check the server correctly closed the connection */ \
	facund_server_check_data(conn_c, "</facund-server>"); \
	/* Disconnect */ \
	facund_close(conn_c); \
	facund_cleanup(conn_s); \
} while(0)

#define facund_server_test_bad_data(type, data) \
do { \
	struct facund_conn *conn_s, *conn_c; \
	const char msg[] = "<facund-client version=\"0\">" \
	    "<call name=\"foo\" id=\"1\">" \
	    "<data type=\""type"\">"data"</data>" \
	    "</call>" \
	    "</facund-client>"; \
	/* Connect */ \
	facund_server_pre(conn_s, conn_c); \
	/* Check we connected correctly */ \
	facund_server_check_data(conn_c, "<facund-server version=\"0\">"); \
	/* Add a call handler and attempt to call it */ \
	facund_dummy_cb_called = 0; \
	fail_unless(facund_server_add_call("foo", facund_dummy_cb) == 0,NULL); \
	fail_unless(facund_send(conn_c, msg, sizeof msg) == sizeof msg, NULL); \
	/* Parse the call */ \
	fail_unless(facund_server_get_request(conn_s) == 0, NULL); \
	/* Check it succeeded */ \
	facund_server_check_data(conn_c, \
	    "<response id=\"1\" code=\"401\" message=\"Invalid value\">" \
	    "</response>"); \
	fail_unless(facund_dummy_cb_called == 0, NULL); \
	fail_unless(facund_dummy_cb_obj_value == 0, NULL); \
	fail_unless(facund_server_finish(conn_s) == 0, NULL); \
	/* Check the server correctly closed the connection */ \
	facund_server_check_data(conn_c, "</facund-server>"); \
	/* Disconnect */ \
	facund_close(conn_c); \
	facund_cleanup(conn_s); \
} while (0)

static struct facund_response *facund_dummy_cb(const char *,
    struct facund_object *);

static int facund_dummy_cb_called = 0;
static int facund_dummy_cb_obj_value = 0;

static struct facund_response *
facund_dummy_cb(const char *id, struct facund_object *obj)
{
	struct facund_object *ret;

	facund_dummy_cb_called = 1;
	if (facund_object_get_type(obj) == FACUND_INT) {
		facund_dummy_cb_obj_value = facund_object_get_int(obj);
	} else {
		facund_dummy_cb_obj_value = -1;
	}
	ret = facund_object_new_bool();
	facund_object_set_bool(ret, 1);
	return facund_response_new(id, RESP_GOOD, "message", ret);
}

START_TEST(facund_server_null)
{
	fail_unless(facund_server_start(NULL, 0) == -1, NULL);
	fail_unless(facund_server_get_request(NULL) == -1, NULL);
	fail_unless(facund_server_finish(NULL) == -1, NULL);
	fail_unless(facund_server_add_call(NULL, NULL) == -1, NULL);
	fail_unless(facund_server_add_call("foo", NULL) == -1, NULL);
	fail_unless(facund_server_add_call(NULL, facund_dummy_cb) == -1, NULL);
}
END_TEST

/*
 * Tests if the server will complain when adding a call with the same name
 */
START_TEST(facund_server_multiple_add_call)
{
	fail_unless(facund_server_add_call("foo", facund_dummy_cb) == 0, NULL);
	fail_unless(facund_server_add_call("foo", facund_dummy_cb) == -1, NULL);
}
END_TEST

/*
 * Tests if the server sends the correct data when connecting/disconnecting
 */
START_TEST(facund_server_create)
{
	struct facund_conn *conn_s, *conn_c;

	facund_server_pre(conn_s, conn_c);

	/* Check the server correctly started the connection */
	facund_server_check_data(conn_c, "<facund-server version=\"0\">");

	fail_unless(facund_server_finish(conn_s) == 0, NULL);

	/* Check the server correctly closed the connection */
	facund_server_check_data(conn_c, "</facund-server>");
	
	facund_close(conn_c);
	facund_cleanup(conn_s);
}
END_TEST

/*
 * Test if calls work as expected
 */
START_TEST(facund_server_call)
{
	struct facund_conn *conn_s, *conn_c;
	const char msg[] = "<facund-client version=\"0\">"
	    "<call name=\"foo\" id=\"1\"><data type=\"int\">10</data></call>"
	    "</facund-client>";
	char buf[1024];

	memset(buf, 0, sizeof buf);

	/* Connect */
	facund_server_pre(conn_s, conn_c);

	/* Check we connected correctly */
	facund_server_check_data(conn_c, "<facund-server version=\"0\">");

	/* Add a call handler and attempt to call it */
	facund_dummy_cb_called = 0;
	fail_unless(facund_server_add_call("foo", facund_dummy_cb) == 0, NULL);
	fail_unless(facund_send(conn_c, msg, sizeof msg) == sizeof msg, NULL);

	/* Parse the call */
	fail_unless(facund_server_get_request(conn_s) == 0, NULL);

	/* Check it succeeded */
	facund_server_check_data(conn_c,
	    "<response id=\"1\" code=\"0\" message=\"message\">"
	    "<data type=\"bool\">true</data></response>");
	fail_unless(facund_dummy_cb_called == 1, NULL);
	fail_unless(facund_dummy_cb_obj_value == 10, NULL);

	fail_unless(facund_server_finish(conn_s) == 0, NULL);

	/* Check the server correctly closed the connection */
	facund_server_check_data(conn_c, "</facund-server>");

	/* Disconnect */
	facund_close(conn_c);
	facund_cleanup(conn_s);
}
END_TEST

/*
 * Test if calls work as expected when the call is missing
 */
START_TEST(facund_server_call_missing)
{
	struct facund_conn *conn_s, *conn_c;
	const char msg[] = "<facund-client version=\"0\">"
	    "<call name=\"foo\" id=\"1\"><data type=\"int\">10</data></call>"
	    "</facund-client>";
	char buf[1024];

	memset(buf, 0, sizeof buf);

	/* Connect */
	facund_server_pre(conn_s, conn_c);

	/* Check we connected correctly */
	facund_server_check_data(conn_c, "<facund-server version=\"0\">");

	/* Add a call handler and attempt to call it */
	facund_dummy_cb_called = 0;
	fail_unless(facund_send(conn_c, msg, sizeof msg) == sizeof msg, NULL);

	/* Parse the call */
	facund_server_get_request(conn_s);

	/* Check it succeeded */
	facund_server_check_data(conn_c,
	    "<response id=\"1\" code=\"300\" message=\"Unknown call\">"
	    "</response>");
	fail_unless(facund_dummy_cb_called == 0, NULL);
	fail_unless(facund_dummy_cb_obj_value == 0, NULL);

	fail_unless(facund_server_finish(conn_s) == 0, NULL);

	/* Check the server correctly closed the connection */
	facund_server_check_data(conn_c, "</facund-server>");

	/* Disconnect */
	facund_close(conn_c);
	facund_cleanup(conn_s);
}
END_TEST

/* Test if calls work as expected with an empty bool */
START_TEST(facund_server_call_empty_bool)
{
	facund_server_test_empty_data("bool");
}
END_TEST

/* Test if calls work as expected with an empty int */
START_TEST(facund_server_call_empty_int)
{
	facund_server_test_empty_data("int");
}
END_TEST

/* Test if calls work as expected with an invalid int */
START_TEST(facund_server_call_bad_int)
{
	facund_server_test_bad_data("int", "foo");
}
END_TEST

/* Test if calls work as expected with an empty unsigned int */
START_TEST(facund_server_call_empty_uint)
{
	facund_server_test_empty_data("unsigned int");
}
END_TEST

/* Test if calls work as expected with an invalid unsigned int */
START_TEST(facund_server_call_bad_uint)
{
	facund_server_test_bad_data("unsigned int", "foo");
}
END_TEST

/* Test if calls work as expected with an empty string */
START_TEST(facund_server_call_empty_string)
{
	facund_server_test_empty_data("string");
}
END_TEST

/* Test if calls work as expected with an empty array */
START_TEST(facund_server_call_empty_array)
{
	facund_server_test_empty_data("array");
}
END_TEST

/* Test if calls work as expected with an invalid array */
START_TEST(facund_server_call_bad_array)
{
	facund_server_test_bad_data("array", "foo");
}
END_TEST

Suite *
facund_server_suite()
{
	Suite *s;
	TCase *tc;

	s = suite_create("facund_server");

	tc = tcase_create("server");
	tcase_add_test(tc, facund_server_null);
	tcase_add_test(tc, facund_server_multiple_add_call);
	tcase_add_test(tc, facund_server_create);
	tcase_add_test(tc, facund_server_call);
	tcase_add_test(tc, facund_server_call_missing);
	tcase_add_test(tc, facund_server_call_empty_bool);
	tcase_add_test(tc, facund_server_call_empty_int);
	tcase_add_test(tc, facund_server_call_bad_int);
	tcase_add_test(tc, facund_server_call_empty_uint);
	tcase_add_test(tc, facund_server_call_bad_uint);
	tcase_add_test(tc, facund_server_call_empty_string);
	tcase_add_test(tc, facund_server_call_empty_array);
	tcase_add_test(tc, facund_server_call_bad_array);
	suite_add_tcase(s, tc);

	return s;
}

