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


#ifndef _CONF_H_
#define _CONF_H_

#define VERSION "0.0.1"

/*@{*/
/** Defines */
/** How many times should we try detecting the interface with the default route
 * (in seconds).  If set to 0, it will keep retrying forever */
#define NUM_EXT_INTERFACE_DETECT_RETRY 0
/** How long we should wait per try
 *  to detect the interface with the default route if it isn't up yet (interval in seconds) */
#define EXT_INTERFACE_DETECT_RETRY_INTERVAL 1
#define MAC_ALLOW 0 /** macmechanism to block MAC's unless allowed */
#define MAC_BLOCK 1 /** macmechanism to allow MAC's unless blocked */

/** Defaults configuration values */
#ifndef SYSCONFDIR
#define DEFAULT_CONFIGFILE "hackdream.conf"
#else
#define DEFAULT_CONFIGFILE SYSCONFDIR"hackdream.conf"
#endif
#define DEFAULT_DAEMON 1
#define DEFAULT_DEBUGLEVEL LOG_NOTICE
#define DEFAULT_MAXCLIENTS 20

#define DEFAULT_LOG_SYSLOG 0
#define DEFAULT_SYSLOG_FACILITY LOG_DAEMON
#define DEFAULT_HACK_SOCK "/tmp/hack.sock"

/**
* Firewall targets
*/
typedef enum {
	TARGET_DROP,
	TARGET_REJECT,
	TARGET_ACCEPT,
	TARGET_LOG,
	TARGET_ULOG
} t_firewall_target;

/**
 * Firewall rules
 */
typedef struct _firewall_rule_t {
	t_firewall_target target;	/**< @brief t_firewall_target */
	char *protocol;		/**< @brief tcp, udp, etc ... */
	char *port;			/**< @brief Port to block/allow */
	char *mask;			/**< @brief Mask for the rule *destination* */
	struct _firewall_rule_t *next;
} t_firewall_rule;

/**
 * Firewall rulesets
 */
typedef struct _firewall_ruleset_t {
	char *name;
	char *emptyrulesetpolicy;
	t_firewall_rule *rules;
	struct _firewall_ruleset_t *next;
} t_firewall_ruleset;

/**
 * MAC Addresses
 */
typedef struct _MAC_t {
	char *mac;
	struct _MAC_t *next;
} t_MAC;


/**
 * Configuration structure
 */
typedef struct {
	char configfile[255];		/**< @brief name of the config file */
	char *hack_sock;		/**< @brief ndsctl path to socket */
	int daemon;			/**< @brief if daemon > 0, use daemon mode */
	int debuglevel;		/**< @brief Debug information verbosity */
	int maxclients;		/**< @brief Maximum number of clients allowed */
	int log_syslog;		/**< @brief boolean, whether to log to syslog */
	int syslog_facility;		/**< @brief facility to use when using syslog for logging */
} s_config;

/** @brief Get the current gateway configuration */
s_config *config_get_config(void);

/** @brief Initialise the conf system */
void config_init(void);

/** @brief Initialize the variables we override with the command line*/
void config_init_override(void);

/** @brief Reads the configuration file */
void config_read(const char *filename);

/** @brief Check that the configuration is valid */
void config_validate(void);

/** config API, used in commandline.c */
int set_log_level(int);

#define LOCK_CONFIG() do { \
	debug(LOG_DEBUG, "Locking config"); \
	pthread_mutex_lock(&config_mutex); \
	debug(LOG_DEBUG, "Config locked"); \
} while (0)

#define UNLOCK_CONFIG() do { \
	debug(LOG_DEBUG, "Unlocking config"); \
	pthread_mutex_unlock(&config_mutex); \
	debug(LOG_DEBUG, "Config unlocked"); \
} while (0)

#endif /* _CONF_H_ */
