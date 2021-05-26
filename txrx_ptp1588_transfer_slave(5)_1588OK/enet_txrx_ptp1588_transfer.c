/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_enet.h"
#include "fsl_phy.h"
#include "fsl_pit.h"
#if defined(FSL_FEATURE_MEMORY_HAS_ADDRESS_OFFSET) && FSL_FEATURE_MEMORY_HAS_ADDRESS_OFFSET
#include "fsl_memory.h"
#endif
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_ENET ENET
#define EXAMPLE_ENET2 ENET2
#define EXAMPLE_PHY 0x02U
#define CORE_CLK_FREQ CLOCK_GetFreq(kCLOCK_IpgClk)
#define PTP_CLK_FREQ 50000000U
#define ENET_RXBD_NUM (4)
#define ENET_TXBD_NUM (4)
#define ENET_RXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#define ENET_TXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#define ENET_DATA_LENGTH (1000)
#define ENET_TRANSMIT_DATA_NUM (20)
#define ENET_PTP_SYNC_MSG 0x00U
#ifndef APP_ENET_BUFF_ALIGNMENT
#define APP_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT

#define ENET_NANOSECOND_ONE_SECOND 1000000000U
#define HALF_SEC (ENET_NANOSECOND_ONE_SECOND>>1)
#define ONE_SEC 1000000000U
#define PTP_CLOCK_FRE_RT 25000000U
#define PTP_AT_INC              (ENET_NANOSECOND_ONE_SECOND/PTP_CLOCK_FRE_RT)
#define PTP_TEST_APP_ENABLE true
#define PTP_TEST_APP_CHANNEL kENET_PtpTimerChannel3
#define EXAMPLE_SW_GPIO GPIO2
#define EXAMPLE_SW_GPIO_PIN (2U)
#define EXAMPLE_SW_IRQ GPIO2_Combined_0_15_IRQn
#define EXAMPLE_GPIO_IRQHandler GPIO2_Combined_0_15_IRQHandler
#define EXAMPLE_SW_NAME "PPS"
#define PIT_HANDLER PIT_IRQHandler
#define PIT_IRQ_ID PIT_IRQn
#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_OscClk)
#define THREEUS 3000
#endif
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*! @brief Build ENET PTP event frame. */
static void ENET_BuildPtpEventFrame(void);
static void ethernet_ptptime_enablepps(ENET_Type *base, enet_handle_t handle, enet_ptp_timer_channel_t tmr_ch);
static void ethernet_ptptime_init(ENET_Type *base , uint32_t ptp_clk_freq, enet_handle_t handle, \
	bool pps_en, enet_ptp_timer_channel_t tmr_ch);
void ethernet_ptptime_adjfreq(ENET_Type *base, int32_t incps);
void ethernet_ptptime_adjtime(ENET_Type *base, enet_handle_t handle,int32_t second, int32_t nano_second);
void ethernet_ptptime_settime(ENET_Type *base, enet_handle_t handle,enet_ptp_time_t *timestamp);
void ethernet_ptptime_gettime(ENET_Type *base, enet_handle_t handle,enet_ptp_time_t *timestamp);


