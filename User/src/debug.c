#include "uart_cmd.h"
#include "includes.h"
#include "moter.h"
#include "turn.h"

static int cmd_reset(int argc, char *argv[]);
static int cmd_help(int argc, char *argv[]);
static int cmd_pid(int argc, char *argv[]);
static int cmd_speed(int argc, char *argv[]);
static int cmd_stop(int argc, char *argv[]);
static int cmd_forward(int argc, char *argv[]);
static int cmd_battery(int argc, char *argv[]);
static int cmd_start(int argc, char *argv[]);

const CommandProcess UartCmd_Tab[] = {
    { "reset",      cmd_reset },
    { "help",       cmd_help },
    { "pid",        cmd_pid },
    { "speed",      cmd_speed },
    { "stop",       cmd_stop },
    { "star",       cmd_stop }, // ����ͣ��ָ��, startɾ��t�γ�
    { "forward",    cmd_forward },
    { "battery",    cmd_battery },
    { "start",      cmd_start },
    { NULL,         NULL }
};

static int cmd_reset(int argc, char *argv[])
{
    printf("\nReset...\n");
    NVIC_SystemReset();
    return 0;
}

static int cmd_help(int argc, char *argv[])
{
    const CommandProcess *tab = UartCmd_Tab;
    
    printf("NXP���ܳ�����ͷֱ���� - Creeper��.\n"
        );
    printf("ָ���б�:\n");
    while (tab->name) {
        printf("    %s\n", tab->name);
        ++tab;
    }
    return 0;
}

// PID������������
static int cmd_pid(int argc, char *argv[])
{
    PID *pid;

    if (argc == 1) { // ��ʾPID������Ϣ
        pid = &moterControl.standPID;
        printf("ֱ���� P: %f, I: %f, D: %f.\n",
            (float)pid->p / pid->scale,
            (float)pid->i / pid->scale,
            (float)pid->d / pid->scale);
        pid = &moterControl.speedPID;
        printf("�ٶȻ� P: %f, I: %f, D: %f.\n",
            (float)pid->p / pid->scale,
            (float)pid->i / pid->scale,
            (float)pid->d / pid->scale);
        pid = &turnControl->pid;
        printf("���� P: %f, I: %f, D: %f.\n",
            (float)pid->p / pid->scale,
            (float)pid->i / pid->scale,
            (float)pid->d / pid->scale);
    } else if (argc == 5) { // ����PID����
        float p = atof(argv[2]);
        float i = atof(argv[3]);
        float d = atof(argv[4]);

        if (!strcmp(argv[1], "stand")) { // ֱ����
            pid = &moterControl.standPID;
        } else if (!strcmp(argv[1], "speed")) { // �ٶȻ�
            pid = &moterControl.speedPID;
        } else if (!strcmp(argv[1], "turn")) { // ����
            pid = &turnControl->pid;
        } else {
            printf("error: ������PID������ \"%s\"\n", argv[1]);
            return 1;
        }
        PID_SetParams(pid, p, i, d);
    } else {
        printf("error: �����ʽ����. eg:\n"
               "  pid [stand/speed/turn] (float)P (float)I (float)D.\n");
        return 1;
    }
    return 0;
}

// ���ó���
static int cmd_speed(int argc, char *argv[])
{
    if (argc == 1) { // ��ʾ�ٶ�
        printf("С����ǰ�ٶ�: %d\n", moterControl.speedPID.value);
    } else if (argc == 2) { // �����ٶ�
        PID_SetValue(&moterControl.speedPID, atoi(argv[1]));
    } else {
        printf("error: �����ʽ����. eg:\n"
               "  speed: value.\n");
        return 1;
    }
    return 0;
}

// ����ͣ��
static int cmd_stop(int argc, char *argv[])
{
    BSP_MoterDisable();
    printf("Moter driver is disabled.\n");
    return 0;
}

// ����ǰհ����
static int cmd_forward(int argc, char *argv[])
{
    if (turnControl == NULL) {
        printf("�����������δ��ʼ��!\n");
        return 1;
    }
    if (argc == 1) { // ��ʾǰհ����
        printf("ǰհ����: %d\n", turnControl->forward);
    } else { // ����ǰհ����
        turnControl->forward = atoi(argv[1]);
    }
    return 0;
}

static int cmd_battery(int argc, char *argv[])
{
    printf("\tVoltage: %.2fV.\n", BSP_GetBatteryVoltage());
    return 0;
}

static int cmd_start(int argc, char *argv[])
{
    BSP_MoterEnable();
    printf("Moter driver is enabled.\n");
    return 0;
}
