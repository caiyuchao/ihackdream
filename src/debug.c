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
#include <errno.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "conf.h"

/** @internal
Do not use directly, use the debug macro */
void
_debug(char *filename, int line, int level, char *format, ...)
{
	char buf[28];
	va_list vlist;
	s_config *config = config_get_config();
	time_t ts;
	sigset_t block_chld;

	time(&ts);

	if (config->debuglevel >= level) {
		sigemptyset(&block_chld);
		sigaddset(&block_chld, SIGCHLD);
		sigprocmask(SIG_BLOCK, &block_chld, NULL);

		if (level <= LOG_WARNING) {
			fprintf(stderr, "[%d][%.24s][%u](%s:%d) ", level, ctime_r(&ts, buf), getpid(),
					filename, line);
			va_start(vlist, format);
			vfprintf(stderr, format, vlist);
			va_end(vlist);
			fputc('\n', stderr);
		} else if (!config->daemon) {
			fprintf(stdout, "[%d][%.24s][%u](%s:%d) ", level, ctime_r(&ts, buf), getpid(),
					filename, line);
			va_start(vlist, format);
			vfprintf(stdout, format, vlist);
			va_end(vlist);
			fputc('\n', stdout);
			fflush(stdout);
		}

		if (config->log_syslog) {
			openlog("hackdream", LOG_PID, config->syslog_facility);
			va_start(vlist, format);
			vsyslog(level, format, vlist);
			va_end(vlist);
			closelog();
		}

		sigprocmask(SIG_UNBLOCK, &block_chld, NULL);
	}
}
