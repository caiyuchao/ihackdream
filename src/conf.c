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

#include <pthread.h>

#include <string.h>
#include <ctype.h>

#include "common.h"
#include "safe.h"
#include "debug.h"
#include "conf.h"

#include "util.h"


/** @internal
 * Holds the current configuration of the gateway */
static s_config config;

/**
 * Mutex for the configuration file, used by the auth_servers related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** @internal
 * A flag.  If set to 1, there are missing or empty mandatory parameters in the config
 */
static int missing_parms;

/** @internal
 The different configuration options */
typedef enum {
	oBadOption,
	oDaemon,
	oDebugLevel,
} OpCodes;

/** @internal
 The config file keywords for the different configuration options */
static const struct {
	const char *name;
	OpCodes opcode;
	int required;
} keywords[] = {
	{ "daemon", oDaemon },
	{ "debuglevel", oDebugLevel },
	{ NULL, oBadOption },
};

static void config_notnull(const void *parm, const char *parmname);
static int parse_boolean_value(char *);
static int _parse_firewall_rule(t_firewall_ruleset *ruleset, char *leftover);
static void parse_firewall_ruleset(const char *, FILE *, const char *, int *);

static OpCodes config_parse_opcode(const char *cp, const char *filename, int linenum);

/** @internal
Strip comments and leading and trailing whitespace from a string.
Return a pointer to the first nonspace char in the string.
*/
static char* _strip_whitespace(char* p1);

/** Accessor for the current gateway configuration
@return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */
s_config *
config_get_config(void)
{
	return &config;
}

/** Sets the default config parameters and initialises the configuration system */
void
config_init(void)
{
	t_firewall_ruleset *rs;

	debug(LOG_DEBUG, "Setting default config parameters");
	strncpy(config.configfile, DEFAULT_CONFIGFILE, sizeof(config.configfile));
	config.debuglevel = DEFAULT_DEBUGLEVEL;
	config.log_syslog = DEFAULT_LOG_SYSLOG;
	config.daemon = -1;
	config.hack_sock = safe_strdup(DEFAULT_HACK_SOCK);
}

/**
 * If the command-line didn't specify a config, use the default.
 */
void
config_init_override(void)
{
	if (config.daemon == -1) config.daemon = DEFAULT_DAEMON;
}

/** @internal
Attempts to parse an opcode from the config file
*/
static OpCodes
config_parse_opcode(const char *cp, const char *filename, int linenum)
{
	int i;

	for (i = 0; keywords[i].name; i++)
		if (strcasecmp(cp, keywords[i].name) == 0)
			return keywords[i].opcode;

	debug(LOG_ERR, "%s: line %d: Bad configuration option: %s",
		  filename, linenum, cp);
	return oBadOption;
}

/**
Advance to the next word
@param s string to parse, this is the next_word pointer, the value of s
	 when the macro is called is the current word, after the macro
	 completes, s contains the beginning of the NEXT word, so you
	 need to save s to something else before doing TO_NEXT_WORD
@param e should be 0 when calling TO_NEXT_WORD(), it'll be changed to 1
	 if the end of the string is reached.
*/
#define TO_NEXT_WORD(s, e) do { \
	while (*s != '\0' && !isblank(*s)) { \
		s++; \
	} \
	if (*s != '\0') { \
		*s = '\0'; \
		s++; \
		while (isblank(*s)) \
			s++; \
	} else { \
		e = 1; \
	} \
} while (0)

/** @internal
Strip comments and leading and trailing whitespace from a string.
Return a pointer to the first nonspace char in the string.
*/
static char*
_strip_whitespace(char* p1)
{
	char *p2, *p3;

	p3 = p1;
	while (p2 = strchr(p3,'#')) {  /* strip the comment */
		/* but allow # to be escaped by \ */
		if (p2 > p1 && (*(p2 - 1) == '\\')) {
			p3 = p2 + 1;
			continue;
		}
		*p2 = '\0';
		break;
	}

	/* strip leading whitespace */
	while(isspace(p1[0])) p1++;
	/* strip trailing whitespace */
	while(p1[0] != '\0' && isspace(p1[strlen(p1)-1]))
		p1[strlen(p1)-1] = '\0';

	return p1;
}

