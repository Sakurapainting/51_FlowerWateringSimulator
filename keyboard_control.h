#ifndef KEYBOARD_CONTROL_H
#define KEYBOARD_CONTROL_H

#include "reg51.h"
#include "pca.h"
#include "uart.h" 

// 按键定义
sbit KEY_AUTO = P1^2;         // 启动/停止定时浇水
sbit KEY_TIME_UP = P1^3;      // 增加浇水时间
sbit KEY_TIME_DOWN = P1^4;    // 减少浇水时间
sbit KEY_VOL_UP = P1^5;       // 增加浇水毫升数
sbit KEY_VOL_DOWN = P1^6;     // 减少浇水毫升数
sbit KEY_MODE = P1^7;         // 切换参数设置模式

// 定时浇水配置结构
typedef struct TimedWatering {
    unsigned char enabled;              // 是否启用定时浇水
    unsigned char start_hour;           // 开始浇水的小时
    unsigned char start_min;            // 开始浇水的分钟
    unsigned char start_sec;            // 开始浇水的秒
    unsigned int water_volume_ml;       // 单次浇水毫升数
    unsigned char is_watering;          // 是否正在浇水
    unsigned int watering_volume_left;  // 剩余浇水毫升数
    unsigned char triggered_today;      // 今天是否已经触发过
    // 新增：浇水记录相关字段
    unsigned long start_total_flow;     // 开始时的累计流量
    WateringRecord current_record;      // 当前浇水记录
} TimedWatering;

// 参数设置模式定义
#define PARAM_MODE_HOUR      0    // 设置开始小时
#define PARAM_MODE_MIN       1    // 设置开始分钟
#define PARAM_MODE_SEC       2    // 设置开始秒
#define PARAM_MODE_VOLUME    3    // 设置浇水毫升数

// 显示模式定义
#define DISPLAY_MODE_CLOCK    0    // 时钟显示模式
#define DISPLAY_MODE_AUTO     1    // 自动浇水参数显示模式

// 全局变量声明
extern TimedWatering xdata timed_watering; 
extern unsigned char auto_display_mode;
extern unsigned char param_mode;
extern bit display_update_flag; 

// 手动浇水记录变量
extern WateringRecord xdata manual_watering_record;

// 函数声明
void KeyboardControl_Init(void);
void KeyboardControl_Scan(void);
void TimedWatering_Update(void);
void TimedWatering_Start(void);
void TimedWatering_Stop(void);
void DisplayAutoWateringParams(void);
void CheckAndUpdateAutoDisplay(void);

// 浇水记录相关函数 - 避免传参
void StartManualWateringRecord(void);     // 开始手动浇水记录
void EndManualWateringRecord(void);       // 结束手动浇水记录
void StartAutoWateringRecord(void);       // 开始自动浇水记录
void EndAutoWateringRecord(void);         // 结束自动浇水记录

#endif
