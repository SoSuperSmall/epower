#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "cmds.h"

#define FIFO_HELP "dump-send/dump-recv"

void fifo_cmd(int argc, char *argv[])
{
	printf("fifo cmd test %d \r\n",argc);
	
}

void add_cmd_set()
{
	add_new_cmd("fifo",3,fifo_cmd,FIFO_HELP);
}


