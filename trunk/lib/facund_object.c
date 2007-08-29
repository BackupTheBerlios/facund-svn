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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "facund_object.h"
#include "facund_private.h"

static const char *obj_names[] = { "bool", "int", "unsigned int", "string",
    "array" };
static struct facund_object	*facund_object_new(void);

/* Internal function to create an empty facund_object */
static struct facund_object *
facund_object_new()
{
	struct facund_object *obj;

	obj = calloc(1, sizeof(struct facund_object));
	return obj;
}

/*
 * These are the creator, setter and getter.
 * They are all named facund_object_{new,set,get}_<type>
 */
struct facund_object *
facund_object_new_bool()
{
	struct facund_object *obj;
	
	obj = facund_object_new();
	if (obj == NULL)
		return NULL;

	obj->obj_type = FACUND_BOOL;
	return obj;
}

int
facund_object_set_bool(struct facund_object *obj, int value)
{
	if (obj == NULL) {
		return -1;
	} else if (obj->obj_type != FACUND_BOOL) {
		obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return -1;
	}

	obj->obj_int = (value ? 1 : 0);
	obj->obj_assigned = 1;
	obj->obj_error = FACUND_OBJECT_ERROR_NONE;

	return 0;
}

int
facund_object_get_bool(const struct facund_object *obj)
{
	struct facund_object *real_obj;

	if (obj == NULL) {
		return -1;
	}

	/* Get an object we can edit */
	real_obj =  __DECONST(struct facund_object *, obj);

	if (obj->obj_type != FACUND_BOOL) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return -1;
	}

	if (obj->obj_assigned == 0) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_UNASSIGNED;
		return -1;
	}
	assert(obj->obj_assigned == 1);

	real_obj->obj_error = FACUND_OBJECT_ERROR_NONE;
	return obj->obj_int;
}

struct facund_object *
facund_object_new_int()
{
	struct facund_object *obj;
	
	obj = facund_object_new();
	if (obj == NULL)
		return NULL;

	obj->obj_type = FACUND_INT;
	return obj;
}

int
facund_object_set_int(struct facund_object *obj, int32_t value)
{
	if (obj == NULL) {
		return -1;
	} else if (obj->obj_type != FACUND_INT) {
		obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return -1;
	}

	obj->obj_int = value;
	obj->obj_assigned = 1;
	obj->obj_error = FACUND_OBJECT_ERROR_NONE;

	return 0;
}

int32_t
facund_object_get_int(const struct facund_object *obj)
{
	struct facund_object *real_obj;

	if (obj == NULL) {
		return 0;
	}

	/* Get an object we can edit */
	real_obj =  __DECONST(struct facund_object *, obj);

	if (obj->obj_type != FACUND_INT) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return 0;
	}

	if (obj->obj_assigned == 0) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_UNASSIGNED;
		return 0;
	}
	assert(obj->obj_assigned == 1);

	real_obj->obj_error = FACUND_OBJECT_ERROR_NONE;
	return obj->obj_int;
}

struct facund_object *
facund_object_new_uint()
{
	struct facund_object *obj;
	
	obj = facund_object_new();
	if (obj == NULL)
		return NULL;

	obj->obj_type = FACUND_UINT;
	return obj;
}

int
facund_object_set_uint(struct facund_object *obj, uint32_t value)
{
	if (obj == NULL) {
		return -1;
	} else if (obj->obj_type != FACUND_UINT) {
		obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return -1;
	}

	obj->obj_int = (int32_t)value;
	obj->obj_assigned = 1;
	obj->obj_error = FACUND_OBJECT_ERROR_NONE;

	return 0;
}

uint32_t
facund_object_get_uint(const struct facund_object *obj)
{
	struct facund_object *real_obj;

	if (obj == NULL) {
		return 0;
	}

	/* Get an object we can edit */
	real_obj =  __DECONST(struct facund_object *, obj);

	if (obj->obj_type != FACUND_UINT) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return 0;
	}

	if (obj->obj_assigned == 0) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_UNASSIGNED;
		return 0;
	}
	assert(obj->obj_assigned == 1);

	real_obj =  __DECONST(struct facund_object *, obj);
	real_obj->obj_error = FACUND_OBJECT_ERROR_NONE;
	return (uint32_t)obj->obj_int;
}

struct facund_object *
facund_object_new_string()
{
	struct facund_object *obj;
	
	obj = facund_object_new();
	if (obj == NULL)
		return NULL;

	obj->obj_type = FACUND_STRING;
	return obj;
}

