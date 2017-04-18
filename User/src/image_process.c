#include "image_process.h"
#include <stdio.h>

#ifdef __ARMCC_VERSION
#include "oled_i2c.h"
#include "FreeRTOS.h"
#define t_malloc(size)    pvPortMalloc(size)
#define t_free(p)         vPortFree(p)
#else
#include <malloc.h>
#define t_malloc(size)    malloc(size)
#define t_free(p)         free(p)
#endif

// ����һ��������Ϣ�ṹ
TrackInfo *TrackInfo_Init(uint8_t *image, int width, int height)
{
    TrackInfo *info = t_malloc(sizeof(TrackInfo));

    info->edge[LEFT] = t_malloc(height * sizeof(int16_t));
    info->edge[RIGHT] = t_malloc(height * sizeof(int16_t));
    info->middle = t_malloc(height * sizeof(int16_t));
    info->image = image;
    info->width = width;
    info->height = height;
    info->flags = 0x00;
    info->loopDir = LEFT;
    info->count = 0;
    return info;
}

// �ͷ�TrackInfo�ռ�
void TrackInfo_Free(TrackInfo * info)
{
    t_free(info->edge[LEFT]);
    t_free(info->edge[RIGHT]);
    t_free(info->middle);
    t_free(info);
}

// ɨ��ǰ�漸������
static short scanFront(TrackInfo * info)
{
    char flag = 0; // �߽綪ʧ��־,��������Ѿ����ñ�־
    uint8_t *pImg;
    short middle = info->width / 2; // ͼ���е�
    short x, y;

    info->endLine[LEFT] = 0;
    info->endLine[RIGHT] = 0;
    // ɨ������
    for (y = 0; y < FRONT_LINE; ++y) {
        pImg = info->image + (info->height - 1 - y) * info->width;
        flag = 0x00;
        // Ѱ����߽�
        for (x = middle; x >= 0; --x) {
            if (pImg[x] == 0x00) { // ��ɫ
                info->edge[LEFT][y] = x + 1; // ��¼��Ե
                break;
            }
        }
        if (x < 0) { // ��߽綪ʧ
            flag |= 0x01;
        }
        // Ѱ���ұ߽�
        for (x = middle; x < info->width; ++x) {
            if (pImg[x] == 0x00) { // ��ɫ
                info->edge[RIGHT][y] = x - 1; // ��¼��Ե
                break;
            }
        }
        if (x >= info->width) { // �ұ߽綪ʧ
            flag |= 0x02;
        }
        if (flag) { // �߽綪ʧ
            if (flag & 0x01) { // ��߽綪ʧ
                info->edge[LEFT][y] = 0;
                info->endLine[RIGHT] = y;
            }
            if (flag & 0x02) { // �ұ߽綪ʧ
                info->edge[RIGHT][y] = info->width - 1;
                info->endLine[LEFT] = y;
            }
            if (flag == 0x03) { // ������������ʧ
                // �ڷ���֮ǰ�Ȱ����漸�����ݽ��и�ֵ, ������ֲ�ȷ�������
                for (x = y; x < FRONT_LINE; ++x) {
                    info->edge[LEFT][x] = 0;
                    info->edge[RIGHT][x] = info->width - 1;
                    info->middle[x] = middle;
                }
                return y;
            }
        } else {
            info->endLine[LEFT] = y;
            info->endLine[RIGHT] = y;
        }
        // ��������
        info->middle[y] = (info->edge[LEFT][y] + info->edge[RIGHT][y]) / 2;
    }
    return y;
}

