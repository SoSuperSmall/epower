#define _GNU_SOURCE   
#include "global.h"
//#include "basetype.h"
#include "adio.h"
#include "epower_pub.h"
#include "epshmem.h"
#define MAXINTERFACES 16

//big shm lock
pthread_mutex_t shm1lk;
pthread_mutex_t shm2lk; 
//device shm lock
pthread_mutex_t *dev1lk = NULL;
pthread_mutex_t *dev2lk = NULL;

DEVICE_CFG *gpst_DevCfg=NULL;
short gw_DevCnt = 0; 
short gw_DevIdx = 0;
bool gb_sampleinit = false;
int read_pthread_start=0;
int receive_pthread_start=0;
int sock_raw_fd;     
cpu_set_t cpuset;  
int num=0;
struct sockaddr_ll client; 
struct sockaddr_ll sll; 
struct ifreq req;       
pthread_mutex_t  shm1_mutex[8];
pthread_mutex_t  shm2_mutex[8];
int receive_data_flag,shmdata1_flag,shmdata2_flag;    
//unsigned int tmpdata=0;
struct timeval tBegin, tEnd;
unsigned char mac_list[8][6]={
	{0xd4,0xbe,0xd9,0x45,0x22,0x61},
	{0xd4,0xbe,0xd9,0x45,0x22,0x62},
	{0xd4,0xbe,0xd9,0x45,0x22,0x63},
	{0xd4,0xbe,0xd9,0x45,0x22,0x64},
	{0xd4,0xbe,0xd9,0x45,0x22,0x65},
	{0xd4,0xbe,0xd9,0x45,0x22,0x66},
	{0xd4,0xbe,0xd9,0x45,0x22,0x67},
	{0xd4,0xbe,0xd9,0x45,0x22,0x68},
};

pthread_t pthread_receive_data;
unsigned char send_msg[1456] = {
        //--------------组MAC--------16------
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //dst_mac: ff-ff-ff-ff-ff-ff目的地址
        0x00, 0x0c, 0x29, 0xb0, 0xd9, 0x50, //src_mac: 00:f1:f5:13:06:0b源mac地址
        0x08, 0x66, 0x05, 0xa0,                       //类型：0x0800 IP协议
};

void *receive_pthread(void);
void error_and_die(const char *msg) {
     perror(msg);
     exit(EXIT_FAILURE);
}

static void Sample_devs_print(int num)
{

}