int
facund_object_set_string(struct facund_object *obj, const char *value)
{
	if (obj == NULL) {
		return -1;
	} else if (obj->obj_type != FACUND_STRING) {
		obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return -1;
	}

	if (obj->obj_string == NULL) {
		obj->obj_string = strdup(value);
	} else {
		char *tmp;
		/* TODO: Use realloc */
		tmp = strdup(value);
		if (tmp == NULL) {
			/* TODO: Error handeling */
			return -1;
		}
		free(obj->obj_string);
		obj->obj_string = tmp;
	}
	obj->obj_assigned = 1;
	obj->obj_error = FACUND_OBJECT_ERROR_NONE;

	return 0;
}

const char *
facund_object_get_string(const struct facund_object *obj)
{
	struct facund_object *real_obj;

	if (obj == NULL) {
		return NULL;
	}

	/* Get an object we can edit */
	real_obj =  __DECONST(struct facund_object *, obj);

	if (obj->obj_type != FACUND_STRING) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return NULL;
	}

	if (obj->obj_assigned == 0) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_UNASSIGNED;
		return NULL;
	}
	assert(obj->obj_assigned == 1);

	real_obj->obj_error = FACUND_OBJECT_ERROR_NONE;

	return obj->obj_string;
}

struct facund_object *
facund_object_new_array()
{
	struct facund_object *obj;
	
	obj = facund_object_new();
	if (obj == NULL)
		return NULL;

	obj->obj_type = FACUND_ARRAY;
	return obj;
}

int
facund_object_array_append(struct facund_object *obj,
    struct facund_object *item)
{
	struct facund_object **new;

	if (obj == NULL) {
		return -1;
	} else if (obj->obj_type != FACUND_ARRAY) {
		obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return -1;
	}

	/* Create the array or increase it's size */
	if (obj->obj_array_count == 0) {
		new = calloc(1, sizeof(struct facund_object *));
	} else {
		new = realloc(obj->obj_array,
		   sizeof(struct facund_object *) * (obj->obj_array_count + 1));
	}
	if (new == NULL) {
		/* TODO: error */
		return -1;
	}
	/* Add the item to the array */
	new[obj->obj_array_count] = item;
	obj->obj_array_count++;
	obj->obj_array = new;

	item->obj_parent = obj;

	obj->obj_assigned = 1;
	obj->obj_error = FACUND_OBJECT_ERROR_NONE;

	return 0;
}

const struct facund_object *
facund_object_get_array_item(const struct facund_object *obj, unsigned int pos)
{
	struct facund_object *real_obj;

	if (obj == NULL) {
		return NULL;
	}

	/* Get an object we can edit */
	real_obj =  __DECONST(struct facund_object *, obj);

	if (obj->obj_type != FACUND_ARRAY) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_WRONG_TYPE;
		return NULL;
	}

	if (obj->obj_assigned == 0) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_UNASSIGNED;
		return NULL;
	}
	assert(obj->obj_assigned == 1);

	if (pos >= obj->obj_array_count) {
		real_obj->obj_error = FACUND_OBJECT_ERROR_NO_OBJECT;
		return NULL;
	}

	real_obj->obj_error = FACUND_OBJECT_ERROR_NONE;
	return obj->obj_array[pos];
}

size_t
facund_object_array_size(const struct facund_object *obj)
{
	if (obj == NULL)
		return 0;

	if (obj->obj_type != FACUND_ARRAY)
		return 0;

	return obj->obj_array_count;
}

/*
 * Free an object and it's children
 */
void
facund_object_free(struct facund_object *obj)
{
	if (obj == NULL)
		return;

	if (obj->obj_string != NULL)
		free(obj->obj_string);

	if (obj->obj_array != NULL) {
		unsigned int i;

		for (i = 0; i < obj->obj_array_count; i++) {
			facund_object_free(obj->obj_array[i]);
		}
		free(obj->obj_array);
	}

	if (obj->obj_xml_string != NULL)
		free(obj->obj_xml_string);

	free(obj);
}

struct facund_object *
facund_object_new_from_typestr(const char *type)
{
	if (strcmp(type, "bool") == 0) {
		return facund_object_new_bool();
	} else if (strcmp(type, "int") == 0) {
		return facund_object_new_int();
	} else if (strcmp(type, "unsigned int") == 0) {
		return facund_object_new_uint();
	} else if (strcmp(type, "string") == 0) {
		return facund_object_new_string();
	} else if (strcmp(type, "array") == 0) {
		return facund_object_new_array();
	}
	return NULL;
}

