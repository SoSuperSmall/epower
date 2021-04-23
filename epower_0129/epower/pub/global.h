#ifndef __GLOBAL_H
#define __GLOBAL_H

extern unsigned long global_print_dbg;

#define PROGRAM_NAME "epower_app"
#define VERSION "1.0.0"

typedef enum enDbg_Level{
	DBG_STDO = 0, /* standard output */
	DBG_INFO = 1,
	DBG_ERROR,
	DBG_WARN,
	DBG_BUTT
}DBG_LEVEL;

#define PRINT(mod,fmt,args...) \
	do{\
		if((1<<(mod)) & (global_print_dbg))  \
		{\
			if (DBG_INFO ==(mod)) {\
				fprintf(stdout,"[INFO] "fmt"\n",##args); \
			} else if (DBG_ERROR ==(mod)) {\
				fprintf(stderr,"[%s] FILE:%s:%d:%s() "fmt"\r\n","ERROR",__FILE__,__LINE__,__FUNCTION__,##args);\
			} else if (DBG_WARN ==(mod)) {\
				fprintf(stdout,"[WARN] "fmt"\n",##args); \
			} else {\
				fprintf(stdout,fmt,##args);\
			}\
		}\
	}while(0)
	
typedef struct stCmd_Infos{
	char cmds[32];
	char helps[64];
	unsigned short cmdsize;
	unsigned short cmdkey;
}CMD_INFO_S;
			
#endif