/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Buffer descriptors should be in non-cacheable region and should be align to "ENET_BUFF_ALIGNMENT". */
AT_NONCACHEABLE_SECTION_ALIGN(enet_rx_bd_struct_t g_rxBuffDescrip[ENET_RXBD_NUM], ENET_BUFF_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(enet_tx_bd_struct_t g_txBuffDescrip[ENET_TXBD_NUM], ENET_BUFF_ALIGNMENT);
/*! @brief The data buffers can be in cacheable region or in non-cacheable region.
 * If use cacheable region, the alignment size should be the maximum size of "CACHE LINE SIZE" and "ENET_BUFF_ALIGNMENT"
 * If use non-cache region, the alignment size is the "ENET_BUFF_ALIGNMENT".
 */
SDK_ALIGN(uint8_t g_rxDataBuff[ENET_RXBD_NUM][SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);
SDK_ALIGN(uint8_t g_txDataBuff[ENET_TXBD_NUM][SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT)],
          APP_ENET_BUFF_ALIGNMENT);

/* Buffers for store receive and transmit timestamp. */
enet_ptp_time_data_t g_rxPtpTsBuff[ENET_RXBD_NUM];
enet_ptp_time_data_t g_txPtpTsBuff[ENET_TXBD_NUM];

enet_handle_t g_handle;
uint8_t g_frame[ENET_DATA_LENGTH + 14];
uint32_t g_testTxNum = 0;

static enet_ptp_time_t g_theTime;

/* The MAC address for ENET device. */
uint8_t g_macAddr[6] = {0xd4, 0xbe, 0xd9, 0x45, 0x22, 0x60};

volatile bool g_InputSignal = false;
uint32_t g_temp = 0;

enet_ptp_time_t g_lastTime;
enet_ptp_time_t g_tmpTime;
uint32_t g_array[20]= {0};
uint16_t g_c = 0;
uint8_t g_index = 0;

void delay();

/*******************************************************************************
 * Code
 ******************************************************************************/
void PIT_HANDLER (void)
{
		PIT_ClearStatusFlags (PIT, kPIT_Chnl_0, kPIT_TimerFlag);
		//GPIO_PinWrite (GPIO5, 0U, 0x1 ^ GPIO_PinRead(GPIO5, 0U));
		GPIO_PinWrite (GPIO1, 2U, 0x1 ^ GPIO_PinRead(GPIO1, 2U));
		//GPIO_PinWrite (GPIO2, 13U, 0x1 ^ GPIO_PinRead(GPIO2, 13U));
		__DSB();
}

/*
void EXAMPLE_GPIO_IRQHandler(void)
{
		GPIO_PinWrite (GPIO3, 12U, 0x1 ^ GPIO_PinRead(GPIO3, 12U));
		ENET_Ptp1588GetTimer(EXAMPLE_ENET, &g_handle, &g_theTime);
		g_tmpTime = g_theTime;
		do
		{
				GPIO_PortClearInterruptFlags(EXAMPLE_SW_GPIO, 1U << EXAMPLE_SW_GPIO_PIN);
		}while (GPIO_GetPinsInterruptFlags(EXAMPLE_SW_GPIO) & (1U << EXAMPLE_SW_GPIO_PIN));
		//PRINTF ("SEC = %d   NS = %d\r\n", theTime.second, theTime.nanosecond);                  
		//ENET_Ptp1588GetTimer(EXAMPLE_ENET, &g_handle, &endTime);
		g_InputSignal = true;
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
*/

void EXAMPLE_GPIO_IRQHandler(void)
{
		GPIO_PinWrite (GPIO3, 12U, 0x1 ^ GPIO_PinRead(GPIO3, 12U));
		GPIO_PortClearInterruptFlags(EXAMPLE_SW_GPIO, 1U << EXAMPLE_SW_GPIO_PIN);     //clear interrupt flag
		ENET_Ptp1588GetTimer(EXAMPLE_ENET, &g_handle, &g_theTime);              //get the 1588 timer
			if (g_index == 0)
			{
					g_lastTime = g_theTime;
					g_tmpTime = g_theTime;
					g_index++;
			}
			else
			{
					if (g_theTime.second > g_lastTime.second)
					{
							g_tmpTime = g_theTime;
							g_index = 0;
					}
					g_lastTime = g_theTime;
			}
			
		//delay();	
		//clear interrupt flag
		do
		{
				GPIO_PortClearInterruptFlags(EXAMPLE_SW_GPIO, 1U << EXAMPLE_SW_GPIO_PIN);
		}while (GPIO_GetPinsInterruptFlags(EXAMPLE_SW_GPIO) & (1U << EXAMPLE_SW_GPIO_PIN));
		
		g_InputSignal = true;

#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}


void BOARD_InitModuleClock(void)
{
    const clock_enet_pll_config_t config = {.enableClkOutput = true, .enableClkOutput25M = true, .loopDivider = 1, .src = 0};
    CLOCK_InitEnetPll(&config);
}

void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 400; ++i)      //about 2us
    {
        __asm("NOP"); /* delay */
    }
}


