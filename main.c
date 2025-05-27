#include <reg51.h>
#include <INTRINS.H>
#include "pca.h"
#include "relay.h"     // 包含继电器控制头文件
#include "wavegen.h"   // 包含方波发生器头文件
#include "flowmeter.h" // 包含流量计头文件
#include "uart.h"      // 添加串口通信头文件
#include "keyboard_control.h"  // 添加按键控制头文件
#include "i2c.h"      // 添加I2C头文件

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
    if (KEY == 0 && !keyPressed) {
        keyPressed = 1;
        keyPressTime = 0;
    } 
    else if (KEY == 0 && keyPressed) {
        keyPressTime++;
        
        if (keyPressTime == LONG_PRESS_TIME && sysState == SYS_STATE_OFF) {
            sysState = SYS_STATE_SET_HOUR;
            PCA_SetTimeEditMode(sysState - SYS_STATE_SET_HOUR + 1);
            justEnteredSetMode = 1;
        }
    }
    else if (KEY == 1 && keyPressed) {
        if (keyPressTime < LONG_PRESS_TIME) {
            switch (sysState) {
                case SYS_STATE_OFF:
                    // 检查是否定时浇水正在运行
                    if(!timed_watering.enabled) {
                        sysState = SYS_STATE_WATERING;
                        Relay_On();
                        FlowMeter_Reset();
                        FlowMeter_Start();
                        FlowMeter_SetMode(FLOW_MODE_CURR);
                        FlowMeter_UpdateDisplay();
                        auto_display_mode = DISPLAY_MODE_CLOCK; // 手动浇水时显示时钟
                    }
                    break;
                    
                case SYS_STATE_WATERING:
                    sysState = SYS_STATE_OFF;
                    Relay_Off();
                    FlowMeter_Stop();
                    FlowMeter_SetMode(FLOW_MODE_OFF);
                    break;
                    
                case SYS_STATE_SET_HOUR:
                    PCA_IncreaseTimeValue(HOUR_POS);
                    break;
                    
                case SYS_STATE_SET_MIN:
                    PCA_IncreaseTimeValue(MIN_POS);
                    break;
                    
                case SYS_STATE_SET_SEC:
                    PCA_IncreaseTimeValue(SEC_POS);
                    break;
            }
        } 
        else if (keyPressTime >= LONG_PRESS_TIME) {
            if (justEnteredSetMode) {
                justEnteredSetMode = 0;
            }
            else if (sysState >= SYS_STATE_SET_HOUR && sysState <= SYS_STATE_SET_SEC) {
                sysState++;
                
                if (sysState > SYS_STATE_SET_SEC) {
                    sysState = SYS_STATE_OFF;
                    PCA_ExitTimeEditMode();
                } else {
                    PCA_SetTimeEditMode(sysState - SYS_STATE_SET_HOUR + 1);
                }
            }
        }
        
        keyPressed = 0;
        keyPressTime = 0;
        delay_ms(20);
    }
}

