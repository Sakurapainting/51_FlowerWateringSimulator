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

// 外部变量声明
extern SYS_PARAMS SysPara1;
extern unsigned char dispbuff[8];

// 方波发生器相关定义
#define FLOWMETER_PIN P1_0   // 方波输出引脚 (P1.0)
#define WAVE_FREQ     5      // 方波频率 (Hz)

// 函数声明
void PCA_Init(void);                      // PCA初始化函数
void delay_ms(unsigned int ms);           // 延时函数
void FillDispBuf(BYTE hour, BYTE min, BYTE sec); // 填充显示缓冲区
void SendTo595(unsigned char data_seg, unsigned char data_bit); // 发送数据到595
void disp(void);                          // 显示函数
void Resetdispbuff(void);                 // 重置显示缓冲区
void WaveGen_Init(void);                  // 方波发生器初始化
void WaveGen_Start(void);                 // 启动方波发生器
void WaveGen_Stop(void);                  // 停止方波发生器
BYTE WaveGen_GetState(void);              // 获取方波发生器状态

#endif /* __PCA_H__ */
