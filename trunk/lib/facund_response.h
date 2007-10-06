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

#ifndef FACUND_RESPONSE_H
#define FACUND_RESPONSE_H

typedef enum _facund_response_code {
	RESP_GOOD = 0,

	RESP_UNKNOWN_ELEMENT = 100,	/* The element is unknown */
	RESP_WRONG_CHILD_ELEMENT,	/* The element can't be here */

	RESP_UNKNOWN_ATTRIBUTE = 200,
	RESP_NO_ATTRIBUTE,
	RESP_REPEATED_ATTRIBUTE,

	RESP_UNKNOWN_CALL = 300,	/* There is no call with this name */

	/* These are used in the data elements */
	RESP_EMPTY_VALUE = 400,		/* The data had no value */
	RESP_INCORECT_DATA,		/* The data has incorrectly
					 * formatted data */
} facund_response_code;

struct facund_response;
struct facund_object;

struct facund_response	*facund_response_new(const char *, facund_response_code,
			    const char *, struct facund_object *);
const char		*facund_response_string(struct facund_response *);
int			 facund_response_set_id(struct facund_response *,
			    const char *);
void			 facund_response_free(struct facund_response *);

#endif /* FACUND_RESPONSE_H */
