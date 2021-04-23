/* System headers */
#include <stdio.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

/* public */
#include "global.h"
#include "epower_pub.h"

/*sample procedure inlcude*/
#include "pthread_uart_show.h"
/* Pirvate Header. */

/* global variable*/
/*!< @brief Global print switch 
* global_print_dbg, switch op base on bit
* bit value 1: ON; 0 : OFF
* bit 0 : Enable or disable; DBG_STDO: standard out
* bit 1 : DBG_INFO
* bit 2 : DBG_ERROR
* bit 3 : DBG_WARN
*/
DEVICE_CFG *gpst_Dev;
unsigned long global_print_dbg = 0x01;
//unsigned long global_print_dbg = 0x03;   //printf DBG_INFO;
unsigned char *send_info=NULL;
float MaxVol=5;
float realVol=0;
pthread_t pthread_getdata;
int shm_read();
CMD_INFO_S gparr_cmds[] = {
	{"list","list all sample devices",4,0},
	{"show","show current config",4,0},
	{"init","init all sample devices",4,0},
	{"exit","exit current test",4,0},
	{"open","open all sample devices",4,0},
	{"close","close all sample devices",5,0},
	{"read1","read every sample once time",5,0},
	{"read2","read every sample continuously",5,0},
	{"run1","open and run all sample device continuously",4,0},
	{"debug","debug print switch, with ' on' or ' off' ",5,0},
	{"quit","quit application !",4,0}};

bool cmd_proc(int argc, char *argv[])
{
	bool brt = true;
	int r = -1;
	char cmds[32] = "\0";
	int t; 
	short usIndex = 0;
	short usRunCnt = 0;
	long *deltaTime=NULL;
	deltaTime=(long *)malloc(8);
	send_info=(unsigned char *)malloc(1440);
	printf("#");
	if (NULL == fgets(cmds,32,stdin))
	{
		return brt;	
	}
	
	if (0 == strncmp(cmds,"list",4))
	{
		Sample_devs_get(NULL);	
	}
	else if (0 == strncmp(cmds,"debug",5))
	{
		if (0 == strncmp(cmds+6,"on",2))
		{
			global_print_dbg |= 0xE;			
		}
		else
		{
			global_print_dbg = 0x01;
		}
		PRINT(DBG_STDO,"debug switch is %08lX \n",global_print_dbg);
	}
	else if (0 == strncmp(cmds,"show",4))
	{
		Sample_info_get();	
	}
	else if (0 == strncmp(cmds,"init",4))
	{
		int a = 0, b = 0;
		PRINT(DBG_STDO,"input device number(1~8):\n");
		while(scanf("%d",&a) != 1) {
			PRINT(DBG_ERROR," you should input 1~8");
		}
		PRINT(DBG_STDO,"input libusb debug level(0~4):\n");
		while(scanf("%d",&b) != 1) {
			PRINT(DBG_ERROR," you should input 0~4");
		}
		r=Sample_init(a,b,&gpst_Dev);
		PRINT(DBG_STDO," call sample init ret %d ",r);
	}
	else if (0 == strncmp(cmds,"exit",4))
	{
		r = Sample_exit();	
		PRINT(DBG_STDO," call sample exit ret %d ",r);
	}
	else if (0 == strncmp(cmds,"open",4))
	{
		r = Sample_open();
		PRINT(DBG_STDO," Sample open ret %d ",r);
	}
	else if (0 == strncmp(cmds,"read",4))
	{
		pthread_create(&pthread_getdata,0,shm_read,NULL);
		r=Sample_read();
		PRINT(DBG_STDO,"Sample read ret %d ",r);
	}
	else if (0 == strncmp(cmds,"write",5))
	{
		r=Sample_write(8,1,1);
		PRINT(DBG_STDO,"Sample write ret %d",r);
	}
	else if (0 == strncmp(cmds,"close",5))
	{
		r = Sample_close();
		PRINT(DBG_STDO," Sample close ret %d ",r);
	}
	else if (0 == strncmp(cmds,"quit",4))
	{
		brt = false;			
	}
	else
	{
		PRINT(DBG_ERROR,"unknown cmd:%s (%ld)",cmds,strlen(cmds));		
	}
	printf("\r\n");

	return brt;
}

