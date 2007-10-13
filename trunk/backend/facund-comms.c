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

#include "facund-be.h"

#include <pthread.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include <assert.h>
#include <bsdxml.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libutil.h>
#include <limits.h>
#include <sha256.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Check if there are updates every 30min  */
static const time_t default_check_period = 30 * 60;

static volatile int facund_in_loop = 1;
static volatile int facund_comms_in_loop = 1;

#define DEFAULT_CONFIG_FILE	"/etc/freebsd-update-control.conf"
#define UPDATE_DATA_DIR		"/var/db/freebsd-update"

static struct facund_response * facund_get_update_types(const char *,
    const struct facund_object *, int *, int *);
static const char **facund_get_dir_list(const struct facund_object *);
static struct facund_response *facund_read_type_directory(const char *,
    const struct facund_object *, const char ***, int *, int *);

static struct facund_response *facund_call_ping(const char *,
    struct facund_object *);
static struct facund_response *facund_get_directories(const char *,
    struct facund_object *);
static struct facund_response *facund_call_list_updates(const char *,
    struct facund_object *);
static struct facund_response *facund_call_list_installed(const char *,
    struct facund_object *);
static struct facund_response *facund_call_install_patches(const char *,
    struct facund_object *);
static struct facund_response *facund_call_rollback_patches(const char *,
    struct facund_object *);
static struct facund_response *facund_call_get_services(const char *,
    struct facund_object *obj);
static struct facund_response *facund_call_restart_services(const char *,
    struct facund_object *);

static void	facund_comms_signal_handler(int, siginfo_t *, void *);

struct fbsd_tag_line {
	char	*tag_platform;
	char	*tag_release;
	unsigned int tag_patch;
	char	 tag_tindexhash[65];
	char	 tag_eol[11];
};

/*
 * Structure describing the current state of
 * the freebsd-update database directory
 */
struct fbsd_update_db {
	char	 *db_base;
	char	 *db_dir;
	int	  db_fd;

	volatile unsigned int db_next_patch;
	volatile unsigned int db_rollback_count;

	char	 *db_tag_file;
	struct fbsd_tag_line *db_tag_line;
};

static unsigned int watched_db_count = 0;
static struct fbsd_update_db *watched_db = NULL;

static struct utsname facund_uname;
static char *password_hash = NULL;

/* When called the front end died without disconnecting
 * Cleanup and wait for a new connection
 */
static void
facund_comms_signal_handler(int sig __unused, siginfo_t *info __unused,
    void *uap __unused)
{
	facund_comms_in_loop = 0;
}

static long facund_salt = 0;

void *
do_communication(void *data)
{
	struct sigaction sa;
	struct facund_conn *conn = (struct facund_conn *)data;

	sa.sa_sigaction = facund_comms_signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGPIPE, &sa, NULL);

	while(1) {
		int ret = 0;

		/* We are now in the loop. This will change on SIGPIPE */
		facund_comms_in_loop = 1;

		assert(facund_salt == 0);
		do {
			facund_salt = random();
		} while (facund_salt == 0);
		if(facund_server_start(conn, facund_salt) == -1) {
			if (facund_in_loop != 0) {
				/*
				 * When we are not quiting tell
				 * the user there was a problem
				 */
				fprintf(stderr,
				    "ERROR: Couldn't start the connection\n");
			}
			break;
		}
		if (facund_comms_in_loop == 0) {
			facund_salt = 0;
			continue;
		}

		while(ret == 0) {
			ret = facund_server_get_request(conn);
			if (ret == -1) {
				if (facund_in_loop != 0) {
					fprintf(stderr, "ERROR: Couldn't read "
					    "data from network\n");
				}
			}
			if (facund_comms_in_loop == 0)
				break;
		}
		facund_salt = 0;
		if (facund_comms_in_loop == 0)
			continue;

		facund_server_finish(conn);
		if (facund_comms_in_loop == 0)
			continue;
		if (ret == -1)
			break;
	}

	return NULL;
}

