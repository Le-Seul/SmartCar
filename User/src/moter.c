#include "moter.h"
#include <math.h>
#include "bsp.h"
#include "mpu6050.h"
#include "sendwave.h"
#include "uart_debug.h"
#include "kalman_filter.h"
#include "oled_i2c.h"
#include <stdio.h>
#include "FreeRTOS.h"

// ������ʾģʽ����
// 0: ����ʾ
// 1: ����ʾ����
// 2: ���������������Ʈ(1000�β���)
// 3: ��ʾ�ٶȻ�����
#define DISP_WAVE 0

// ���ģʽ, �ǵ���ʱӦ������Ϊ0
// 0: Ĭ��ģʽ(ֱ����, �ٶȻ�, ����)
// 1: ֻ��ֱ����
// 2: ֻ��ֱ�������ٶȻ�
#define OUTPUT_MODE 0

// ��������Ʈ, ��λdps
#define GYRO_OFFSET 1.17f

// ����������ѹPWMֵ
#define MOTER_LEFT_THRESHOLD   100u

// ����������ѹPWMֵ
#define MOTER_RIGHT_THRESHOLD  100u

// ֱ��ʱ���ĽǶ�(���ټ������ǲ���ֵ)
#define SENSOR_STAND_ANGLE     12.0f

// ��ȫ�Ƕ�
#define SPEED_SAFE_ANGLE       4.0f

// ���������
MoterControl moterControl;

// ������ƻ���ʼ��
void Moter_ControlInit(MoterControl *moter)
{
    moter->angle = 0;
    // ֱ����
    PID_Init(&moter->standPID, 0.4f, 0.0f, 0.05f, 1000);
    PID_Init(&moter->speedPID, 0.0f, 0.01f, 0.0f, 10);
    PID_SetValue(&moter->standPID, 0); // ֱ�����趨ֵ
    PID_SetValue(&moter->speedPID, 0); // �ٶȻ��趨ֵ
    moter->standOut = 0;
    moter->speedOut = 0;
}

// ���õ��ת��
void Moter_SetSpeed(MoterControl *moter, int speed)
{
    moter->speedPID.value = speed;
}

// ����ת��
void Moter_SetAngle(MoterControl *moter, int angle)
{
    taskENTER_CRITICAL();
    moter->angle = angle;
    taskEXIT_CRITICAL();
}

// ֱ��������
void Moter_StandIteration(MoterControl *moter)
{
    int err;
    int16_t data[3];
    float angle;
    static float angle_y, gyro_y;     //��static���Σ���Ϊ��������������ʱ���ڱ�ģ�����ÿ�α��ʱ���ǰһ�ε�ֵ��Ҫ��֤����ǰһ�ε�

    // ��ȡ�����Ǻͼ��ټƵĶ���
    err = mpu6050_read_accel(data);
    if (!err) { // û�д���ʱ��ֵ����
        angle_y = -atan2f(data[0], data[2]) * 180.0f / 3.1415926535f;
    }
    err = mpu6050_read_gyro(data);
    if (!err) { // û�д���ʱ��ֵ����
        gyro_y = (data[1] / 16.38f - GYRO_OFFSET); // ��������Ʈ����
    }

    // Kalman�˲����ںϲ���
    angle = Kalman_Filter(angle_y, gyro_y, &gyro_y) - SENSOR_STAND_ANGLE;
    // ֱ��PD����
    moter->standOut =  (int)(moter->standPID.p * angle + moter->standPID.d * gyro_y / 22.2f);

#if DISP_WAVE == 1 // ��ʾ����
    {
        char buf[83];

        ws_frame_init(buf);
        ws_add_float(buf, CH1, angle_y);
        ws_add_float(buf, CH2, gyro_y);
        ws_add_float(buf, CH3, angle);
        ws_add_float(buf, CH4, moter->standOut);
        uart_senddata(buf, ws_frame_length(buf));
    }
#elif DISP_WAVE == 2 // ������������Ʈ
    {
        static int gyro_count;
        static float gryo_sum;
        
        if (gyro_count++ < 1000) {
            gryo_sum += gyro_y;
        } else {
            printf("���ټƶ���ƽ��ֵ: %f\n", gryo_sum / 1000.0f);
            gyro_count = 0;
            gryo_sum = 0;
        }
    }
#endif
}

