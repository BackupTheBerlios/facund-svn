/*-
 * Copyright (c) 2007 Andrew Turner
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

#include <sys/types.h>

#include <assert.h>
#include <db.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "facund_connection.h"
#include "facund_object.h"
#include "facund_response.h"
#include "facund_private.h"

#define BUF_SIZE	128

static struct facund_response *facund_server_call(const char *, const char *,
    struct facund_object *);
static void facund_server_start_tag(void *, const XML_Char *, const XML_Char**);
static void facund_server_end_tag(void *, const XML_Char *);
static void facund_server_text(void *, const XML_Char *, int);

static DB *call_db = NULL;

/*
 * Waits for a client to connect and send the start message
 * next it replys with the server start and returns
 */
int
facund_server_start(struct facund_conn *conn, long salt)
{
	char *str;

	conn->close = 0;
	conn->parser = XML_ParserCreate(NULL);
	if (facund_accept(conn) == -1) {
		XML_ParserFree(conn->parser);
		conn->parser = NULL;
		return -1;
	}

	XML_SetUserData(conn->parser, conn);
	XML_SetElementHandler(conn->parser, facund_server_start_tag,
	    facund_server_end_tag);
	XML_SetCharacterDataHandler(conn->parser, facund_server_text);

	if (salt == 0) {
		str = strdup("<facund-server version=\"0\">");
	} else {
		asprintf(&str, "<facund-server version=\"0\" salt=\"%ld\">",
		    salt);
	}
	if (str == NULL) {
		return -1;
	}
	facund_send(conn, str, strlen(str));
	free(str);

	return 0;
}

/*
 * Reads data from a connection and parses it returning any data requested
 * It should be called from a loop waiting for it to return a done message
 * Returns: -1 on error, 0 on success, 1 on close stream
 */
int
facund_server_get_request(struct facund_conn *conn)
{
	char *buf;
	ssize_t len;

	if (conn->close == 1) {
		return 1;
	}

	buf = XML_GetBuffer(conn->parser, BUF_SIZE);
	if (buf == NULL)
		return -1;

	len = facund_recv(conn, buf, BUF_SIZE);
	if (len == -1)
		return -1;

	XML_ParseBuffer(conn->parser, len, len == 0);

	return (len == 0 ? 1 : 0);
}

int
facund_server_finish(struct facund_conn *conn)
{
	const char *str;

	str = "</facund-server>";
	facund_send(conn, str, strlen(str));

	if (conn->parser != NULL)
		XML_ParserFree(conn->parser);

	facund_close(conn);

	return 0;
}

/*
 * Calls the correct function for the given call
 */
static struct facund_response *
facund_server_call(const char *name, const char *id, struct facund_object *arg)
{
	struct facund_response *resp;
	facund_call_cb *cb;
	DBT key, data;
	int ret;

	resp = NULL;

	/* Find the callback and execute it if it exists */
	key.data = __DECONST(void *, name);
	key.size = (strlen(name) + 1) * sizeof(char);
	ret = call_db->get(call_db, &key, &data, 0);
	if (ret == 0) {
		/* Get the callback and execute it */
		cb = *(facund_call_cb **)data.data;
		assert(cb != NULL);
		resp = cb(id, arg);
		if (resp == NULL) {
			/* TODO: Remove Magic Number */
			resp = facund_response_new(id, 1,
			    "Method returned an invalid response", NULL);
		}
	} else {
		/* TODO: Remove Magic Number */
		resp = facund_response_new(id, 1, "Invalid request", NULL);
	}

	return resp;
}

/*
 * Registers a callback to the system for when a remote call is made
 */
int
facund_server_add_call(const char *call, facund_call_cb *cb)
{
	DBT key, data;
	int ret;

	if (call == NULL || cb == NULL)
		return -1;

	if (call_db == NULL) {
		call_db = dbopen(NULL, O_CREAT|O_RDWR, 0, DB_HASH, NULL);
	}

	/* Store a pointer to the callback for this function */
	key.data = __DECONST(void *, call);
	key.size = (strlen(call) + 1) * sizeof(char);
	data.data = &cb;
	data.size = sizeof(facund_call_cb *);
	ret = call_db->put(call_db, &key, &data, R_NOOVERWRITE);

	return ((ret == 0) ? 0 : -1);
}