/*! @brief Build Frame for transmit. */
static void ENET_BuildPtpEventFrame(void)
{
    uint8_t mGAddr[6] = {0x01, 0x00, 0x5e, 0x01, 0x01, 0x1};
    /* Build for PTP event message frame. */
    memcpy(&g_frame[0], &mGAddr[0], 6);
    /* The six-byte source MAC address. */
    memcpy(&g_frame[6], &g_macAddr[0], 6);
    /* The type/length: if data length is used make sure it's smaller than 1500 */
    g_frame[12]    = 0x08U;
    g_frame[13]    = 0x00U;
    g_frame[0x0EU] = 0x40;
    /* Set the UDP PTP event port number. */
    g_frame[0x24U] = (kENET_PtpEventPort >> 8) & 0xFFU;
    g_frame[0x25U] = kENET_PtpEventPort & 0xFFU;
    /* Set the UDP protocol. */
    g_frame[0x17U] = 0x11U;
    /* Add ptp event message type: sync message. */
    g_frame[0x2AU] = ENET_PTP_SYNC_MSG;
    /* Add sequence id. */
    g_frame[0x48U]     = 0;
    g_frame[0x48U + 1] = 0;
}

static void ethernet_ptptime_enablepps(ENET_Type *base, enet_handle_t handle, enet_ptp_timer_channel_t tmr_ch)
{
    uint32_t next_counter = 0;
    uint32_t tmp_val = 0;
 
 //   sys_mutex_lock(&ptp_mutex);

    /* clear capture or output compare interrupt status if have.
     */
    ENET_Ptp1588ClearChannelStatus(base, tmr_ch);
    
    /* It is recommended to double check the TMODE field in the
     * TCSR register to be cleared before the first compare counter
     * is written into TCCR register. Just add a double check.
     */
    tmp_val = base->CHANNEL[tmr_ch].TCSR;
    do {
        tmp_val &= ~(ENET_TCSR_TMODE_MASK);
        base->CHANNEL[tmr_ch].TCSR = tmp_val;
        tmp_val = base->CHANNEL[tmr_ch].TCSR;
    } while (tmp_val & ENET_TCSR_TMODE_MASK);
		
    tmp_val = (ENET_NANOSECOND_ONE_SECOND >> 1);
		//tmp_val = ENET_NANOSECOND_ONE_SECOND / 2;

    ENET_Ptp1588SetChannelCmpValue(base, tmr_ch, tmp_val);

    /* Calculate the second the compare event timestamp */
    next_counter = tmp_val;

    /* Compare channel setting. */
    ENET_Ptp1588ClearChannelStatus(base, tmr_ch);
    
    ENET_Ptp1588SetChannelOutputPulseWidth(base, tmr_ch, false, 31, true);
    
    /* Write the second compare event timestamp and calculate
     * the third timestamp. Refer the TCCR register detail in the spec.
     */
    ENET_Ptp1588SetChannelCmpValue(base, tmr_ch, next_counter);
    /* Update next counter */
    //next_counter = (next_counter + ETH_PTP_NSEC_PER_SEC) & ENET_TCCR_TCC_MASK;
    handle.ptpNextCounter = next_counter;
		

//    sys_mutex_unlock(&ptp_mutex);
}

static void ethernet_ptptime_init(ENET_Type *base , uint32_t ptp_clk_freq, enet_handle_t handle,
                                   bool pps_en, enet_ptp_timer_channel_t tmr_ch)
{
    enet_ptp_config_t ptp_cfg;

    assert(base);

    /* Config 1588 */
    memset(&ptp_cfg, 0, sizeof(enet_ptp_config_t));
    ptp_cfg.channel = tmr_ch;
    ptp_cfg.ptpTsRxBuffNum = ENET_RXBD_NUM;
    ptp_cfg.ptpTsTxBuffNum = ENET_TXBD_NUM;
    ptp_cfg.rxPtpTsData = &g_rxPtpTsBuff[0];
    ptp_cfg.txPtpTsData = &g_txPtpTsBuff[0];
    ptp_cfg.ptp1588ClockSrc_Hz = ptp_clk_freq;
    ENET_Ptp1588Configure(base, &handle, &ptp_cfg);

    if (true == pps_en)
    {
        //ethernet_ptptime_enablepps(base, handle, tmr_ch);
    }
    else
    {
        ENET_Ptp1588SetChannelMode(base, tmr_ch, kENET_PtpChannelDisable, false);
    }
}

