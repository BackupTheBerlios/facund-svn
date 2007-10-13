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

#include <facund_connection.h>
#include <facund_object.h>
#include <facund_response.h>

/* Check if there are updates every 30min  */
static const time_t default_check_period = 30 * 60;

static volatile int facund_in_loop = 1;
static volatile int facund_comms_in_loop = 1;

#define DEFAULT_CONFIG_FILE	"/etc/freebsd-update-control.conf"
#define UPDATE_DATA_DIR		"/var/db/freebsd-update"

static struct fbsd_tag_line *facund_tag_decode_line(const char *);
static void	  facund_tag_free(struct fbsd_tag_line *);
static int	  facund_has_update(unsigned int);
static void	 *look_for_updates(void *);
static int	  facund_read_base_dirs(const char *);


static int	facund_signals[] = { SIGHUP, SIGINT, SIGTERM };
static void	facund_signal_handler(int, siginfo_t *, void *);

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

/*
 * Decodes the data in a line from the tag file
 */
static struct fbsd_tag_line *
facund_tag_decode_line(const char *buf)
{
	struct fbsd_tag_line *line;
	unsigned int len, item;
	char *num_buf;
	const char *str, *ptr, *errstr;

	if (buf == NULL)
		return NULL;

	line = calloc(sizeof(struct fbsd_tag_line), 1);
	if (line == NULL)
		return NULL;

	str = buf;
	ptr = NULL;

	for (item = 0, ptr = strchr(str, '|'); ptr != NULL;
	    ptr = strchr(str, '|')) {
		len = ptr - str;

		switch (item) {
		case 0:
			if (strncmp("freebsd-update", str, len) != 0)
				goto facund_decode_tag_line_exit;
			break;
		case 1:
			line->tag_platform = malloc(len + 1);
			if (line->tag_platform == NULL)
				goto facund_decode_tag_line_exit;

			strlcpy(line->tag_platform, str, len + 1);
			break;
		case 2:
			line->tag_release = malloc(len + 1);
			if (line->tag_release == NULL)
				goto facund_decode_tag_line_exit;

			strlcpy(line->tag_release, str, len + 1);
			break;
		case 3:
			num_buf = malloc(len + 1);
			if (num_buf == NULL)
				goto facund_decode_tag_line_exit;

			strlcpy(num_buf, str, len + 1);
			line->tag_patch = strtonum(num_buf, 0, UINT_MAX,
			    &errstr);
			free(num_buf);
			if (errstr != NULL)
				goto facund_decode_tag_line_exit;
			break;
		case 4:
			if (len != 64)
				goto facund_decode_tag_line_exit;

			strlcpy(line->tag_tindexhash, str, 65);
			break;
		}

		str = ptr + 1;
		item++;
	}
	if (item != 5) {
		goto facund_decode_tag_line_exit;
	} else {
		if (strlen(str) != 11)
			goto facund_decode_tag_line_exit;

		strlcpy(line->tag_eol, str, 11);
	}

	return line;

facund_decode_tag_line_exit:
	/* Clean up on failure */
	facund_tag_free(line);

	return NULL;
}

static void
facund_tag_free(struct fbsd_tag_line *line)
{
	if (line == NULL)
		return;

	if (line->tag_platform != NULL)
		free(line->tag_platform);

	if (line->tag_release != NULL)
		free(line->tag_release);

	free(line);
}

/*
 * Looks for updates on the system with a root of basedir
 */
