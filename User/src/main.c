#include "includes.h"
#include "uart_debug.h"
#include "oled_i2c.h"
#include "os_timers.h"
#include "uart_cmd.h"
#include "OV7725.h"
#include "turn.h"
#include "key.h"

// ����½���һ������, ��Ҫ��������һЩ���ԵĴ���
void task(void * argument)
{
//    char data[] = { 0x0b, 0xBB };
    TrunController *turn;

    // ����������ѹ
    //printf("������ѹ����...\n");
    //BSP_MoterThresholdTest();
    turn = TrunController_Init(image_buff, CAMERA_W, CAMERA_H);
    turnControl = turn;
    while (1) {
        ov7725_get_img();
        TrunController_Iteration(turn); // ���򻷵���
        debug_disp_image(turn->track);

//        uart_senddata(data, 2);
//        uart_senddata((const char *)image_buff, 600);
        if (
#if 0
            READ_BITS(turn->track->flags, ON_CROSS_WAY) ||
#endif
            READ_BITS(turn->track->flags, ON_LOOP_WAY)) {
            BSP_BuzzerSwitch(1);
        } else {
            BSP_BuzzerSwitch(0);
        }
    }
}

// ������ң������
void key_task(void * argument)
{   
    while (1) {
        BSP_RemoteProcess();
        delay_ms(20);
    }
}

// ��������, �����������г�ʼ��Ӳ��, ��������main������,
// ��Ϊ��ʱ����ϵͳ�ṩ�ķ����Լ��������ʹ��, ������ʱ����,
// �����main������ʹ��FreeRTOS����ʱ���ܻῨ��.
void startTask(void * argument)
{
    uint32_t interval = 500, count; // Ĭ����˸Ƶ��Ϊ1Hz
    board_init(); // MCU����Ͱ��������ʼ��
    // ��������, ע���ջ��С�Ƿ��㹻!!!!
    xTaskCreate(task, "task", 200U, NULL, 1, NULL);
    xTaskCreate(cmd_task, "cmd task", 400U, NULL, 0, NULL);
    xTaskCreate(key_task, "key task", 200U, NULL, 0, NULL);
    Create_Timers(); // ������Ҫ�������ʱ��
    
    // LED��˸, ��������ָʾ��
    SIM_HAL_EnableClock(SIM, kSimClockGatePortB);
    PORT_HAL_SetMuxMode(PORTB, 19, kPortMuxAsGpio);
    GPIO_HAL_SetPinDir(PTB, 19, kGpioDigitalOutput);

    while (1) {
        GPIO_HAL_WritePinOutput(PTB, 19, 1);
        delay_ms(interval);
        GPIO_HAL_WritePinOutput(PTB, 19, 0);
        delay_ms(interval);
        
        // һ����ص�ѹ�ܵ;Ϳ�����˸����, ���Ҳ��ָ�������Ƶ��.
        if (interval == 500) {
            // 7.0V��ָ������ʱ�ĵ�ص�ѹ, ��ʱ��ؿ��ص�ѹ��ԼΪ7.2V.
            if (BSP_GetBatteryVoltage() < 7.0f) {
                if (count++ >= 5) { // ����5�ε�ѹ����7.0V, Ҳ����5s
                    interval = 50; // 10Hz��˸Ƶ��
                }
            } else {
                count = 0;
            }
        }
    }
}

// ������
int main(void)
{
    xTaskCreate(startTask, "startTask", 200U, NULL, 0, NULL);
    // ����������������ʼִ��
    vTaskStartScheduler();
    // ��������²������е�����
    while (1);
}
