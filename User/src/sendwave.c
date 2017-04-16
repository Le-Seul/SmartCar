/*****************************************************************************
 * �ļ���: sendwave.c
 *   �汾: V1.0
 *   ����: ������
 *   ����: 2017/2/12
 *   ˵��: ���ļ�����SerialTool����Ĳ�����ʾ���ܵ���λ���ο�����, �����ǽ���
 *         ֵת��ΪSerialTool����ʶ���֡, �û���ʵ�ִ��ڷ��ͺ���, ��ϱ�����
 *         ����ʵ�ִ��ڷ��Ͳ��ε���ʾ, �������ʺ�SerialTool v1.0.11�������汾.
 *
 * SerialToolԴ������: https://github.com/Le-Seul/SerialTool
 * SerialTool��װ������: https://github.com/Le-Seul/SerialTool/releases
 *
 *****************************************************************************/

#include "sendwave.h"

/* �˴�����һЩ����, �����޸�! */
enum {
    Ch_Num          = 16,
    Frame_Head      = 0xA3,     // ֡ͷʶ����
    Frame_PointMode = 0xA8,     // ��ģʽʶ����
    Frame_SyncMode  = 0xA9,     // ͬ��ģʽʶ����
    Format_Int8     = 0x10,     // int8ʶ����
    Format_Int16    = 0x20,     // int16ʶ����
    Format_Int32    = 0x30,     // int32ʶ����
    Format_Float    = 0x00      // floatʶ����
};

/* ��������: ����int8��������
 * ��������:
 *     buffer : ���뻺����, ��Ҫ4byte
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, 8bit�з�������
 *     ����ֵ : ����֡����(��λΪbyte)
 **/
char ws_point_int8(char *buffer, char channel, int8_t value)
{
    if (channel > Ch_Num) { // ͨ��ֵ���Ϸ�ֱ�ӷ���
        return 0;
    }
    // ֡ͷ
    *buffer++ = Frame_Head;
    *buffer++ = Frame_PointMode;
    *buffer++ = channel | Format_Int8; // ͨ�������ݸ�ʽ��Ϣ
    *buffer = value; // ������ӵ�֡
    return 4; // ����֡����
}

/* ��������: ����int16��������
 * ��������:
 *     buffer : ���뻺����, ��Ҫ5byte
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, 16bit�з�������
 *     ����ֵ : ����֡����(��λΪbyte)
 **/
char ws_point_int16(char *buffer, char channel, int16_t value)
{
    if (channel > Ch_Num) { // ͨ��ֵ���Ϸ�ֱ�ӷ���
        return 0;
    }
    // ֡ͷ
    *buffer++ = Frame_Head;
    *buffer++ = Frame_PointMode;
    *buffer++ = channel | Format_Int16; // ͨ�������ݸ�ʽ��Ϣ
    // ������ӵ�֡
    *buffer++ = (value >> 8) & 0xFF;
    *buffer = value & 0xFF;
    return 5; // ����֡����
}

/* ��������: ����int32��������
 * ��������:
 *     buffer : ���뻺����, ��Ҫ7byte
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, 32bit�з�������
 *     ����ֵ : ����֡����(��λΪbyte)
 **/
char ws_point_int32(char *buffer, char channel, int32_t value)
{
    if ((uint8_t)channel > Ch_Num) { // ͨ��ֵ���Ϸ�ֱ�ӷ���
        return 0;
    }
    // ֡ͷ
    *buffer++ = Frame_Head;
    *buffer++ = Frame_PointMode;
    *buffer++ = channel | Format_Int32; // ͨ�������ݸ�ʽ��Ϣ
    // ������ӵ�֡
    *buffer++ = (value >> 24) & 0xFF;
    *buffer++ = (value >> 16) & 0xFF;
    *buffer++ = (value >> 8) & 0xFF;
    *buffer = value & 0xFF;
    return 7; // ����֡����
}

/* ��������: ����float��������
 * ��������:
 *     buffer : ���뻺����, ��Ҫ7byte
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, ����Ϊ�����ȸ���(32bit)
 *     ����ֵ : ����֡����(��λΪbyte)
 **/
char ws_point_float(char *buffer, char channel, float value)
{
    // ������ϱ�������ʵ�ָ��㵽���εı任
    union {
        float f;
        uint32_t i;
    } temp;

    if (channel > Ch_Num) { // ͨ��ֵ���Ϸ�ֱ�ӷ���
        return 0;
    }
    temp.f = value;
    // ֡ͷ
    *buffer++ = Frame_Head;
    *buffer++ = Frame_PointMode;
    *buffer++ = channel | Format_Float; // ͨ�������ݸ�ʽ��Ϣ
    // ������ӵ�֡
    *buffer++ = (temp.i >> 24) & 0xFF;
    *buffer++ = (temp.i >> 16) & 0xFF;
    *buffer++ = (temp.i >>  8) & 0xFF;
    *buffer = temp.i & 0xFF;
    return 7; // ����֡����
}

