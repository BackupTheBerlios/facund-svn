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

#ifndef FACUND_PRIVATE_H
#define FACUND_PRIVATE_H

#include "facund_object.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <bsdxml.h>

struct facund_object {
	enum facund_type obj_type;
	int		 obj_assigned;
	enum facund_object_error obj_error;

	/* Used when this is part of an array */
	struct facund_object *obj_parent;

	int32_t		 obj_int;	/* Used in bool, int and uint types */
	char		*obj_string;	/* Used in string type */
	struct facund_object **obj_array;/* Used in array type */
	size_t		 obj_array_count;

	char		*obj_xml_string;

	/* Used for indenting arrays */
	unsigned int	 obj_depth;
};

struct facund_conn {
	int		 do_unlink;	/* If true unlink the socket on close */
	struct sockaddr_un local;	/* The local connection */
	struct sockaddr_un remote;	/* The remote connection */
	socklen_t	 sock_len;	/* sizeof(remote) */
	int		 sock;		/* The socket fd */
	int		 fd;		/* The fd to the remote device */
	int		 close;		/* True when we should
					 * close the connection */

	XML_Parser	 parser;
	char		 current_call[32];
	char		 call_id[8];
	struct facund_object *call_arg;
	struct facund_response *resp;
};

#endif /* FACUND_PRIVATE_H */

