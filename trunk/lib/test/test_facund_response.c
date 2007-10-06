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

#include <facund_object.h>
#include <facund_response.h>

#include <string.h>

START_TEST(facund_response_null)
{
	fail_unless(facund_response_new(NULL, RESP_GOOD, NULL, NULL) == NULL,
	    NULL);
	fail_unless(facund_response_new("id", RESP_GOOD, NULL, NULL) == NULL,
	    NULL);
}
END_TEST

START_TEST(facund_response_correct)
{
	struct facund_response *resp;

	fail_unless((resp = facund_response_new("id", RESP_GOOD, "msg", NULL))
	    != NULL, NULL);
	fail_unless(strcmp(
	    "<response id=\"id\" code=\"0\" message=\"msg\"></response>",
	    facund_response_string(resp)) == 0, NULL);
	fail_unless(facund_response_set_id(resp, "foo") == 0, NULL);
	fail_unless(strcmp(
	    "<response id=\"foo\" code=\"0\" message=\"msg\"></response>",
	    facund_response_string(resp)) == 0, NULL);
	facund_response_free(resp);
}
END_TEST

START_TEST(facund_response_correct_data)
{
	struct facund_response *resp;
	struct facund_object *obj;

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
	facund_object_set_bool(obj, 0);

	fail_unless((resp = facund_response_new("id", RESP_GOOD, "msg", obj))
	    != NULL, NULL);
	fail_unless(strcmp(
	    "<response id=\"id\" code=\"0\" message=\"msg\">"
	    "<data type=\"bool\">false</data></response>",
	    facund_response_string(resp)) == 0, NULL);
	facund_response_free(resp);
}
END_TEST

START_TEST(facund_response_correct_no_id)
{
	struct facund_response *resp;

	fail_unless((resp = facund_response_new(NULL, RESP_GOOD, "msg", NULL))
	    != NULL, NULL);
	fail_unless(strcmp(
	    "<response code=\"0\" message=\"msg\"></response>",
	    facund_response_string(resp)) == 0, NULL);
	fail_unless(facund_response_set_id(resp, "foo") == 0, NULL);
	fail_unless(strcmp(
	    "<response id=\"foo\" code=\"0\" message=\"msg\"></response>",
	    facund_response_string(resp)) == 0, NULL);
	facund_response_free(resp);
}
END_TEST

START_TEST(facund_response_correct_no_id_data)
{
	struct facund_response *resp;
	struct facund_object *obj;

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
	facund_object_set_bool(obj, 0);

	fail_unless((resp = facund_response_new(NULL, RESP_GOOD, "msg", obj))
	    != NULL, NULL);
	fail_unless(strcmp(
	    "<response code=\"0\" message=\"msg\">"
	    "<data type=\"bool\">false</data></response>",
	    facund_response_string(resp)) == 0, NULL);
	facund_response_free(resp);
}
END_TEST

Suite *
facund_response_suite()
{
	Suite *s;
	TCase *tc;

	s = suite_create("facund_response");

	tc = tcase_create("response");
	tcase_add_test(tc, facund_response_null);
	tcase_add_test(tc, facund_response_correct);
	tcase_add_test(tc, facund_response_correct_data);
	tcase_add_test(tc, facund_response_correct_no_id);
	tcase_add_test(tc, facund_response_correct_no_id_data);
	suite_add_tcase(s, tc);

	return s;
}

