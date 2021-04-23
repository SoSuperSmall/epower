#include "global.h"
#include "adio.h"
#include "epower_pub.h"

short v=0;
int i=0,j=0;
int cycle=0;
int data_flag=0;
float realV=0;
float MaxV=5;
pthread_t pthread_getdata1;
pthread_t pthread_getdata2;
extern int read_pthread_start;
float *rV=NULL;
long *deltaTime=NULL;
unsigned short *tmpd=NULL;
unsigned short *tmpv=NULL;
extern DEVICE_CFG *gpst_DevCfg;
/**
 * @brief adjust sample data and convert into realVol
 * @param unsigned char* tmpdata:need adjust data  
 * @return void
 * @macthup data
 * 		tmpv[0]=tmpd[0];tmpv[1]=tmpd[2];tmpv[2]=tmpd[4];tmpv[3]=tmpd[6];
 *		tmpv[4]=tmpd[8];tmpv[5]=tmpd[10];tmpv[6]=tmpd[12];tmpv[7]=tmpd[14];
 *		tmpv[8]=tmpd[1];tmpv[9]=tmpd[3];tmpv[10]=tmpd[5];tmpv[11]=tmpd[7];
 *		tmpv[12]=tmpd[9];tmpv[13]=tmpd[11];tmpv[14]=tmpd[13];tmpv[15]=tmpd[15];
**/
void adjust_data(unsigned char* tmpdata){
	memcpy(tmpv,(short*)tmpdata,1440);
	memcpy(tmpd,(short*)tmpdata,1440);
	for(j=0;j<30;j++)
	{
		for(int i = 0; i < 16;i++){
			if(i<8){
				tmpv[(j*24)+4+i]=tmpd[(j*24+4)+(i*2)];
			}else if(i>7){
				tmpv[(j*24)+4+i]=tmpd[(j*24+4)+((i%8)*2+1)];
			}
		}
	}
	memcpy(tmpdata,(unsigned char *)tmpv,1440);
}
/**
 * @brief analysis shmdata1
 * @param void 
 * @return void
**/
void *analysis_shmdata1(void)
{	
	
	short usRunCnt =0;
	short usIndex = 0;
	unsigned char tmpmac[6]; 
	ADC_DEV_CFG *pstAdcCfg = NULL;
	ADC_DEV_CFG *pstTmpCfg = NULL;
	if (gpst_DevCfg == NULL) 
	{
		PRINT(DBG_ERROR,"Not initialization ");
		return 1;
	}

	usRunCnt = gpst_DevCfg->usRunCnt;
	pstAdcCfg = gpst_DevCfg->pstAdcCfg;
	unsigned char *tmpdata;
	tmpdata=(unsigned char *)malloc(1440);
	while(1){
		usleep(1);
		//PRINT(DBG_STDO,"analysis shmdata1 shmdata=====%d\r\n",gpst_DevCfg->global_shm1flag);
		//PRINT(DBG_STDO,"analysis shmdata1 shmdata=====%d\r\n",gpst_DevCfg->global_shm1flag);
		pthread_mutex_lock (&shm1lk);
		if(gpst_DevCfg->global_shm1flag==0x02){	
			memcpy(tmpdata,gpst_DevCfg->global_shm1data+16,1440);	
			memcpy(tmpmac,gpst_DevCfg->global_shm1data+6,6);
			gpst_DevCfg->global_shm1flag=0x01;		
			PRINT(DBG_INFO,"analysis shmdata1 shmdata=====%d\r\n",gpst_DevCfg->global_shm1flag);
			pthread_mutex_unlock(&shm1lk);
			adjust_data(tmpdata);
			//printf("analysis shmdata1 shmdata=====%d\r\n",gpst_DevCfg->global_shm1flag);
			for (usIndex = 0; usIndex < usRunCnt; usIndex++)
			{
				PRINT(DBG_STDO," Dest Address %02x:%02x:%02x:%02x:%02x:%02x Src Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                     tmpmac[0], tmpmac[1], tmpmac[2], tmpmac[3], tmpmac[4], tmpmac[5], pstAdcCfg[usIndex].opposite_mac[0], pstAdcCfg[usIndex].opposite_mac[1],\
					 pstAdcCfg[usIndex].opposite_mac[2], pstAdcCfg[usIndex].opposite_mac[3],
                     pstAdcCfg[usIndex].opposite_mac[4], pstAdcCfg[usIndex].opposite_mac[5]);
				//if(pstAdcCfg[usIndex].shm1_flag==0x01)

				if (0 == memcmp(tmpmac, pstAdcCfg[usIndex].opposite_mac, 6))
				{
					pthread_mutex_lock (&(dev1lk[usIndex]));
					if (pstAdcCfg[usIndex].shm1_flag == 0x01)
					{

						pthread_mutex_unlock(&(dev1lk[usIndex]));
					}
					else
					{
						pthread_mutex_unlock(&(dev1lk[usIndex]));
						pthread_mutex_lock(&(dev2lk[usIndex]));
						if (pstAdcCfg[usIndex].shm2_flag == 0x01)
						{
							
						}
						pthread_mutex_unlock(&(dev2lk[usIndex]));
					}
				}
#if 0
				if((pstAdcCfg[usIndex].shm1_flag==0x01) && (0 == memcmp(tmpmac,pstAdcCfg[usIndex].opposite_mac,6)) )
				{
					memcpy(pstAdcCfg[usIndex].shm1_data,tmpdata,1440);
					pstAdcCfg[usIndex].shm1_flag=0x02;
					printf("if ok\r\n");

				
/*
				memcpy(tmpdata,pstAdcCfg[usIndex].shm1_data,1440);
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
						realV = -1 * MaxV * (v+1) / 32768;
						PRINT(DBG_STDO,"\r\n 负数");				
					}else{
						realV = MaxV * v / 32768;
						PRINT(DBG_STDO,"\r\n 正数");
					}
					PRINT(DBG_STDO,"v5电压值为 ==%2.6fv  ",realV);
					PRINT(DBG_STDO,"\n");
				//}
				memcpy(deltaTime,&tmpdata[(i*48)+40],8);
				printf("deltaTime==%ldus\r\n",*deltaTime);
*/
					break;
					//printf("analysis shm1_flag=====%d\r\n",pstAdcCfg[usIndex].shm1_flag
				}
#endif
			}
		}
	}
}
/**
 * @brief adjust sample data and convert into realVol
 * @param unsigned char* tmpdata:need adjust data  
 * @return void
**/
void *analysis_shmdata2(void){
	
	short usRunCnt =0;
	short usIndex = 0;
	unsigned char tmpmac[6];
	ADC_DEV_CFG *pstAdcCfg = NULL;
	ADC_DEV_CFG *pstTmpCfg = NULL;
	if (gpst_DevCfg == NULL) 
	{
		PRINT(DBG_ERROR,"Not initialization ");
		return 1;
	}
	usRunCnt = gpst_DevCfg->usRunCnt;
	pstAdcCfg = gpst_DevCfg->pstAdcCfg;
	unsigned char *tmpdata;
	tmpdata=(unsigned char *)malloc(1440);
	//PRINT(DBG_INFO,"analysis shmdata2 shmdata=====%d\r\n",gpst_DevCfg->global_shm2flag);
	while(1){
		usleep(1);
		//PRINT(DBG_STDO,"analysis shmdata2 shmdata=====%d\r\n",gpst_DevCfg->global_shm2flag);
		//PRINT(DBG_STDO,"analysis shmdata2 shmdata=====%d\r\n",gpst_DevCfg->global_shm2flag);
		pthread_mutex_lock (&shm2lk);
		if(gpst_DevCfg->global_shm2flag==0x02){
	
			memcpy(tmpdata,gpst_DevCfg->global_shm2data+16,1440);
			memcpy(tmpmac,gpst_DevCfg->global_shm2data+6,6);
			gpst_DevCfg->global_shm2flag=0x01;

			PRINT(DBG_INFO,"analysis shmdata2 shmdata=====%d\r\n",gpst_DevCfg->global_shm2flag);
			pthread_mutex_unlock (&shm2lk);
			adjust_data(tmpdata);	
			
			//printf("analysis shmdata2 shmflag=====%d\r\n",gpst_DevCfg->global_shm2flag);
			for (usIndex = 0; usIndex < usRunCnt; usIndex++)
			{
				/*PRINT(DBG_INFO," Dest Address %02x:%02x:%02x:%02x:%02x:%02x Src Address %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                     tmpmac[0], tmpmac[1], tmpmac[2], tmpmac[3], tmpmac[4], tmpmac[5], pstAdcCfg[usIndex].opposite_mac[0], pstAdcCfg[usIndex].opposite_mac[1],\
					 pstAdcCfg[usIndex].opposite_mac[2], pstAdcCfg[usIndex].opposite_mac[3],
                     pstAdcCfg[usIndex].opposite_mac[4], pstAdcCfg[usIndex].opposite_mac[5]);*/
				//if(pstAdcCfg[usIndex].shm1_flag==0X01)

				if (0 == memcmp(tmpmac, pstAdcCfg[usIndex].opposite_mac, 6))
				{
					pthread_mutex_lock (&(dev1lk[usIndex]));
					if (pstAdcCfg[usIndex].shm1_flag == 0x01)
					{

						pthread_mutex_unlock(&(dev1lk[usIndex]));
					}
					else
					{
						pthread_mutex_unlock(&(dev1lk[usIndex]));
						pthread_mutex_lock(&(dev2lk[usIndex]));
						if (pstAdcCfg[usIndex].shm2_flag == 0x01)
						{
							
						}
						pthread_mutex_unlock(&(dev2lk[usIndex]));
					}
				}

#if 0
				if((0 == memcmp(tmpmac,pstAdcCfg[usIndex].opposite_mac,6)) && (pstAdcCfg[usIndex].shm2_flag==0x01))
				{
					memcpy(pstAdcCfg[usIndex].shm2_data,tmpdata,1440);
					pstAdcCfg[usIndex].shm2_flag=0x02;
					printf("if ok\r\n");
/*					
				memcpy(tmpdata,pstAdcCfg[usIndex].shm2_data,1440);
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
						realV = -1 * MaxV * (v+1) / 32768;
						PRINT(DBG_STDO,"\r\n 负数");				
					}else{
						realV = MaxV * v / 32768;
						PRINT(DBG_STDO,"\r\n 正数");
					}
					PRINT(DBG_STDO,"v5电压值为 ==%2.6fv  ",realV);
					PRINT(DBG_STDO,"\n");
				//}
				memcpy(deltaTime,&tmpdata[(i*48)+40],8);
				printf("deltaTime==%ldus\r\n",*deltaTime);
*/
					break;
					//printf("analysis shm2_flag=====%d\r\n",pstAdcCfg[usIndex].shm2_flag);
					pthread_mutex_unlock (&(dev2lk[usIndex]));
				}
#endif
			}
		}
	}
}
/**
 * @brief adjust sample data and convert into realVol
 * @param unsigned char* tmpdata:need adjust data  
 * @return void
**/
int read_data_pthread(void)
{
	tmpd=(short*)malloc(1440);
	tmpv=(short*)malloc(1440);
	rV=(float*)malloc(1440);
	deltaTime=(long *)malloc(8);
	printf("read data pthread\r\n");
	pthread_create(&pthread_getdata1,0,analysis_shmdata1,NULL);
	pthread_create(&pthread_getdata2,0,analysis_shmdata2,NULL);

	return 0;
}
