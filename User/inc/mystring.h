#ifndef __MYSTRING_H
#define __MYSTRING_H

#define ISDIGIT(c)  (c >= '0' && c <= '9')
#define ISLETTER(c) (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c == '_')

/* �����ո�,����,�Ʊ�� */
#define JUMP_SPACE(_S) { while (*_S && (*_S == ' ' \
    || *_S == '\n' || *_S == '\t')) ++_S; }

void mystrlwr(char *s);

#endif
