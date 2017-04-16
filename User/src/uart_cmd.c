#include "uart_cmd.h"
#include "fsl_uart_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "mystring.h"

// �������֧�ֵĲ�������
#define CMD_MAX_PARAM_COUNT  20

// ������Ϣ����
static xQueueHandle cmdQueue;

// ��ȡһ���ַ�
static int cmd_getchar(char *rxbuf, int rxpos, char ch)
{
    if (ch == '#') { // '#'��ʾǿ�ƽ���������������ǰ�������
        rxpos = 0;
        rxbuf[0] = '\0';
    } else if (ch == '\n' || ch == ';' || ch == '\r') { // ���������
        rxbuf[rxpos] = '\0';
        rxpos = 0;
    } else {
        rxbuf[rxpos++] = ch;
    }
    return rxpos;
}

// ��������
static void analyze_cmd(char *lineBuf)
{
    int argc = 0, len;
    char *argv[CMD_MAX_PARAM_COUNT];
    const CommandProcess *tab = UartCmd_Tab;
    
    mystrlwr(lineBuf); // ת����Сд
    while (*lineBuf && argc < (CMD_MAX_PARAM_COUNT - 1)) {
        char *p, *param;
        
        JUMP_SPACE(lineBuf); // ������Ч�ַ�
        if (*lineBuf == '\0') {
            break;
        }
        // �����һ�����ʵĳ���
        p = lineBuf;
        while (*p && *p != ' ' && *p != '\t') { // �ո���Ʊ���Ƿָ���
            ++p;
        }
        len = (size_t)p - (size_t)lineBuf + 1;
        param = pvPortMalloc(len);
        strncpy(param, lineBuf, len - 1); // ���Ʋ�������
        param[len - 1] = '\0';
        argv[argc++] = param;
        lineBuf = p; // ����һ������
    }
    // �������б���ƥ����ʵ�����
    while (tab->name && strcmp(tab->name, argv[0])) {
        ++tab;
    }
    // ��������ص���������������
    if (tab->func) {
        tab->func(argc, argv);
    } else {
        printf("Can not found the command: \"%s\".\n", argv[0]);
    }
    // �ͷſռ�
    while (argc--) {
        vPortFree(argv[argc]);
    }
}

// �����������
void cmd_task(void * argument)
{
    int count = 0;
    static char lineBuf[1000]; // ָ���, ����ɾ�̬������Լ��ٶ�ջ����
    
    cmdQueue = xQueueCreate(100, 1); // ��������
    while (1) {
        char ch;
        
        // �����в�Ϊ��ʱ��ȡ����
        xQueueReceive(cmdQueue, &ch, portMAX_DELAY);
        count = cmd_getchar(lineBuf, count, ch);
        if (count == 0 && *lineBuf) { // ���յ�һ������
            printf(">%s\n", lineBuf);
            // ��������
            analyze_cmd(lineBuf);
        }
    }
}

// �����жϷ�����
void UART1_RX_TX_IRQHandler(void)
{
    if (UART_HAL_GetStatusFlag(UART1, kUartRxDataRegFull)) {
        char ch = UART1->D;
        
        xQueueSendToBack(cmdQueue, &ch, 0); // ������м�������
        UART_HAL_ClearStatusFlag(UART1, kUartRxDataRegFull);
    }
}
