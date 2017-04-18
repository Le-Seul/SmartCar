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
        if (d1 < -3 && d1 > -20 && d2 > 1 && d2 < 4) {
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
        if (d1 > 3 && d1 < 20 && d2 < -1 && d2 > -4) {
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
                if (i - l > 6) {
                    return l;
                }
            }
        }
    }
    return 0;
}

// Բ������
static short _loopGetLine(TrackInfo * info)
{
    int i, line, dir, out;
    int y0, x0, y1, dx, dy;
    int width = info->width, height = info->height;
    uint8_t *img = info->image + (height - 1) * width;

    for (dir = 0, y0 = info->height; dir < 2; ++dir) {
        line = info->endLine[dir] - 4;
        // Ѱ�ҹյ���
        for (i = 5; i < line; ++i) {
            // �ж��Ƿ��ǹյ�
            if ((info->edge[dir][i] - info->edge[dir][i - 2])
                * (info->edge[dir][i - 2] - info->edge[dir][i - 5]) < -4) {
                break;
            }
        }
        y0 = min_int(y0, i); // ��¼�յ�����
    }
    dir = info->loopDir;
    // Ѱ�Ҷ�����
    out = min_int(info->endLine[LEFT], info->endLine[RIGHT]);
    for (line = 0; line < out; ++line) {
        if (info->edge[LEFT][line] <= 0
            || info->edge[RIGHT][line] >= width - 1) {
            break;
        }
    }
    if (y0 >= 30 && line > 40) { // ˵���Ѿ���Բ��
        if (info->slope < 40) {
            return 0;
        }
    }

//    printf("line: %d, y0: %d, middle: %d\n", line, y0, info->middle[y0]);
    // ���û�йյ��˵��������·��, ��ʱ���߻ᶪ��, ��ɨ���x��������Ϊwidth / 2,
    // ����˵��ǰ��һ�ξ������·��, �Թյ��е�������Ϊɨ��x����
    x0 = y0 < line ? info->middle[y0] : info->middle[line];
    // ��ǰѰ��, ֱ�����ٶ���(������ɫ)
    for (i = y0; i < height - 1; ++i) {
        img = info->image + (height - 1 - i) * width;
        if (img[x0] == 0x00) { // ǰ��Ϊ��ɫʱ����
            break;
        }
    }
    y1 = i; // ��¼ɨ������

    // û���ҵ��յ���˵����������·��
    if (y0 >= line) {
        for (i = 0, out = 0; i < 10; ++i) {
            out += info->edge[RIGHT][i] - info->edge[LEFT][i];
        }
        out /= 10; // ����ƽ�����
//        printf("line: %d\n", out);
        if (out < 65) {
            return -1;
        }
        y0 = line; // û�йյ�, ����ָ��y0Ϊ��ʼ���ߵ���
        // Ѱ�ұ߽�: ������ɫ���ؽ�������
        if (dir == LEFT) { // �����������ʱ
            for (i = 0; i < width && img[i] == 0x00; ++i); // Ѱ����ߵı���
            info->edge[LEFT][y1] = i;
            for (; i < width && img[i]; ++i); // Ѱ���ұ߱���
            info->edge[RIGHT][y1] = i;
        } else { // ���������ұ�ʱ
            for (i = width; i > 0 && img[i] == 0x00; ++i); // Ѱ���ұߵı���
            info->edge[LEFT][y1] = i;
            for (; i > 0 && img[i]; ++i); // Ѱ����ߵı���
            info->edge[RIGHT][y1] = i;
        }
    } else { // �����ҵ��յ�˵��ǰ���ǻ���·��
        // Ѱ��ǰ������(һ������Ȧ��Բ��)
        if (dir == LEFT) { // ���·������վ�Ѱ���ұ���
            for (i = x0; i > 0 && !img[i]; --i); // ֱ��ͼ���Ϊ��ɫ
            info->edge[RIGHT][y1] = i; // �ұ���
            for (; i > 0 && img[i]; --i); // ֱ����ɫ
            info->edge[LEFT][y1] = i; // �ұ���
        } else { // ����Ѱ�������
            for (i = x0; i < width && !img[i]; ++i); // ֱ��ͼ���Ϊ��ɫ
            info->edge[LEFT][y1] = i;
        }
        --y0;
    }

    info->middle[y1] = (info->edge[LEFT][y1] + info->edge[RIGHT][y1]) / 2;
    // ��������
    x0 = info->middle[y0];
    dx = info->middle[y1] - x0;
    dy = y1 - y0;
    for (i = y0; i < y1; ++i) {
        info->middle[i] = x0 + (i - y0) * dx / dy;
    }
    return y1 + 1;
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
        if (_fingLoop(info) || READ_BITS(info->flags, ON_LOOP_WAY)) { // ����Ƿ���Բ����
            y = _loopGetLine(info);
            if (y > 0) {
                scanNext(info, y);
                SET_BITS(info->flags, ON_LOOP_WAY); // ����Բ��·�ڱ�־
                CLEAR_BITS(info->flags, ON_CROSS_WAY); // ���ʮ��·�ڱ�־
            } else if (y == 0) {
                CLEAR_BITS(info->flags, ON_LOOP_WAY);
            }
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
        if (READ_BITS(info->flags, ON_LOOP_WAY)) { // ��Բ����
            y = _loopGetLine(info);
            if (y) {
                scanNext(info, y);
            }
        }
    }
}

// ��С���˷���ϸ���������б�ʺ�ƫ��
int Track_GetOffset(TrackInfo *info)
{
    int meanX = 0, meanY = 0, sum1 = 0, sum2 = 0, n, i;
    int middle = info->width >> 1;

    // ����Ҫ��ϵĵ���
    n = max_int(info->endLine[LEFT], info->endLine[RIGHT]);
    n = min_int(n - 5, 45); // ������ǰ45������, ����, �ڶ���ǰ�ļ������ݿ��ܺܲ��ȶ�, ��ʹ��
    if (n <= 5) { // ������������5��ʱֱ�ӷ���, ��ʹ����һ�ε�ֵ
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
