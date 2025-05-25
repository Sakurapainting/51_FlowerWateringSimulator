#include <reg51.h>
#include <INTRINS.H>
#include "pca.h"
#include "relay.h"     // 包含继电器控制头文件
#include "wavegen.h"   // 包含方波发生器头文件
#include "flowmeter.h" // 包含流量计头文件

#define multiplier 1.085

// 系统状态定义
#define SYS_STATE_OFF      0  // 系统关闭
#define SYS_STATE_WATERING 1  // 系统浇水中
#define SYS_STATE_SET_HOUR 2  // 设置小时
#define SYS_STATE_SET_MIN  3  // 设置分钟
#define SYS_STATE_SET_SEC  4  // 设置秒

// 当前系统状态
BYTE sysState = SYS_STATE_OFF;

// 按键和浇水控制相关变量
sbit KEY = P3^3;            // 按键连接到P3.3
bit keyPressed = 0;         // 按键按下标志
bit justEnteredSetMode = 0; // 标志是否刚刚进入设置模式
unsigned int keyPressTime = 0; // 按键按下持续时间（以10ms为单位）
#define LONG_PRESS_TIME 500   // 长按时间阈值（约1秒）

// 按键处理函数
void processKey() {
    if (KEY == 0 && !keyPressed) {  // 按键按下（低电平有效）
        keyPressed = 1;
        keyPressTime = 0;  // 重置按键计时器
    } 
    else if (KEY == 0 && keyPressed) {  // 按键持续按下
        keyPressTime++;
        
        // 检测长按（约1秒）- 仅在系统关闭状态下有效
        if (keyPressTime == LONG_PRESS_TIME && sysState == SYS_STATE_OFF) {
            // 进入时间设置模式
            sysState = SYS_STATE_SET_HOUR;
            // 设置小时位闪烁
            PCA_SetTimeEditMode(sysState - SYS_STATE_SET_HOUR + 1);
            justEnteredSetMode = 1;  // 设置刚刚进入设置模式标志
        }
    }
    else if (KEY == 1 && keyPressed) {  // 按键释放
        if (keyPressTime < LONG_PRESS_TIME) {  // 短按
            switch (sysState) {
                case SYS_STATE_OFF:
                    // 开启浇水系统
                    sysState = SYS_STATE_WATERING;
                    Relay_On();                    // 打开继电器
                    
                    // 重置和启动流量计
                    FlowMeter_Reset();             // 重置流量计计数
                    FlowMeter_Start();             // 启动流量计
                    FlowMeter_SetMode(FLOW_MODE_CURR); // 设置为显示当前流量
                    
                    // 强制立即更新显示
                    FlowMeter_UpdateDisplay();
                    break;
                    
                case SYS_STATE_WATERING:
                    // 关闭浇水系统
                    sysState = SYS_STATE_OFF;
                    Relay_Off();                  // 关闭继电器
                    FlowMeter_Stop();             // 停止流量计
                    FlowMeter_SetMode(FLOW_MODE_OFF); // 设置为显示时间
                    break;
                    
                case SYS_STATE_SET_HOUR:
                    // 小时值加1
                    PCA_IncreaseTimeValue(HOUR_POS);
                    break;
                    
                case SYS_STATE_SET_MIN:
                    // 分钟值加1
                    PCA_IncreaseTimeValue(MIN_POS);
                    break;
                    
                case SYS_STATE_SET_SEC:
                    // 秒值加1
                    PCA_IncreaseTimeValue(SEC_POS);
                    break;
            }
        } 
        else if (keyPressTime >= LONG_PRESS_TIME) {  // 长按释放
            if (justEnteredSetMode) {
                // 如果刚刚进入设置模式，不做任何状态转换，只清除标志
                justEnteredSetMode = 0;
            }
            else if (sysState >= SYS_STATE_SET_HOUR && sysState <= SYS_STATE_SET_SEC) {
                // 在设置模式下，长按释放后切换到下一个设置项
                sysState++;
                
                if (sysState > SYS_STATE_SET_SEC) {  // 完成所有设置
                    sysState = SYS_STATE_OFF;
                    PCA_ExitTimeEditMode();  // 退出时间编辑模式
                } else {
                    // 设置相应位闪烁
                    PCA_SetTimeEditMode(sysState - SYS_STATE_SET_HOUR + 1);
                }
            }
        }
        
        keyPressed = 0;
        keyPressTime = 0;
        delay_ms(20);  // 简单消抖
    }
}

void main(void) {
    EA = 1;               // 开启总中断
    P0 = 0xFF;
    
    /* 浇花系统引脚连接与工作说明：
     * 1. P1.1 - 输出：控制继电器，高电平开启浇水，低电平关闭浇水
     * 2. P1.0 - 输出：方波发生器，输出5Hz脉冲，模拟流量计信号
     * 3. P3.2 (INT0) - 输入：外部中断，接收流量计脉冲进行计数
     * 4. P3.3 - 输入：按键输入，控制浇水系统的开关
     * 5. P2.0-P2.3 - 输出：连接到74HC595，控制数码管显示
     * 
     * 浇水工作过程：
     * - 按下P3.3按键后，系统开始浇水：P1.1输出高电平，继电器闭合
     * - 继电器闭合使P1.0的方波信号传导到P3.2 (INT0)
     * - INT0接收脉冲信号并计数，根据脉冲数计算流量
     * - 显示屏轮流显示当前流量和累计流量
     * - 再次按下按键，系统停止浇水：P1.1输出低电平，继电器断开
     * - 断开后不再有脉冲传到INT0，流量计数停止
     * - 显示屏切换回时间显示模式
     * 
     * 时间设置说明：
     * - 系统关闭状态下，长按按键进入时间设置模式
     * - 短按按键增加当前设置的时间值
     * - 长按并释放切换到下一个设置项（小时->分钟->秒->退出）
     */
    
    PCA_Init();           // 初始化PCA模块，包含时钟功能
    Relay_Init();         // 初始化继电器
    WaveGen_Init();       // 初始化方波发生器
    WaveGen_Start();      // 启动方波发生器产生5Hz方波
    FlowMeter_Init();     // 初始化流量计
    
    while (1) {
        processKey();               // 处理按键输入
        FlowMeter_UpdateDisplay();  // 检查并更新流量显示
        
        // 添加一个简短延时以避免CPU高速空转
        delay_ms(10);
    }
}