// ɨ��ʣ�µ���
int scanNext(TrackInfo * info, short y)
{
    char flag = 0; // ������ʧ��־, ��ȡ������־
    short x, pos;
    int width = info->width;
    uint8_t *line1, *line2, color, dir;

    line2 = info->image + (int)(info->height - y) * width;
    // �����������ɨ��
    for (; y < info->height; ++y) {
        line1 = line2;
        line2 = info->image + (int)(info->height - 1 - y) * width;
        flag &= ~0x03; // ���߱�־����
        for (dir = 0; dir < 2; ++dir) {
            x = info->edge[dir][y - 1]; // ��һ�б�����������һ�еĿ�ʼ����λ��
            x = min_int(max_int(x, 0), width - 1); // ǯλ, �����Ӧ����������, ֻ��Ϊ�˱���
            if ((x == 0 || x == width - 1) && line2[x] == 0xFF) { // ͼ������ǰ�ɫʱֱ�ӱ�Ƕ���
                info->edge[dir][y] = x;
            } else { // ����ʼɨ����������
                if (x == 0) { // �߽��������ʱֻ��������
                    pos = 1;
                } else if (x == width - 1) { // �߽������ұ�ʱֻ��������
                    pos = -1;
                } else { // ��һ��x��ߵ�ֵ�͵�ǰ��xֵ��ͬʱ��������, ������������
                    pos = line1[x - 1] == line2[x] ? 1 : -1;
                }
                color = line2[x] ? 0x00 : 0xFF; // ��ǰ��x����ɫΪ��ɫ��ƥ��Ŀ��Ϊ��ɫ, ����Ϊ��ɫ
                for (; x >= 0 && x < info->width; x += pos) {
                    if (line2[x] == color) { // ƥ����ɫʱ���
                        // ɨ��ƥ����ɫ�ǰ�ɫʱֱ�ӱ���xֵ, �Ǻ�ɫʱ��Ҫ���� x - pos
                        x -= color == 0xFF ? 0 : pos;
                        info->edge[dir][y] = x;
                        break;
                    }
                }
                // �����������ʱ��ʾ�߽綪ʧ����Ҫ����
                if (x < 0 || x >= info->width || abs_int(x - info->edge[dir][y - 1]) > 10) {
                    flag |= 1 << dir;
                } else {
                    flag &= ~(1 << (dir + 2)); // �������������¼��־
                }
            }
        }
        if (flag) { // ��Ҫ����
            if (flag & 0x01) { // �����Ҫ����
                info->edge[LEFT][y] = 0;
                if (!(flag & 0x04)) { // ���֮ǰû�б�ǹ��������¼��������
                    info->endLine[LEFT] = y;
                    flag |= 0x04;
                }
            }
            if (flag & 0x02) { // �ұ���Ҫ����
                info->edge[RIGHT][y] = info->width - 1;
                if (!(flag & 0x08)) {  // ���֮ǰû�б�ǹ��������¼��������
                    info->endLine[RIGHT] = y;
                    flag |= 0x08;
                }
            }
            if ((flag & 0x03) == 0x03) { // ���߶�����
                return y;
            }
        }
        // ��������
        info->middle[y] = (info->edge[LEFT][y] + info->edge[RIGHT][y]) / 2;
    }
    info->endLine[LEFT] = y;
    info->endLine[RIGHT] = y;
    return y;
}

/* ��ʮ��·���������հ׵����� */
static int scanJumpWhite(TrackInfo * info, int y)
{
    uint8_t flag = 0, *img;
    short x, endline, i, dx, dy;
    short width = info->width; /* ͼ���� */
    short middle = width / 2; /* ͼ���е� */

    // ��¼��ʼ���ߵ���
    endline = y;
    /* ����û�б߽������(ʮ��·���к��ŵ�����),
       forѭ��ֱ��ɨ��������ͼ������������߾��ҵ�����ʱ���� */
    for (; y < info->height; ++y) {
        img = info->image + (int)(info->height - 1 - y) * width;
        flag = 0x00;
        for (x = middle; x >= 0 && img[x]; --x); /* Ѱ����߽� */
        if (x >= 0) { // ������߽��־
            // �����߱仯�ٶ�С��6����ÿ��ʱ��¼
            if (y > 1 && INT_ABS(x - info->edge[LEFT][y - 1]) < 6) {
                flag |= 0x01;
            }
            info->edge[LEFT][y] = x;
        } else {
            info->edge[LEFT][y] = 0;
        }
        for (x = middle; x < width && img[x]; ++x); /* Ѱ���ұ߽� */
        if (x < width) { /* �����ұ߽��־ */
            // �����߱仯�ٶ�С��6����ÿ��ʱ��¼
            if (y > 1 && INT_ABS(x - info->edge[RIGHT][y - 1]) < 6) {
                flag |= 0x02;
            }
            info->edge[RIGHT][y] = x;
        } else {
            info->edge[RIGHT][y] = width - 1;
        }
        if (flag == 0x03) { // �������߶��ҵ���
            break;
        }
    }
    info->middle[y] = (info->edge[LEFT][y] + info->edge[RIGHT][y]) / 2;

    // ��������
    if (endline == 0) {
        info->middle[0] = middle;
    } else if (endline >= 3) {
        endline = endline > 3 ? endline - 3 : 0;
    }
    x = info->middle[endline];
    dx = info->middle[y] - info->middle[endline];
    dy = y - endline;
    for (i = endline; i < y; ++i) {
        info->middle[i] = x + (i - endline) * dx / dy;
    }
    return y + 1;
}