/**
* @brief Get all ethernet device list
* 
* @param void* pst, not using now
* @return < 0 : no ethernet device, =0: success 
*           
* @retval 
*/
int Sample_devs_get(void *pst)
{

	return 0;
}
/**
* @brief Get sample configuration 
* 
* @param void 
* @return void 
*           
* @retval 
*/
void Sample_info_get(void)
{
	DEVICE_CFG * pstTemp = NULL;
	ADC_DEV_CFG *pstAdc = NULL;
	
	printf("============================\r\n");
	if (NULL != gpst_DevCfg)
	{
		pstTemp = gpst_DevCfg ;
		if ((NULL == pstTemp->pstAdcCfg))
		{
			printf(" no init for device context or config \r\n");		
		}
		else
		{
			printf(" Initialized Device Cnt %d \r\n",pstTemp->usDevCnt);
			printf(" Running Device Cnt %d \r\n",pstTemp->usRunCnt);
			printf(" adconfig %p \r\n",pstTemp->pstAdcCfg);
			
			printf("-------- ADC CFG List --------\r\n");
			for (int i = 0; i < pstTemp->usRunCnt; i++)
			{
				pstAdc = (pstTemp->pstAdcCfg)+i;	
				printf("+++++++++\r\n");
				printf("ADC Device[%d]:\r\n",i);
				/*printf("RxData: %p [flag %d, size %d ]\r\n",
				pstAdc->pstRxData,pstAdc->pstRxData->wflag,pstAdc->pstRxData->rxsize);
				printf("TxData: %p \r\n",pstAdc->pstTxData);	*/			
			}
		}
	}
	else
	{
		printf(" no init for device  \r\n");	
	}
}
/**
* @brief  Initialize a specified number of devices 
* 
* @param ADC_DEV_CFG *pstCfg : ADC Config 
* @param int devnum :number of device to be iSample_initnit
* @param int level: libusb debug level (0~4)
* @return int, 0: success, other values : failed 
*           
* @retval 
*/
int Sample_init(int devnum,int level,DEVICE_CFG **ppDevCfg)
{
	int r = -1;
	int j=0;
	ADC_DEV_CFG *pstAdcCfg = NULL;
	char *pdata = NULL;
	char *pdataT = NULL;
	int data_totalsize = 0;
	int data_unitsize = 0;
	receive_data_flag=0,shmdata1_flag=0,shmdata2_flag=0;
	short *shm_tmpaddr;
	int shm_size=0;
	receive_data_flag=0;
	if (devnum < 1 || devnum > 16) {
		PRINT(DBG_ERROR,"invalid devm num %d \r\n",devnum);
		return 1;	
	}
	/*
	* check level : ENET_LOG_LEVEL_NONE ~ ENET_LOG_LEVEL_DEBUG
	*/
	
	if (gpst_DevCfg) {
		PRINT(DBG_ERROR,"Device Config already init, you should 'exit' before init ");
		return 2;
	}
	
	if ( pthread_mutex_init (&shm1lk, NULL) != 0 )
	{
		perror ("shm1lk init failed");
	}
	if (pthread_mutex_init (&shm2lk, NULL) != 0)
	{
		perror ("shm2lk init failed");
	}

	dev1lk = (pthread_mutex_t *)malloc (devnum * sizeof(pthread_mutex_t));
	dev2lk = (pthread_mutex_t *)malloc (devnum * sizeof(pthread_mutex_t));
	memset (dev1lk, 0, devnum * sizeof (pthread_mutex_t));
	memset (dev2lk, 0, devnum * sizeof (pthread_mutex_t));
	for (int i = 0; i < devnum; i++)
	{
		if (pthread_mutex_init (&(dev1lk[i]), NULL) != 0)
		{
			perror ("dev1shm lock init error");
		}
		if (pthread_mutex_init (&(dev2lk[i]), NULL) != 0)
		{
			perror ("dev2shm lock init error");
		}
	}

	gpst_DevCfg = (DEVICE_CFG *)malloc(sizeof(DEVICE_CFG));
	if (gpst_DevCfg== NULL) {
		PRINT(DBG_ERROR,"malloc gpst_DevCfg failed");
		return 3;
	}
	memset(gpst_DevCfg,0,sizeof(DEVICE_CFG));
	gpst_DevCfg->usDevCnt = devnum;
	gpst_DevCfg->usRunCnt = devnum;

	data_totalsize = devnum * 2 * data_unitsize;
	pdataT = (char *)malloc(data_totalsize);
	pdata =pdataT;
	pstAdcCfg = (ADC_DEV_CFG *)malloc((sizeof(ADC_DEV_CFG)+2880) * devnum);
	ADC_DEV_CFG *psttmpCfg = NULL;	
	printf("adc_dev_cfg=========%d\n",sizeof(ADC_DEV_CFG)*devnum);
	if ((NULL != pstAdcCfg)&&(NULL != pdata)) {
		memset(pstAdcCfg,0,sizeof(ADC_DEV_CFG) * devnum);		
		gpst_DevCfg->pstAdcCfg = pstAdcCfg;
		memset(pdata,0,data_totalsize);
		free_shared_mem(shm_tmpaddr);
		//shm config for alloc_shared_mem
		shm_size=(1456*2)+(devnum*2880)*2;
		PRINT(DBG_INFO,"shm_size====%d\n",shm_size);
		shm_tmpaddr=SHMADDR;
		PRINT(DBG_INFO,"shm_tmpaddr====%p\n",shm_tmpaddr);
		shm_tmpaddr=alloc_shared_mem(IPC_PRIVATE,shm_tmpaddr,shm_size);
		gpst_DevCfg->global_shm1flag=0x01;
		gpst_DevCfg->global_shm2flag=0x01;
		gpst_DevCfg->global_shm1data=shm_tmpaddr;
		gpst_DevCfg->global_shm2data=shm_tmpaddr+(1456*2);
		char buffer[1440];
        //every device partition shared mem
		for (int i = 0; i < devnum; i++) {
			memset(pstAdcCfg[i].opposite_mac,0,6);
			pstAdcCfg[i].shm1_flag=0x01;
			pstAdcCfg[i].shm2_flag=0x01;
			pstAdcCfg[i].shm1_data=(shm_tmpaddr+1456*2)+(i*2880);
			pstAdcCfg[i].shm2_data=(shm_tmpaddr+1456*2)+(i*2880)+1440;
			pthread_mutex_init(&shm1_mutex[i],NULL);
			pthread_mutex_init(&shm2_mutex[i],NULL);
			if(shm_tmpaddr==NULL){
					error_and_die("read_shm  open failed\n");
			}else{
					printf("111111111111\r\n");
			}
		}
	}else{
		PRINT(DBG_ERROR,"no enough resources");
		if (gpst_DevCfg) free(gpst_DevCfg);
		if (pdataT) free(pdataT);
		if (shm_tmpaddr) free_shared_mem(shm_tmpaddr);
		return 3;
	}

	gb_sampleinit = true;
	*ppDevCfg=gpst_DevCfg;
	return 0;
}

