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


#ifndef _DEBUG_H_
#define _DEBUG_H_


/** @brief Used to output messages.
 *The messages will include the finlname and line number, and will be sent to syslog if so configured in the config file
 */
#define debug(level, format...) _debug(__BASE_FILE__, __LINE__, level, format)

/** @internal */
void _debug(char *filename, int line, int level, char *format, ...);

#endif /* _DEBUG_H_ */
