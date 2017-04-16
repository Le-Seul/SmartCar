#ifndef __UART_CMD_H
#define __UART_CMD_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    int (*func)(int argc, char *argv[]);
} CommandProcess;


// ��������ص���������ָ��
extern const CommandProcess UartCmd_Tab[];

void cmd_task(void * argument);

#endif