struct facund_response *
facund_call_authenticate(const char *id, struct facund_object *obj)
{
	char *buf, sum[65];

	if (facund_salt == 0) {
		return facund_response_new(id, 1, "Already authenticated",NULL);
	}
	if (facund_object_get_type(obj) != FACUND_STRING) {
		return facund_response_new(id, 1, "Incorrect Data", NULL);
	}

	/* Check the password */
	asprintf(&buf, "%s%ld", password_hash, facund_salt);
	SHA256_Data(buf, strlen(buf), sum);
	free(buf);
	printf("%s\n%s\n\n", sum, facund_object_get_string(obj));
	if (strcmp(sum, facund_object_get_string(obj)) != 0) {
		return facund_response_new(id, 1, "Incorrect Password", NULL);
	}

	/* Add the callbacks for each call */
	facund_server_add_call("ping", facund_call_ping);
	facund_server_add_call("get_directories", facund_get_directories);
	facund_server_add_call("list_updates", facund_call_list_updates);
	facund_server_add_call("list_installed", facund_call_list_installed);
	facund_server_add_call("install_patches", facund_call_install_patches);
	facund_server_add_call("rollback_patches",facund_call_rollback_patches);
	facund_server_add_call("get_services", facund_call_get_services);
	facund_server_add_call("restart_services",facund_call_restart_services);

	return facund_response_new(id, 0, "No Error", NULL);
}

static struct facund_response *
facund_call_ping(const char *id, struct facund_object *obj __unused)
{
	struct facund_response *resp;
	struct facund_object *pong;

	pong = facund_object_new_string();
	facund_object_set_string(pong, "pong");
	resp = facund_response_new(id, 0, "No error", pong);
	return resp;
}

static struct facund_response *
facund_get_directories(const char *id, struct facund_object *obj __unused)
{
	struct facund_object *dirs, *item;
	unsigned int pos;

	dirs = facund_object_new_array();
	for (pos = 0; pos < watched_db_count; pos++) {
		item = facund_object_new_string();
		facund_object_set_string(item, watched_db[pos].db_base);
		facund_object_array_append(dirs, item);
	}
	return facund_response_new(id, 0, "No Error", dirs);
}

/*
 * Takes either a facund array or a facund string and sets base
 * or ports to true if they are contained in the object
 */
static struct facund_response *
facund_get_update_types(const char *id, const struct facund_object *obj,
    int *base, int *ports)
{
	const struct facund_object *area_objs[2];
	const char *areas[2];
	enum facund_type type;

	assert(base != NULL);
	assert(ports != NULL);

	type = facund_object_get_type(obj);
	if (type == FACUND_ARRAY) {
		if (facund_object_array_size(obj) != 2) {
			return facund_response_new(id, 1,
			    "Wrong number of arguments", NULL);
		}
		area_objs[0] = facund_object_get_array_item(obj, 0);
		area_objs[1] = facund_object_get_array_item(obj, 1);

		areas[0] = facund_object_get_string(area_objs[0]);
		areas[1] = facund_object_get_string(area_objs[1]);

		if (strcmp(areas[0], "base") == 0 || strcmp(areas[1], "base"))
			*base = 1;

		if (strcmp(areas[0], "ports") == 0 || strcmp(areas[1], "ports"))
			*ports = 1;

	} else if (type == FACUND_STRING) {
		areas[0] = facund_object_get_string(obj);
		if (strcmp(areas[0], "base") == 0) {
			*base = 1;
		} else if (strcmp(areas[0], "ports") == 0) {
			*ports = 1;
		}
	} else {
		return facund_response_new(id, 1, "Incorrect data type", NULL);
	}
	return NULL;
}

/*
 * Takes a either facund array of string objects or a single string
 * object and returns a C array of C strings of the objects
 * TODO: Rename as it is a generic function to extract data from an array
 */
