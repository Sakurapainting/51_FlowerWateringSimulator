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

// 扩展的系统参数结构体定义 - 支持年月日
typedef struct {
    WORD year;      // 年份 (如 2025)
    BYTE month;     // 月份 (1-12)
    BYTE day;       // 日期 (1-31)
    BYTE hour;      // 小时 (0-23)
    BYTE min;       // 分钟 (0-59)
    BYTE sec;       // 秒 (0-59)
} SYS_PARAMS;

// 显示相关定义
#define DISP_PORT P2  // 八位数码管连接端口
#define SEG_OFF 0x00  // 字段全灭

// 时间位置定义
#define YEAR_POS  1
#define MONTH_POS 2
#define DAY_POS   3
#define HOUR_POS  4
#define MIN_POS   5
#define SEC_POS   6

// 显示模式定义
#define DISPLAY_TIME_MODE    0  // 显示时分秒 (HHMMSS，右侧6位)
#define DISPLAY_DATE_MODE    1  // 显示年月日 (YYYYMMDD，全部8位)

// 外部变量声明
extern SYS_PARAMS SysPara1;
extern unsigned char xdata dispbuff[8];
extern BYTE datetime_display_mode;  // 日期时间显示模式

// 基础函数声明
void PCA_Init(void);                      // PCA初始化函数
void delay_ms(unsigned int ms);           // 延时函数
void FillDispBuf(BYTE hour, BYTE min, BYTE sec); // 填充时间显示缓冲区 (6位)
void FillDateBuf(WORD year, BYTE month, BYTE day); // 填充日期显示缓冲区 (8位)
void SendTo595(unsigned char data_seg, unsigned char data_bit); // 发送数据到595
void disp(void);                          // 显示函数
void Resetdispbuff(void);                 // 重置显示缓冲区
void FillCustomDispBuf(BYTE val1, BYTE val2, BYTE val3, BYTE val4, BYTE val5, BYTE val6); // 自定义显示缓冲区填充 (6位)
void FillCustomDispBuf8(BYTE val1, BYTE val2, BYTE val3, BYTE val4, BYTE val5, BYTE val6, BYTE val7, BYTE val8); // 8位自定义显示缓冲区填充

// 时间设置相关函数声明
void PCA_SetTimeEditMode(BYTE position);   // 设置时间编辑模式
void PCA_ExitTimeEditMode(void);          // 退出时间编辑模式
void PCA_IncreaseTimeValue(BYTE position); // 增加时间值
void PCA_SetTime(BYTE hour, BYTE min, BYTE sec); // 设置时分秒
void PCA_SetDate(WORD year, BYTE month, BYTE day); // 设置年月日
void PCA_SetDateTime(WORD year, BYTE month, BYTE day, BYTE hour, BYTE min, BYTE sec); // 设置完整日期时间

// 获取时间相关函数声明
WORD PCA_GetYear(void);    // 获取年份
BYTE PCA_GetMonth(void);   // 获取月份
BYTE PCA_GetDay(void);     // 获取日期
BYTE PCA_GetHour(void);    // 获取小时
BYTE PCA_GetMin(void);     // 获取分钟
BYTE PCA_GetSec(void);     // 获取秒

// 日期时间显示控制函数
void PCA_SetDisplayMode(BYTE mode);       // 设置显示模式 (时间/日期)
BYTE PCA_GetDisplayMode(void);            // 获取当前显示模式
void PCA_ToggleDisplayMode(void);         // 切换显示模式
void PCA_ResetAutoToggle(void);           // 重置自动轮换计数器
void PCA_ManualToggleDisplay(void);       // 手动切换显示模式并重置轮换

// 日期计算辅助函数
BYTE PCA_GetDaysInMonth(WORD year, BYTE month); // 获取指定月份的天数
bit PCA_IsLeapYear(WORD year);            // 判断是否为闰年
void PCA_UpdateDateTime(void);            // 更新日期时间（内部调用）

void PCA_ProcessTimeUpdate(void);     // 处理时间更新（在主循环中调用）
void PCA_ProcessDisplayUpdate(void);  // 处理显示更新（在主循环中调用）
void PCA_ProcessBlinkUpdate(void);


#endif /* __PCA_H__ */