// ʶ��ʮ��·��
static int _findCros(TrackInfo * info)
{
    int out, i, endl, endr;

    out = max_int(info->endLine[LEFT], info->endLine[RIGHT]);
    // ʶ��ֱ��ʮ��·��
    if (out > 10 || READ_BITS(info->flags, ON_CROSS_WAY)) {
        endl = 0;
        endr = 0;
        out = min_int(out, 30);
        for (i = 0; i < out; ++i) {
            uint8_t flag = 0x00;

            if (info->edge[LEFT][i] == 0) {
                flag |= 0x01;
                if (!endl) { // ��¼��߿�ʼ��������
                    endl = i;
                }
            }
            if (info->edge[RIGHT][i] == info->width - 1) {
                flag |= 0x02;
                if (!endr) { // ��¼�ұ߿�ʼ��������
                    endr = i;
                }
            }
            // ����ͬʱ�����򷵻����ȶ��ߵ���
            if (flag == 0x03) {
                return min_int(endl, endr);
            }
        }
    }
    // ʮ��·�ڱ�־�Ѿ���λ
    if (READ_BITS(info->flags, ON_CROSS_WAY)) {
        // �����������������30�б�ʾʮ��·�ڽ���
        if (max_int(info->endLine[LEFT], info->endLine[RIGHT]) > 30) {
            CLEAR_BITS(info->flags, ON_CROSS_WAY); // ���ʮ��·�ڱ�־
        } else { // ���򷵻ؿ�ʼ������
            return min_int(info->endLine[LEFT], info->endLine[RIGHT]);
        }
    }
    return 0;
}

// �ж�Բ���ķ���
static void _getLoopDir(TrackInfo * info)
{
    static int count = 0;

    // ֻ����Բ���в��ж�Բ���ķ���
    if (READ_BITS(info->flags, ON_LOOP_WAY)) {
        // Բ�������־��Ч
        if (!READ_BITS(info->flags, LOOPDIR_VALID)) {
            int y, end, width = info->width - 1, l, r;
            int16_t *edge;

            // ʶ����ߵĳ���
            end = info->endLine[LEFT];
            edge = info->edge[LEFT];
            for (y = 1; y < end; ++y) {
                // ��⵽ǰ������
                if (edge[y] == 0 && edge[y] - edge[y - 1] < -5) {
                    l = y;
                    break;
                }
            }
            // ʶ���ұߵĳ���
            end = info->endLine[RIGHT];
            edge = info->edge[RIGHT];
            for (y = 1; y < end; ++y) {
                // ��⵽ǰ������
                if (edge[y] == width && edge[y] - edge[y - 1] > 5) {
                    r = y;
                    break;
                }
            }
            info->loopDir = l < r ? LEFT : RIGHT;
            SET_BITS(info->flags, LOOPDIR_VALID); // Բ��������Ч��־��λ
            count = 0; // ��λ��ʱ��
        }
    } else {
        if (count < 20) {
            ++count; // ���־��ʱ
        } else { // ��������ﵽ10����Բ��������Ч��־����
            CLEAR_BITS(info->flags, LOOPDIR_VALID);
        }
    }
}

// �ж�ǰ���Ƿ���Բ��, ��������Ƿ���������������
static short _fingLoop(TrackInfo * info)
{
    int16_t *edge;
    int i, out, l = 0, r = 0, d1, d2;

    // �����ߵĹս�
    out = info->endLine[LEFT];
    edge = info->edge[LEFT];
    for (i = 6; i < out; ++i) {
        d1 = edge[i] - edge[i - 3];
        d2 = edge[i - 3] - edge[i - 6];
        // �սǴ�ǰ�漸����������, ���漸����������
        if (d1 < -3 && d1 > -14 && d2 > 1 && d2 < 4) {
            l = i - 3;
            break;
        }
    }

    // ����ұߵĹս�
    out = info->endLine[RIGHT];
    edge = info->edge[RIGHT];
    for (i = 6; i < out; ++i) {
        d1 = edge[i] - edge[i - 3];
        d2 = edge[i - 3] - edge[i - 6];
        // �սǴ�ǰ�漸����������, ���漸����������
        if (d1 > 3 && d1 < 14 && d2 < -1 && d2 > -4) {
            r = i - 3;
            break;
        }
    }

    // ���߶����ڹս�
    if (l && r) {
        int middle, height = info->height, width = info->width;

        // Ѱ��ǰ����Բ��
        l = min_int(l, r);
        out = min_int(l + 20, info->height);
        middle = info->middle[l];
        for (i = l; i < out; ++i) {
            uint8_t *img = info->image + (int)(height - 1 - i) * width;
            if (!img[middle]) { // ������ɫ
                if (i - l > 10) {
                    return l;
                }
            }
        }
    }
    return 0;
}

