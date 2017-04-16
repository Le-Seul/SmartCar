#ifndef __TURN_H
#define __TURN_H

#include <stdint.h>

#include "pid.h"
#include "image_process.h"

typedef struct {
    PID pid;            // ת��PID������
    TrackInfo *track;   // ������Ϣ
    uint8_t *image;     // ����ͷλͼָ��
    int forward;        // ǰհ����
} TrunController;

extern TrunController *turnControl;

TrunController *TrunController_Init(uint8_t *image, int width, int height);
void TrunController_Iteration(TrunController *obj);

#endif
