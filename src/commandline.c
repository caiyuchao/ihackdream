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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "debug.h"
#include "safe.h"
#include "conf.h"


static void usage(void);

/** @internal
 * @brief Print usage
 *
 * Prints usage, called when hackdream is run with -h or with an unknown option
 */
static void
usage(void)
{
	printf("Usage: hackdream [options]\n");
	printf("\n");
	printf("  -c [filename] Use this config file\n");
	printf("  -f            Run in foreground\n");
	printf("  -d <level>    Debug level\n");
	printf("  -s            Log to syslog\n");
	printf("  -w <path>     Ndsctl socket path\n");
	printf("  -h            Print usage\n");
	printf("  -v            Print version information\n");
	printf("\n");
}

/** Uses getopt() to parse the command line and set configuration values
 */
void parse_commandline(int argc, char **argv)
{
	int c;
	int i;

	s_config *config = config_get_config();

	while (-1 != (c = getopt(argc, argv, "c:hfd:sw:vi:"))) {

		switch(c) {

		case 'h':
			usage();
			exit(1);
			break;

		case 'c':
			if (optarg) {
				strncpy(config->configfile, optarg, sizeof(config->configfile));
			}
			break;

		case 'w':
			if (optarg) {
				free(config->hack_sock);
				config->hack_sock = safe_strdup(optarg);
			}
			break;

		case 'f':
			config->daemon = 0;
			break;

		case 'd':
			if (optarg) {
				set_log_level(atoi(optarg));
			}
			break;

		case 's':
			config->log_syslog = 1;
			break;

		case 'v':
			printf("This is hackdream version " VERSION "\n");
			exit(1);
			break;

		default:
			usage();
			exit(1);
			break;
		}
	}
}
