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


#ifndef _UTIL_H_
#define _UTIL_H_

#define STATUS_BUF_SIZ	16384

/** @brief Execute a shell command
 */
int execute(char *cmd_line, int quiet);
struct in_addr *wd_gethostbyname(const char *name);

/* @brief Get IP address of an interface */
char *get_iface_ip(const char *ifname);

/* @brief Get MAC address of an interface */
char *get_iface_mac(const char *ifname);

/* @brief Get interface name of default gateway */
char *get_ext_iface (void);

/* @brief Sets hint that an online action (dns/connect/etc using WAN) succeeded */
void mark_online();
/* @brief Sets hint that an online action (dns/connect/etc using WAN) failed */
void mark_offline();
/* @brief Returns a guess (true or false) on whether we're online or not based on previous calls to mark_online and mark_offline */
int is_online();

/* @brief Sets hint that an auth server online action succeeded */
void mark_auth_online();
/* @brief Sets hint that an auth server online action failed */
void mark_auth_offline();
/* @brief Returns a guess (true or false) on whether we're an auth server is online or not based on previous calls to mark_auth_online and mark_auth_offline */
int is_auth_online();

/*
 * @brief Mallocs and returns hackdream uptime string
 */
char * get_uptime_string();
/*
 * @brief Creates a human-readable paragraph of the status of the hackdream process
 */
char * get_status_text();
/*
 * @brief Creates a machine-readable dump of currently connected clients
 */
char * get_clients_text();

/** @brief cheap random */
unsigned short rand16(void);

#define LOCK_GHBN() do { \
	debug(LOG_DEBUG, "Locking wd_gethostbyname()"); \
	pthread_mutex_lock(&ghbn_mutex); \
	debug(LOG_DEBUG, "wd_gethostbyname() locked"); \
} while (0)

#define UNLOCK_GHBN() do { \
	debug(LOG_DEBUG, "Unlocking wd_gethostbyname()"); \
	pthread_mutex_unlock(&ghbn_mutex); \
	debug(LOG_DEBUG, "wd_gethostbyname() unlocked"); \
} while (0)


#endif /* _UTIL_H_ */
