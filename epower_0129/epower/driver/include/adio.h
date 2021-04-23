#ifndef __ADIO_H
#define __ADIO_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <memory.h>
#include <pthread.h>
#include <sys/ioctl.h>	
#include <sys/ipc.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h> 
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <errno.h>
#include <netinet/ether.h>
#include <net/if.h>
 #include <sched.h>
#include <sys/time.h>
#include <fcntl.h>
#include "epower_pub.h"

extern int stop_sample_flag;
#define SAMPLE_BUFF_LEN 2880
#define BUFF_DEEPTH     2

#define SAMPLE_FLAG_SET(x) do{ stop_sample_flag=x; }while(0)
#define SAMPLE_FLAG_GET(ret) do{ ret=stop_sample_flag; }while(0)
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(x) ((char*)(x))[0],((char*)(x))[1],((char*)(x))[2],((char*)(x))[3],((char*)(x))[4],((char*)(x))[5]


int read_data_pthread(void);
int shm_read();
pthread_t pthread_get_data;

#endif // ADIO_H