static int
facund_has_update(unsigned int pos)
{
	struct stat sb;
	FILE *tag_fd;
	char install_link[PATH_MAX], sha_base[PATH_MAX], sum[65], buf[1024];
	char link_target[PATH_MAX];
	struct fbsd_tag_line *line;
	unsigned int rollback_count;
	int link_len;

	assert(pos < watched_db_count);
	snprintf(sha_base, PATH_MAX, "%s\n", watched_db[pos].db_base);
	SHA256_Data(sha_base, strlen(sha_base), sum);

	/* Read in the tag file */
	tag_fd = fopen(watched_db[pos].db_tag_file, "r");
	if (tag_fd != NULL) {
		if (watched_db[pos].db_tag_line != NULL)
			facund_tag_free(watched_db[pos].db_tag_line);

		while (fgets(buf, sizeof buf, tag_fd) != NULL) {
			line = facund_tag_decode_line(buf);
			watched_db[pos].db_tag_line = line;
		}
		fclose(tag_fd);
	}
	
	seteuid(0);

	/* Look for the install link and check if it is a symlink */
	snprintf(install_link, PATH_MAX, "%s/%s-install",
	    watched_db[pos].db_dir, sum);
	if (watched_db[pos].db_tag_line != NULL &&
	    lstat(install_link, &sb) == 0 && S_ISLNK(sb.st_mode)) {
		watched_db[pos].db_next_patch =
		    watched_db[pos].db_tag_line->tag_patch;
	} else {
		watched_db[pos].db_next_patch = 0;
	}

	/* Look for the rollback link and check if it is a symlink */
	snprintf(install_link, PATH_MAX, "%s/%s-rollback",
	    watched_db[pos].db_dir, sum);
	rollback_count = 0;
	errno = 0;
	while ((lstat(install_link, &sb) == 0) && S_ISLNK(sb.st_mode)) {
		rollback_count++;
		link_len = readlink(install_link, link_target,
		    (sizeof link_target) - 1);
		if (link_len == -1) {
			return -1;
		}
		link_target[link_len] = '\0';
		snprintf(install_link, PATH_MAX, "%s/%s/rollback",
		    watched_db[pos].db_dir, link_target);
		errno = 0;
	}
	if (errno != 0 && errno != ENOENT)
		return -1;

	seteuid(getuid());

	watched_db[pos].db_rollback_count = rollback_count;

	return 0;
}

/*
 * Loops looking for updates.
 * There are two ways, the first is to use kqueue to wait
 * for an EVFILT_VNODE event on the database directory with
 * a timeout to allow for the directory to have changed
 * between the time we last checked for updates and the
 * call to kqueue. The second is to sleep for a set time
 * period. The only advantage the kqueue method has over
 * sleeping is we may see the update sooner this way.
 */
void *
look_for_updates(void *data __unused)
{
	struct timespec timeout;
	int kq, use_kqueue;
	struct kevent event, changes;
	size_t pos, signals;
	int error, first_loop;

	signals = 1;

	/* Create the kqueue to wait for events */
	kq = kqueue();
	if (kq == -1)
		use_kqueue = 0;

	for (pos = 0; pos < watched_db_count; pos++) {
		watched_db[pos].db_fd = open(watched_db[pos].db_dir, O_RDONLY);

		/* Create an event to look for files being added to the dir */
		EV_SET(&event, watched_db[pos].db_fd, EVFILT_VNODE,
		    EV_ADD | EV_CLEAR, NOTE_WRITE | NOTE_DELETE | NOTE_EXTEND,
		    0, (void *)pos);

		kevent(kq, &event, 1, NULL, 0, NULL);
	}

	/* Add a signal event to check if there was a signal sent */
	EV_SET(&event, SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, NULL);
	kevent(kq, &event, 1, NULL, 0, NULL);
	for (pos = 0; pos < sizeof(facund_signals) / sizeof(facund_signals[0]);
	    pos++) {
		EV_SET(&event, facund_signals[pos], EVFILT_SIGNAL, EV_ADD,
		    0, 0, NULL);
		kevent(kq, &event, 1, NULL, 0, NULL);
	}

	use_kqueue = 1;

	timeout.tv_sec = default_check_period;
	timeout.tv_nsec = 0;

	/* Scan all directories on the first run */
	pos = watched_db_count;

	first_loop = 1;

	/*
	 * This is the main loop to check for updates. It will
	 * either wait for file system activity on the update
	 * directories of just sleep for a fixed amount of time
	 * then scan all directories.
	 */
	while(facund_in_loop != 0) {
		assert(pos <= watched_db_count);
		if (use_kqueue == 0 || pos == watched_db_count) {
			/*
			 * We are using sleep to wait for updates or
			 * kqueue timed out. This means we have to check
			 * all directories to see if they have an update.
			 */
			for (pos = 0; pos < watched_db_count; pos++) {
				facund_has_update(pos);
			}
			/* Check we have looked at all directories */
			assert(pos == watched_db_count);
		} else {
			/*
			 * We are using kqueue to wait for updates.
			 * pos will contain the position in base_dirs of
			 * the directory that had file system activity.
			 */
			if (pos < watched_db_count) {
				facund_has_update(pos);
			}
		}
		pos = watched_db_count;

		/*
		 * Before we sleep again check if we are to stop running
		 */
		if (facund_in_loop == 0) {
			break;
		}

		/* Wait for posible updates */
		if (use_kqueue == 1) {
			/* Wait for posible updates ready to be installed */
			/* There are no changes, use the same events */
			error = kevent(kq, NULL, 0, &changes, 1, &timeout);

			if (error == -1) {
				/*
				 * There was an error in
				 * kqueue, change to sleep
				 */
				use_kqueue = 0;
			} else if (error > 0) {
				if (changes.filter == EVFILT_VNODE) {
					/* Find in the item that changed */
					pos = (size_t)changes.udata;
				}
			}
		} else {
			sleep(default_check_period);
		}
	}

	for (pos = 0; pos < watched_db_count; pos++) {
		close(watched_db[pos].db_fd);
	}
	return NULL;
}

