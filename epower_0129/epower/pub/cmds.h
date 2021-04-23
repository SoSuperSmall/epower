#ifndef __CMDS_H
#define __CMDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#define CMD_NAME_LENGTH 64
#define CMD_MAX_HELP_LEN 1024

typedef void(* CMD_ROUTINE)(int argc, char *argv[]);

struct cmd_interaction{
	int max_param;
	CMD_ROUTINE routine;
	struct list_head list;
	char name[CMD_NAME_LENGTH];
	char help[CMD_MAX_HELP_LEN];
};

typedef struct cmd_interaction* CMD_INTERACTION;

extern int init_term();
extern void release_term();
extern int add_new_cmd(const char *name, int max,CMD_ROUTINE routine,const char *help);

extern void add_cmd_set(void);


#ifdef __cplusplus
}
#endif

#endif