void ethernet_ptptime_adjfreq(ENET_Type *base, int32_t incps)
{
    int32_t neg_adj = 0;
    uint32_t corr_inc, corr_period;

    assert(base);

    /*
     * incps means the increment rate (nanseconds per second)by which to
     * slow down or speed up the slave timer.
     * Positive ppb need to speed up and negative value need to slow down.
    */

    if (0 == incps)
    {
        base->ATCOR &= ~ENET_ATCOR_COR_MASK; /* Reset PTP frequency */
        return;
    }
	
    if (incps < 0)
    {
        incps = -incps;
        neg_adj = 1;
    }

    corr_period = (uint32_t)PTP_CLOCK_FRE_RT/incps;

    /* neg_adj = 1, slow down timer, neg_adj = 0, speed up timer */
    corr_inc = (neg_adj) ? (PTP_AT_INC - 1) : (PTP_AT_INC + 1);	

    ENET_Ptp1588AdjustTimer(base, corr_inc, corr_period);

}

void ethernet_ptptime_adjtime(ENET_Type *base, enet_handle_t handle, int32_t second, int32_t nano_second)
{
    enet_ptp_time_t tmp_ts;
    enet_ptp_time_t current_ts;
    int32_t tmp_adj_sec = 0, tmp_adj_nsec = 0;
    int32_t carry_flag = 0;

    assert(nano_second < ENET_NANOSECOND_ONE_SECOND);
    assert(nano_second > -ENET_NANOSECOND_ONE_SECOND);
    
    ethernet_ptptime_gettime(base, handle,&current_ts);
    tmp_adj_nsec = current_ts.nanosecond - nano_second;
	tmp_adj_sec = current_ts.second - second;
	
    if (tmp_adj_nsec > ENET_NANOSECOND_ONE_SECOND)
    {
        tmp_adj_sec += 1;
        tmp_adj_nsec -= ENET_NANOSECOND_ONE_SECOND;
    }

	if (tmp_adj_sec > 0  && tmp_adj_nsec < 0)
    {
        carry_flag = - 1;
        tmp_adj_nsec += ENET_NANOSECOND_ONE_SECOND;
    } else if (tmp_adj_sec < 0  && tmp_adj_nsec > 0)
    {
        carry_flag = 1;
		tmp_adj_nsec -= ENET_NANOSECOND_ONE_SECOND;
    }
	
    tmp_adj_sec = current_ts.second + carry_flag;
    assert(tmp_adj_sec >= 0);
    assert(tmp_adj_nsec >= 0);

    tmp_ts.second = tmp_adj_sec;
    tmp_ts.nanosecond = tmp_adj_nsec;
    ethernet_ptptime_settime(base, handle, &tmp_ts);
}

/*******************************************************************************
* Function Name  : ethernet_ptptime_settime
* Description    : Initialize time base
* Input          : Time with sign
* Output         : None
* Return         : None
*******************************************************************************/
void ethernet_ptptime_settime(ENET_Type *base, enet_handle_t handle,enet_ptp_time_t *timestamp)
{
    assert(base);

    ENET_Ptp1588SetTimer(base, &handle, timestamp);

//    ethernet_ptptime_enablepps(ptp_ethernetif, ETH_PTP_DEFAULT_PPS_CHANNEL);
}

void ethernet_ptptime_gettime(ENET_Type *base, enet_handle_t handle, enet_ptp_time_t *timestamp)
{
    assert(base);

    ENET_Ptp1588GetTimer(base, &handle, timestamp);
}


