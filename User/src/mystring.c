#include "mystring.h"
#include <stdio.h>

/* ���ַ����еĴ�д��ĸת��ΪСд */
void mystrlwr(char *s)
{
    char c;

    while ((c = *s) != 0) {
        if (c >= 'A' && c <= 'Z') {
            *s += 32;
        }
        ++s;
    }
}
