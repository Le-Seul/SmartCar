#include "bsp.h"
#include "includes.h"
#include "uart_debug.h"
#include "mpu6050.h"
#include "fsl_adc16_hal.h"
#include "OLED_I2C.h"
#include <OLED_I2C.h>

uint8_t image_buff[CAMERA_SIZE];

// FTM3��ʼ��
static void FTM3_Init(void)
{
    ftm_pwm_param_t ftmParam = {
        .mode                   = kFtmEdgeAlignedPWM,
        .edgeMode               = kFtmHighTrue,
    };

    // FTM3ʱ��ʹ��
    SIM_HAL_EnableClock(SIM, kSimClockGateFtm3);
    FTM_HAL_Init(FTM3);
    FTM_HAL_SetSyncMode(FTM3, kFtmUseSoftwareTrig); // �������
    FTM_HAL_SetClockSource(FTM3, kClock_source_FTM_SystemClk); // FTMʱ��Դ

    // PWM��ʼ��
    /* Clear the overflow flag */
    FTM_HAL_ClearTimerOverflow(FTM3);

    // ��ߵ������
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 0);
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 1);

    // �ұߵ������
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 2);
    FTM_HAL_EnablePwmMode(FTM3, &ftmParam, 3);
    FTM_HAL_SetMod(FTM3, MOTER_PWM_RANGE - 1);

    // PWMռ�ձȳ�ʼ��Ϊ 0%
    FTM_HAL_SetChnCountVal(FTM3, 0, 0);
    FTM_HAL_SetChnCountVal(FTM3, 1, 0);
    FTM_HAL_SetChnCountVal(FTM3, 2, 0);
    FTM_HAL_SetChnCountVal(FTM3, 3, 0);
    FTM_HAL_SetClockPs(FTM3, kFtmDividedBy1); // ��Ƶ
    FTM_HAL_SetSoftwareTriggerCmd(FTM3, true); // ��������
}

// ���PWM��ʼ��
static void motor_init(void)
{
    // ���ʹ��FTM3
    SIM_HAL_EnableClock(SIM, kSimClockGatePortD);

    // PWM������ŵĶ˿ڸ���Ϊ FTM3_CHn
    PORT_HAL_SetMuxMode(PORTD, 0, kPortMuxAlt4);
    PORT_HAL_SetMuxMode(PORTD, 1, kPortMuxAlt4);
    PORT_HAL_SetMuxMode(PORTD, 2, kPortMuxAlt4);
    PORT_HAL_SetMuxMode(PORTD, 3, kPortMuxAlt4);

    // �������ģ���Ƭѡ�ź�
    PORT_HAL_SetMuxMode(PORTD, 4, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTD, 4, kGpioDigitalOutput);
    GPIO_HAL_WritePinOutput(PTD, 4, 1); // BTN7971�ߵ�ƽʹ��
    
    FTM3_Init();
}

// ���������ʹ��
void BSP_MoterEnable(void)
{
    GPIO_HAL_WritePinOutput(PTD, 4, 1);
}

// ���������ʧ��
void BSP_MoterDisable(void)
{
    GPIO_HAL_WritePinOutput(PTD, 4, 0);
}

// ������������ʼ��
void FTM_QuadPhaseEncodeInit(FTM_Type *ftmBase)
{
    FTM_HAL_SetQuadMode(ftmBase, kFtmQuadPhaseEncode);
    FTM_HAL_SetQuadPhaseAFilterCmd(ftmBase, true);

    /* Set Phase A filter value if phase filter is enabled */
    FTM_HAL_SetChnInputCaptureFilter(ftmBase, CHAN0_IDX, 10);
    FTM_HAL_SetQuadPhaseBFilterCmd(ftmBase, true);

    /* Set Phase B filter value if phase filter is enabled */
    FTM_HAL_SetChnInputCaptureFilter(ftmBase, CHAN1_IDX, 10);
    FTM_HAL_SetQuadPhaseAPolarity(ftmBase, kFtmQuadPhaseNormal);
    FTM_HAL_SetQuadPhaseBPolarity(ftmBase, kFtmQuadPhaseNormal);

    FTM_HAL_SetQuadDecoderCmd(ftmBase, true);
    FTM_HAL_SetMod(ftmBase, 0xFFFF); // �˲���û��ʲô����

    /* Set clock source to start the counter */
    FTM_HAL_SetClockSource(ftmBase, kClock_source_FTM_SystemClk);
}