/*!
 * @brief Main function
 */
int main(void)
{
    enet_config_t config;
    uint32_t length = 0;
    uint32_t sysClock;
    uint32_t ptpClock;
    uint32_t count = 0;
    bool link      = false;
    phy_speed_t speed;
    phy_duplex_t duplex;
    status_t result;
    uint32_t txnumber = 0;
    enet_ptp_time_data_t ptpData;
    enet_ptp_time_t ptpTime;
		enet_ptp_time_t setTime;
    uint32_t totalDelay;
    status_t status;
    uint8_t mGAddr[6] = {0x01, 0x00, 0x5e, 0x01, 0x01, 0x1};
		
		int my_offset = 0;
		
		pit_config_t pitConfig;
		
    /* Hardware Initialization. */
    //gpio_pin_config_t gpio_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
		//gpio_pin_config_t sw_config = {kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge,};
		gpio_pin_config_t sw_config = {kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge};

    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitModuleClock();
		
		CLOCK_EnableClock (kCLOCK_Gpio1);
		//CLOCK_EnableClock (kCLOCK_Gpio5);
		//CLOCK_EnableClock (kCLOCK_Gpio2);
		//CLOCK_SetMux (kCLOCK_PerclkMux, 1U);
		//CLOCK_SetDiv (kCLOCK_PerclkDiv, 0U);
		
		//GPIO_PinWrite (GPIO5, 0U, 0);
		//GPIO5->GDIR |= (1U << 0U);
		GPIO_PinWrite (GPIO1, 2U, 0);
		GPIO1->GDIR |= (1U << 2U);
		/*converst*/
		//GPIO_PinWrite (GPIO2, 13U, 0);
		//GPIO2->GDIR |= (1U << 13U);
		
		CLOCK_EnableClock (kCLOCK_Gpio3);
		GPIO_PinWrite (GPIO3, 12U, 0);
		GPIO3->GDIR |= (1U << 12U);
		
		//PIT_GetDefaultConfig (&pitConfig);

    IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, true);
		
		//PIT_Init (PIT, &pitConfig);
		//PIT_SetTimerPeriod (PIT, kPIT_Chnl_0, NSEC_TO_COUNT(2500U, PIT_SOURCE_CLOCK));
		
		
		EnableIRQ(EXAMPLE_SW_IRQ);
		//EnableIRQ(PIT_IRQ_ID);
		
		//CLOCK_EnableClock (kCLOCK_Gpio1);
		
		GPIO_PinInit(EXAMPLE_SW_GPIO, EXAMPLE_SW_GPIO_PIN, &sw_config);
		GPIO_PortEnableInterrupts(EXAMPLE_SW_GPIO, 1U << EXAMPLE_SW_GPIO_PIN);
		//PIT_EnableInterrupts (PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
		
		//PIT_StartTimer (PIT, kPIT_Chnl_0);		
//    GPIO_PinInit(GPIO1, 9, &gpio_config);
//		//GPIO_PinInit(GPIO1, 2, &gpio_config);
//    
//		GPIO_PinInit(GPIO1, 19, &gpio_config);
//		GPIO_WritePinOutput(GPIO1, 19, 0);
//		GPIO_PinInit(GPIO1, 10, &gpio_config);
//    /* pull up the ENET_INT before RESET. */
//    GPIO_WritePinOutput(GPIO1, 10, 1);
//    GPIO_WritePinOutput(GPIO1, 9, 0);
//    delay();
//    GPIO_WritePinOutput(GPIO1, 9, 1);		
		//GPIO_WritePinOutput(GPIO1, 2, 0);
    //delay();
    //GPIO_WritePinOutput(GPIO1, 2, 1);
		//GPIO_PinInit (GPIO1, 18, &gpio_config);
		//GPIO_WritePinOutput (GPIO1, 18U, 0);

    PRINTF("\r\n ENET PTP 1588 example start.\r\n");


    /* prepare the buffer configuration. */
    enet_buffer_config_t buffConfig[] = {{
        ENET_RXBD_NUM,
        ENET_TXBD_NUM,
        SDK_SIZEALIGN(ENET_RXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT),
        SDK_SIZEALIGN(ENET_TXBUFF_SIZE, APP_ENET_BUFF_ALIGNMENT),
        &g_rxBuffDescrip[0],
        &g_txBuffDescrip[0],
        &g_rxDataBuff[0][0],
        &g_txDataBuff[0][0],
    }};

    sysClock = CORE_CLK_FREQ;
    ptpClock = PTP_CLK_FREQ;
		
		
		
    /* Prepare the PTP configure */