/**
@param filename Full path of the configuration file to be read
*/
void
config_read(const char *filename)
{
	FILE *fd;
	char line[MAX_BUF], *s, *p1, *p2;
	int linenum = 0, opcode, value;

	debug(LOG_INFO, "Reading configuration file '%s'", filename);

	if (!(fd = fopen(filename, "r"))) {
		debug(LOG_ERR, "FATAL: Could not open configuration file '%s', "
			  "exiting...", filename);
		exit(1);
	}

	while (fgets(line, MAX_BUF, fd)) {
		linenum++;
		s = _strip_whitespace(line);

		/* if nothing left, get next line */
		if(s[0] == '\0') continue;

		/* now we require the line must have form: <option><whitespace><arg>
		 * even if <arg> is just a left brace, for example
		 */

		/* find first word (i.e. option) end boundary */
		p1 = s;
		while ((*p1 != '\0') && (!isspace(*p1))) p1++;
		/* if this is end of line, it's a problem */
		if(p1[0] == '\0') {
			debug(LOG_ERR, "Option %s requires argument on line %d in %s", s, linenum, filename);
			debug(LOG_ERR, "Exiting...");
			exit(-1);
		}

		/* terminate option, point past it */
		*p1 = '\0';
		p1++;

		/* skip any additional leading whitespace, make p1 point at start of arg */
		while (isblank(*p1)) p1++;

		debug(LOG_DEBUG, "Parsing option: %s, arg: %s", s, p1);
		opcode = config_parse_opcode(s, filename, linenum);

		switch(opcode) {
		case oDaemon:
			if (config.daemon == -1 && ((value = parse_boolean_value(p1)) != -1)) {
				config.daemon = value;
			}
			break;
		case oBadOption:
			debug(LOG_ERR, "Bad option %s on line %d in %s", s, linenum, filename);
			debug(LOG_ERR, "Exiting...");
			exit(-1);
			break;
		}
	}

	fclose(fd);

	debug(LOG_INFO, "Done reading configuration file '%s'", filename);
}

/** @internal
Parses a boolean value from the config file
*/
static int
parse_boolean_value(char *line)
{
	if (strcasecmp(line, "no") == 0 ||
			strcasecmp(line, "false") == 0 ||
			strcmp(line, "0") == 0
	   ) {
		return 0;
	}
	if (strcasecmp(line, "yes") == 0 ||
			strcasecmp(line, "true") == 0 ||
			strcmp(line, "1") == 0
	   ) {
		return 1;
	}

	return -1;
}

/* Parse a string to see if it is valid decimal dotted quad IP V4 format */
int check_ip_format(const char *possibleip)
{
	unsigned int a1,a2,a3,a4;

	return (sscanf(possibleip,"%u.%u.%u.%u",&a1,&a2,&a3,&a4) == 4
			&& a1 < 256 && a2 < 256 && a3 < 256 && a4 < 256);
}


/* Parse a string to see if it is valid MAC address format */
int check_mac_format(char *possiblemac)
{
	char hex2[3];
	return
		sscanf(possiblemac,
			   "%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]:%2[A-Fa-f0-9]",
			   hex2,hex2,hex2,hex2,hex2,hex2) == 6;
}



/** Set the debug log level.  See syslog.h
 *  Return 0 on success.
 */
int set_log_level(int level)
{
	config.debuglevel = level;
	return 0;
}

/** Verifies if the configuration is complete and valid.  Terminates the program if it isn't */
void
config_validate(void)
{
#if 0
	config_notnull(config.gw_interface, "GatewayInterface");

	if (missing_parms) {
		debug(LOG_ERR, "Configuration is not complete, exiting...");
		exit(-1);
	}
#endif
}

/** @internal
    Verifies that a required parameter is not a null pointer
*/
static void
config_notnull(const void *parm, const char *parmname)
{
	if (parm == NULL) {
		debug(LOG_ERR, "%s is not set", parmname);
		missing_parms = 1;
	}
}
