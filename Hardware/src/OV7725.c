#include "includes.h"
#include "fsl_edma_hal.h"
#include "fsl_dmamux_hal.h"
#include "SCCB.h"
#include "OV7725.h"
#include "OV7725_REG.h"
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define DEBUG_OUT   printf

#define OV7725_Delay_ms(time)  delay_ms(time)

#define OV7725_ID           0x21

uint8_t 	*IMG_BUFF;

xSemaphoreHandle mutex;

volatile IMG_STATE	    img_flag = IMG_FINISH;		//ͼ��״̬

static uint8_t Ov7725_reg_Init(void);

static edma_software_tcd_t stcd;

// �˿�����
void data_port_config(void)
{
    int i;
    
    for (i = 0; i < 8; ++i) {
        PORT_HAL_SetMuxMode(PORTB, i, kPortMuxAsGpio); // ����ΪGPIOģʽ
        GPIO_HAL_SetPinDir(PTB, i, kGpioDigitalInput); // ��������ģʽ
    }
}

// DMA��������
void dma_trans_config()
{
    edma_transfer_config_t config = {
        .srcAddr = (uint32_t)&PTB->PDIR,
        .destAddr = (uint32_t)IMG_BUFF,
        .srcTransferSize = kEDMATransferSize_1Bytes,
        .destTransferSize = kEDMATransferSize_1Bytes,
        .srcOffset = 0,
        .destOffset = 1,
        .srcLastAddrAdjust = 0,
        .destLastAddrAdjust = 0,
        .srcModulo = kEDMAModuloDisable,
        .destModulo = kEDMAModuloDisable,
        .minorLoopCount = 1,
        .majorLoopCount = CAMERA_SIZE
    };
    
    /* Set the software TCD memory to default value. */
    memset(&stcd, 0, sizeof(edma_software_tcd_t));
    
    // DMA����ر�, DMA�жϴ�
    EDMA_HAL_STCDSetBasicTransfer(DMA0, &stcd, &config, true, true);
    EDMA_HAL_STCDSetDisableDmaRequestAfterTCDDoneCmd(&stcd, true); // ���δ���ģʽ
    EDMA_HAL_HTCDClearReg(DMA0, 0);
    EDMA_HAL_PushSTCDToHTCD(DMA0, 0, &stcd);
}

void Ov7725_exti_Init(void)
{
    mutex = xSemaphoreCreateBinary(); // �����ź���
    
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    SIM_HAL_EnableClock(SIM, kSimClockGatePortB);
    SIM_HAL_EnableClock(SIM, kSimClockGateDma0);
    SIM_HAL_EnableClock(SIM, kSimClockGateDmamux0);
    
    data_port_config();
    
    // DMAͨ��0��ʼ����PTA27�����ش���DMA���䣬Դ��ַΪPTD_BYTE0_IN��Ŀ�ĵ�ַΪ��BUFF ��ÿ�δ���1Byte������CAMERA_SIZE�κ�ֹͣ����
    DMAMUX_HAL_Init(DMAMUX);
    DMAMUX_HAL_SetChannelCmd(DMAMUX, 0, true);
    DMAMUX_HAL_SetPeriodTriggerCmd(DMAMUX, 0, false); // �������Զ�����
    DMAMUX_HAL_SetTriggerSource(DMAMUX, 0, 49); // ����DMAMUX�Ĵ���ԴΪPORTA (49��)

    EDMA_HAL_Init(DMA0);
    EDMA_HAL_SetChannelArbitrationMode(DMA0, kEDMAChnArbitrationRoundrobin);
    EDMA_HAL_SetHaltOnErrorCmd(DMA0, false);
    dma_trans_config(); // DMA����
    NVIC_EnableIRQ(DMA0_DMA16_IRQn); // DMA�ж�ʹ��
    
    // PCLK����
    PORT_HAL_SetMuxMode(PORTA, 27, kPortMuxAsGpio); // ����ΪGPIOģʽ
    GPIO_HAL_SetPinDir(PTA, 27, kGpioDigitalInput); // ��������ģʽ
    PORT_HAL_SetPinIntMode(PORTA, 27, kPortDmaFallingEdge); // DMA����Դ
    PORT_HAL_SetPullMode(PORTA, 27, kPortPullDown); // ����
    PORT_HAL_SetPassiveFilterCmd(PORTA, 27, true); // ��ͨ�˲�
    
    // ���ж�����
    PORT_HAL_SetMuxMode(PORTA, 29, kPortMuxAsGpio); // ����ΪGPIOģʽ
    GPIO_HAL_SetPinDir(PTA, 29, kGpioDigitalInput); // ��������ģʽ
    PORT_HAL_SetPullMode(PORTA, 29, kPortPullDown); // ����
    PORT_HAL_SetPinIntMode(PORTA, 29, kPortIntDisabled); // �ر�PTA29�ж�
    NVIC_EnableIRQ(PORTA_IRQn); // ʹ��PTA���ж�
}