int main(int argc, char *argv[])
{	
	bool bloop = true;
	sleep(1);
	printf("\r\n Welcome ePower platform...");
	printf("\r\n plese input command: \r\n");	
	
	while(bloop)
	{
		bloop = cmd_proc(argc, argv);
	}
	return 0;	
}
/*
get sample data from share memory
*/
int shm_read()  
{
	int i=0,j=0,r=0;
	int gus_cycles=0;
	int cycle=0;
	short usIndex = 0;
	short usRunCnt = 0;
	unsigned char *tmpdata=NULL;
	int data_flag=0;
   	int count=0;
	short v=0;
    char sztmp[50]={0};
    float realtmp=0;
	float rtmp=0;
	int sampledata=0;
	unsigned char tmpmac[6];
	ADC_DEV_CFG *pstAdcCfg = NULL; 
	ADC_DEV_CFG *pstTemp = NULL; 
	long *deltaTime=NULL;
	//memcpy(tmp_buff,read_buff->read_buff,sizeof(read_buff));
	//tmp_buff=(WORD)read_buff->read_buff;
	tmpdata=(unsigned char *)malloc(1440);
	deltaTime=(long *)malloc(8);
	time_t tNow =time(NULL);  
	usRunCnt = gpst_Dev->usRunCnt;
	pstAdcCfg = gpst_Dev->pstAdcCfg;
	while(1){
		//PRINT(DBG_STDO,"-----------");
		for (usIndex = 0; usIndex < usRunCnt; usIndex++)
		{
			//pstTemp = &(pstAdcCfg[usIndex]);
			pthread_mutex_lock (&(dev1lk[usIndex]));
			if(pstAdcCfg[usIndex].shm1_flag==0x02){
				//printf("read_shm1  flag======%x\n",pstAdcCfg[usIndex].shm1_flag);
				//pthread_mutex_lock(&shm1_mutex[usIndex]);
				memcpy(tmpdata,pstAdcCfg[usIndex].shm1_data,1440);
				pstAdcCfg[usIndex].shm1_flag=0x01;	
				memcpy(&data_flag,tmpdata,4);
				memcpy(&cycle,tmpdata+4,4);
//*/			
				printf("Sample Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
						pstAdcCfg[usIndex].opposite_mac[0], pstAdcCfg[usIndex].opposite_mac[1], pstAdcCfg[usIndex].opposite_mac[2], pstAdcCfg[usIndex].opposite_mac[3], \
						pstAdcCfg[usIndex].opposite_mac[4], pstAdcCfg[usIndex].opposite_mac[5]);
				printf("data_flag -----0x%x\r\n",data_flag);
				printf("sample cycle -----%d\r\n",cycle);
				//for(int i = 0; i < 16;i++)
				//{
					memcpy(&v,&tmpdata[14],2);
					PRINT(DBG_STDO,"sample voltage ====%x\r\n",v);
					if(v&0x8000)
					{
						v= ~v;
						realVol = -1 * MaxVol * (v+1) / 32768;
						PRINT(DBG_STDO,"\r\n 负数");				
					}else{
						realVol = MaxVol * v / 32768;
						PRINT(DBG_STDO,"\r\n 正数");
					}
					PRINT(DBG_STDO,"v4电压值为 ==%2.6fv  ",realVol);
					//PRINT(DBG_STDO,"v1电liu值为 ==%2.6fA  ",(realVol/140)*2000);
					PRINT(DBG_STDO,"\n");
				//}
				memcpy(deltaTime,&tmpdata[(i*48)+40],8);
				//printf("deltaTime==%ldus\r\n",*deltaTime);
				//printf("read_shm1  flag======%x\n",pstAdcCfg[usIndex].shm1_flag);
				//pthread_mutex_unlock(&shm1_mutex[usIndex]);
//*/				PRINT(DBG_INFO,"pstAdcCfg[%d]  read_shm1 end\n",usIndex);
				PRINT(DBG_INFO,"read_shm1  flag======%x\n",pstAdcCfg[usIndex].shm1_flag);
				pthread_mutex_unlock (&(dev1lk[usIndex]));
			}else
			{
				pthread_mutex_unlock (&(dev1lk[usIndex]));
				pthread_mutex_lock (&dev2lk[usIndex]); 
				if(pstAdcCfg[usIndex].shm2_flag==0x02){
				//printf("read_shm2   flag======%x\n",pstAdcCfg[usIndex].shm2_flag);
				//pthread_mutex_lock(&shm2_mutex[usIndex]);
				memcpy(tmpdata,pstAdcCfg[usIndex].shm2_data,1440);
				pstAdcCfg[usIndex].shm2_flag=0x01;
///*	
				printf("Sample Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
						pstAdcCfg[usIndex].opposite_mac[0], pstAdcCfg[usIndex].opposite_mac[1], pstAdcCfg[usIndex].opposite_mac[2], pstAdcCfg[usIndex].opposite_mac[3], \
						pstAdcCfg[usIndex].opposite_mac[4], pstAdcCfg[usIndex].opposite_mac[5]);
				memcpy(&data_flag,tmpdata,4);
				memcpy(&cycle,tmpdata+4,4);
				printf("data_flag -----0x%x\r\n",data_flag);
				printf("sample cycle -----%d\r\n",cycle);
				//for(int i = 0; i < 16;i++)
				//{
					memcpy(&v,&tmpdata[14],2);
					PRINT(DBG_STDO,"sample voltage ====%x\r\n",v);
					if(v&0x8000)
					{
						v= ~v;
						realVol = -1 * MaxVol * (v+1) / 32768;
						PRINT(DBG_STDO,"\r\n 负数");				
					}else{
						realVol = MaxVol * v / 32768;
						PRINT(DBG_STDO,"\r\n 正数");
					}
					PRINT(DBG_STDO,"v4电压值为 ==%2.6fv  ",realVol);
					//PRINT(DBG_STDO,"v1电liu值为 ==%2.6fA  ",(realVol/140)*2000);
					PRINT(DBG_STDO,"\n");
				//}
				memcpy(deltaTime,&tmpdata[(i*48)+40],8);
				//printf("deltaTime==%ldus\r\n",*deltaTime);
				//printf("read_shm2   flag======%x\n",pstAdcCfg[usIndex].shm2_flag);
//*/				//pthread_mutex_lock(&shm2_mutex[usIndex]);
				PRINT(DBG_INFO,"pstAdcCfg[%d]  read_shm2 end\n",usIndex);
				PRINT(DBG_INFO,"read_shm2   flag======%x\n",pstAdcCfg[usIndex].shm2_flag);
				}
				pthread_mutex_unlock (&dev2lk[usIndex]);
			}
		}
	}
	return 0;
}