static const char **
facund_get_dir_list(const struct facund_object *obj)
{
	const char **dirs;
	const struct facund_object *cur;
	size_t items, pos;
	assert(obj != NULL);

	switch(facund_object_get_type(obj)) {
	case FACUND_STRING:
		dirs = malloc(2 * sizeof(char *));
		if (dirs == NULL)
			return NULL;

		dirs[0] = facund_object_get_string(
		    __DECONST(struct facund_object *, obj));
		dirs[1] = NULL;
		break;
	case FACUND_ARRAY:
		items = facund_object_array_size(obj);
		if (items == 0)
			return NULL;

		dirs = malloc((items + 1) * sizeof(char *));
		if (dirs == NULL)
			return NULL;

		for (pos = 0;
		    (cur = facund_object_get_array_item(obj, pos)) != NULL;
		     pos++) {
			dirs[pos] = facund_object_get_string(cur);
		}
		dirs[pos] = NULL;
		assert(pos == items);

		break;
	default:
		return NULL;
	}
	return dirs;
}

static struct facund_response *
facund_read_type_directory(const char *id, const struct facund_object *obj,
    const char ***base_dirs, int *base, int *ports)
{
	const struct facund_object *cur;
	struct facund_response *ret;
	unsigned int pos;

	assert(id != NULL);
	assert(obj != NULL);
	assert(base_dirs != NULL);
	assert(base != NULL);
	assert(ports != NULL);

	if (facund_object_get_type(obj) != FACUND_ARRAY) {
		return facund_response_new(id, 1, "Bad data sent", NULL);
	}

	for (pos = 0; (cur = facund_object_get_array_item(obj, pos)) != NULL;
	    pos++) {
		switch (pos) {
		case 0:
			/* Read in the type of updates to list */
			ret = facund_get_update_types(id, cur, base, ports);
			if (ret != NULL)
				return ret;
			break;
		case 1:
			/* Read in the directories to get updates for */
			*base_dirs = facund_get_dir_list(cur);
			if (*base_dirs == NULL)
				return facund_response_new(id, 1,
				    "Malloc failed", NULL);
			break;
		default:
			if (*base_dirs != NULL)
				free(*base_dirs);

			return facund_response_new(id, 1, "Too many arguments",
			    NULL);
		}
	}
	if (pos != 2) {
		if (base_dirs != NULL)
			free(base_dirs);
		return facund_response_new(id, 1,
		    "Not enough arguments", NULL);
	}
	return NULL;
}

static struct facund_response *
facund_call_list_updates(const char *id, struct facund_object *obj)
{
	struct facund_response *ret;
	struct facund_object *args;
	const char **base_dirs;
	int get_base, get_ports;
	unsigned int pos;

	get_base = get_ports = 0;
	base_dirs = NULL;

	if (obj == NULL) {
		/* TODO: Don't use magic numbers */
		return facund_response_new(id, 1, "No data sent", NULL);
	}

	/* Read in the arguments */
	ret = facund_read_type_directory(id, obj, &base_dirs, &get_base,
	    &get_ports);
	if (ret != NULL) {
		if (base_dirs != NULL)
			free(base_dirs);
		return ret;
	}
	/* This should have been assigned as ret is NULL when successful */
	assert(base_dirs != NULL);

	/*
	 * If any of these asserts fail there was
	 * incorrect logic checking arguments
	 */
	assert(get_ports == 1 || get_base == 1);
	assert(base_dirs[0] != NULL);

	args = facund_object_new_array();
	for (pos = 0; base_dirs[pos] != NULL; pos++) {
		struct facund_object *pair, *item, *updates;
		unsigned int i;
		char *buf;

		for (i = 0; i < watched_db_count; i++) {
			if (strcmp(watched_db[i].db_base, base_dirs[pos]) != 0)
				continue;

			if (watched_db[i].db_next_patch == 0)
				break;

			pair = facund_object_new_array();

			/* Add the directory to the start of the array */
			item = facund_object_new_string();
			facund_object_set_string(item, base_dirs[pos]);
			facund_object_array_append(pair, item);

			/* Add a list of updates to the array */
			updates = facund_object_new_array();
			item = facund_object_new_string();
			asprintf(&buf, "%s-p%u", facund_uname.release,
			    watched_db[i].db_next_patch);
			if (buf == NULL)
				return facund_response_new(id, 1,
				    "Malloc failed", NULL);

			facund_object_set_string(item, buf);
			free(buf);
			facund_object_array_append(updates, item);
			facund_object_array_append(pair, updates);

			/*
			 * Add the directory on to the
			 * end of the arguments to return
			 */
			facund_object_array_append(args, pair);
			break;
		}
	}

	if (facund_object_array_size(args) == 0) {
		facund_object_free(args);
		args = NULL;
	}

	free(base_dirs);
	return facund_response_new(id, RESP_GOOD, "Success", args);
}