/**
* @brief open all 7606 device with specified config
*
* @param void
*
* @return int ,0: success, other values: failed
* @retval
*/
int Sample_open()
{
	int n=0;
	int rt = -1;
	bool result = false;
	int send_flag=1;
	int mac_flag=0;
	int fdcnt = 0; /* counter for matched device */
	size_t i = 0;
	unsigned char tmpmac[6];
	unsigned char cmpmac[6];
	//define for device loop
	short usRunCnt =0;
	short usIndex = 0;
	ADC_DEV_CFG *pstAdcCfg = NULL;
	ADC_DEV_CFG *pstTmpCfg = NULL;   
	memset(cmpmac,0,6);
	//check loop struct
	if (gpst_DevCfg == NULL) 
	{
		PRINT(DBG_ERROR,"Not initialization ");
		return 1;
	}    
	usRunCnt = gpst_DevCfg->usRunCnt;
	pstAdcCfg = gpst_DevCfg->pstAdcCfg;

	unsigned char buffer[1456];
	unsigned char data[20];
	short data_size=0;
	socklen_t addr_length = sizeof(struct sockaddr_ll);


	system("ifconfig enp1s0 up");
	int len=sprintf(send_msg+16,"%s","connect");
	//strncpy(req.ifr_name, "enp1s0", IFNAMSIZ);    //指定网卡名称
	strncpy (req.ifr_name, "ens33", IFNAMSIZ);

	if ((sock_raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("socket");
		exit(1);
	}
	//if (ioctl (sock_raw_fd, SIOCGIFHWADDR, &req) < 0)
	//{
		//perror ("ioctl");
		//return -1;
	//}
	//memcpy(&send_msg[6],(unsigned char*)&req.ifr_hwaddr.sa_data,6);

	if(-1 == ioctl(sock_raw_fd, SIOCGIFINDEX, &req))    //获取网络接口
    {
        perror("ioctl");
        close(sock_raw_fd);
        exit(-1);
    }
	/*将网络接口赋值给原始套接字地址结构*/
    bzero(&sll, sizeof(sll));
    sll.sll_ifindex = req.ifr_ifindex;
	/*send connect info to 1061*/
	while (1) {
		if(send_flag==1){
			len = sendto(sock_raw_fd, send_msg, 16+len, 0 , (struct sockaddr *)&sll, sizeof(sll));
			printf("send_msg +16 data ===%s\n",send_msg+16);
			if(len == -1)
			{
					perror("sendto");
			}
		}
		n = recvfrom(sock_raw_fd, buffer,1456,0, NULL, NULL);
		//n = recvfrom(sock_raw_fd, buffer,1454,0, NULL, NULL);
		if (n < 16) {
			printf("n=======%d\n",n);
			continue;
		}else {//if(n == 18){
			//printf("n=======%d\n",n);
			memcpy(&data_size,buffer+14,2);
			//if(data_size == 0x02){
				memcpy(data,buffer+16,2);
				printf("receive data is %s\r\n",data);
				printf(" Dest Address %02x:%02x:%02x:%02x:%02x:%02x Src Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
						buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9],
						buffer[10], buffer[11]);
				if(strncmp(data,"ok",2)==0){
					if(send_flag==1){
						send_flag=0;
					}
					memcpy(tmpmac,buffer+6,6);
					for (usIndex=0; usIndex < usRunCnt; usIndex++)
					{
						printf("usIndex===%d usRunCnt====%d\r\n",usIndex,usRunCnt);
						pstTmpCfg = &(pstAdcCfg[usIndex]);
						if(0==memcmp(pstTmpCfg->opposite_mac,tmpmac,6))
						{
							memset(tmpmac,0,6);
						}else{
							if((memcmp(pstTmpCfg->opposite_mac,cmpmac,6)==0) && (0!=memcmp(pstTmpCfg->opposite_mac,tmpmac,6))){
								//printf("usIndex===%d  usRuncnt====%d\r\n",usIndex,usRunCnt);	
								memcpy(pstTmpCfg->opposite_mac,tmpmac,6);
								/*for(int i=0;i<8;i++)
								{
									if(0==memcpy(pstTmpCfg->opposite_mac,mac_list[i],6))
									{
										pstTmpCfg->board_info->serial=i+1;
									}
								}*/
								printf("mac save ok\r\n");
								mac_flag++;
								if(usRunCnt==mac_flag){
									receive_data_flag=1;
									printf("send_flag ===%d receive_data_flag====%d\r\n",send_flag,receive_data_flag);
									break;
								}else{
									break;
								}
							}
							
						}
					}	
				}
			//}else{
			//	printf("data_size not  0x02\r\n");
			//}
		}
		if(receive_data_flag==1){
			for (usIndex = 0; usIndex < usRunCnt; usIndex++)
			{
				pstTmpCfg = &(pstAdcCfg[usIndex]);
				memcpy(tmpmac,pstTmpCfg->opposite_mac,6);
				printf("device[%d]--MAC Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",usIndex,
                     tmpmac[0], tmpmac[1], tmpmac[2], tmpmac[3], tmpmac[4], tmpmac[5], tmpmac[6]);	
			}	
			break;
		}
	}
return 0;
}

