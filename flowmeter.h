#ifndef __FLOWMETER_H__
#define __FLOWMETER_H__

#include "reg51.h"
#include "pca.h"

// 定义流量计模式
#define FLOW_MODE_OFF    0    // 流量计关闭状态
#define FLOW_MODE_CURR   1    // 显示当前流量
#define FLOW_MODE_TOTAL  2    // 显示累计流量

// 模式指示符和单位定义
#define flow_mode_indicator 5   // 使用数字5表示当前流量模式
#define FLOW_SPEED_MULTIPLIER 1 // 流量倍增因子设为1，每个脉冲=1毫升

/*
 * 流量计算说明：
 * - T0定时器目标输出5Hz方波信号
 * - INT0设置为下降沿触发（IT0=1），每秒应该产生5个中断
 * - 每个中断计数一次，代表1毫升水流
 * - 当前流量计算：每秒统计脉冲数，即为毫升/秒
 * - 累计流量计算：每秒累加一次当前流量值
 * 
 * 当前问题分析（显示18毫升/秒）：
 * 1. T0可能实际输出18Hz而不是5Hz
 *    - 原因：T0_INIT_VAL计算错误
 *    - 解决：重新计算并使用软件分频
 * 
 * 2. 可能的其他原因：
 *    - 继电器触点抖动产生额外脉冲
 *    - 信号干扰或噪声
 *    - INT0中断设置问题
 * 
 * 新的设计方案：
 * - T0使用50ms定时，产生10Hz基频
 * - 软件2分频，输出5Hz方波
 * - 每秒应该产生5个下降沿，对应5个脉冲
 */

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
WORD FlowMeter_GetCurrentFlow(void);    // 获取当前流量（毫升/秒）
unsigned long FlowMeter_GetTotalFlow(void); // 获取累计流量（毫升）
void UpdateCurrentFlowDisplay(void);    // 更新当前流量显示（毫升/秒）
void UpdateTotalFlowDisplay(void);      // 更新累计流量显示（毫升）
void FlowMeter_UpdateDisplay(void);     // 检查并更新显示（在主循环中调用）

// 新增24C02存储相关函数
void SaveTotalFlowToEEPROM(void);       // 保存累计流量到24C02
unsigned long LoadTotalFlowFromEEPROM(void); // 从24C02读取累计流量
void FlowMeter_ForceSave(void);         // 强制保存当前累计流量
unsigned long FlowMeter_GetLastSavedFlow(void); // 获取上次保存的累计流量
bit FlowMeter_HasUnsavedChanges(void);  // 检查是否有未保存的变化

#endif /* __FLOWMETER_H__ */