static struct facund_response *
facund_call_list_installed(const char *id, struct facund_object *obj)
{
	struct facund_response *ret;
	struct facund_object *args;
	const char **base_dirs;
	int get_base, get_ports;
	unsigned int pos;

	get_base = get_ports = 0;
	base_dirs = NULL;

	if (obj == NULL) {
		/* TODO: Don't use magic numbers */
		return facund_response_new(id, 1, "No data sent", NULL);
	}

	/* Read in the arguments */
	ret = facund_read_type_directory(id, obj, &base_dirs, &get_base,
	    &get_ports);
	if (ret != NULL) {
		if (base_dirs != NULL)
			free(base_dirs);
		return ret;
	}
	/* This should have been assigned as ret is NULL when successful */
	assert(base_dirs != NULL);

	/*
	 * If any of these asserts fail there was
	 * incorrect logic checking arguments
	 */
	assert(get_ports == 1 || get_base == 1);
	assert(base_dirs[0] != NULL);

	args = facund_object_new_array();
	for (pos = 0; base_dirs[pos] != NULL; pos++) {
		struct facund_object *pair, *item, *updates;
		unsigned int i;
		char *buf;

		for (i = 0; i < watched_db_count; i++) {
			unsigned int rollback_pos;

			if (strcmp(watched_db[i].db_base, base_dirs[pos]) != 0)
				continue;

			if (watched_db[i].db_rollback_count == 0)
				break;

			pair = facund_object_new_array();

			/* Add the directory to the start of the array */
			item = facund_object_new_string();
			facund_object_set_string(item, base_dirs[pos]);
			facund_object_array_append(pair, item);

			/* Add a list of updates to the array */
			updates = facund_object_new_array();

			for (rollback_pos = 0;
			     rollback_pos < watched_db[i].db_rollback_count;
			     rollback_pos++) {
				unsigned int level;

				/* Calculate the patch level */
				level = watched_db[i].db_tag_line->tag_patch;
				level -= rollback_pos - 1;
				if (watched_db[i].db_next_patch > 0)
					level--;

				asprintf(&buf, "%s-p%u", facund_uname.release,
				    level);
				if (buf == NULL)
					return facund_response_new(id, 1,
					    "Malloc failed", NULL);

				/* Create the item and add it to the array */
				item = facund_object_new_string();
				facund_object_set_string(item, buf);
				facund_object_array_append(updates, item);

				free(buf);
			}
			/* If there were no rollbacks we shouldn't be here */
			assert(rollback_pos > 0);

			facund_object_array_append(pair, updates);

			/*
			 * Add the directory on to the
			 * end of the arguments to return
			 */
			facund_object_array_append(args, pair);
			break;
		}
	}
	/* There are no updates avaliable */
	if (facund_object_array_size(args) == 0) {
		facund_object_free(args);
		args = NULL;
	}

	free(base_dirs);
	return facund_response_new(id, RESP_GOOD, "Success", args);
}

static struct facund_response *
facund_read_directory_patchlevel(const char *id,
    const struct facund_object *obj, const char **base_dir,
    const char ***patches)
{
	const struct facund_object *cur, *dir, *patch;

	if (facund_object_get_type(obj) != FACUND_ARRAY) {
		return facund_response_new(id, 1, "Bad data sent", NULL);
	}

	cur = facund_object_get_array_item(obj, 0);
	if (cur == NULL) {
		facund_response_new(id, 1, "Bad data sent", NULL);
	}
	if (facund_object_get_type(cur) == FACUND_STRING) {
		/* Get the data for this directory */
		dir = cur;
		patch = facund_object_get_array_item(obj, 1);
		if (patch == NULL) {
			return facund_response_new(id, 1, "Bad data sent",NULL);
		}

		/* Get the directory and patch level */
		*base_dir = facund_object_get_string(dir);
		*patches = facund_get_dir_list(patch);

		if (*base_dir == NULL || *patches == NULL) {
			return facund_response_new(id, 1, "Malloc failed",NULL);
		}
	} else {
		return facund_response_new(id, 1, "Bad data sent", NULL);
	}

	return NULL;
}

