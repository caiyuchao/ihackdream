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
#include <syslog.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#if defined(__NetBSD__)
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <util.h>
#endif

#ifdef __linux__
#include <netinet/in.h>
#include <net/if.h>
#endif

#include <string.h>
#include <pthread.h>
#include <netdb.h>

#include "common.h"
#include "safe.h"
#include "util.h"
#include "conf.h"
#include "debug.h"


static pthread_mutex_t ghbn_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Defined in gateway.c */
extern time_t started_time;

/* Defined in clientlist.c */
extern	pthread_mutex_t	client_list_mutex;
extern	pthread_mutex_t	config_mutex;

/* Defined in auth.c */
extern unsigned int authenticated_since_start;


/** Fork a child and execute a shell command.
 * The parent process waits for the child to return,
 * and returns the child's exit() value.
 * @return Return code of the command
 */
int
execute(char *cmd_line, int quiet)
{
	int status, retval;
	pid_t pid, rc;
	struct sigaction sa, oldsa;
	const char *new_argv[4];
	new_argv[0] = "/bin/sh";
	new_argv[1] = "-c";
	new_argv[2] = cmd_line;
	new_argv[3] = NULL;

	/* Temporarily get rid of SIGCHLD handler (see gateway.c), until child exits.
	 * Will handle SIGCHLD here with waitpid() in the parent. */
	debug(LOG_DEBUG,"Setting default SIGCHLD handler SIG_DFL");
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	if (sigaction(SIGCHLD, &sa, &oldsa) == -1) {
		debug(LOG_ERR, "sigaction() failed to set default SIGCHLD handler: %s", strerror(errno));
	}

	pid = safe_fork();

	if (pid == 0) {    /* for the child process:         */

		if (quiet) close(2); /* Close stderr if quiet flag is on */
		if (execvp("/bin/sh", (char *const *)new_argv) == -1) {    /* execute the command  */
			debug(LOG_ERR, "execvp(): %s", strerror(errno));
		} else {
			debug(LOG_ERR, "execvp() failed");
		}
		exit(1);

	} else {        /* for the parent:      */

		debug(LOG_DEBUG, "Waiting for PID %d to exit", (int)pid);
		do {
			rc = waitpid(pid, &status, 0);
			if(rc == -1) {
				if(errno == ECHILD) {
					debug(LOG_DEBUG, "waitpid(): No child exists now. Assuming normal exit for PID %d", (int)pid);
					retval = 0;
				} else {
					debug(LOG_ERR, "Error waiting for child (waitpid() returned -1): %s", strerror(errno));
					retval = -1;
				}
				break;
			}
			if(WIFEXITED(status)) {
				debug(LOG_DEBUG, "Process PID %d exited normally, status %d", (int)rc, WEXITSTATUS(status));
				retval = (WEXITSTATUS(status));
			}
			if(WIFSIGNALED(status)) {
				debug(LOG_DEBUG, "Process PID %d exited due to signal %d", (int)rc, WTERMSIG(status));
				retval = -1;
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));

		debug(LOG_DEBUG, "Restoring previous SIGCHLD handler");
		if (sigaction(SIGCHLD, &oldsa, NULL) == -1) {
			debug(LOG_ERR, "sigaction() failed to restore SIGCHLD handler! Error %s", strerror(errno));
		}

		return retval;
	}
}

struct in_addr *
wd_gethostbyname(const char *name) {
	struct hostent *he;
	struct in_addr *h_addr, *in_addr_temp;

	/* XXX Calling function is reponsible for free() */

	h_addr = safe_malloc(sizeof(struct in_addr));

	LOCK_GHBN();

	he = gethostbyname(name);

	if (he == NULL) {
		free(h_addr);
		UNLOCK_GHBN();
		return NULL;
	}

	in_addr_temp = (struct in_addr *)he->h_addr_list[0];
	h_addr->s_addr = in_addr_temp->s_addr;

	UNLOCK_GHBN();

	return h_addr;
}

