#ifndef __EPOWER_PUB_H
#define __EPOWER_PUB_H

#define DEVMNUM_MIN 1
#define DEVMNUM_MAX 8

#define MAC_LEN 6
#define SAMPLE_DATA_LEN 1440
#define SEND_MSG_LEN 1456

#define SHMADDR    0x60000000
#define SHM_IN     0x01
#define SHM_OUT	   0x02

#ifndef bool
#define bool int
#endif

#define false 0
#define true  1

#pragma pack(push)
#pragma pack(4)

typedef struct stBoard_info{
	int serial; /*1,2...8 : board serial; max=8*/
	int route;  /*1,2...16: corresponding route serial; max=16*/
	int route_status; /*corresponding route status*/
	int device_type; /* 0: input; 1: output*/
}Board_info;

typedef struct stADC_DevCfg{
	short shm1_flag;
	short shm2_flag;
	char *shm1_data;
	char *shm2_data;
	int status; /*0: close, 1: open*/
	Board_info *board_info;/*corresponding board info*/
	char rsvd[2]; /*reserve*/
	unsigned char opposite_mac[6]; //opposite mac address
}ADC_DEV_CFG;

typedef struct stDevice_Cfg{
	short global_shm1flag;
	short global_shm2flag;
	unsigned char *global_shm1data;
	unsigned char *global_shm2data;
	int usDevCnt;
	int usRunCnt;
	ADC_DEV_CFG *pstAdcCfg;
}DEVICE_CFG;
#pragma pack(pop)

int Sample_init(int devnum,int level,DEVICE_CFG **);
int Sample_open(void);
int Sample_read(void);
int Sample_write(int serial,int route,int cmd);
int Sample_stop(void);
int Sample_close(void);

extern pthread_mutex_t shm1lk;
extern pthread_mutex_t shm2lk;
extern pthread_mutex_t *dev1lk;
extern pthread_mutex_t *dev2lk;

#endif // EPOWER_PUB_H