static struct facund_response *
facund_call_install_patches(const char *id, struct facund_object *obj)
{
	const char *base_dir, **patches;
	struct facund_response *ret;
	unsigned int pos;
	int failed;

	if (obj == NULL) {
		/* TODO: Don't use magic numbers */
		return facund_response_new(id, 1, "No data sent", NULL);
	}

	base_dir = NULL;
	patches = NULL;
	ret = facund_read_directory_patchlevel(id, obj, &base_dir, &patches);
	if (ret != NULL)
		return ret;

	/* Check the directory is being watched */
	for (pos = 0; pos < watched_db_count; pos++) {
		if (strcmp(watched_db[pos].db_base, base_dir) == 0) {
			break;
		}
	}
	if (pos == watched_db_count) {
		return facund_response_new(id, 1, "Incorrect directory", NULL);
	}

	/* In the all case we will install all avaliable patches */
	failed = 0;
	if (strcmp(patches[0], "base") == 0) {
		if (facund_run_update("install", base_dir) != 0) {
			failed = 1;
		}
	} else {
		return facund_response_new(id, 1, "Unsupported patch", NULL);
	}

	if (failed != 0) {
		return facund_response_new(id, 1,
		    "Some updates failed to install", NULL);
	}
	return facund_response_new(id, 0, "All updates installed", NULL);
}

static struct facund_response *
facund_call_rollback_patches(const char *id, struct facund_object *obj)
{
	const char *base_dir, **patches;
	struct facund_response *ret;
	unsigned int pos;
	int failed;

	if (obj == NULL) {
		/* TODO: Don't use magic numbers */
		return facund_response_new(id, 1, "No data sent", NULL);
	}

	base_dir = NULL;
	patches = NULL;
	ret = facund_read_directory_patchlevel(id, obj, &base_dir, &patches);
	if (ret != NULL)
		return ret;

	/* Check the directory is being watched */
	for (pos = 0; pos < watched_db_count; pos++) {
		if (strcmp(watched_db[pos].db_base, base_dir) == 0) {
			break;
		}
	}
	if (pos == watched_db_count) {
		return facund_response_new(id, 1, "Incorrect directory", NULL);
	}

	failed = 0;

	if (strcmp(patches[0], "base") == 0) {
		/* Rollback the top most base patch */
		if (facund_run_update("rollback", base_dir) != 0) {
			failed = 1;
		}
	} else {
		return facund_response_new(id, 1, "Unsupported patch", NULL);
	}

	if (failed != 0) {
		return facund_response_new(id, 1,
		    "Some patches failed to rollback", NULL);
	}
	return facund_response_new(id, 0, "Success", NULL);
}

static struct facund_response *
facund_call_get_services(const char *id __unused, struct facund_object *obj __unused)
{
	struct facund_object *dirs, *cur_dir;
	const char *base_dir;
	struct dirent *de;
	unsigned int pos;
	DIR *d;

	if (obj == NULL) {
		/* TODO: Don't use magic numbers */
		return facund_response_new(id, 1, "No data sent", NULL);
	}

	/* Read in the base dir to get the services for */
	base_dir = NULL;
	if (facund_object_get_type(obj) != FACUND_STRING) {
		return facund_response_new(id, 1, "Incorrect data", NULL);
	}
	base_dir = facund_object_get_string(obj);
	if (base_dir == NULL) {
		return facund_response_new(id, 1, "Malloc failed", NULL);
	}
	if (strcmp(base_dir, "/") != 0) {
		return facund_response_new(id, 1,
		    "Can only restart services in /", NULL);
	}
	for (pos = 0; pos < watched_db_count; pos++) {
		if (strcmp(watched_db[pos].db_base, base_dir) == 0) {
			break;
		}
	}
	if (pos == watched_db_count) {
		return facund_response_new(id, 1, "Unknown base dir", NULL);
	}

	d = opendir("/etc/rc.d/");
	if (d == NULL) {
		return facund_response_new(id, 1, "Could not open /etc/rc.d/",
		    NULL);
	}

	dirs = facund_object_new_array();
	while ((de = readdir(d)) != NULL) {
		/* Don't look at hidden files */
		if (de->d_name[0] == '.')
			continue;

		cur_dir = facund_object_new_string();
		facund_object_set_string(cur_dir, de->d_name);
		facund_object_array_append(dirs, cur_dir);
	}
	if (facund_object_array_size(dirs) == 0) {
		facund_object_free(dirs);
		return facund_response_new(id, 1, "No services found", NULL);
	}

	return facund_response_new(id, 0, "Services found", dirs);
}