char *
get_iface_ip(const char *ifname)
{
#if defined(__linux__)
	struct ifreq if_data;
	struct in_addr in;
	char *ip_str;
	int sockd;
	u_int32_t ip;

	/* Create a socket */
	if ((sockd = socket (AF_INET, SOCK_PACKET, htons(0x8086))) < 0) {
		debug(LOG_ERR, "socket(): %s", strerror(errno));
		return NULL;
	}

	/* Get IP of internal interface */
	strcpy (if_data.ifr_name, ifname);

	/* Get the IP address */
	if (ioctl (sockd, SIOCGIFADDR, &if_data) < 0) {
		debug(LOG_ERR, "ioctl(): SIOCGIFADDR %s", strerror(errno));
		return NULL;
	}
	memcpy ((void *) &ip, (void *) &if_data.ifr_addr.sa_data + 2, 4);
	in.s_addr = ip;

	ip_str = inet_ntoa(in);
	close(sockd);
	return safe_strdup(ip_str);
#elif defined(__NetBSD__)
	struct ifaddrs *ifa, *ifap;
	char *str = NULL;

	if (getifaddrs(&ifap) == -1) {
		debug(LOG_ERR, "getifaddrs(): %s", strerror(errno));
		return NULL;
	}
	/* XXX arbitrarily pick the first IPv4 address */
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifa->ifa_name, ifname) == 0 &&
				ifa->ifa_addr->sa_family == AF_INET)
			break;
	}
	if (ifa == NULL) {
		debug(LOG_ERR, "%s: no IPv4 address assigned");
		goto out;
	}
	str = safe_strdup(inet_ntoa(
						  ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
out:
	freeifaddrs(ifap);
	return str;
#else
	return safe_strdup("0.0.0.0");
#endif
}

char *
get_iface_mac(const char *ifname)
{
#if defined(__linux__)
	int r, s;
	struct ifreq ifr;
	char *hwaddr, mac[13];

	strcpy(ifr.ifr_name, ifname);

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == s) {
		debug(LOG_ERR, "get_iface_mac socket: %s", strerror(errno));
		return NULL;
	}

	r = ioctl(s, SIOCGIFHWADDR, &ifr);
	if (r == -1) {
		debug(LOG_ERR, "get_iface_mac ioctl(SIOCGIFHWADDR): %s", strerror(errno));
		close(s);
		return NULL;
	}

	hwaddr = ifr.ifr_hwaddr.sa_data;
	close(s);
	snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X",
			 hwaddr[0] & 0xFF,
			 hwaddr[1] & 0xFF,
			 hwaddr[2] & 0xFF,
			 hwaddr[3] & 0xFF,
			 hwaddr[4] & 0xFF,
			 hwaddr[5] & 0xFF
			);

	return safe_strdup(mac);
#elif defined(__NetBSD__)
	struct ifaddrs *ifa, *ifap;
	const char *hwaddr;
	char mac[13], *str = NULL;
	struct sockaddr_dl *sdl;

	if (getifaddrs(&ifap) == -1) {
		debug(LOG_ERR, "getifaddrs(): %s", strerror(errno));
		return NULL;
	}
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifa->ifa_name, ifname) == 0 &&
				ifa->ifa_addr->sa_family == AF_LINK)
			break;
	}
	if (ifa == NULL) {
		debug(LOG_ERR, "%s: no link-layer address assigned");
		goto out;
	}
	sdl = (struct sockaddr_dl *)ifa->ifa_addr;
	hwaddr = LLADDR(sdl);
	snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X",
			 hwaddr[0] & 0xFF, hwaddr[1] & 0xFF,
			 hwaddr[2] & 0xFF, hwaddr[3] & 0xFF,
			 hwaddr[4] & 0xFF, hwaddr[5] & 0xFF);

	str = safe_strdup(mac);
out:
	freeifaddrs(ifap);
	return str;
#else
	return NULL;
#endif
}

/** Get name of external interface (the one with default route to the net).
 *  Caller must free.
 */