/*!
 *  @brief      ӥ��ov7725���жϷ�����
 *  @since      v5.0
 */
void ov7725_eagle_vsync(void)
{
    PORTA->ISFR  = ~0; // ������жϱ�־, ��ֹDMA�ഫ������
    // ���ж���Ҫ�ж��ǳ��������ǳ���ʼ
    if(img_flag == IMG_START) // ��Ҫ��ʼ�ɼ�ͼ��
    {
        img_flag = IMG_GATHER; // ���ͼ��ɼ���
        
        PORT_HAL_SetPinIntMode(PORTA, 29, kPortIntDisabled); // �ر�PTA29�ж�
        DMA0->SERQ = 0; // ʹ��ͨ��0��������
    }
    else //ͼ��ɼ�����
    {
        PORT_HAL_SetPinIntMode(PORTA, 29, kPortIntDisabled); // �ر�PTA29�ж�
        img_flag = IMG_FAIL;  // ���ͼ��ɼ�ʧ��
    }
}

/*!
 *  @brief      ӥ��ov7725 DMA�жϷ�����
 *  @since      v5.0
 */
void ov7725_eagle_dma(void)
{
    static BaseType_t xHigherPriorityTaskWoken;
    
    img_flag = IMG_FINISH;
    EDMA_HAL_ClearIntStatusFlag(DMA0, kEDMAChannel0); // ���жϱ�־
    xSemaphoreGiveFromISR(mutex, &xHigherPriorityTaskWoken); // ���ж��з����ź���
}

/*!
 *  @brief      ӥ��ov7725�ɼ�ͼ�񣨲ɼ��������ݴ洢�� ��ʼ��ʱ���õĵ�ַ�ϣ�
 *  @since      v5.0
 */
void ov7725_get_img(void)
{
    img_flag = IMG_START;					                // ��ʼ�ɼ�ͼ��
    
    EDMA_HAL_HTCDClearReg(DMA0, 0);                         // DMAͨ��0��ʼ��
    EDMA_HAL_PushSTCDToHTCD(DMA0, 0, &stcd);                // DMAͨ��0����
    PORT_HAL_ClearPortIntFlag(PORTA);                       // ���жϱ�־λ
    PORT_HAL_SetPinIntMode(PORTA, 29, kPortIntFallingEdge); // PTA29�½��ش����ж�
    while(img_flag != IMG_FINISH)                           // �ȴ�ͼ��ɼ����
    {
        // �ȴ��ź���, �������Բ�ռ��CPU(�����һֱ��ѯ)
        xSemaphoreTake(mutex, portMAX_DELAY);
        if(img_flag == IMG_FAIL)                            // ����ͼ��ɼ����������¿�ʼ�ɼ�
        {
            img_flag = IMG_START;			                // ��ʼ�ɼ�ͼ��
            EDMA_HAL_HTCDClearReg(DMA0, 0);                 // DMAͨ��0��ʼ��
            EDMA_HAL_PushSTCDToHTCD(DMA0, 0, &stcd);        // DMAͨ��0����
            PORT_HAL_ClearPortIntFlag(PORTA);               // ���жϱ�־λ
            PORT_HAL_SetPinIntMode(PORTA, 29, kPortIntFallingEdge); // PTA29�½��ش����ж�
        }
    }
}