//    enet_ptp_config_t ptpConfig = {
//        ENET_RXBD_NUM, ENET_TXBD_NUM, &g_rxPtpTsBuff[0], &g_txPtpTsBuff[0], kENET_PtpTimerChannel1, ptpClock,
//    };

    /*
     * config.miiMode = kENET_RmiiMode;
     * config.miiSpeed = kENET_MiiSpeed100M;
     * config.miiDuplex = kENET_MiiFullDuplex;
     * config.rxMaxFrameLen = ENET_FRAME_MAX_FRAMELEN;
     */
    ENET_GetDefaultConfig(&config);
    //status = PHY_Init(EXAMPLE_ENET, EXAMPLE_PHY, sysClock);
		#if 0
		status = PHY_Init(EXAMPLE_ENET, EXAMPLE_PHY, sysClock);
    while (status != kStatus_Success)
    {
        PRINTF("\r\nPHY Auto-negotiation failed. Please check the cable connection and link partner setting.\r\n");
        //status = PHY_Init(EXAMPLE_ENET, EXAMPLE_PHY, sysClock);
			status = PHY_Init(EXAMPLE_ENET, EXAMPLE_PHY, sysClock);
    }

    //PHY_GetLinkStatus(EXAMPLE_ENET, EXAMPLE_PHY, &link);
		PHY_GetLinkStatus(EXAMPLE_ENET, EXAMPLE_PHY, &link);
    if (link)
    {
        /* Get the actual PHY link speed. */
        //PHY_GetLinkSpeedDuplex(EXAMPLE_ENET, EXAMPLE_PHY, &speed, &duplex);
			  PHY_GetLinkSpeedDuplex(EXAMPLE_ENET, EXAMPLE_PHY, &speed, &duplex);
        /* Change the MII speed and duplex for actual link status. */
        config.miiSpeed  = (enet_mii_speed_t)speed;
        config.miiDuplex = (enet_mii_duplex_t)duplex;
    }
		#endif
		
		config.interrupt |= kENET_RxFrameInterrupt | kENET_TxFrameInterrupt;
		
    /* Initialize ENET. */
    ENET_Init(EXAMPLE_ENET, &g_handle, &config, &buffConfig[0], &g_macAddr[0], sysClock);
    /* Configure PTP */
    //ENET_Ptp1588Configure(EXAMPLE_ENET, &g_handle, &ptpConfig);
		ethernet_ptptime_init (EXAMPLE_ENET, PTP_CLOCK_FRE_RT, g_handle,PTP_TEST_APP_ENABLE, PTP_TEST_APP_CHANNEL);
    
		
		
		/* Add to multicast group to receive ptp multicast frame. */
    //ENET_AddMulticastGroup(EXAMPLE_ENET, &mGAddr[0]);
    /* Active ENET read. */
    //ENET_ActiveRead(EXAMPLE_ENET);
		//PRINTF ("Pragram over\r\n");
	
		setTime.second = 0;
		setTime.nanosecond = HALF_SEC + 500;
		//setTime.nanosecond = 0;
		
		//uint32_t array[10] = {0};
		
		//test while
		/*
		while (1)
		{
				if (g_InputSignal)
				{
						
								g_array[g_c] = g_theTime.nanosecond;
								g_c += 1;
								ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2500);        //positive speed up
								if (g_c == 10)
								{
										for (int i = 0; i < 10; i++)
										{
												PRINTF ("array[%d] = %d\r\n", i, g_array[i]);
										}
										g_c = 0;
								}
							
						g_InputSignal = false;
				}
		}
		*/
		
		//GPIO Interrupt
		while (1)
		{
				if (g_InputSignal)
				{

						//PRINTF ("PPS IN\r\n");
					
						//delay();
						//if (1 == GPIO_PinRead(EXAMPLE_SW_GPIO, EXAMPLE_SW_GPIO_PIN))
						//{
								//PRINTF ("temp = %d\r\n", g_temp);
								//PRINTF ("startTime = %d\r\n endTime = %d\r\n", theTime.nanosecond, endTime.nanosecond);
								//g_temp = 0;
								g_array[g_c] = g_tmpTime.nanosecond;
								g_c += 1;
							
									#if 0
									if (g_tmpTime.nanosecond > HALF_SEC && g_tmpTime.nanosecond < (HALF_SEC + 5000))
									{
											my_offset += (g_tmpTime.nanosecond - HALF_SEC);
									}
									else if (g_tmpTime.nanosecond < HALF_SEC && g_tmpTime.nanosecond > (HALF_SEC - 5000))
									{
											my_offset += (g_tmpTime.nanosecond - HALF_SEC);
									}
									else
									{
											//setTime.second = g_theTime.second;
											//ethernet_ptptime_adjfreq (EXAMPLE_ENET, 0);
											//ethernet_ptptime_settime (EXAMPLE_ENET, g_handle, &setTime);
											//goto END_INT;
									}
									
									/*if (my_offset >= 0)
									{
											setTime.second = g_tmpTime.second;
									}
									else
									{
											setTime.second = g_tmpTime.second + 1;
									}*/
									setTime.second = g_tmpTime.second;
									ethernet_ptptime_settime (EXAMPLE_ENET, g_handle, &setTime);
									
									/*
									if (my_offset > 1000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2400);
									}
									else if (my_offset < -1000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 4000);
									}
									else
									{
											if (my_offset < 0)
											{
													ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2400);
											}
											else
											{
													ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2600);
											}
									}
									//my_offset = 0;
									*/
									g_array[g_c] = my_offset;
									//g_array[g_c] = g_tmpTime.second;
									g_c += 1;
									
									if (my_offset < -4000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 4000);
									}
									else if (my_offset < -3000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 3800);
									}
									else if (my_offset < -2000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 3250);
									}
									else if (my_offset < -1000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 3000);
									}
									else if (my_offset < 0 && my_offset > -500)
									{
											//ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2750);
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2850);
									}
									else if (my_offset > 4000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, -2000);
									}
									else if (my_offset > 3000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, -1000);
									}
									else if (my_offset > 2000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2250);
									}
									else if (my_offset > 1000)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2925);
									}
									else if (my_offset > 0 && my_offset < 500)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 3100);
									}
									else if (my_offset > 0)
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2950);
									}
									else
									{
											ethernet_ptptime_adjfreq (EXAMPLE_ENET, 2900);
									}
									
									
									my_offset = 0;
									#endif
									
								if (g_tmpTime.nanosecond > HALF_SEC && g_tmpTime.nanosecond < (HALF_SEC + 10000))
									{
											my_offset += (g_tmpTime.nanosecond - HALF_SEC);
									}
									else if (g_tmpTime.nanosecond < HALF_SEC && g_tmpTime.nanosecond > (HALF_SEC - 10000))
									{
											my_offset += (g_tmpTime.nanosecond - HALF_SEC);
									}
								
									setTime.second = g_tmpTime.second;
									ethernet_ptptime_settime (EXAMPLE_ENET, g_handle, &setTime);
									ethernet_ptptime_adjfreq (EXAMPLE_ENET, -my_offset);
									
									//my_offset = 0;
									
								
								if (g_c == 20)
								{
										for (int i = 0; i < 20; i++)
										{
												PRINTF ("array[%d] = %d\r\n", i, g_array[i]);
										}
										
										g_c = 0;
								}
							
						//}	
						g_InputSignal = false;
				}
		}
}