/* ��������: ͬ������ģʽ��������ʼ��
 * ��������:
 *     buffer : ���뻺����, ͬ��ģʽ���ռ��83bytes
 **/
void ws_frame_init(char *buffer)
{
    *buffer++ = Frame_Head;
    *buffer++ = Frame_SyncMode;
    *buffer = 0;
}

/* ��������: ��ȡͬ��ģʽ����������(��λbytes)
 * ��������:
 *     buffer : ͬ��ģʽ֡������
 **/
char ws_frame_length(const char *buffer)
{
    return buffer[2] + 3;
}

/* ��������: ����ͬ����֡�м���һ��int8��������
 * ��������:
 *     buffer : �Ѿ���ʼ����֡������
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, ����Ϊint8
 *     ����ֵ : 0, ����ɹ�, 1, ֡�����Ѿ��ﵽ����
 **/
char ws_add_int8(char *buffer, char channel, int8_t value)
{
    char count = buffer[2];
    char *p = buffer + count + 3; // ����ǰ������

    count += 2;
    if (count < 69) { // ֡����ֽ���
        buffer[2] = count;
        *p++ = channel | Format_Int8; // ͨ�������ݸ�ʽ��Ϣ
        *p = value; // ������ӵ�֡
        return 1;
    }
    return 0;
}

/* ��������: ����ͬ����֡�м���һ��int16��������
 * ��������:
 *     buffer : �Ѿ���ʼ����֡������
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, ����Ϊint16
 *     ����ֵ : 0, ����ɹ�, 1, ֡�����Ѿ��ﵽ����
 **/
char ws_add_int16(char *buffer, char channel, int16_t value)
{
    char count = buffer[2];
    char *p = buffer + count + 3; // ����ǰ������

    count += 3;
    if (count < 69) { // ֡����ֽ���
        buffer[2] = count;
        *p++ = channel | Format_Int16; // ͨ�������ݸ�ʽ��Ϣ
        // ������ӵ�֡
        *p++ = (value >> 8) & 0xFF;
        *p = value & 0xFF;
        return 1;
    }
    return 0;
}

/* ��������: ����ͬ����֡�м���һ��int32��������
 * ��������:
 *     buffer : �Ѿ���ʼ����֡������
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, ����Ϊint32
 *     ����ֵ : 0, ����ɹ�, 1, ֡�����Ѿ��ﵽ����
 **/
char ws_add_int32(char *buffer, char channel, int32_t value)
{
    char count = buffer[2];
    char *p = buffer + count + 3; // ����ǰ������

    count += 5;
    if (count < 69) { // ֡����ֽ���
        buffer[2] = count;
        *p++ = channel | Format_Int32; // ͨ�������ݸ�ʽ��Ϣ
        // ������ӵ�֡
        *p++ = (value >> 24) & 0xFF;
        *p++ = (value >> 16) & 0xFF;
        *p++ = (value >> 8) & 0xFF;
        *p = value & 0xFF;
        return 1;
    }
    return 0;
}

/* ��������: ����ͬ����֡�м���һ��float��������
 * ��������:
 *     buffer : �Ѿ���ʼ����֡������
 *     channel: ͨ��, ȡֵ��ΧΪ0~15
 *     value  : ͨ������ֵ, ����Ϊfloat
 *     ����ֵ : 0, ����ɹ�, 1, ֡�����Ѿ��ﵽ����
 **/
char ws_add_float(char *buffer, char channel, float value)
{
    char count = buffer[2];
    char *p = buffer + count + 3; // ����ǰ������

    count += 5;
    if (count < 69) { // ֡����ֽ���
        union {
            float f;
            uint32_t i;
        } temp;
        buffer[2] = count;
        temp.f = value;
        *p++ = channel | Format_Float; // ͨ�������ݸ�ʽ��Ϣ
        // ������ӵ�֡
        *p++ = (temp.i >> 24) & 0xFF;
        *p++ = (temp.i >> 16) & 0xFF;
        *p++ = (temp.i >>  8) & 0xFF;
        *p = temp.i & 0xFF;
        return 1;
    }
    return 0;
}

/* end of file sendwave.c */
