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

#ifndef FACUND_TYPE_H
#define FACUND_TYPE_H

#include <inttypes.h>

enum facund_type {
	FACUND_BOOL,
	FACUND_INT,
	FACUND_UINT,
	FACUND_STRING,
	FACUND_ARRAY,
};

enum facund_object_error {
	FACUND_OBJECT_ERROR_NONE,
	FACUND_OBJECT_ERROR_NO_OBJECT,
	FACUND_OBJECT_ERROR_UNASSIGNED,
	FACUND_OBJECT_ERROR_WRONG_TYPE,
	FACUND_OBJECT_ERROR_BADSTRING,
};

struct facund_object;

struct facund_object	*facund_object_new_bool(void);
int			 facund_object_set_bool(struct facund_object *, int);
int			 facund_object_get_bool(const struct facund_object *);

struct facund_object	*facund_object_new_int(void);
int			 facund_object_set_int(struct facund_object *, int32_t);
int32_t			 facund_object_get_int(const struct facund_object *);

struct facund_object	*facund_object_new_uint(void);
int			 facund_object_set_uint(struct facund_object *,
			    uint32_t);
uint32_t		 facund_object_get_uint(const struct facund_object *);

struct facund_object	*facund_object_new_string(void);
int			 facund_object_set_string(struct facund_object *,
			    const char *);
const char		*facund_object_get_string(const struct facund_object *);

struct facund_object	*facund_object_new_array(void);
int			 facund_object_array_append(struct facund_object *,
			    struct facund_object *);
const struct facund_object *facund_object_get_array_item(
			    const struct facund_object *, unsigned int);
size_t			 facund_object_array_size(const struct facund_object *);

void			 facund_object_free(struct facund_object *);

struct facund_object	*facund_object_new_from_typestr(const char *);
int			 facund_object_set_from_str(struct facund_object *,
			    const char *);

enum facund_object_error facund_object_get_error(const struct facund_object*);
enum facund_type	 facund_object_get_type(const struct facund_object *);
int			 facund_object_is_assigned(const struct facund_object*);
const char		*facund_object_xml_string(struct facund_object *);
void			 facund_object_print(struct facund_object *);

#endif /*FACUND_TYPE_H */