static void
facund_server_start_tag(void *data, const XML_Char *name,
    const XML_Char **attrs)
{
	struct facund_conn *conn;
	char *str;

	conn = data;

	if (conn->current_call[0] == '\0' && strcmp(name, "call") == 0) {
		unsigned int i;
		const char *call_name, *id;

		assert(conn->resp == NULL);

		if (attrs == NULL) {
			conn->resp = facund_response_new(NULL,
			    RESP_NO_ATTRIBUTE,
			    "No call attributes were sent", NULL);
			return;
		}
		call_name = id = NULL;
		for (i = 0; attrs[i] != NULL && attrs[i+1] != NULL; i += 2) {
			if (strcmp(attrs[i], "name") == 0) {
				if (call_name != NULL && conn->resp == NULL) {
					conn->resp = facund_response_new(NULL,
					    RESP_REPEATED_ATTRIBUTE,
					    "Call name was set multiple times",
					    NULL);
				}
				call_name = attrs[i + 1];
			} else if (strcmp(attrs[i], "id") == 0) {
				if (id != NULL && conn->resp == NULL) {
					/* TODO: Don't use a magic number */
					conn->resp = facund_response_new(NULL,
					    RESP_REPEATED_ATTRIBUTE,
					    "Call ID was set multiple times",
					    NULL);
				}
				id = attrs[i + 1];
			} else if (conn->resp == NULL) {
				/*
				 * This is entered when there is
				 * an unknown attribute sent and
				 * no other errors have occured.
				 */
				conn->resp = facund_response_new(NULL,
				    RESP_UNKNOWN_ATTRIBUTE,
				    "Unknown attribute was sent", NULL);
			}
		}
		strlcpy(conn->current_call, call_name,
		    sizeof(conn->current_call));
		strlcpy(conn->call_id, id, sizeof(conn->call_id));

		/* Attempt to set the ID if it can be */
		facund_response_set_id(conn->resp, id);
	} else if (strcmp(name, "data") == 0) {
		struct facund_object *obj;

		if (attrs == NULL) {
			conn->resp = facund_response_new(NULL,
			    RESP_NO_ATTRIBUTE,
			    "No data attributes were sent", NULL);
			return;
		}
		obj = NULL;

		if (strcmp(attrs[0], "type") == 0 && attrs[1] != NULL &&
		    attrs[2] == NULL) {
			obj = facund_object_new_from_typestr(attrs[1]);
		}
		
		if (obj == NULL) {
			/* TODO: Return an error */
			return;
		}

		if (conn->call_arg != NULL) {
			facund_object_array_append(conn->call_arg, obj);
		}
		conn->call_arg = obj;
	} else if (strcmp(name, "facund-client") == 0) {
		/* Pass */
	} else {
		asprintf(&str, "<unknown element=\"%s\"/>", name);
		if (str == NULL)
			return;
		facund_send(conn, str, strlen(str));
		free(str);
	}
}

static void
facund_server_end_tag(void *data, const XML_Char *name)
{
	struct facund_conn *conn;

	conn = data;

	if (strcmp(name, "call") == 0) {
		const char *msg;
		struct facund_response *resp;

		if (conn->resp != NULL) {
			resp = conn->resp;
			conn->resp = NULL;
		} else {
			resp = facund_server_call(conn->current_call,
			    conn->call_id, conn->call_arg);
		}

		/* Get the response string and send it to the client */
		msg = facund_response_string(resp);
		if (msg != NULL) {
			facund_send(conn, msg, strlen(msg));
		}
		facund_response_free(resp);

		conn->current_call[0] = '\0';
		conn->call_id[0] = '\0';
		facund_object_free(conn->call_arg);
		conn->call_arg = NULL;
	} else if (strcmp(name, "data") == 0) {
		/*
		 *  Set the argument to the item's parent
		 *  if it has one. ie. it's in an array
		 */
		if (conn->call_arg->obj_parent != NULL) {
			conn->call_arg = conn->call_arg->obj_parent;
		}
	} else if (strcmp(name, "facund-client") == 0) {
		const char *msg = "</facund-server>";
		facund_send(conn, msg, strlen(msg));
		conn->close = 1;
	}
}

static void
facund_server_text(void *data, const XML_Char *str, int len)
{
	struct facund_conn *conn;
	char *text;

	conn = (struct facund_conn *)data;
	if (conn->call_arg == NULL) {
		return;
	} else if (conn->call_arg->obj_assigned == 1) {
		/* TODO: Return an error */
		return;
	} else if (conn->call_arg->obj_type == FACUND_ARRAY) {
		/* Arrays must not have any text within them */
		/* TODO: Return an error */
		return;
	}

	text = malloc((len + 1) * sizeof(char));
	if (text == NULL) {
		/* TODO: Return an error */
		return;
	}
	strlcpy(text, str, len + 1);
	facund_object_set_from_str(conn->call_arg, text);

	free(text);
}
