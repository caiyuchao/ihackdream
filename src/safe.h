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


#ifndef _SAFE_H_
#define _SAFE_H_


#include <stdarg.h> /* For va_list */
#include <sys/types.h> /* For fork */
#include <unistd.h> /* For fork */


/** @brief Safe version of malloc
 */
void * safe_malloc (size_t size);

/* @brief Safe version of strdup
 */
char * safe_strdup(const char *s);

/* @brief Safe version of asprintf
 */
int safe_asprintf(char **strp, const char *fmt, ...);

/* @brief Safe version of vasprintf
 */
int safe_vasprintf(char **strp, const char *fmt, va_list ap);

/* @brief Safe version of fork
 */

pid_t safe_fork(void);


#endif /* _SAFE_H_ */