int senddata()
{
  short v=0;
  int n = 0;
  int cycle=0;
  int i=0,j=0;
  int count=0;
  float MaxVol=5;
  float realVol=0;
  int data_flag=0;
  ssize_t seek = 0;
  uint8_t usIndex = 0;
  uint8_t usRunCnt = 0;
  ADC_DEV_CFG *pstAdcCfg = NULL;
  ADC_DEV_CFG *pstTmpCfg = NULL;
  unsigned char *tmpdata=NULL;
  unsigned char *filedata=NULL;
  filedata = (unsigned char *)malloc (1456);  
  tmpdata =  (unsigned char *)malloc(1440);
  memset (filedata, 0, 1456);
  memset (tmpdata, 0, 1440);

  if (gpst_DevCfg == NULL)
  {
    PRINT (DBG_ERROR, "Not initialization");
    return 1;
  }

  usRunCnt = gpst_DevCfg->usRunCnt;
  pstAdcCfg = gpst_DevCfg->pstAdcCfg;
  /*
  *open virtual sample data from ../pub/SampleData.txt
  */
  int filefd = open ("../pub/SampleData.txt", O_EXCL | O_RDONLY);
  if (filefd < 0)
  {
    perror ("File not exist or open failed!\n");
  }
  //printf ("file open\n");
  while (1)
  {
    if (lseek (filefd, seek, SEEK_SET) == -1)
    {
    perror ("Set seek error!\n");
    }
	//printf ("set seek ok\n");
    while ((n = read (filefd, filedata, 1456)) > 0)
    {
		printf ("IN while\n");
      memcpy (tmpdata, (filedata + 16), 1440);
	  printf ("cpy over\n");
      //adjust_data(&tmpdata);//adjust sample data 
	  printf ("adjust done\n"); 
      for (usIndex = 0; usIndex < usRunCnt; usIndex++)
      {
        pstTmpCfg = &(pstAdcCfg[usIndex]);
		pthread_mutex_lock (&(dev1lk[usIndex]));
		printf ("lock1\n");

        if (pstAdcCfg[usIndex].shm1_flag == 0x01)
        {
          memcpy (pstAdcCfg[usIndex].shm1_data, tmpdata, 1440);
          pstAdcCfg[usIndex].shm1_flag = 0x02;
		  pthread_mutex_unlock (&(dev1lk[usIndex]));
        }
        else 
        {
			pthread_mutex_unlock (&(dev1lk[usIndex]));
			pthread_mutex_lock (&(dev2lk[usIndex]));
			if (pstAdcCfg[usIndex].shm2_flag == 0x01)
			{
          		memcpy (pstAdcCfg[usIndex].shm2_data, tmpdata, 1440);
          		pstAdcCfg[usIndex].shm2_flag = 0x02;
			}
			pthread_mutex_unlock (&(dev2lk[usIndex]));
        }
      }
    }
    if (n < 0)
    {
    perror ("Read error!\n");
    }
  }
}
int Sample_read()
{
	senddata();
}

