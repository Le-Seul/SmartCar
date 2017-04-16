#ifndef __PID_H
#define __PID_H

typedef struct {
    int p, i, d;        // PID����
    int value;          // �趨ֵ
    int error;          // �ϴ����
    int sum;            // ������ֵ
    int scale;          // ���ζԸ������������
} PID;

void PID_Init(PID *pid, float p, float i, float d, int scale);
void PID_SetValue(PID *pid, int value);
void PID_SetParams(PID *pid, float p, float i, float d);
int PID_Iteration(PID *pid, int measured);

#endif