void main(void) {
    EA = 1;
    P0 = 0xFF;
    
    /* 
     * ========================================
     * 智能浇花系统 - 定时定量浇水功能
     * ========================================
     * 
     * 【系统特点】
     * ============
     * ✓ 精确时间控制：可设定每天固定时间自动浇水
     * ✓ 定量浇水：按毫升数精确控制浇水量
     * ✓ 数据保存：累计流量掉电不丢失
     * ✓ 智能防重：每天只执行一次，避免重复浇水
     * ✓ 手动控制：支持手动立即浇水
     * ✓ 串口设置：支持通过串口设置系统时间
     * 
     * 【硬件连接】
     * ============
     * 控制部分：
     * - P1.1: 继电器控制 (低电平触发)
     * - P1.0: 方波发生器输出 (5Hz脉冲信号)
     * - P3.2: INT0中断输入 (流量计脉冲计数)
     * 
     * 按键输入：
     * - P1.2: 启动/停止定时浇水
     * - P1.3: 增加参数值
     * - P1.4: 减少参数值  
     * - P1.7: 切换设置模式
     * - P3.3: 手动浇水/时间设置
     * 
     * 存储设备：
     * - P2.0: I2C数据线SDA (连接24C02)
     * - P2.1: I2C时钟线SCL (连接24C02)
     * 
     * 【快速使用指南】
     * ================
     * 
     * 🕐 第一步：设置浇水时间
     *    ┌─────────────────────────────────────┐
     *    │ 1. 按P1.7 → 数码管显示"HH0002"      │
     *    │    用P1.3/P1.4设置小时(0-23)       │
     *    │ 2. 按P1.7 → 数码管显示"MM0003"      │
     *    │    用P1.3/P1.4设置分钟(0-59)       │
     *    │ 3. 按P1.7 → 数码管显示"SS0001"      │
     *    │    用P1.3/P1.4设置秒(0-59)         │
     *    └─────────────────────────────────────┘
     * 
     * 💧 第二步：设置浇水量
     *    ┌─────────────────────────────────────┐
     *    │ 4. 按P1.7 → 数码管显示"MMMM05"      │
     *    │    用P1.3/P1.4设置毫升数            │
     *    │    (50-9999ml，步长50ml)            │
     *    └─────────────────────────────────────┘
     * 
     * ▶️ 第三步：启动定时浇水
     *    ┌─────────────────────────────────────┐
     *    │ 5. 按P1.2启动定时浇水功能           │
     *    │    系统进入自动模式，等待设定时间    │
     *    └─────────────────────────────────────┘
     * 
     * 【数码管显示说明】
     * ==================
     * 
     * 时钟模式：显示当前时间 "HHMMSS"
     * 例如：145023 = 14:50:23
     * 
     * 参数设置模式：
     * ┌──────────┬─────────────┬───────────────┐
     * │ 显示格式  │    含义     │    示例      │
     * ├──────────┼─────────────┼───────────────┤
     * │ HH0003   │  设置小时   │ 060003=6点   │
     * │ MM0002   │  设置分钟   │ 300002=30分  │
     * │ SS0001   │  设置秒     │ 450001=45秒  │
     * │ MMMM05   │  设置毫升   │ 150005=150ml │
     * └──────────┴─────────────┴───────────────┘
     * 
     * 浇水状态：
     * - 等待浇水：参数闪烁显示
     * - 正在浇水：显示剩余毫升数 "XXXX05"
     * - 浇水完成：返回时钟显示
     * 
     * 【实际使用例子】
     * ================
     * 
     * 🌱 场景1：每天早上7点浇150毫升水
     * 步骤：
     * 1. P1.7 → 设置"070003" (7点)
     * 2. P1.7 → 设置"000002" (0分) 
     * 3. P1.7 → 设置"000001" (0秒)
     * 4. P1.7 → 设置"150005" (150毫升)
     * 5. P1.2 → 启动定时浇水
     * 结果：每天7:00:00自动浇水150毫升
     * 
     * 🌱 场景2：傍晚6点半浇100毫升水  
     * 步骤：
     * 1. P1.7 → 设置"180003" (18点)
     * 2. P1.7 → 设置"300002" (30分)
     * 3. P1.7 → 设置"000001" (0秒) 
     * 4. P1.7 → 设置"100005" (100毫升)
     * 5. P1.2 → 启动定时浇水
     * 结果：每天18:30:00自动浇水100毫升
     * 
     * 【故障处理】
     * ============
     * 
     * Q: 设置时间后不浇水？
     * A: 检查是否按P1.2启动定时功能
     * 
     * Q: 浇水量不准确？
     * A: 检查流量计连接和方波发生器频率
     * 
     * Q: 断电后累计流量丢失？
     * A: 检查24C02连接和I2C通信
     * 
     * Q: 每天浇水多次？
     * A: 系统设计为每天只浇一次，如出现请检查时钟
     */
    
    PCA_Init();
    Relay_Init();
    WaveGen_Init();
    WaveGen_Start();
    UART_Init();
    I2C_Init();  
    FlowMeter_Init();
    KeyboardControl_Init();  // 初始化按键控制
    
    // 发送英文启动信息到串口
    UART_SendString("\r\nWatering System Started\r\n");
    UART_SendString("Time setting available\r\n");
    UART_SendString("Auto watering initialized\r\n");
    
    while (1) {
        processKey();
        KeyboardControl_Scan();
        CheckAndUpdateAutoDisplay();
        FlowMeter_UpdateDisplay();
        UART_ProcessCommand();
        
        delay_ms(10);
    }
}