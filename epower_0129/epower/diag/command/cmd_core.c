#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <stddef.h>
#include <stdbool.h>
#include "global.h"
#include "cmds.h"
#include "fd.h"

#define START_CMD_STR "cmd"
#define PARAM_LENGTH 1024
#define MAX_CMD_NUM 16
#define MAX_CMD_ARG_LEN 64
#define MAX_CMD_HIS_NUM 10
//#define CMD_ONLINE cmd_online

struct cmd_struct{
	struct cmd_struct *pre;
	struct cmd_struct *next;
	size_t cur;
	char str[PARAM_LENGTH];
};

static struct cmd_buf{
	int num;
	char *array[MAX_CMD_NUM];
}cmd_array;

//static bool cmd_online;
//int global_print_dbg;
int cmd_hist_sum = 0;
struct cmd_struct *current_cmd=NULL;
struct cmd_struct *cmd_hist_cur=NULL;
LIST_HEAD(cmd_list_head);

int add_new_cmd(const char *name, int max,CMD_ROUTINE routine,const char *help)
{
	struct cmd_interaction *new_cmd;
	if(name == NULL || help == NULL || routine == NULL)
	{
		PRINT(DBG_ERROR,"received an error address !");
		return -1;
	}
	if(max > MAX_CMD_NUM)
	{
		PRINT(DBG_ERROR,"cmd max arguments 16!");
	}
	
	new_cmd = calloc(1,sizeof(struct cmd_interaction));
	if(new_cmd == NULL)
	{
		PRINT(DBG_ERROR, "memory allocation failed!");
		return -1;
	}
	memcpy(new_cmd->name,name,(strlen(name)<CMD_NAME_LENGTH)?strlen(name): CMD_NAME_LENGTH-1);
	memcpy(new_cmd->help,help,(strlen(help)<CMD_MAX_HELP_LEN)?strlen(help): CMD_MAX_HELP_LEN-1);
	new_cmd->max_param = max;
	new_cmd->routine = routine;
	PRINT(DBG_INFO,"added a new cmd:%s ",new_cmd->name);

	list_add_tail(&new_cmd->list, &cmd_list_head);

	return 0;
}

int parse_cmd(int argc, char *argv[])
{
	int i;
	struct cmd_interaction *cmd=NULL;
	bool match=false;
	PRINT(DBG_INFO,"(%d)",argc);
	for(i=0;i<argc;i++)
		PRINT(DBG_INFO,",%s",argv[i]);
	
	list_for_each_entry(cmd, &cmd_list_head, list)
	{
		if(!strcmp(cmd->name,argv[0]))
		{
			cmd->routine(argc,argv);
			match = true;
			break;
		}
	}
	if(!match)
		printf("command not found!\n");
	return 0;
}


static int term_set(void)
{
	struct termios ori_attr, cur_attr;
	if ( tcgetattr(STDIN_FILENO, &ori_attr) )
		return -1;
	
	memcpy(&cur_attr, &ori_attr, sizeof(cur_attr) );
	cur_attr.c_lflag &= ~ICANON;
	cur_attr.c_lflag &= ~(ECHO | ECHOE);
	cur_attr.c_cc[VMIN] = 1;

	if (tcsetattr(STDIN_FILENO,
				TCSANOW, &cur_attr) != 0)
		return -1;

	return 0;
}

static int term_clear(void)
{
	struct termios ori_attr, cur_attr;
	if ( tcgetattr(STDIN_FILENO, &ori_attr) )
		return -1;

	memcpy(&cur_attr, &ori_attr, sizeof(cur_attr) );
	cur_attr.c_lflag |= ICANON;
	cur_attr.c_lflag |= (ECHO | ECHOE);
	cur_attr.c_cc[VMIN] = 0;

	if (tcsetattr(STDIN_FILENO,
				TCSANOW, &cur_attr) != 0)
		return -1;

	return 0;
}