// ������������ʼ��
static void RotaryEncoder_Init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    // ��߱�����
    PORT_HAL_SetMuxMode(PORTA, 10, kPortMuxAlt6);
    PORT_HAL_SetMuxMode(PORTA, 11, kPortMuxAlt6);
    // �ұ߱�����
    PORT_HAL_SetMuxMode(PORTA, 12, kPortMuxAlt7);
    PORT_HAL_SetMuxMode(PORTA, 13, kPortMuxAlt7);

    // ����FTMʱ��
    SIM_HAL_EnableClock(SIM, kSimClockGateFtm1);
    SIM_HAL_EnableClock(SIM, kSimClockGateFtm2);

    FTM_QuadPhaseEncodeInit(FTM1);
    FTM_QuadPhaseEncodeInit(FTM2);
}

// ң�س�ʼ��
static void remote_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    PORT_HAL_SetMuxMode(PORTA, 15u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTA, 15u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTA, 15u, kPortPullDown); // ����
}

// ��������ʼ��
static void buzzer_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortA);
    PORT_HAL_SetMuxMode(PORTA, 14, kPortMuxAsGpio);
    PORT_HAL_SetDriveStrengthMode(PORTA, 14, kPortHighDriveStrength);
    GPIO_HAL_SetPinDir(PTA, 14, kGpioDigitalOutput);
}

// ADC��ʼ��(���ڲ�����ص�ѹ)
void adc_init(void)
{
    adc16_converter_config_t config = {
        .lowPowerEnable = true,
        .clkDividerMode = kAdc16ClkDividerOf8,
        .longSampleTimeEnable = 8,
        .resolution = kAdc16ResolutionBitOfSingleEndAs12, // 12bit
        .clkSrc = kAdc16ClkSrcOfAsynClk,
        .asyncClkEnable = true,
        .highSpeedEnable = false,
        .longSampleCycleMode = kAdc16LongSampleCycleOf24,
        .hwTriggerEnable = false,
        .refVoltSrc = kAdc16RefVoltSrcOfVref,
        .continuousConvEnable = false,
        .dmaEnable = false
    };

    // Enable clock for ADC.
    SIM_HAL_EnableClock(SIM, kSimClockGateAdc0);
    ADC16_HAL_Init(ADC0);
    ADC16_HAL_ConfigConverter(ADC0, &config);
}

// ������ʼ��
static void key_init(void)
{
    SIM_HAL_EnableClock(SIM, kSimClockGatePortB);

    PORT_HAL_SetMuxMode(PORTB, 20u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 20u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 20u, kPortPullUp); // ����
    PORT_HAL_SetMuxMode(PORTB, 21u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 21u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 21u, kPortPullUp); // ����
    PORT_HAL_SetMuxMode(PORTB, 22u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 22u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 22u, kPortPullUp); // ����
    PORT_HAL_SetMuxMode(PORTB, 23u, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 23u, kGpioDigitalInput);
    PORT_HAL_SetPullMode(PORTB, 23u, kPortPullUp); // ����
    
}

// Ӳ����ʼ��
void board_init(void)
{
    uart_debug_init(115200);
    key_init();
    adc_init();
    motor_init();
    RotaryEncoder_Init();
    mpu6050_init();
    Ov7725_Init(image_buff);
    OLED_Init();
    remote_init();
    buzzer_init();
}

// ��ȡ����Ƶ��
uint32_t BSP_GetBusClockFreq(void)
{
    return SystemCoreClock / (CLOCK_HAL_GetOutDiv2(SIM) + 1);
}

// ��ȡ��ת����������ֵ
// left: 1:��߱�����, 0: �ұ߱�����
int BSP_GetEncoderCount(int left)
{
    int count;
    FTM_Type *ftmBase = left == LEFT ? FTM2 : FTM1;

    count = (int16_t)FTM_HAL_GetCounter(ftmBase); 
    FTM_HAL_SetCounter(ftmBase, 0);
    if (left == RIGHT) { // ���ַ����෴
        count = -count;
    }
    return count;
}