// �ٶȻ�����
void Moter_SpeedIteration(MoterControl *moter)
{
    static float outNew, outOld, integral; // �µ��ٶ����ֵ, �ɵ��ٶ����ֵ, ������
    static int speed, count = 0; // �ٶ��ۼ�ֵ, �ٶȻ��ۻ�������
    float trueSpeed, fetw; // ʵ���ٶ�ֵ, ������

    // �����������ɼ������ٶ�, ÿ�β������ɼ�, ��ֹ����������
    speed += BSP_GetEncoderCount(LEFT) + BSP_GetEncoderCount(RIGHT);
    // �ٶ�ƽ���˲�
    if (count++ >= 19) { // �ٶȻ���ֱ��������20�����ں���е���
        int error;
        float kp = (float)moter->speedPID.p / moter->speedPID.scale;
        float ki = (float)moter->speedPID.i / moter->speedPID.scale;

        // ʵ���ٶȵļ���ԭ��: �����PWMֵΪ1000ʱ100ms����µ����������ɼ�ֵ��ԼΪ6490,
        // �����ϵ: trueSpeed = speed / (6490.0f * 2.0f / 1000.0f) = speed / 12.98f.
        trueSpeed = speed / 12.98f;
        error = moter->speedPID.value - trueSpeed;   // ���㱾�����
        if (error < 200) { // ���ַ���, �����������200ʱ�����������
            integral += error * ki; // ������ֵ
            integral = clamp_float(integral, 4000); // �����޷�
        } else {
            integral = 0;
        }
        fetw = clamp_float(kp * error, 2000); // �����޷�
        outOld = outNew;                 // ���¾ɵ��ٶȿ���ֵ
        outNew = fetw + integral;  // PI����
        outNew = clamp_float(outNew, 3000.0f); // �޷�
        count = 0;
        speed = 0; // �����ۼƱ���������
    }
    // ��20�����
    moter->speedOut = (int)((outNew - outOld) * count / 20.0f + outOld);
#if DISP_WAVE == 3 // �ٶȻ�������ʾ����
    {
        char buf[83];

        ws_frame_init(buf);
        ws_add_float(buf, CH1, trueSpeed);
        ws_add_int32(buf, CH2, moter->speedOut);
        ws_add_float(buf, CH3, integral);
        ws_add_float(buf, CH4, fetw);
        uart_senddata(buf, ws_frame_length(buf));
    }
#endif
}

// ����������
void Moter_Output(MoterControl *moter)
{
    static int turnAngle;
    int speed, sum, diff;

#if OUTPUT_MODE == 0 || OUTPUT_MODE == 2
    // ����ֱ�������ٶȻ��������
    sum = moter->standOut - moter->speedOut;
#else // ֻ��ֱ�����ĵ���ģʽ
    sum = moter->standOut;
#endif

#if OUTPUT_MODE == 0
    taskENTER_CRITICAL();
    turnAngle = (moter->angle * 6 + turnAngle * 4) / 10; // �ٶ��˲�
    taskEXIT_CRITICAL();
    diff = turnAngle; // �Ƕ�ת��Ϊ���ֲ���
#else // ����ģʽû�з���
    diff = 0;
#endif

    // ���ֿ���
    speed = sum + diff;
    // ������ѹ����
    speed += speed >= 0 ? MOTER_LEFT_THRESHOLD : -MOTER_LEFT_THRESHOLD;
    BSP_SetMotorSpeed(LEFT, clamp_int(speed, 3500)); // ����޷�

    // ���ֿ���
    speed = sum - diff;
    // ������ѹ����
    speed += speed >= 0 ? MOTER_RIGHT_THRESHOLD : -MOTER_LEFT_THRESHOLD;
    BSP_SetMotorSpeed(RIGHT, clamp_int(speed, 3500)); // ����޷�
}
