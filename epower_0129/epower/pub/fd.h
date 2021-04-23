#ifndef __INC_FD_H
#define __INC_FD_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FD		 	16

#define ACTIVE_NOW		1
#define ACTIVE_LATER		0

typedef struct
{
	int active;
   	int		fd;
	void	(*read)(void *);
   	void	*arg;
}POLL_FD;

extern void initFdNode(void );
extern int addFdNode(int fd,void (*func)(void *),void *arg);
extern void delFdNode(int fd);
extern void fds_select(void);


#ifdef __cplusplus
}
#endif

#endif  /*__INC_FD_H*/