int alloc_term()
{
	current_cmd = calloc(1,sizeof(struct cmd_struct));
	if(current_cmd == NULL)
	{
		fprintf(stderr,"init_term failed!\n");
		return -1;
	}
	current_cmd->pre = current_cmd;
	current_cmd->next = current_cmd;
	cmd_hist_cur = current_cmd;
	term_set();
	return 0;
}

void del_cmd(struct cmd_struct *cmd)
{
	if(cmd == NULL)
	{
		fprintf(stderr,"del_cmd failed!\n");
		return;
	}
	cmd->pre->next = cmd->next;
	cmd->next->pre = cmd->pre;
	free(cmd);
}

static int add_cmd(const char *str)
{
	struct cmd_struct *cmd;
	cmd_hist_cur = current_cmd;
	if(cmd_hist_sum >= MAX_CMD_HIS_NUM)
		del_cmd(current_cmd->pre);
	else if((current_cmd->next != current_cmd)&&
		!strcmp(str,current_cmd->next->str))
		return 0;
	else
		cmd_hist_sum++;

	cmd = calloc(1,sizeof(struct cmd_struct));
	if(cmd == NULL)
	{
		fprintf(stderr,"add_cmd failed,malloc failed\n");
		return -1;
	}
	current_cmd->next->pre = cmd;
	cmd->next = current_cmd->next;
	current_cmd->next = cmd;
	cmd->pre = current_cmd;
	cmd->cur = strlen(str);
	memcpy(cmd->str, str, cmd->cur+1);
	return 0;
}

void print_char(const char *p)
{
	write(1,p,1);
}

void print_str(char *p, size_t size)
{
	write(1,p,size);
}

static void print_prompt()
{
	write(1,"->",2);
}

void control_del()
{
	print_char("\b");
	print_char(" ");
	print_char("\b");

}

void history_forw()
{
	int i;
	if(cmd_hist_cur->next == current_cmd)
		return;
	
	for(i=0;i<cmd_hist_cur->cur;i++)
		control_del();
	cmd_hist_cur = cmd_hist_cur->next;
	print_str(cmd_hist_cur->str,cmd_hist_cur->cur);
	memcpy(current_cmd->str,cmd_hist_cur->str,PARAM_LENGTH);
	current_cmd->cur = cmd_hist_cur->cur;
}

void history_back()
{
	int i;
	if(cmd_hist_cur == current_cmd)
		return;

	for(i=0;i<cmd_hist_cur->cur;i++)
		control_del();
	cmd_hist_cur = cmd_hist_cur->pre;
	print_str(cmd_hist_cur->str,cmd_hist_cur->cur);
	memcpy(current_cmd->str,cmd_hist_cur->str,PARAM_LENGTH);
	current_cmd->cur = cmd_hist_cur->cur;

}

void show_comp(const char *str, size_t size)
{
	CMD_INTERACTION cmd=NULL;
	CMD_INTERACTION found=NULL;
	int match_num=0;
	list_for_each_entry(cmd,&cmd_list_head,list){
		if(!strncmp(str,cmd->name,size))
		{
			match_num++;
			found = cmd;
		}
	}
	if(match_num == 1)
	{
		print_str(&found->name[size],strlen(&found->name[size]));
		memcpy(current_cmd->str,found->name,strlen(found->name));
		current_cmd->cur = strlen(found->name);
	}
	else if(match_num > 1)
	{
		puts("");
		list_for_each_entry(cmd,&cmd_list_head,list){
			if(!strncmp(str,cmd->name,size))
			{
				print_str(cmd->name,strlen(cmd->name));
				print_char(" ");
			}
		}
		puts("");
		print_prompt();
		print_str(current_cmd->str,current_cmd->cur);
	}
}