#if 0
/**
* @brief read sample data to share memory
*
* @param  void
* @return int ,0: success, other values: failed
* @retval
*/
int Sample_read()
{
	int rc;
	//CPU_ZERO(&cpuset);
	//CPU_SET(1,&cpuset);
	//rc=pthread_setaffinity_np(pthread_receive_data,sizeof(cpu_set_t),&cpuset);
	if(rc!= 0)
	{
		printf("set cpu error\r\n");
	}
	read_data_pthread();
	sleep(1);
	printf("sample read\r\n");
	pthread_create(&pthread_receive_data,0,receive_pthread,NULL);
	return 0;	
}
#endif

/**
* @brief write data to all 7606 device with specified mode
*
* @param mode, sample to sample_read
* @param void *pdata, data to be write
* @param int len, data size that to be write
* @return int ,0: success, other values: failed
* @retval
* this function on doing
*/
int Sample_write(int serial,int route,int cmd)
{
	int len;
	int rt=0;
	short send_type=0x0c;
	short usRunCnt =0;
	short usIndex = 0;
	ADC_DEV_CFG *pstAdcCfg = NULL;
	ADC_DEV_CFG *pstTmpCfg = NULL;

	pstAdcCfg=gpst_DevCfg->pstAdcCfg;
	len =strlen(send_msg);
	memcpy(send_msg+16,&route,4);
	memcpy(send_msg+20,&cmd,4);
	for(usIndex =0;usIndex < usRunCnt; usIndex++)
	{
		pstTmpCfg = &(pstAdcCfg[usIndex]);
		if(pstTmpCfg->board_info->serial==serial)
		{
			memcpy(send_msg,pstTmpCfg->opposite_mac,6);
			memcpy(send_msg+14,&send_type,2);
			if(rt=sendto(sock_raw_fd, send_msg, 16+len, 0 , (struct sockaddr *)&sll, sizeof(sll))<0)
			{
				PRINT(DBG_INFO,"sento ad7606 failed\r\n");
			}
		}
	}
	return 0;
}