// ���õ��ת��
void BSP_SetMotorSpeed(int left, int value)
{
    uint16_t pwm1, pwm2;

    if (value > 0) {
        pwm1 = value;
        pwm2 = 0;
    } else {
        pwm1 = 0;
        pwm2 = -value;
    }
    if (left == LEFT) { // ����
        FTM_HAL_SetChnCountVal(FTM3, 0, pwm2);
        FTM_HAL_SetChnCountVal(FTM3, 1, pwm1);
    } else { // ����
        FTM_HAL_SetChnCountVal(FTM3, 2, pwm1);
        FTM_HAL_SetChnCountVal(FTM3, 3, pwm2);
    }
    FTM_HAL_SetSoftwareTriggerCmd(FTM3, true); // ��������
}

// �������������ѹ(PWMֵ)����
void BSP_MoterThresholdTest(void)
{
    int i, speedCnt, cnt_L = 0, cnt_R = 0;

    // PWMֵ�����Ӵ�
    for (i = 0; i < MOTER_PWM_RANGE / 10; ++i) {
        BSP_SetMotorSpeed(LEFT, i);
        BSP_SetMotorSpeed(RIGHT, i);
        // ����������ѹ����
        speedCnt = BSP_GetEncoderCount(LEFT);
        if (speedCnt > 10 && cnt_L++ < 20) { // ���˱�����С���ȵĶ���, ������20��ֵ
            printf("Left moter PWM: %d, speed count: %d\n", i, speedCnt);
        }
        // ����������ѹ����
        speedCnt = BSP_GetEncoderCount(RIGHT);
        if (speedCnt > 10 && cnt_R++ < 200) { // ���˱�����С���ȵĶ���, ������20��ֵ
            printf("Right moter PWM: %d, speed count: %d\n", i, speedCnt);
        }
        delay_ms(100);
    }
}

// ���Ե��ת�ٺͱ������ٶȵĹ�ϵ
void BSP_MoterSpeedTest(void)
{
    BSP_SetMotorSpeed(LEFT, 1000);
    BSP_SetMotorSpeed(RIGHT, 1000);
    while (1) {
        delay_ms(100);
        printf("PWM: 1000, time: 100ms left speed: %d, right speed: %d\n",
            BSP_GetEncoderCount(LEFT), BSP_GetEncoderCount(RIGHT));
    }
}

// ң�ش���
void BSP_RemoteProcess(void)
{
    /*
    if (GPIO_HAL_ReadPinInput(PTA, 15) && GPIO_HAL_ReadPinInput(PTD, 4)) {
        BSP_MoterDisable(); // ���ʧ��
        printf("Moter driver is disabled.\n");
    }
    */
}

// ����������
void BSP_BuzzerSwitch(int8_t sw)
{
    if (sw) {
        GPIO_HAL_WritePinOutput(PTA, 14, 1);
    } else {
        GPIO_HAL_WritePinOutput(PTA, 14, 0);
    }
}

// ����ADCֵ
float BSP_GetBatteryVoltage(void)
{
    int value;
    adc16_chn_config_t ch_cfg = {
        .chnIdx = kAdc16Chn23,
        .convCompletedIntEnable = false,
        .diffConvEnable = false
    };

    // Software trigger the conversion.
    ADC16_HAL_ConfigChn(ADC0, 0, &ch_cfg);
    // Wait for the conversion to be done.
    while (!ADC16_HAL_GetChnConvCompletedFlag(ADC0, 0));
    // Fetch the conversion value.
    value = ADC16_HAL_GetChnConvValue(ADC0, 0);
    return value / 4095.0f * 3.3f * 3.0f;
}

// ��ȡ������ƽ
uint8_t BSP_GetKeyValue(uint8_t key)
{
    switch (key) {
        case KEY1:
            key = 20;
            break;
        case KEY2:
            key = 21;
            break;
        case KEY3:
            key = 22;
            break;
        case KEY4:
            key = 23;
            break;
        default:
            return 0;
    }
    return GPIO_HAL_ReadPinInput(PTB, key) == 0;
}

