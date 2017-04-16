#include "os_timers.h"
#include "FreeRTOS.h"
#include "timers.h"
#include <stdio.h>
#include "moter.h"

// ������ƶ�ʱ��
void Moter_Timer(TimerHandle_t xTimer)
{
    // ֱ��������
    Moter_StandIteration(&moterControl);
    // �ٶȻ�����
    Moter_SpeedIteration(&moterControl);
    Moter_Output(&moterControl);
}

void Create_Timers(void)
{
    TimerHandle_t timer;
    
    // ������Ƶ�Ԫ��ʼ��
    Moter_ControlInit(&moterControl);
    
    // ������ʱ��
    timer = xTimerCreate("Moter Timer", 5, pdTRUE, 0, Moter_Timer);
    xTimerStart(timer, 0);
}