/**
* @brief close all 7606 device with specified config
*
* @param void
*
* @return int ,0: success, other values: failed
* @retval
*/
int Sample_close(void)
{
	short usRunCnt =0;
	short usIndex = 0;
	ADC_DEV_CFG *pstAdcCfg = NULL;
	ADC_DEV_CFG *pstTmpCfg = NULL;
	
	if (gpst_DevCfg == NULL) 
	{
		PRINT(DBG_ERROR,"Not initialization ");
		return 1;
	}

	usRunCnt = gpst_DevCfg->usRunCnt;
	pstAdcCfg = gpst_DevCfg->pstAdcCfg;

	for (usIndex = 0; usIndex < usRunCnt; usIndex++)
	{
		pstTmpCfg = &(pstAdcCfg[usIndex]);
	}

	gpst_DevCfg->usRunCnt = 0;
	PRINT(DBG_STDO," close %d device successfully \r\n",usRunCnt);
	
	return 0;	
}
/**
* @brief Exit sample for desrtroy all variable
*
* @param void
*
* @return int ,0: success, other values: failed
* @retval
*/
int Sample_exit(void)
{
	int index;
	ADC_DEV_CFG *pstCfg = NULL;

	if (!gpst_DevCfg) {
		PRINT(DBG_ERROR,"Null config or no init");
		return 1;
	}
	for (index = 0; index < gpst_DevCfg->usDevCnt; index++)
	{
		pstCfg = &(gpst_DevCfg->pstAdcCfg[index]);
		if (pstCfg->status) { 
		}	
		pthread_mutex_destroy(&shm1_mutex[index]);
		pthread_mutex_destroy(&shm2_mutex[index]);
	}
	free(gpst_DevCfg);
	gpst_DevCfg = NULL;

	return 0;
}
void *receive_pthread(void){

	short usRunCnt =0;
	short usIndex = 0;
	ADC_DEV_CFG *pstAdcCfg = NULL;
	ADC_DEV_CFG *pstTmpCfg = NULL;
	usRunCnt = gpst_DevCfg->usRunCnt;
	pstAdcCfg = gpst_DevCfg->pstAdcCfg;
	int i=0;
	int j=0;
	int n;
	char d[4];
	int dest=0;
	int cycle=0;
	short v=0;
	int data_flag=0;
	short data_size=0;
	float MaxVol=5;
	float realVol=0;
	long *deltaTime=NULL;
	deltaTime=(long *)malloc(8);
	unsigned char *tmpdata=NULL;
	deltaTime=(long *)malloc(8);
	tmpdata=(unsigned char*)malloc(1440);
	int len=sprintf(send_msg+16,"%s","ok");
	unsigned char buffer[1456];
	printf("receive_pthread\r\n");
	while (1) {
		//usleep(1);
        if(receive_data_flag==1){
			printf("start send\r\n");
			for(i=0;i<10;i++)
			{
				len = sendto(sock_raw_fd, send_msg, 16+len, 0 , (struct sockaddr *)&sll, sizeof(sll));
				if(len == -1)
				{
					perror("sendto");
				}
				if(i==9){
					PRINT(DBG_INFO,"sample device receive ready");
					receive_data_flag=0;
				}
			}
		}
		n = recvfrom(sock_raw_fd, buffer,1456,0, NULL, NULL);
		if (n < 16) {
			printf("n=======%d\n",n);
			continue;
		}else if(n == 1456){
			//printf("n=======%d\n",n);
			//memcpy(&data_size,buffer+14,2);
			//printf("buffer[13]===%x  buffer[14]===%x\r\n",buffer[13],buffer[14]);
			//printf("data_size === %x\r\n",data_size);
			//if(data_size == 0x05a0){
				//for(j=0;j<30;j++)
				//{
				/*printf("Sample Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
						buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
				memcpy(tmpdata,buffer+16,1440);
					memcpy(&data_flag,&tmpdata[j*48],4);
					memcpy(&cycle,&tmpdata[(j*48)+4],4);
					printf("data_flag -----0x%x\r\n",data_flag);
					printf("sample cycle -----%x\r\n",cycle);
					//for(int i = 0; i < 16;i++)
					//{
						memcpy(&v,&tmpdata[(j*48)+20],2);
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
						PRINT(DBG_STDO,"v5电压值为 ==%2.6fv  ",realVol);
						PRINT(DBG_STDO,"\n");
					//}
					memcpy(deltaTime,&tmpdata[(i*48)+40],8);
					printf("deltaTime==%ldus\r\n",*deltaTime);
				//}
				PRINT(DBG_INFO," A frame received. the length %d ", n);
				PRINT(DBG_INFO," Dest Address %02x:%02x:%02x:%02x:%02x:%02x Src Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
						buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9],
						buffer[10], buffer[11]);*/
				//receive data to share mem
				//printf("global_shm1flag ===%d global_shm2flag ===%d \r\n",gpst_DevCfg->global_shm1flag,gpst_DevCfg->global_shm2flag);
				pthread_mutex_lock (&shm1lk);
				if(gpst_DevCfg->global_shm1flag==0x01){
					//printf("receive success and save to share memory   1111\r\n");
					memcpy(gpst_DevCfg->global_shm1data,buffer,1456);
					gpst_DevCfg->global_shm1flag=0x02; 
					//printf("shmdata1_flag=====%d\r\n",gpst_DevCfg->global_shm1flag);
					//pthread_mutex_unlock(&shm1_mutex[index]);
				}else if(gpst_DevCfg->global_shm2flag==0x01)
				{
					//printf("receive success and save to share memory   2222s\r\n");
					memcpy(gpst_DevCfg->global_shm2data,buffer,1456);	
					gpst_DevCfg->global_shm2flag=0x02;
					//printf("shmdata2_flag=====%d\r\n",gpst_DevCfg->global_shm2flag);
					//pthread_mutex_unlock(&shm2_mutex[index]);
				}
				pthread_mutex_unlock (&shm2lk);
			}//else{
				//printf("data_size not  0x05a0\r\n");
			//}
		//}
	}
}