int
facund_object_set_from_str(struct facund_object *obj, const char *value)
{
	if (obj == NULL) {
		return -1;
	}
	if (value == NULL) {
		obj->obj_error = FACUND_OBJECT_ERROR_BADSTRING;
		return -1;
	}

	switch(obj->obj_type) {
	case FACUND_BOOL:
		return facund_object_set_bool(obj,
		    strcasecmp(value, "true") == 0);
	case FACUND_INT: {
		int32_t data;
		const char *errstr;

		data = strtonum(value, INT32_MIN, INT32_MAX, &errstr);
		if (errstr != NULL) {
			obj->obj_error = FACUND_OBJECT_ERROR_BADSTRING;
			return -1;
		}
		return facund_object_set_int(obj, data);
	}
	case FACUND_UINT: {
		uint32_t data;
		const char *errstr;

		data = strtonum(value, 0, UINT32_MAX, &errstr);
		if (errstr != NULL) {
			obj->obj_error = FACUND_OBJECT_ERROR_BADSTRING;
			return -1;
		}
		return facund_object_set_uint(obj, data);
	}
	case FACUND_STRING:
		return facund_object_set_string(obj, value);
	case FACUND_ARRAY:
		return -1;
	}

	/* Should never reach here */
	assert(0);
	return -1;
}

enum facund_object_error
facund_object_get_error(const struct facund_object *obj)
{
	if (obj == NULL) {
		return FACUND_OBJECT_ERROR_NO_OBJECT;
	}
	return obj->obj_error;
}

enum facund_type
facund_object_get_type(const struct facund_object *obj)
{
	return obj->obj_type;
}

const char *
facund_object_xml_string(struct facund_object *obj __unused)
{
	if (obj == NULL || obj->obj_assigned == 0)
		return NULL;

	if (obj->obj_xml_string == NULL) {
		char *data;

		assert(obj->obj_assigned == 1);

		data = NULL;
		switch (obj->obj_type) {
		case FACUND_BOOL:
			asprintf(&data, "%s",
			    (facund_object_get_bool(obj) ? "true" : "false"));
			break;
		case FACUND_INT:
			asprintf(&data, "%d", facund_object_get_int(obj));
			break;
		case FACUND_UINT:
			asprintf(&data, "%u", facund_object_get_uint(obj));
			break;
		case FACUND_STRING:
			data = strdup(facund_object_get_string(obj));
			break;
		case FACUND_ARRAY: {
			size_t pos;

			for (pos = 0; pos < obj->obj_array_count; pos++) {
				struct facund_object *curobj;
				const char *tmpdata;
				char *olddata;

				curobj = __DECONST(struct facund_object *,
				    facund_object_get_array_item(obj, pos));
				assert(curobj->obj_assigned == 1);
				tmpdata = facund_object_xml_string(curobj);

				/* Append the new data to the end of the data */
				olddata = data;
				if (data == NULL) {
					asprintf(&data, "%s", tmpdata);
				} else {
					asprintf(&data, "%s%s", data, tmpdata);
				}
				free(olddata);
			}

			break;
		}
		}
		if (data != NULL) {
			asprintf(&obj->obj_xml_string,
			    "<data type=\"%s\">%s</data>",
			    obj_names[obj->obj_type], data);
			free(data);
		}
	}

	return obj->obj_xml_string;
}

/*
 * Debugging function to print the type and contents of an object
 * If the object is an array it will also recurse into the array
 */
void
facund_object_print(struct facund_object *obj)
{
	unsigned int depth;

	if (obj == NULL) {
		printf("No object\n");
		return;
	}

	for (depth = 0; depth < obj->obj_depth; depth++) {
		putchar(' ');
	}

	if (obj->obj_assigned == 1) {
		switch (obj->obj_type) {
		case FACUND_BOOL:
			printf("%s",
			    (facund_object_get_bool(obj) ? "true" : "false"));
			break;
		case FACUND_INT:
			printf("%d", facund_object_get_int(obj));
			break;
		case FACUND_UINT:
			printf("%u", facund_object_get_uint(obj));
			break;
		case FACUND_STRING:
			printf("%s", facund_object_get_string(obj));
			break;
		case FACUND_ARRAY: {
			size_t pos;

			printf("{\n");
			for (pos = 0; pos < obj->obj_array_count; pos++) {
				struct facund_object *curobj;

				curobj = __DECONST(struct facund_object *,
				    facund_object_get_array_item(obj, pos));
				curobj->obj_depth = obj->obj_depth + 1;
				facund_object_print(curobj);
				curobj->obj_depth = 0;
			}
			printf("}");
			break;
		}
		}
	} else {
		assert(obj->obj_assigned == 0);
		printf("Unassigned");
	}

	printf(" (%s)\n", obj_names[obj->obj_type]);
}