/*OV7725��ʼ�����ñ�*/
Register_Info ov7725_eagle_reg[] =
{
    //�Ĵ������Ĵ���ֵ
    {OV7725_COM4         , 0xC0}, /* 0xC0: PLL:8x, Full Window */
    {OV7725_CLKRC        , 0x01}, /* 0x00: 150Fps */
    {OV7725_COM2         , 0x03},
    {OV7725_COM3         , 0xD0},
    {OV7725_COM7         , 0x40},
    {OV7725_HSTART       , 0x3F},
    {OV7725_HSIZE        , 0x50},
    {OV7725_VSTRT        , 0x03},
    {OV7725_VSIZE        , 0x78},
    {OV7725_HREF         , 0x00},
    {OV7725_SCAL0        , 0x0A},
    {OV7725_AWB_Ctrl0    , 0xE0},
    {OV7725_DSPAuto      , 0xff},
    {OV7725_DSP_Ctrl2    , 0x0C},
    {OV7725_DSP_Ctrl3    , 0x00},
    {OV7725_DSP_Ctrl4    , 0x00},

#if (CAMERA_W == 80)
    {OV7725_HOutSize     , 0x14},
#elif (CAMERA_W == 160)
    {OV7725_HOutSize     , 0x28},
#elif (CAMERA_W == 240)
    {OV7725_HOutSize     , 0x3c},
#elif (CAMERA_W == 320)
    {OV7725_HOutSize     , 0x50},
#else

#endif

#if (CAMERA_H == 60 )
    {OV7725_VOutSize     , 0x1E},
#elif (CAMERA_H == 120 )
    {OV7725_VOutSize     , 0x3c},
#elif (CAMERA_H == 180 )
    {OV7725_VOutSize     , 0x5a},
#elif (CAMERA_H == 240 )
    {OV7725_VOutSize     , 0x78},
#else

#endif

    {OV7725_EXHCH        , 0x00},
    {OV7725_GAM1         , 0x0c},
    {OV7725_GAM2         , 0x16},
    {OV7725_GAM3         , 0x2a},
    {OV7725_GAM4         , 0x4e},
    {OV7725_GAM5         , 0x61},
    {OV7725_GAM6         , 0x6f},
    {OV7725_GAM7         , 0x7b},
    {OV7725_GAM8         , 0x86},
    {OV7725_GAM9         , 0x8e},
    {OV7725_GAM10        , 0x97},
    {OV7725_GAM11        , 0xa4},
    {OV7725_GAM12        , 0xaf},
    {OV7725_GAM13        , 0xc5},
    {OV7725_GAM14        , 0xd7},
    {OV7725_GAM15        , 0xe8},
    {OV7725_SLOP         , 0x20},
    {OV7725_LC_RADI      , 0x00},
    {OV7725_LC_COEF      , 0x13},
    {OV7725_LC_XC        , 0x08},
    {OV7725_LC_COEFB     , 0x14},
    {OV7725_LC_COEFR     , 0x17},
    {OV7725_LC_CTR       , 0x05},
    {OV7725_BDBase       , 0x99},
    {OV7725_BDMStep      , 0x03},
    {OV7725_SDE          , 0x04},
    {OV7725_BRIGHT       , 0x00},
    {OV7725_CNST         , 0x60},  /* �Աȶ�(��ֵ), ���� */
    //{OV7725_CNST         , 0x50},  /* �Աȶ�(��ֵ), ���� */
    {OV7725_SIGN         , 0x06},
    {OV7725_UVADJ0       , 0x11},
    {OV7725_UVADJ1       , 0x02},
};

uint8_t cfgnum = sizeof(ov7725_eagle_reg) / sizeof(ov7725_eagle_reg[0]); /*�ṹ�������Ա��Ŀ*/



/************************************************
 * ��������Ov7725_Init
 * ����  ��Sensor��ʼ��
 * ����  ����
 * ���  ������1�ɹ�������0ʧ��
 * ע��  ����
 ************************************************/
uint8_t Ov7725_Init(uint8_t *imgaddr)
{
    IMG_BUFF = imgaddr;
    while(Ov7725_reg_Init() == 0);
    Ov7725_exti_Init();
    return 0;
}

/************************************************
 * ��������Ov7725_reg_Init
 * ����  ��Sensor �Ĵ��� ��ʼ��
 * ����  ����
 * ���  ������1�ɹ�������0ʧ��
 * ע��  ����
 ************************************************/
uint8_t Ov7725_reg_Init(void)
{
    uint16_t i = 0;
    uint8_t Sensor_IDCode = 0;
    SCCB_GPIO_init();

    OV7725_Delay_ms(50);
    while( 0 == SCCB_WriteByte ( 0x12, 0x80 ) ) /*��λsensor */
    {
        i++;
        if(i == 20)
        {
            DEBUG_OUT("����:SCCBд���ݴ���.\n");
            //OV7725_Delay_ms(50);
            return 0 ;
        }
    }
    OV7725_Delay_ms(50);
    if( 0 == SCCB_ReadByte( &Sensor_IDCode, 1, 0x0b ) )	 /* ��ȡsensor ID��*/
    {
        DEBUG_OUT("����:��ȡIDʧ��.\n");
        return 0;
    }
    DEBUG_OUT("Get ID success��SENSOR ID is 0x%x\n", Sensor_IDCode);
    DEBUG_OUT("Config Register Number is %d\n", cfgnum);
    if(Sensor_IDCode == OV7725_ID)
    {
        for( i = 0 ; i < cfgnum ; i++ )
        {
            if( 0 == SCCB_WriteByte(ov7725_eagle_reg[i].Address, ov7725_eagle_reg[i].Value) )
            {
                DEBUG_OUT("����:д�Ĵ���0x%xʧ��.\n", ov7725_eagle_reg[i].Address);
                return 0;
            }
        }
    }
    else
    {
        return 0;
    }
    DEBUG_OUT("OV7725 Register Config Success!\n");
    return 1;
}


