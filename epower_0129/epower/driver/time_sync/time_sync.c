/* 
 * TTY testing utility (using tty driver) - Copyright (C) 2020 WCH Corporation.
 * Author: TECH39 <zhangj@wch.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I /path/to/cross-kernel/include
 *
 * Version: V1.1
 * 
 * Update Log:
 * V1.0 - initial version
 * V1.1 - add hardflow control
 		- add sendbreak
 		- add uart to file function
		- VTIME and VMIN changed
 */
#include "time_sync.h"


/**
* @brief  all 7606 and local time sync 
* @param  void
* @return int ,0: success,other values: failed
* @retval
*/ 
int time_sync()
{
	//response DP83640 interrupt event
	//get local time and send to 7606 with uart
	//

}