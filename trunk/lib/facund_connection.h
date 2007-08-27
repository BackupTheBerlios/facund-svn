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

#ifndef FACUND_CONNECT_H
#define FACUND_CONNECT_H

#include <sys/types.h>

struct facund_conn;
struct facund_object;
struct facund_response;

struct facund_conn	*facund_connect_server(const char *);
struct facund_conn	*facund_connect_client(const char *);
void			 facund_cleanup(struct facund_conn *);
int			 facund_accept(struct facund_conn *);
ssize_t			 facund_send(struct facund_conn *, const char *,size_t);
ssize_t			 facund_recv(struct facund_conn *, void *, size_t);
void			 facund_close(struct facund_conn *);

/* Server specific functions */
int			 facund_server_start(struct facund_conn *, long);
int			 facund_server_finish(struct facund_conn *);
int			 facund_server_get_request(struct facund_conn *);

/* This is the call handler. The data passed in is the call id and argument */
typedef struct facund_response *facund_call_cb(const char *, 
    struct facund_object *);

int	facund_server_add_call(const char *call, facund_call_cb *);

#endif /* FACUND_CONNECT_H */
