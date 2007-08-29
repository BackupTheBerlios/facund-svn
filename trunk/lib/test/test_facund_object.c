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

#define test_object_create(objtype, typeenum, errvalue) \
do { \
	struct facund_object *theobj; \
	fail_unless((theobj = facund_object_new_##objtype()) != NULL, NULL); \
	fail_unless(facund_object_get_type(theobj) == typeenum, NULL); \
	fail_unless(facund_object_get_##objtype(theobj) == errvalue, NULL); \
	fail_unless(facund_object_xml_string(theobj) == NULL, NULL); \
	facund_object_free(theobj); \
} while(0)

#define test_object_assign(objtype, strtype, value, strvalue) \
do { \
	struct facund_object *theobj; \
	fail_unless((theobj = facund_object_new_##objtype()) != NULL, NULL); \
	fail_unless(facund_object_set_##objtype(theobj, value) == 0, NULL); \
	fail_unless(facund_object_get_##objtype(theobj) == value, NULL); \
	fail_unless(strcmp(facund_object_xml_string(theobj), \
	    "<data type=\""strtype"\">"strvalue"</data>") == 0, NULL); \
	facund_object_free(theobj); \
} while (0)

#define test_object_assign_from_string(objtype, strtype, value, strvalue, \
    expstrvalue) \
do { \
	struct facund_object *theobj; \
	fail_unless((theobj = facund_object_new_##objtype()) != NULL, NULL); \
	fail_unless(facund_object_set_from_str(theobj, strvalue) == 0, NULL); \
	fail_unless(facund_object_get_##objtype(theobj) == value, NULL); \
	fail_unless(strcmp(facund_object_xml_string(theobj), \
	    "<data type=\""strtype"\">"expstrvalue"</data>") == 0, NULL); \
	facund_object_free(theobj); \
} while(0)

#define test_object_error(objtype) \
do { \
	struct facund_object *theobj; \
	fail_unless((theobj = facund_object_new_##objtype()) != NULL, NULL); \
	fail_unless(facund_object_get_error(theobj) == \
	    FACUND_OBJECT_ERROR_NONE, NULL); \
	/* This should cause an error flag to be set */ \
	facund_object_get_##objtype(theobj); \
	fail_unless(facund_object_get_error(theobj) == \
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL); \
	/* This should reset the error flag */ \
	facund_object_set_##objtype(theobj, 0); \
	fail_unless(facund_object_get_error(theobj) == \
	    FACUND_OBJECT_ERROR_NONE, NULL); \
	facund_object_free(theobj); \
} while (0)

#define test_is_not_type(realtype, testtype, value, errvalue) \
do { \
	struct facund_object *newobj; \
	fail_unless((newobj = facund_object_new_##realtype()) != NULL, NULL); \
		facund_object_set_##realtype(newobj, value); \
		fail_unless(facund_object_get_##testtype(newobj) == errvalue, \
		    NULL); \
		fail_unless(facund_object_get_error(newobj) == \
		    FACUND_OBJECT_ERROR_WRONG_TYPE, NULL); \
	facund_object_free(newobj); \
} while(0)

#define test_object_is_not_array(objtype, value) \
do { \
	struct facund_object *theobj; \
	fail_unless((theobj = facund_object_new_##objtype()) != NULL, NULL); \
	facund_object_set_##objtype(theobj, value); \
	fail_unless(facund_object_get_array_item(theobj, 0) == NULL, NULL); \
	fail_unless(facund_object_get_error(theobj) == \
	    FACUND_OBJECT_ERROR_WRONG_TYPE, NULL); \
	facund_object_free(theobj); \
} while (0)

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
	test_object_create(bool, FACUND_BOOL, -1);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_true)
{
	struct facund_object *obj;

	/* Test accessing an object set to true will succeed */
	test_object_assign(bool, "bool", 1, "true");

	/* Test setting something that is not 1 is also true */
	fail_unless((obj = facund_object_new_bool()) != NULL, NULL);
	fail_unless(facund_object_set_bool(obj, 2) == 0, NULL);
	fail_unless(facund_object_get_bool(obj) == 1, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_bool_false)
{
	/* Test accessing an object set to false will succeed */
	test_object_assign(bool, "bool", 0, "false");
}
END_TEST

START_TEST(pkg_freebsd_object_bool_true_from_str)
{
	/* Test accessing an object set to false from a string will succeed*/
	test_object_assign_from_string(bool, "bool", 1, "true", "true");
	test_object_assign_from_string(bool, "bool", 1, "TRUE", "true");
	test_object_assign_from_string(bool, "bool", 1, "TrUe", "true");
}
END_TEST

START_TEST(pkg_freebsd_object_bool_false_from_str)
{
	/* Test accessing an object set to false from a string will succeed*/
	test_object_assign_from_string(bool, "bool", 0, "false", "false");
	test_object_assign_from_string(bool, "bool", 0, "FALSE", "false");
	test_object_assign_from_string(bool, "bool", 0, "FaLsE", "false");
	test_object_assign_from_string(bool, "bool", 0, "string", "false");
}
END_TEST

START_TEST(pkg_freebsd_object_bool_error)
{
	/* Test errors are set/reset correctly */
	test_object_error(bool);

	/* Test accessing with the wrong accessor fails */
	test_is_not_type(bool, int, 0, 0);
	test_is_not_type(bool, uint, 0, 0);
	test_is_not_type(bool, string, 0, 0);

	test_object_is_not_array(bool, 0);
}
END_TEST

/*
 * Tests for an integer facund_object
 */
START_TEST(pkg_freebsd_object_int_null)
{
	/* Test if int functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_int(NULL, -1) == -1, NULL);
	fail_unless(facund_object_set_int(NULL, 0) == -1, NULL);
	fail_unless(facund_object_set_int(NULL, 1) == -1, NULL);
	fail_unless(facund_object_get_int(NULL) == 0, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_int_create)
{
	/* Tests accessing an unassigned int will fail */
	test_object_create(int, FACUND_INT, 0);
}
END_TEST

START_TEST(pkg_freebsd_object_int_zero)
{
	/* Test accessing an object set to 0 will succeed */
	test_object_assign(int, "int", 0, "0");
}
END_TEST

START_TEST(pkg_freebsd_object_int_zero_from_str)
{
	/* Test accessing an object set to 0 from a string will succeed */
	test_object_assign_from_string(int, "int", 0, "0", "0");
}
END_TEST

START_TEST(pkg_freebsd_object_int_min)
{
	/* Test accessing an object set to -2147483648 will succeed */
	test_object_assign(int, "int", INT32_MIN, "-2147483648");
}
END_TEST

START_TEST(pkg_freebsd_object_int_min_from_str)
{
	test_object_assign_from_string(int, "int", INT32_MIN, "-2147483648",
	    "-2147483648");
}
END_TEST

START_TEST(pkg_freebsd_object_int_bad_min_from_str)
{
	struct facund_object *obj;

	/* Test setting an object to a number too small will fail */
	fail_unless((obj = facund_object_new_int()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "-2147483649") == -1, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_BADSTRING, NULL);
	fail_unless(facund_object_get_int(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_int_max)
{
	/* Test accessing an object set to 2147483647 will succeed */
	test_object_assign(int, "int", INT32_MAX, "2147483647");
}
END_TEST

START_TEST(pkg_freebsd_object_int_max_from_str)
{
	test_object_assign_from_string(int, "int", INT32_MAX, "2147483647",
	    "2147483647");
}
END_TEST

START_TEST(pkg_freebsd_object_int_bad_max_from_str)
{
	struct facund_object *obj;

	/* Test setting an object to a number too large will fail */
	fail_unless((obj = facund_object_new_int()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "2147483648") == -1, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_BADSTRING, NULL);
	fail_unless(facund_object_get_int(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_int_bad_from_str)
{
	struct facund_object *obj;

	/* Test setting an object to a bas string will fail */
	fail_unless((obj = facund_object_new_int()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "1 f1234") == -1, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_BADSTRING, NULL);
	fail_unless(facund_object_get_int(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_int_error)
{
	/* Test errors are set/reset correctly */
	test_object_error(int);

	/* Test accessing with the wrong accessor fails */
	test_is_not_type(int, bool, 0, -1);
	test_is_not_type(int, uint, 0, 0);
	test_is_not_type(int, string, 0, 0);

	test_object_is_not_array(int, 0);
}
END_TEST

/*
 * Tests for an unsigned integer facund_object
 */
START_TEST(pkg_freebsd_object_uint_null)
{
	/* Test if uint functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_uint(NULL, 0) == -1, NULL);
	fail_unless(facund_object_set_uint(NULL, 1) == -1, NULL);
	fail_unless(facund_object_get_uint(NULL) == 0, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_uint_create)
{
	/* Test accessing an unassigned unsigned int will fail */
	test_object_create(uint, FACUND_UINT, 0);
}
END_TEST

START_TEST(pkg_freebsd_object_uint_min)
{
	/* Test accessing an object set to 0 will succeed */
	test_object_assign(uint, "unsigned int", 0, "0");
}
END_TEST

START_TEST(pkg_freebsd_object_uint_min_from_str)
{
	test_object_assign_from_string(uint, "unsigned int", 0, "0", "0");
}
END_TEST

START_TEST(pkg_freebsd_object_uint_bad_min_from_str)
{
	struct facund_object *obj;

	/* Test setting an object to a number too small will fail */
	fail_unless((obj = facund_object_new_uint()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "-1") == -1, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_BADSTRING, NULL);
	fail_unless(facund_object_get_uint(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_uint_max)
{
	/* Test accessing an object set to 4294967295 will succeed */
	test_object_assign(uint, "unsigned int", UINT32_MAX, "4294967295");
}
END_TEST

START_TEST(pkg_freebsd_object_uint_max_from_str)
{
	test_object_assign_from_string(uint, "unsigned int", UINT32_MAX,
	    "4294967295", "4294967295");
}
END_TEST

START_TEST(pkg_freebsd_object_uint_bad_max_from_str)
{
	struct facund_object *obj;

	/* Test setting an object to a number too large will fail */
	fail_unless((obj = facund_object_new_uint()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "4294967296") == -1, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_BADSTRING, NULL);
	fail_unless(facund_object_get_uint(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_uint_bad_from_str)
{
	struct facund_object *obj;

	/* Test setting an object to a bad number will fail */
	fail_unless((obj = facund_object_new_uint()) != NULL, NULL);

	fail_unless(facund_object_set_from_str(obj, "1 f1234") == -1, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_BADSTRING, NULL);
	fail_unless(facund_object_get_uint(obj) == 0, NULL);
	fail_unless(facund_object_get_error(obj) ==
	    FACUND_OBJECT_ERROR_UNASSIGNED, NULL);
	fail_unless(facund_object_xml_string(obj) == NULL, NULL);

	facund_object_free(obj);
}
END_TEST

START_TEST(pkg_freebsd_object_uint_error)
{
	/* Test errors are set/reset correctly */
	test_object_error(uint);

	/* Test accessing an object with the wrong accessor fails */
	test_is_not_type(uint, bool, 0, -1);
	test_is_not_type(uint, int, 0, 0);
	test_is_not_type(uint, string, 0, 0);
	test_object_is_not_array(uint, 0);
}
END_TEST

/*
 * Tests for a string facund_object
 */
START_TEST(pkg_freebsd_object_string_null)
{
	/* Test if string functions will fail correctly when passes NULL */
	fail_unless(facund_object_set_string(NULL, NULL) == -1, NULL);
	fail_unless(facund_object_set_string(NULL, "string") == -1, NULL);
	fail_unless(facund_object_get_string(NULL) == NULL, NULL);
}
END_TEST

START_TEST(pkg_freebsd_object_string_create)
{
	/* Test accessing an unassigned string will fail */
	test_object_create(string, FACUND_STRING, 0);
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
	tcase_add_test(tc, pkg_freebsd_object_int_zero);
	tcase_add_test(tc, pkg_freebsd_object_int_zero_from_str);
	tcase_add_test(tc, pkg_freebsd_object_int_min);
	tcase_add_test(tc, pkg_freebsd_object_int_min_from_str);
	tcase_add_test(tc, pkg_freebsd_object_int_bad_min_from_str);
	tcase_add_test(tc, pkg_freebsd_object_int_max);
	tcase_add_test(tc, pkg_freebsd_object_int_max_from_str);
	tcase_add_test(tc, pkg_freebsd_object_int_bad_max_from_str);
	tcase_add_test(tc, pkg_freebsd_object_int_bad_from_str);
	tcase_add_test(tc, pkg_freebsd_object_int_error);
	suite_add_tcase(s, tc);

	tc = tcase_create("unsigned integer");
	tcase_add_test(tc, pkg_freebsd_object_uint_null);
	tcase_add_test(tc, pkg_freebsd_object_uint_create);
	tcase_add_test(tc, pkg_freebsd_object_uint_min);
	tcase_add_test(tc, pkg_freebsd_object_uint_min_from_str);
	tcase_add_test(tc, pkg_freebsd_object_uint_bad_min_from_str);
	tcase_add_test(tc, pkg_freebsd_object_uint_max);
	tcase_add_test(tc, pkg_freebsd_object_uint_max_from_str);
	tcase_add_test(tc, pkg_freebsd_object_uint_bad_max_from_str);
	tcase_add_test(tc, pkg_freebsd_object_uint_bad_from_str);
	tcase_add_test(tc, pkg_freebsd_object_uint_error);
	suite_add_tcase(s, tc);

	tc = tcase_create("string");
	tcase_add_test(tc, pkg_freebsd_object_string_null);
	tcase_add_test(tc, pkg_freebsd_object_string_create);
	suite_add_tcase(s, tc);

	return s;
}

