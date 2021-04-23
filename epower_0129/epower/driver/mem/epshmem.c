#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>

//set your memory addr,please refer to shared-mem address
#define SHMADDR 0x60000000

static void * shmaddr = NULL;
static int shmid;

/*kernel support we don't care the status*/
void *alloc_shared_mem(int key,void *addr_start, unsigned int memsize)
{
/*we can get shm memory any time don't care shm status*/
	shmid = shmget(key,memsize,IPC_CREAT|0666);
	if(shmid < 0)
	{
		perror("shmget");
		return (void*)-1;
	}
	if(addr_start == NULL)
		shmaddr = shmat(shmid,NULL,0);
	else
		shmaddr = shmat(shmid,(const void *)addr_start,SHM_RND|SHM_REMAP);
	if(shmaddr == (void *)-1)
	{
		perror("shmat");
		return (void*)-1;
	}

	return shmaddr;
}

/*remap shared-memory to addr_start(virtual address in current memory space)*/
void *allocated_shared_mem(int key,void *addr_start, unsigned int memsize)
{
/*we can get shm memory any time don't care shm status*/
	shmid = shmget(key,memsize,IPC_EXCL);
	if(shmid < 0)
	{
		perror("shmget");
		return (void*)-1;
	}
	if(addr_start == NULL)
		shmaddr = shmat(shmid,NULL,0);
	else
		shmaddr = shmat(shmid,(const void *)addr_start,SHM_RND|SHM_REMAP);
	if(shmaddr == (void *)-1)
	{
		perror("shmat");
		return (void*)-1;
	}

	return shmaddr;
	
}

int free_shared_mem(void* shmaddr)
{
	if(shmdt(shmaddr) == -1)  
	{  
		//perror("shmdt");  
		return errno;
	}  

	if(shmctl(shmid, IPC_RMID, 0) == -1)  
	{  
		perror("shmctl");  
		return errno;
	} 
	return 0;
}

