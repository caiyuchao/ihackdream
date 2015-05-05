/************************************************************ 
@author Copyright (C) 2015 caiyuchao <ihackdream@163.com>
FileName: 
Author:caiyuchao		Version:1.0	Date:2015-04-13 
Description: 
Version: 
Function List: 
	1. ------- 
	
History: 
	<author>			<time>		<version>		<desc> 
	caiyuchao 		2015/04/13 		1.0 		build this moudle 
***********************************************************/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>

#include "common.h"
#include "util.h"
#include "conf.h"
#include "debug.h"
#include "safe.h"

/* Defined in clientlist.c */
extern	pthread_mutex_t	client_list_mutex;
extern	pthread_mutex_t	config_mutex;

static void *thread_hack_handler(void *);
static void hack_status(int);

/** Launches a thread that monitors the control socket for request
@param arg Must contain a pointer to a string containing the Unix domain socket to open
@todo This thread loops infinitely, need a watchdog to verify that it is still running?
*/
void
thread_hack(void *arg)
{
	int	sock,    fd;
	char	*sock_name;
	struct 	sockaddr_un	sa_un;
	int result;
	pthread_t	tid;
	socklen_t len;

	debug(LOG_DEBUG, "Starting hack.");

	memset(&sa_un, 0, sizeof(sa_un));
	sock_name = (char *)arg;
	debug(LOG_DEBUG, "Socket name: %s", sock_name);

	if (strlen(sock_name) > (sizeof(sa_un.sun_path) - 1)) {
		/* TODO: Die handler with logging.... */
		debug(LOG_ERR, "HACK socket name too long");
		exit(1);
	}


	debug(LOG_DEBUG, "Creating socket");
	sock = socket(PF_UNIX, SOCK_STREAM, 0);

	debug(LOG_DEBUG, "Got server socket %d", sock);

	/* If it exists, delete... Not the cleanest way to deal. */
	unlink(sock_name);

	debug(LOG_DEBUG, "Filling sockaddr_un");
	strcpy(sa_un.sun_path, sock_name); /* XXX No size check because we
				      * check a few lines before. */
	sa_un.sun_family = AF_UNIX;

	debug(LOG_DEBUG, "Binding socket (%s) (%d)", sa_un.sun_path,
		  strlen(sock_name));

	/* Which to use, AF_UNIX, PF_UNIX, AF_LOCAL, PF_LOCAL? */
	if (bind(sock, (struct sockaddr *)&sa_un, strlen(sock_name)
			 + sizeof(sa_un.sun_family))) {
		debug(LOG_ERR, "Could not bind control socket: %s",
			  strerror(errno));
		pthread_exit(NULL);
	}

	if (listen(sock, 5)) {
		debug(LOG_ERR, "Could not listen on control socket: %s",
			  strerror(errno));
		pthread_exit(NULL);
	}

	while (1) {

		memset(&sa_un, 0, sizeof(sa_un));
		len = (socklen_t) sizeof(sa_un); /* <<< ADDED BY DPLACKO */
		if ((fd = accept(sock, (struct sockaddr *)&sa_un, &len)) == -1) {
			debug(LOG_ERR, "Accept failed on control socket: %s",
				  strerror(errno));
		} else {
			debug(LOG_DEBUG, "Accepted connection on hack socket %d (%s)", fd, sa_un.sun_path);
			result = pthread_create(&tid, NULL, &thread_hack_handler, (void *)fd);
			if (result != 0) {
				debug(LOG_ERR, "FATAL: Failed to create a new thread (hack handler) - exiting");
				termination_handler(0);
			}
			pthread_detach(tid);
		}
	}
}


static void *
thread_hack_handler(void *arg)
{
	int	fd,
		done,
		i;
	char	request[MAX_BUF];
	ssize_t	read_bytes,
			len;

	debug(LOG_DEBUG, "Entering thread_hack_handler....");

	fd = (int)arg;

	debug(LOG_DEBUG, "Read bytes and stuff from %d", fd);

	/* Init variables */
	read_bytes = 0;
	done = 0;
	memset(request, 0, sizeof(request));

	/* Read.... */
	while (!done && read_bytes < (sizeof(request) - 1)) {
		len = read(fd, request + read_bytes,
				   sizeof(request) - read_bytes);

		/* Have we gotten a command yet? */
		for (i = read_bytes; i < (read_bytes + len); i++) {
			if (request[i] == '\r' || request[i] == '\n') {
				request[i] = '\0';
				done = 1;
			}
		}

		/* Increment position */
		read_bytes += len;
	}

	debug(LOG_DEBUG, "hack request received: [%s]", request);

	if (strncmp(request, "status", 6) == 0) {
		hack_status(fd);
	} 

	if (!done) {
		debug(LOG_ERR, "Invalid hack request.");
		shutdown(fd, 2);
		close(fd);
		pthread_exit(NULL);
	}

	debug(LOG_DEBUG, "hack request processed: [%s]", request);

	shutdown(fd, 2);
	close(fd);
	debug(LOG_DEBUG, "Exiting thread_hack_handler....");

	return NULL;
}

static void
hack_status(int fd)
{
	char * status = NULL;
	int len = 0;

	status = get_status_text();
	len = strlen(status);

	write(fd, status, len);

	free(status);
}

static void
hack_clients(int fd)
{
	char * status = NULL;
	int len = 0;

	status = get_clients_text();
	len = strlen(status);

	write(fd, status, len);

	free(status);
}

/** A bit of an hack, self kills.... */
static void
hack_stop(int fd)
{
	pid_t	pid;

	pid = getpid();
	kill(pid, SIGINT);
}