/*
 * Takes in a string of space seperated directories and returns
 * a NULL terminated array of pointers to each directory
 */
static int
facund_read_base_dirs(const char *str)
{
	const char *ptr, *next_ptr;
	unsigned int pos, len;

	/* An empty string will contain no directories */
	if (str == NULL || str[0] == '\0')
		return -1;

	/* Check we havn't already read in the directories */
	if (watched_db_count != 0 || watched_db != NULL)
		return -1;

	ptr = str;
	while (ptr != NULL) {
		/* Skip leading spaces */
		while (ptr[0] == ' ')
			*ptr++;

		ptr = strchr(ptr, ' ');
		watched_db_count++;
	}

	/*
	 * There must be at least one directory utherwise
	 * the empty string check would have returned
	 */
	assert(watched_db_count > 0);

	/* create an array to hold pointers to the dir names */
	watched_db = calloc(watched_db_count,
	    sizeof(struct fbsd_update_db));

	/* Set the point the ret array to the directories */
	ptr = str;
	pos = 0;
	while (ptr != NULL) {
		/* Skip leading spaces */
		while (ptr[0] == ' ')
			*ptr++;

		next_ptr = strchr(ptr, ' ');
		if (next_ptr == NULL) {
			next_ptr = strchr(ptr, '\0');
		}
		assert(next_ptr > ptr);
		len = next_ptr - ptr;
		len++;

		watched_db[pos].db_base = calloc(len, sizeof(char *));
		if (watched_db[pos].db_base == NULL) {
			/* TODO: Clean up */
			return -1;
		}
		strlcpy(watched_db[pos].db_base, ptr, len);
		asprintf(&watched_db[pos].db_dir, "%s" UPDATE_DATA_DIR,
		    watched_db[pos].db_base);
		if (watched_db[pos].db_dir == NULL) {
			free(watched_db[pos].db_base);
			return -1;
		}

		asprintf(&watched_db[pos].db_tag_file, "%s/tag",
		    watched_db[pos].db_dir);
		if (watched_db[pos].db_dir == NULL) {
			free(watched_db[pos].db_base);
			free(watched_db[pos].db_dir);
			return -1;
		}

		watched_db[pos].db_next_patch = 0;

		ptr = next_ptr;
		if (ptr[0] == '\0') {
			return 0;
		}

		pos++;
	}
	return -1;
}

