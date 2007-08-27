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

#include <string.h>

/*
 * Tests for a boolean facund_object
 */
START_TEST(pkg_freebsd_object_bool_null)
{
	/* Test if bool functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_bool(NULL, 0) == -1, NULL);
	fail_unless(facund_object_set_bool(NULL, 1) == -1, NULL);
	fail_unless(facund_object_get_bool(NULL) == -1, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_create)
{
	struct facund_object *obj;

	/* Test accessing an unassigned object will fail */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
	fail_unless(facund_object_get_type(obj) == FACUND_BOOL, NULL);
	fail_unless(facund_object_get_bool(obj) == -1, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_true)
{
	struct facund_object *obj;

	/* Test accessing an object set to true will succeed */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_bool(obj, 1) == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 1, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">true</data>") == 0, NULL);

	facund_object_free(obj);

	/* Test setting something that is not 1 is also true */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
	fail_unless(facund_object_set_bool(obj, 2) == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 1, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_false)
{
	struct facund_object *obj;

	/* Test accessing an object set to false will succeed */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_bool(obj, 0) == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 0, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">false</data>") == 0, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_true_from_str)
{
	struct facund_object *obj;

	/* Test accessing an object set to false from a string will succeed*/
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "true") == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 1, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">true</data>") == 0, NULL);

	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "TRUE") == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 1, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">true</data>") == 0, NULL);

	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "TrUe") == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 1, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">true</data>") == 0, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_false_from_str)
{
	struct facund_object *obj;

	/* Test accessing an object set to false from a string will succeed*/
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "false") == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 0, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">false</data>") == 0, NULL);

	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "FALSE") == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 0, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">false</data>") == 0, NULL);

	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "FaLsE") == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 0, NULL);
	fail_unless(strcmp(facund_object_xml_string(obj),
	    "<data type=\"bool\">false</data>") == 0, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_error)
{
	struct facund_object *obj;

	/* Test errors are set/reset correctly */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
	fail_unless(facund_object_get_error(obj) == FACUND_OBJECT_ERROR_NONE,
	    NULL);

	/* This should cause an error flag to be set */
	facund_object_get_bool(obj);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);

	/* This should reset the error flag */
	facund_object_set_bool(obj, 0);
	fail_unless(facund_object_get_error(obj) == FACUND_OBJECT_ERROR_NONE,
	    NULL);

	facund_object_free(obj);


	/* These should fail as they are the wrong types */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
		facund_object_set_bool(obj, 0);
		fail_unless(facund_object_get_int(obj) == 0, NULL);
		fail_unless(facund_object_get_error(obj) ==
		    FACUND_OBJECT_ERROR_WRONG_TYPE, NULL);
	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
		facund_object_set_bool(obj, 0);
		fail_unless(facund_object_get_uint(obj) == 0, NULL);
		fail_unless(facund_object_get_error(obj) ==
		    FACUND_OBJECT_ERROR_WRONG_TYPE, NULL);
	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
		facund_object_set_bool(obj, 0);
		fail_unless(facund_object_get_string(obj) == NULL), NULL;
		fail_unless(facund_object_get_error(obj) ==
		    FACUND_OBJECT_ERROR_WRONG_TYPE, NULL);
	facund_object_free(obj);

	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
		facund_object_set_bool(obj, 0);
		fail_unless(facund_object_get_array_item(obj, 0) == NULL, NULL);
		fail_unless(facund_object_get_error(obj) ==
		    FACUND_OBJECT_ERROR_WRONG_TYPE, NULL);
	facund_object_free(obj);
}
END_TEST

/*
 * Tests for an integer facund_object
 */
START_TEST(pkg_freebsd_object_int_null)
{
	/* Test if bool functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_int(NULL, -1) == -1, NULL);
	fail_unless(facund_object_set_int(NULL, 0) == -1, NULL);
	fail_unless(facund_object_set_int(NULL, 1) == -1, NULL);
	fail_unless(facund_object_get_int(NULL) == 0, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_int_create)
{
	struct facund_object *obj;

	/* Tests accessing an unassigned int will fail */
	fail_unless((obj = facund_object_new_int()) != NULL, NULL);
	fail_unless(facund_object_get_error(obj) == FACUND_OBJECT_ERROR_NONE,
	    NULL);
	fail_unless(facund_object_get_type(obj) == FACUND_INT, NULL);
	fail_unless(facund_object_get_int(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	facund_object_free(obj);
}
END_TEST

/*
 * Tests for an unsigned integer facund_object
 */
START_TEST(pkg_freebsd_object_uint_null)
{
	/* Test if bool functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_uint(NULL, 0) == -1, NULL);
	fail_unless(facund_object_set_uint(NULL, 1) == -1, NULL);
	fail_unless(facund_object_get_uint(NULL) == 0, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_uint_create)
{
	struct facund_object *obj;

	/* Test accessing an unassigned unsigned int will fail */
	fail_unless((obj = facund_object_new_uint()) != NULL, NULL);
	fail_unless(facund_object_get_error(obj) == FACUND_OBJECT_ERROR_NONE,
	    NULL);
	fail_unless(facund_object_get_type(obj) == FACUND_UINT, NULL);
	fail_unless(facund_object_get_uint(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	facund_object_free(obj);
}
END_TEST

/*
 * Tests for a string facund_object
 */
START_TEST(pkg_freebsd_object_string_null)
{
	/* Test if bool functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_string(NULL, NULL) == -1, NULL);
	fail_unless(facund_object_set_string(NULL, "string") == -1, NULL);
	fail_unless(facund_object_get_string(NULL) == NULL, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_string_create)
{
	struct facund_object *obj;

	/* Test accessing an unassigned string will fail */
	fail_unless((obj = facund_object_new_string()) != NULL, NULL);
	fail_unless(facund_object_get_error(obj) == FACUND_OBJECT_ERROR_NONE,
	    NULL);
	fail_unless(facund_object_get_type(obj) == FACUND_STRING, NULL);
	fail_unless(facund_object_get_string(obj) == NULL, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	facund_object_free(obj);
}
END_TEST

/*
 * Tests for an array facund_object
 * TODO: Write array tests
 */

Suite *
facund_object_suite()
{
	Suite *s;
	TCase *tc;

	s = suite_create("facund_object");

	tc = tcase_create("boolean");
	tcase_add_test(tc, pkg_freebsd_object_bool_null);
	tcase_add_test(tc, pkg_freebsd_object_bool_create);
	tcase_add_test(tc, pkg_freebsd_object_bool_true);
	tcase_add_test(tc, pkg_freebsd_object_bool_false);
	tcase_add_test(tc, pkg_freebsd_object_bool_true_from_str);
	tcase_add_test(tc, pkg_freebsd_object_bool_false_from_str);
	tcase_add_test(tc, pkg_freebsd_object_bool_error);
	suite_add_tcase(s, tc);

	tc = tcase_create("integer");
	tcase_add_test(tc, pkg_freebsd_object_int_null);
	tcase_add_test(tc, pkg_freebsd_object_int_create);
	suite_add_tcase(s, tc);

	tc = tcase_create("unsigned integer");
	tcase_add_test(tc, pkg_freebsd_object_uint_null);
	tcase_add_test(tc, pkg_freebsd_object_uint_create);
	suite_add_tcase(s, tc);

	tc = tcase_create("string");
	tcase_add_test(tc, pkg_freebsd_object_string_null);
	tcase_add_test(tc, pkg_freebsd_object_string_create);
	suite_add_tcase(s, tc);

	return s;
}

