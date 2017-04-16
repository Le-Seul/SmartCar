#include "pid.h"

// PID��ʼ��
void PID_Init(PID *pid, float p, float i, float d, int scale)
{
    pid->p = (int)(p * scale);
    pid->i = (int)(i * scale);
    pid->d = (int)(d * scale);
    pid->scale = scale;
    pid->sum = 0;
    pid->error = 0;
    pid->value = 0;
}

// PID�����趨ֵ
void PID_SetValue(PID *pid, int value)
{
    pid->value = value;
}

// PID���ò���(����)
void PID_SetParams(PID *pid, float p, float i, float d)
{
    pid->p = (int)(p * pid->scale);
    pid->i = (int)(i * pid->scale);
    pid->d = (int)(d * pid->scale);
}

// PID��������
int PID_Iteration(PID *pid, int measured)
{
    int error, derror;
    
    error = pid->value - measured;     // ���㱾�β������
    derror = error - pid->error;       // ���΢��
    pid->sum += error;                 // ����������
    pid->error = error;                // ���������
    /* �������ֵ */
    return (pid->p * error              // ������
        + pid->i * pid->sum             // ������
        + pid->d * derror)              // ΢����
        / pid->scale;                   // �������
}