int
facund_run_update(const char *command, const char *basedir)
{
	char *cmd, *arg;
	int ret;

	assert(command != NULL);
#define FREEBSD_COMMAND "/usr/sbin/freebsd-update"
	arg = NULL;
	if (basedir != NULL) {
		asprintf(&arg, "-b %s", basedir);
		if (arg == NULL)
			return -1;
	}
	asprintf(&cmd, FREEBSD_COMMAND " %s %s", (arg == NULL ? "" : arg),
	    command);

	free(arg);
	if (cmd == NULL) {
		return -1;
	}

	ret = system(cmd);
	free(cmd);

	return ret;
}

static void
facund_signal_handler(int sig __unused, siginfo_t *info __unused,
    void *uap __unused)
{
	facund_in_loop = 0;
}

int
main(int argc, char *argv[])
{
	struct sigaction sa;
	pthread_t update_thread, comms_thread;
	struct facund_conn *conn;
	const char *config_file;
	char *basedirs_string, *uname_r;
	unsigned int pos;
	int config_fd;
	properties config_data;
	char ch;

	/* Drop privileges */
	seteuid(getuid());

	config_file = DEFAULT_CONFIG_FILE;

	while ((ch = getopt(argc, argv, "c:h")) != -1) {
		switch(ch) {
		case 'c':
			config_file = optarg;
			break;
		case 'h':
		default:
			fprintf(stderr, "usage: %s [-c config]\n",
			    getprogname());
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	basedirs_string = NULL;

	for (pos = 0; pos < sizeof(facund_signals) / sizeof(facund_signals[0]);
	    pos++) {
		sa.sa_sigaction = facund_signal_handler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;
		sigaction(facund_signals[pos], &sa, NULL);
	}

	/* Read in the config file */
	config_fd = open(config_file, O_RDONLY);
	if (config_fd == -1 && errno != ENOENT) {
		errx(1, "Could not open the config file");
	} else if (config_fd != -1) {
		/* Read in the config file */
		config_data = properties_read(config_fd);

		if (config_data == NULL) {
			errx(1, "Could not read the config file");
		}

		basedirs_string = property_find(config_data, "base_dirs");
		if (basedirs_string == NULL) {
			errx(1, "Incorrect config file");
		}
		basedirs_string = strdup(basedirs_string);
		if (basedirs_string == NULL) {
			errx(1, "Malloc failed");
		}

		password_hash = property_find(config_data, "password");
		if (password_hash != NULL) {
			password_hash = strdup(password_hash);
			if (password_hash == NULL) {
				errx(1, "Malloc failed");
			}
		}

		properties_free(config_data);
	}

	/* Read in the base dirs */
	if (facund_read_base_dirs(basedirs_string) != 0) {
		errx(1, "No base dirs were given, set base_dirs in %s",
		    config_file);
	}

	/* Create the data connection */
	conn = facund_connect_server("/tmp/facund");
	if (conn == NULL) {
		errx(1, "Could not open a socket: %s\n", strerror(errno));
	}
	chmod("/tmp/facund", 0777);

	/* Get the uname data */
	if (uname(&facund_uname) != 0) {
		errx(1, "Could not get the Operating System version\n");
	}
	uname_r = getenv("UNAME_r");
	if (uname_r != NULL) {
		strlcpy(facund_uname.release, uname_r,
		    sizeof facund_uname.release);
	}

	/* Only allow people to authenticate to begin with */
	facund_server_add_call("authenticate", facund_call_authenticate);

	pthread_create(&update_thread, NULL, look_for_updates, NULL);
	pthread_create(&comms_thread, NULL, do_communication, conn);

	/* Wait for the threads to quit */
	pthread_join(comms_thread, NULL);
	/*
	 * As the communications thread has quit we should
	 * also kill the update thread so we can exit
	 */
	pthread_kill(update_thread, SIGINT);
	pthread_join(update_thread, NULL);

	if (watched_db != NULL) {
		for (pos = 0; pos < watched_db_count; pos++) {
			free(watched_db[pos].db_base);
			free(watched_db[pos].db_dir);
		}
		free(watched_db);
	}

	if (conn != NULL)
		facund_cleanup(conn);

	free(basedirs_string);
	return 0;
}

