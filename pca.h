#ifndef __PCA_H__
#define __PCA_H__

#include "reg51.h"

// 晶振频率和定时器值定义
#define FOSC    11059200L
#define T100Hz  (FOSC / 12 / 100)
#define T1000Hz (FOSC / 12 / 1000)

// 类型定义
typedef unsigned char BYTE;
typedef unsigned int WORD;

// 系统参数结构体定义
typedef struct {
    BYTE hour;
    BYTE min;
    BYTE sec;
} SYS_PARAMS;

// 显示相关定义
#define DISP_PORT P2  // 八位数码管连接端口
#define SEG_OFF 0x00  // 字段全灭

// 时间位置定义
#define HOUR_POS 1
#define MIN_POS  2
#define SEC_POS  3

// 外部变量声明
extern SYS_PARAMS SysPara1;
extern unsigned char dispbuff[8];

// 函数声明
void PCA_Init(void);                      // PCA初始化函数
void delay_ms(unsigned int ms);           // 延时函数
void FillDispBuf(BYTE hour, BYTE min, BYTE sec); // 填充显示缓冲区
void SendTo595(unsigned char data_seg, unsigned char data_bit); // 发送数据到595
void disp(void);                          // 显示函数
void Resetdispbuff(void);                 // 重置显示缓冲区
void FillCustomDispBuf(BYTE val1, BYTE val2, BYTE val3, BYTE val4, BYTE val5, BYTE val6); // 自定义显示缓冲区填充

// 新增函数声明 - 时间设置相关
void PCA_SetTimeEditMode(BYTE position);   // 设置时间编辑模式
void PCA_ExitTimeEditMode(void);          // 退出时间编辑模式
void PCA_IncreaseTimeValue(BYTE position); // 增加时间值

#endif /* __PCA_H__ */
