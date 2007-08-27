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
#include <sys/socket.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "facund_connection.h"
#include "facund_private.h"

static struct facund_conn *facund_create(const char *);

/*
 * Do the common parts to initilise a Facund connection
 */
static struct facund_conn *
facund_create(const char *sock)
{
	struct facund_conn *conn;

	conn = calloc(1, sizeof(struct facund_conn));
	if (conn == NULL) {
		return NULL;
	}

	conn->do_unlink = 0;

	conn->local.sun_family = AF_LOCAL;
	strncpy(conn->local.sun_path, sock, sizeof(conn->local.sun_path));

	conn->fd = -1;

	/* Connect to the socket but not anything else */
	conn->sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (conn->sock == -1) {
		facund_cleanup(conn);
		return NULL;
	}

	return conn;
}

/*
 * Create a Unix Domain socket and listen to it
 */
struct facund_conn *
facund_connect_server(const char *sock)
{
	struct facund_conn *conn;

	conn = facund_create(sock);
	if (conn == NULL) {
		return NULL;
	}

	if (bind(conn->sock, (struct sockaddr *)&conn->local,
	    SUN_LEN(&conn->local)) == -1) {
		facund_cleanup(conn);
		return NULL;
	}

	/* Allow cleanup to unlink the socket file */
	conn->do_unlink = 1;

	if (listen(conn->sock, 1) == -1) {
		facund_cleanup(conn);
		return NULL;
	}

	return conn;
}

/*
 * Connect to an existing Unix Domain socket
 */
struct facund_conn *
facund_connect_client(const char *s)
{
	struct facund_conn *conn;

	conn = facund_create(s);
	if (conn == NULL) {
		return NULL;
	}

	if (connect(conn->sock, (struct sockaddr *)&conn->local,
	    SUN_LEN(&conn->local)) == -1) {
		facund_cleanup(conn);
		return NULL;
	}

	/* We wan't to read and write to conn->fd */
	conn->fd = conn->sock;

	/* Don't close conn->sock */
	conn->sock = -1;

	return conn;
}

/*
 * Cleanup a connection
 */
void
facund_cleanup(struct facund_conn *conn)
{
	if (conn == NULL)
		return;

	if (conn->sock != -1)
		close(conn->sock);

	if (conn->fd != -1)
		close(conn->fd);

	if (conn->do_unlink != 0)
		unlink(conn->local.sun_path);
	free(conn);
}

/*
 * Accept a connection on a server
 */
int
facund_accept(struct facund_conn *conn)
{
	if (conn == NULL)
		return -1;

	/*
	 * Don't accept when there is an existing
	 * connection but don't complain either
	 */
	if (conn->fd != -1)
		return 0;

	/* Wait for the connection */
	conn->sock_len = sizeof(conn->remote);
	conn->fd = accept(conn->sock, (struct sockaddr *)&conn->remote,
	    &conn->sock_len);

	return (conn->fd == -1 ? -1 : 0);
}

/*
 * Encapsulates a message and sends it to the client
 */
ssize_t
facund_send(struct facund_conn *conn, const char *msg, size_t length)
{
	return send(conn->fd, msg, length, 0);
}

/*
 * Receives and decodes a message
 * returns -1 on error
 * if the buffer is too short for the message 
 */
ssize_t
facund_recv(struct facund_conn *conn, void *buf, size_t length)
{
	//return read(conn->fd, buf, length);
	return recv(conn->fd, buf, length, 0);
}

/*
 * Closes a server connection but allows a new one to be accepted
 */
void
facund_close(struct facund_conn *conn)
{
	if (conn == NULL)
		return;
	if (conn->fd == -1)
		return;

	/* Close the connection and mark the fd as closed */
	close(conn->fd);
	conn->fd = -1;
}

