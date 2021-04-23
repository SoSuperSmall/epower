#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include "fd.h"
#include "global.h"

#define POLL_TIME_OUT 100

static POLL_FD	   fdt[MAX_FD];
static int    global_poll_fd_num=0;
static struct pollfd global_poll_fds[MAX_FD];

void initFdNode()
{
	memset(fdt,0,MAX_FD*sizeof(POLL_FD));
}

static void updateFdSet()
{
    int i;
    
    global_poll_fd_num=0;
    for (i=0;i<MAX_FD;i++)
    {
    	if(fdt[i].active)
    	{
	        global_poll_fds[global_poll_fd_num].fd = fdt[i].fd;
	        global_poll_fds[global_poll_fd_num].events = POLLIN;
	        global_poll_fd_num++;
	    }
    }

	PRINT(DBG_INFO,"There are %d FD nodes ready!",global_poll_fd_num);
}

int addFdNode(int fd,void (*func)(void *),void *arg)
{
	int i;
	for(i=0; i<MAX_FD; i++){
		if(fdt[i].active == ACTIVE_LATER){
		    fdt[i].fd=fd;
		    fdt[i].read=func;
		    fdt[i].arg=arg;
			fdt[i].active = ACTIVE_NOW;
			PRINT(DBG_INFO,"add fd:%d node",fd);
			updateFdSet();
			return 0;
		}
	}
    return -1;
}

void delFdNode(int fd)
{
	int i;
	for(i=0; i<MAX_FD; i++){
		if(fdt[i].fd == fd){
			fdt[i].active = ACTIVE_LATER;
			memset(&fdt[fd],0,sizeof(POLL_FD));
			updateFdSet();
			break;
		}
	}
}

void fds_select()
{
	POLL_FD *fdt_ready;
    int i,j,fd_num;
    struct pollfd poll_fd;

	/*if just include POLLIN event,no need to create a new array of FD,
	 *cause poll won't change pollfd.but it is not approprite in here.
	 */
	fd_num=poll(global_poll_fds, global_poll_fd_num, POLL_TIME_OUT);  //timeout 100ms
	if(fd_num==0)
		return;  //if no events happen in 100 ms,return.
    if(fd_num<0 && errno!=EAGAIN && errno!=EINTR)  //non-block read cause EAGAIN,EINTR introduced by signal
    {
		/*some thing wrong in poll,I will find the wrong fd and delete it*/
    	perror("Note:poll wrong!");
    	for(i=0;i<MAX_FD;i++)
    	{
    		if(fdt[i].active==ACTIVE_NOW)
    		{
	    		poll_fd.fd=fdt[i].fd;
				poll_fd.events=POLLIN;
				fd_num=poll(global_poll_fds,1,0);
	    		if((fd_num<0) && ((errno==EBADF)||(errno==EINVAL)))
	    		{
					delFdNode(fdt[i].fd);
					fprintf(stderr,"delete fd node:%d,because of ERRNO:%d\n",poll_fd.fd,errno);
	    		}
	    	}
    	}
		/*notice the reason why something wrong happen in poll!*/
    	return;
    }
    for(i=0; i<fd_num; i++)
 	{
		if(global_poll_fds[i].revents & POLLIN) 
		{
			for(j=0; j<MAX_FD; j++)
			{
				if(fdt[j].fd == global_poll_fds[i].fd){
					fdt_ready = &fdt[j];
					if(fdt_ready->read != NULL){
						fdt_ready->read(fdt_ready);
					}
					break;
				}
			}
			if(j >= MAX_FD)
			{
				fprintf(stderr,"FD:%d have no compatible fd!\n", global_poll_fds[i].fd);
			}
		}
	}

}


