#ifndef __UART_H__
#define __UART_H__

#include "reg51.h"
#include "pca.h"

// 浇水类型定义
#define WATERING_TYPE_MANUAL 0    // 手动浇水
#define WATERING_TYPE_AUTO   1    // 自动浇水

// 浇水记录结构体
typedef struct {
    BYTE type;                    // 浇水类型 (0=手动, 1=自动)
    WORD start_year;              // 开始年份
    BYTE start_month;             // 开始月份
    BYTE start_day;               // 开始日期
    BYTE start_hour;              // 开始小时
    BYTE start_min;               // 开始分钟
    BYTE start_sec;               // 开始秒
    WORD end_year;                // 结束年份
    BYTE end_month;               // 结束月份
    BYTE end_day;                 // 结束日期
    BYTE end_hour;                // 结束小时
    BYTE end_min;                 // 结束分钟
    BYTE end_sec;                 // 结束秒
    unsigned long water_volume;   // 本次浇水量 (毫升)
    unsigned long total_flow;     // 累计流量 (毫升)
    BYTE duration_min;            // 持续时间-分钟
    BYTE duration_sec;            // 持续时间-秒
} WateringRecord;

// 串口命令处理相关定义
#define UART_BUF_SIZE 32    // 缓冲区大小

// 函数声明
void UART_Init(void);                    // 初始化串口
void UART_SendByte(BYTE dat);            // 发送一个字节
void UART_SendString(char *s);           // 发送字符串
void UART_ProcessCommand(void);          // 处理串口命令

// 浇水记录输出函数 - 避免传参
void UART_SendManualWateringRecord(void); // 发送手动浇水记录
void UART_SendAutoWateringRecord(void);   // 发送自动浇水记录

#endif /* __UART_H__ */
