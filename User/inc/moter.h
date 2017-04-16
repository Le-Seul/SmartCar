#ifndef __MOTER_H
#define __MOTER_H

#include "pid.h"

typedef struct {
    int angle;          // ת��ǶȲ���ֵ
    PID standPID;       // ֱ����PID
    PID speedPID;       // �������ֵ��ٶȿ���
    int standOut;       // ֱ�������
    int speedOut;       // �ٶȻ����
} MoterControl;

extern MoterControl moterControl;

// ��������(ǯλ)
static inline int clamp_int(int value, int range)
{
    return value > range ? range : (value < -range ? -range : value);
}

// ��������(ǯλ) float�汾
static inline float clamp_float(float value, float range)
{
    return value > range ? range : (value < -range ? -range : value);
}

void Moter_ControlInit(MoterControl *moter);
void Moter_SetSpeed(MoterControl *moter, int speed);
void Moter_SetAngle(MoterControl *moter, int angle);
void Moter_StandIteration(MoterControl *moter);
void Moter_SpeedIteration(MoterControl *moter);
void Moter_Output(MoterControl *moter);

#endif