void process_cmd(struct cmd_struct *cmd)
{
	int num=0,size=0,i,end=0;
	char *p = cmd->str,*c=NULL;
	cmd->str[cmd->cur] = '\0';
	add_cmd(cmd->str);
/*
	if(!CMD_ONLINE)
	{
		if(!strcmp(cmd->str,START_CMD_STR))
		{
			cmd_online = true;
		}
		return;
	}
*/
	while(!end)
	{
		while(*p == ' ')
			p++;
		c = strchr(p, ' ');
		if(c == NULL)
		{
			if(*p == '\0')
				break;
			c = strchr(p, '\0');
			if(c == NULL)
				fprintf(stderr,"unkown error!\n");
			else
				end = 1;
		}
		size = c-p;
		cmd_array.array[num] = calloc(1,MAX_CMD_ARG_LEN);
		if(cmd_array.array[num] == NULL)
		{
			fprintf(stderr,"process_cmd malloc failed!\n");
			goto malloc_failed;
		}
		memcpy(cmd_array.array[num],p,size);
		cmd_array.array[num][size] = '\0';
		num++;
		p = c;
		if(num >= MAX_CMD_NUM)
		{
			fprintf(stderr,"too much args\n");
			end = 1;
		}
	}
	cmd_array.num = num;

	parse_cmd(cmd_array.num,&cmd_array.array[0]);

malloc_failed:
	for(i=0;i<num;i++)
		free(cmd_array.array[i]);
}

int wait_cmd(char *str, int count)
{
	char *p = str;
	struct cmd_struct *cmd = current_cmd;
	for(p=str;count>0;count--,p++)
	{
		switch (*p){
		case '\n':
			print_char(p);
			if(cmd->cur == 0)
			{
				print_prompt();
				break;
			}
			process_cmd(cmd);
			print_prompt();
			cmd->cur = 0;
			memset(cmd->str,0,PARAM_LENGTH);
			break;
		case 27:
			if(*(++p) == 91)
			{
				count--;
				switch (*(++p)){
				case 65:
					count--;
					history_forw();
					break;
				case 66:
					count--;
					history_back();
					break;
				}
			}
			break;
		case 127:
		case 8:
			if(cmd->cur > 0)
			{
				control_del(cmd);
				cmd->cur--;
			}
			
			break;
		case 9:
			show_comp(cmd->str,cmd->cur);
			break;

		default:
			if(cmd->cur >= PARAM_LENGTH-2)
			{
				cmd->cur = 0;
				return -1;
			}
			*(cmd->str+cmd->cur) = *p;
			cmd->cur++;
			print_char(p);
			PRINT(DBG_INFO,"%d",*p);
		}
	}
	return 0;
}

void cli_parse(void *arg)
{
	POLL_FD *f = (POLL_FD *)arg;
	int count;
	char buf[128];

	memset(buf, 0, 128);
	count = read(f->fd, buf, 128-1);
	if(count >= 128-2)
	{
		fprintf(stderr,"length of param over the buffer!");
		return ;
	}

	if(wait_cmd(buf, count))
	{
		fprintf(stderr,"command out of range!\n");
	}
	
}

void quit_routine(int argc, char *argv[])
{
//	cmd_online = false;
}

void print_cmd_help()
{
	CMD_INTERACTION cmd;
	printf("----------------help---------------\n");
	list_for_each_entry(cmd,&cmd_list_head,list){
		printf("%-20s",cmd->name);
		printf("#%s\n",cmd->help);
	}
	printf("-----------------------------------\n");
}

void add_common_cmd()
{
	add_new_cmd("quit",1,quit_routine,"#quit help");
	add_new_cmd("help",1,print_cmd_help,"#print this");
}

void free_term()
{
	if(current_cmd == NULL)
		return;
	free(current_cmd);
	
	CMD_INTERACTION cmd;
	CMD_INTERACTION tmp;
	list_for_each_entry_safe(cmd, tmp, &cmd_list_head, list){
		list_del(&cmd->list);
		free(cmd);
	}
}


int init_term()
{
	if(alloc_term())
	{
		PRINT(DBG_ERROR,"alloc_term failed!");
		return -1;
	}
	add_common_cmd();
	add_cmd_set();
	return addFdNode(0,cli_parse,NULL);
}

void release_term()
{
	free_term();
	term_clear();
}

int get_hist_record_num()
{
	return cmd_hist_sum;
}
