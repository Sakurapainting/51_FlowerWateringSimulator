#ifndef __FLOWMETER_H__
#define __FLOWMETER_H__

#include "reg51.h"
#include "pca.h"

// 定义流量计模式
#define FLOW_MODE_OFF    0    // 流量计关闭状态
#define FLOW_MODE_CURR   1    // 显示当前流量
#define FLOW_MODE_TOTAL  2    // 显示累计流量

// 模式指示符
#define flow_mode_indicator 5   // 使用数字5表示当前流量模式

// 函数声明
void FlowMeter_Init(void);              // 初始化流量计
void FlowMeter_Start(void);             // 启动流量计
void FlowMeter_Stop(void);              // 停止流量计
void FlowMeter_Reset(void);             // 复位流量计累计值
void FlowMeter_CalcFlow(void);          // 计算流量（每秒调用一次）
void FlowMeter_DisplayCurrent(void);    // 显示当前流量
void FlowMeter_DisplayTotal(void);      // 显示累计流量
void FlowMeter_SetMode(BYTE mode);      // 设置流量显示模式
BYTE FlowMeter_GetMode(void);           // 获取当前流量显示模式
WORD FlowMeter_GetCurrentFlow(void);    // 获取当前流量
unsigned long FlowMeter_GetTotalFlow(void); // 获取累计流量
void UpdateCurrentFlowDisplay(void);    // 更新当前流量显示
void UpdateTotalFlowDisplay(void);      // 更新累计流量显示
void FlowMeter_UpdateDisplay(void);       // 检查并更新显示（在主循环中调用）

#endif /* __FLOWMETER_H__ */
