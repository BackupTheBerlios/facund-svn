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

#include "facund_response.h"
#include "facund_connection.h"
#include "facund_object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct facund_response {
	char	*resp_id;	/* The id of the call */
	facund_response_code resp_code;	/* The return code of the response.
				 * 0=good, not 0=bad */
	char	*resp_msg;	/* The error message or NULL
				 * for the default */
	struct facund_object *resp_return; /* The return data or NULL */

	char	*resp_string;
};

/*
 * Creates a new response object to send to the client
 */
struct facund_response *
facund_response_new(const char *id, facund_response_code code,
    const char *message, struct facund_object *obj)
{
	struct facund_response *resp;

	if (message == NULL) {
		return NULL;
	}

	resp = calloc(1, sizeof(struct facund_response));
	if (resp == NULL)
		return NULL;

	if (id != NULL) {
		resp->resp_id = strdup(id);
	}
	resp->resp_code = code;
	resp->resp_msg = strdup(message);
	resp->resp_return = obj;
	if ((id != NULL && resp->resp_id == NULL) || resp->resp_msg == NULL) {
		facund_response_free(resp);
		return NULL;
	}

	return resp;
}

/*
 * Gets a string to send to the client
 */
const char *
facund_response_string(struct facund_response *resp)
{
	if (resp == NULL)
		return NULL;

	if (resp->resp_string == NULL) {
		const char *data;
		char *str;

		data = facund_object_xml_string(resp->resp_return);
		asprintf(&resp->resp_string, "<response");
		
		if (resp->resp_id != NULL) {
			str = resp->resp_string;
			asprintf(&resp->resp_string, "%s id=\"%s\"", str,
			    resp->resp_id);
			free(str);
		}
		str = resp->resp_string;
		asprintf(&resp->resp_string, "%s code=\"%d\" message=\"%s\">"
		    "%s</response>", str, resp->resp_code, resp->resp_msg,
		    (data != NULL ? data :""));
		free(str);
	}
	return resp->resp_string;
}

/*
 * Free's a response after use
 */
void
facund_response_free(struct facund_response *resp)
{
	if (resp == NULL)
		return;

	if (resp->resp_id != NULL)
		free(resp->resp_id);

	if (resp->resp_msg != NULL)
		free(resp->resp_msg);

	if (resp->resp_return != NULL)
		facund_object_free(resp->resp_return);

	if (resp->resp_string != NULL)
		free(resp->resp_string);

	free(resp);
}