// Բ�����
void _loopRun(TrackInfo * info)
{
    int i, line, dir, y0;
    static int status = 0;

    y0 = min_int(info->endLine[LEFT], info->endLine[RIGHT]);
    for (dir = 0; dir < 2; ++dir) {
        line = info->endLine[dir] - 4;
        // Ѱ�ҹյ���
        for (i = 5; i < line; ++i) {
            // �ж��Ƿ��ǹյ�
            if ((info->edge[dir][i] - info->edge[dir][i - 2])
                * (info->edge[dir][i - 2] - info->edge[dir][i - 5]) < -4) {
                y0 = min_int(y0, i); // ��¼�յ�����
                break;
            }
        }
    }
    if (y0 < 16) {
        SET_BITS(info->flags, LOOP_TURN);
        switch (status) {
            case 0:
                status = 1; // ��·��
                break;
            case 2:
                status = 3; // ��·��
                break;
        default:
                break;
        }
    } else if (y0 > 25) {
        CLEAR_BITS(info->flags, LOOP_TURN);
        switch (status) {
            case 1:
                status = 2; // �Ѿ���·��
                break;
            case 3:
                if (y0 > 40) {
                    status = 0; // �Ѿ���·��
                    CLEAR_BITS(info->flags, ON_LOOP_WAY);
                }
                break;
        default:
                break;
        }
    }
}


// ��ȡ��������
void Track_GetEdgeLine(TrackInfo * info)
{
    int y, width = info->width, height = info->height;

    info->endLine[LEFT] = 0;
    info->endLine[RIGHT] = 0;
    // �������߳�ʼ��
    for (y = 0; y < height; ++y) {
        info->edge[LEFT][y] = 0;
        info->edge[RIGHT][y] = width - 1;
    }
    y = scanFront(info);
    if (y == FRONT_LINE) {
        y = scanNext(info, FRONT_LINE); // ɨ��ʣ�µ�����
        if (READ_BITS(info->flags, ON_LOOP_WAY) || _fingLoop(info)) { // ����Ƿ���Բ����
            SET_BITS(info->flags, ON_LOOP_WAY);
            _getLoopDir(info); // ��ȡԲ�����ڷ���
            _loopRun(info);
        } else { // �������Բ���о��ж��ǲ�����ʮ��·��
            y = _findCros(info);
            if (y) { // ��ʮ��·��
                y = scanJumpWhite(info, y);
                scanNext(info, y);
                SET_BITS(info->flags, ON_CROSS_WAY); // ����ʮ��·�ڱ�־
            }
        }
    } else {
        if (READ_BITS(info->flags, ON_CROSS_WAY)) { // ��ʮ��·����
            y = scanJumpWhite(info, y);
            scanNext(info, y);
        }
        if (READ_BITS(info->flags, ON_LOOP_WAY)) {
            _loopRun(info);
        }
    }
}

// ��ȡ����ƫ��
int Track_GetOffset(TrackInfo *info)
{
    int meanX = 0, meanY = 0, sum1 = 0, sum2 = 0, n, i;
    int middle = info->width >> 1;

    // ����Ҫ��ϵĵ���
    n = max_int(info->endLine[LEFT], info->endLine[RIGHT]);
    n = min_int(n - 5, 45); // ������ǰ45������, ����, �ڶ���ǰ�ļ������ݿ��ܺܲ��ȶ�, ��ʹ��
    if (n <= 5) { // ������������5��ʱֱ�ӷ���
        return info->offset;
    }
    // ��x, yƽ��ֵ
    // ��sigma(x * y)��sigma(x * x)
    for (i = 0; i < n; ++i) {
        meanX += i;
        meanY += (info->middle[i] - middle);
        sum1 += i * (info->middle[i] - middle);
        sum2 += i * i;
    }
    meanX /= n;
    meanY /= n;

    if ((sum2 - n * meanX * meanX) != 0) { // �жϳ����Ƿ�Ϊ0
        // ��б��, б�ʷŴ�100��
        info->slope = (sum1 - n * meanX * meanY) * 100 / (sum2 - n * meanX * meanX);
        // ��ƫ��, ƫ��Ŵ�100��
        info->offset = meanY * 100 - info->slope * meanX;
    }
    return info->offset;
}

// ������������
int Track_GetCurvity(TrackInfo *info)
{
    // ��ȡ������Ч����

    return 0;
}
