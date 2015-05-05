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

#ifndef _HACKCLI_H_
#define _HACKCLI_H_

#define DEFAULT_SOCK	"/tmp/hack.sock"

#define HACK_TERMINATOR	"\r\n\r\n"

#define HACK_UNDEF		0
#define HACK_STATUS		1


typedef struct {
	char	*socket;
	int	command;
	char	*param;
} s_config;


#endif /* _NDSCTL_H_ */
