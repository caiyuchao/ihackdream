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
#include <errno.h>

#include "hackcli.h"


/* N.B. this is hack.h s_config, not conf.h s_config */
s_config config;

static void usage(void);
static void init_config(void);
static void parse_commandline(int, char **);
static int connect_to_server(char *);
static int send_request(int, char *);
static void hack_print(char *);
static void hack_status(void);

/** @internal
 * @brief Print usage
 *
 * Prints usage, called when hack is run with -h or with an unknown option
 */
static void
usage(void)
{
	printf("Usage: hack [options] command [arguments]\n");
	printf("\n");
	printf("options:\n");
	printf("  -s <path>         Path to the socket\n");
	printf("  -h                Print usage\n");
	printf("\n");
	printf("commands:\n");
	printf("  status            View the status of hackdream\n");
	printf("\n");
}

/** @internal
 *
 * Init default values in config struct
 */
static void
init_config(void)
{
	config.socket = strdup(DEFAULT_SOCK);
	config.command = HACK_UNDEF;
}

/** @internal
 *
 * Uses getopt() to parse the command line and set configuration values
 */
void
parse_commandline(int argc, char **argv)
{
	extern int optind;
	int c;

	while (-1 != (c = getopt(argc, argv, "s:h"))) {
		switch(c) {
		case 'h':
			usage();
			exit(1);
			break;

		case 's':
			if (optarg) {
				free(config.socket);
				config.socket = strdup(optarg);
			}
			break;

		default:
			usage();
			exit(1);
			break;
		}
	}

	if ((argc - optind) <= 0) {
		usage();
		exit(1);
	}

	if (strcmp(*(argv + optind), "status") == 0) {
		config.command = HACK_STATUS;
	} else {
		fprintf(stderr, "hack: Error: Invalid command \"%s\"\n", *(argv + optind));
		usage();
		exit(1);
	}
}

static int
connect_to_server(char *sock_name)
{
	int sock;
	struct sockaddr_un	sa_un;

	/* Connect to socket */
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&sa_un, 0, sizeof(sa_un));
	sa_un.sun_family = AF_UNIX;
	strncpy(sa_un.sun_path, sock_name, (sizeof(sa_un.sun_path) - 1));

	if (connect(sock, (struct sockaddr *)&sa_un,
				strlen(sa_un.sun_path) + sizeof(sa_un.sun_family))) {
		fprintf(stderr, "hack: hackdream probably not started (Error: %s)\n", strerror(errno));
		exit(1);
	}

	return sock;
}

static int
send_request(int sock, char *request)
{
	ssize_t	len, written;

	len = 0;
	while (len != strlen(request)) {
		written = write(sock, (request + len), strlen(request) - len);
		if (written == -1) {
			fprintf(stderr, "Write to hackdream failed: %s\n",
					strerror(errno));
			exit(1);
		}
		len += written;
	}

	return((int)len);
}


/* Perform a hack action, printing to stdout the server response.
 *  Action given by cmd.
 */
static void
hack_print(char * cmd)
{
	int sock;
	char buffer[4096];
	char request[32];
	int len;

	sock = connect_to_server(config.socket);

	snprintf(request, sizeof(request)-strlen(HACK_TERMINATOR), "%s", cmd);
	strcat(request, HACK_TERMINATOR);

	len = send_request(sock, request);

	while ((len = read(sock, buffer, sizeof(buffer)-1)) > 0) {
		buffer[len] = '\0';
		printf("%s", buffer);
	}

	if(len<0) {
		fprintf(stderr, "hack: Error reading socket: %s\n", strerror(errno));
	}

	shutdown(sock, 2);
	close(sock);
}


static void
hack_status(void)
{
	hack_print("status");
}

int
main(int argc, char **argv)
{
	/* Init configuration */
	init_config();
	parse_commandline(argc, argv);

	switch(config.command) {
	case HACK_STATUS:
		hack_status();
		break;

	default:
		/* XXX NEVER REACHED */
		fprintf(stderr, "Unknown opcode: %d\n", config.command);
		exit(1);
		break;
	}
	exit(0);
}