static struct facund_response *
facund_call_restart_services(const char *id, struct facund_object *obj)
{
	const char *base_dir, *service;
	const struct facund_object *cur;
	char service_script[PATH_MAX], *cmd;
	unsigned int pos;
	struct stat sb;

	if (obj == NULL) {
		/* TODO: Don't use magic numbers */
		return facund_response_new(id, 1, "No data sent", NULL);
	}

	base_dir = NULL;
	service = NULL;

	if (facund_object_get_type(obj) != FACUND_ARRAY) {
		return facund_response_new(id, 1, "Incorrect data", NULL);
	}
	if (facund_object_array_size(obj) != 2) {
		return facund_response_new(id, 1, "Incorrect data", NULL);
	}

	/* Find the base dir */
	cur = facund_object_get_array_item(obj, 0);
	if (facund_object_get_type(cur) != FACUND_STRING) {
		return facund_response_new(id, 1, "Incorrect data", NULL);
	}
	base_dir = facund_object_get_string(cur);
	if (base_dir == NULL) {
		return facund_response_new(id, 1, "Malloc failed", NULL);
	}
	/*
	 * We can only restart a service if the base dir
	 * is / as we don't know how to signal any other's.
	 * eg. if it is in a jail we will have to use jexec
	 * but we don't know if this base is in a jail
	 */
	if (strcmp(base_dir, "/") != 0) {
		return facund_response_new(id, 1,
		    "Can only restart services in /", NULL);
	}
	for (pos = 0; pos < watched_db_count; pos++) {
		if (strcmp(watched_db[pos].db_base, base_dir) == 0) {
			break;
		}
	}
	if (pos == watched_db_count) {
		return facund_response_new(id, 1, "Unknown base dir", NULL);
	}

	/* Find the service to restart */
	cur = facund_object_get_array_item(obj, 1);
	if (facund_object_get_type(cur) != FACUND_STRING) {
		return facund_response_new(id, 1, "Incorrect data", NULL);
	}
	service = facund_object_get_string(cur);
	if (service == NULL) {
		return facund_response_new(id, 1, "Malloc failed", NULL);
	}
	do {
		/* Try services in /etc/rc.d */
		snprintf(service_script, PATH_MAX, "/etc/rc.d/%s", service);
		if (stat(service_script, &sb) == 0) {
			break;
		}
		
		/* Try services in /usr/local/etc/rc.d */
		snprintf(service_script, PATH_MAX, "/usr/local/etc/rc.d/%s",
		    service);
		if (stat(service_script, &sb) == 0) {
			break;
		}

		return facund_response_new(id, 1, "Unknown service", NULL);
	} while (0);

	/* Attempt to restart the service */
	asprintf(&cmd, "%s restart", service_script);
	if (cmd == NULL) {
		return facund_response_new(id, 1, "Malloc failed", NULL);
	}
	seteuid(0);
	if (system(cmd) != 0) {
		free(cmd);
		seteuid(getuid());
		return facund_response_new(id, 1, "Service restart failed",
		    NULL);
	}
	free(cmd);
	seteuid(getuid());
	return facund_response_new(id, 0, "Service restart successful", NULL);
}