char *
get_ext_iface (void)
{
#ifdef __linux__
	FILE *input;
	char *device, *gw;
	int i = 1;
	int keep_detecting = 1;
	pthread_cond_t		cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t		cond_mutex = PTHREAD_MUTEX_INITIALIZER;
	struct	timespec	timeout;
	device = (char *)malloc(16);
	gw = (char *)malloc(16);
	debug(LOG_DEBUG, "get_ext_iface(): Autodectecting the external interface from routing table");
	while(keep_detecting) {
		input = fopen("/proc/net/route", "r");
		while (!feof(input)) {
			/* XXX scanf(3) is unsafe, risks overrun */
			fscanf(input, "%s %s %*s %*s %*s %*s %*s %*s %*s %*s %*s\n", device, gw);
			if (strcmp(gw, "00000000") == 0) {
				free(gw);
				debug(LOG_INFO, "get_ext_iface(): Detected %s as the default interface after try %d", device, i);
				return device;
			}
		}
		fclose(input);
		debug(LOG_ERR, "get_ext_iface(): Failed to detect the external interface after try %d (maybe the interface is not up yet?).  Retry limit: %d", i, NUM_EXT_INTERFACE_DETECT_RETRY);
		/* Sleep for EXT_INTERFACE_DETECT_RETRY_INTERVAL seconds */
		timeout.tv_sec = time(NULL) + EXT_INTERFACE_DETECT_RETRY_INTERVAL;
		timeout.tv_nsec = 0;
		/* Mutex must be locked for pthread_cond_timedwait... */
		pthread_mutex_lock(&cond_mutex);
		/* Thread safe "sleep" */
		pthread_cond_timedwait(&cond, &cond_mutex, &timeout);
		/* No longer needs to be locked */
		pthread_mutex_unlock(&cond_mutex);
		//for (i=1; i<=NUM_EXT_INTERFACE_DETECT_RETRY; i++) {
		if (NUM_EXT_INTERFACE_DETECT_RETRY != 0 && i>NUM_EXT_INTERFACE_DETECT_RETRY) {
			keep_detecting = 0;
		}
		i++;
	}
	debug(LOG_ERR, "get_ext_iface(): Failed to detect the external interface after %d tries, aborting", i);
	exit(1);
	free(device);
	free(gw);
#endif
	return NULL;
}

/* Malloc's */
char * format_time(unsigned long int secs)
{
	unsigned int days, hours, minutes, seconds;
	char * str;

	days = secs / (24 * 60 * 60);
	secs -= days * (24 * 60 * 60);
	hours = secs / (60 * 60);
	secs -= hours * (60 * 60);
	minutes = secs / 60;
	secs -= minutes * 60;
	seconds = secs;

	safe_asprintf(&str,"%ud %uh %um %us", days, hours, minutes, seconds);
	return str;
}

/* Caller must free. */
char * get_uptime_string()
{
	return format_time(time(NULL)-started_time);
}


/*
 * @return A string containing human-readable status text.
 * MUST BE free()d by caller
 */
char * get_status_text()
{
	char buffer[STATUS_BUF_SIZ];
	char timebuf[32];
	char * str;
	ssize_t len;
	s_config *config;
	int	   indx;
	unsigned long int now, uptimesecs, durationsecs = 0;
	unsigned long long int download_bytes, upload_bytes;
	t_MAC *trust_mac;
	t_MAC *allow_mac;
	t_MAC *block_mac;

	config = config_get_config();

	len = 0;
	snprintf(buffer, (sizeof(buffer) - len), "==================\nNoDogSplash Status\n====\n");
	len = strlen(buffer);

	now = time(NULL);
	uptimesecs = now - started_time;

	snprintf((buffer + len), (sizeof(buffer) - len), "Version: " VERSION "\n");
	len = strlen(buffer);

	str = format_time(uptimesecs);
	snprintf((buffer + len), (sizeof(buffer) - len), "Uptime: %s\n", str);
	len = strlen(buffer);
	free(str);
	return safe_strdup(buffer);
}


unsigned short rand16(void)
{
	static int been_seeded = 0;

	if (!been_seeded) {
		int fd, n = 0;
		unsigned int c = 0, seed = 0;
		char sbuf[sizeof(seed)];
		char *s;
		struct timeval now;

		/* not a very good seed but what the heck, it needs to be quickly acquired */
		gettimeofday(&now, NULL);
		seed = now.tv_sec ^ now.tv_usec ^ (getpid() << 16);

		srand(seed);
		been_seeded = 1;
	}

	/* Some rand() implementations have less randomness in low bits
	 * than in high bits, so we only pay attention to the high ones.
	 * But most implementations don't touch the high bit, so we
	 * ignore that one.
	 **/
	return( (unsigned short) (rand() >> 15) );
}
