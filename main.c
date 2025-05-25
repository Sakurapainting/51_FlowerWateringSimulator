#include <reg51.h>
#include <INTRINS.H>
#include "pca.h"
#include "relay.h"     // 包含继电器控制头文件
#include "wavegen.h"   // 包含方波发生器头文件
#include "flowmeter.h" // 包含流量计头文件

#define multiplier 1.085

// 系统状态定义
#define SYS_STATE_OFF     0  // 系统关闭
#define SYS_STATE_WATERING 1  // 系统浇水中

// 当前系统状态
BYTE sysState = SYS_STATE_OFF;

// 按键和浇水控制相关变量
sbit KEY = P3^3;            // 按键连接到P3.3
bit keyPressed = 0;         // 按键按下标志

// 按键处理函数
void processKey() {
    if (KEY == 0 && !keyPressed) {  // 按键按下（低电平有效）
        keyPressed = 1;
        
        // 切换系统状态
        if (sysState == SYS_STATE_OFF) {
            // 开启浇水系统
            sysState = SYS_STATE_WATERING;
            Relay_On();                    // 打开继电器
            
            // 重置和启动流量计
            FlowMeter_Reset();             // 重置流量计计数
            FlowMeter_Start();             // 启动流量计
            FlowMeter_SetMode(FLOW_MODE_CURR); // 设置为显示当前流量
            
            // 强制立即更新显示
            FlowMeter_UpdateDisplay();
        } else {
            // 关闭浇水系统
            sysState = SYS_STATE_OFF;
            Relay_Off();                  // 关闭继电器
            FlowMeter_Stop();             // 停止流量计
            FlowMeter_SetMode(FLOW_MODE_OFF); // 设置为显示时间
        }
        
        delay_ms(20);  // 简单消抖
    } 
    else if (KEY == 1 && keyPressed) {  // 按键释放
        keyPressed = 0;
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