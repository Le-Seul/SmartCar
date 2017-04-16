#ifndef __IMAGE_PROCESS_H
#define __IMAGE_PROCESS_H

#include <stdint.h>

/* ������ʼ�� */
#define FRONT_LINE     4

/* ��������ֵ���� */
#define INT_ABS(X) ((X) > 0 ? (X) : -(X))

/* ������Сֵ */
#define INT_MIN(X, Y) ((X) < (Y) ? (X) : (Y))

/* �������ֵ */
#define INT_MAX(X, Y) ((X) > (Y) ? (X) : (Y))

/* ���x�����Ƿ񳬳��߽� */
#define CHECK_EDGE(X, W) ((X) < (W) ? ((X) > 0 ? (X) : 0) : (W) - 1)

// ʮ��·�ڱ�־
#define ON_CROSS_WAY    0x02
// ��Բ���б�־
#define ON_LOOP_WAY     0x04

#define SET_BITS(a, b)      ((a) |= (b))
#define CLEAR_BITS(a, b)    ((a) &= ~(b))
#define READ_BITS(a, b)     (((a) & (b)) == b)

#ifndef LEFT
  #define LEFT                0u
#endif

#ifndef RIGHT
  #define RIGHT               1u
#endif

typedef struct {
    int16_t *edge[2];       // �������ߵı���
    int16_t *middle;        // ����
    int16_t endLine[2];      // ������
    uint8_t *image;         // λͼ����
    int width, height;      // λͼ�ߴ�
    int curvity;            // ��������
    int slope;              // ����б��
    int offset;             // ƫ��
    uint8_t flags;          // ��־
    uint8_t loopDir;        // Բ������
    int count;
} TrackInfo;

// �������ֵ
static inline int max_int(int x, int y)
{
    return x > y ? x : y;
}

// ������Сֵ
static inline int min_int(int x, int y)
{
    return x < y ? x : y;
}

// �������ֵ
static inline int abs_int(int x)
{
    return x > 0 ? x : -x;
}

TrackInfo *TrackInfo_Init(uint8_t *image, int width, int height);
void TrackInfo_Free(TrackInfo * info);
void Track_GetEdgeLine(TrackInfo * info);
int Track_GetOffset(TrackInfo *info);
int Track_GetCurvity(TrackInfo *info);


#endif
