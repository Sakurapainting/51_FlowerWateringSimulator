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

// 系统状态定义 - 扩展支持年月日设置
#define SYS_STATE_OFF      0  // 系统关闭
#define SYS_STATE_WATERING 1  // 系统浇水中
#define SYS_STATE_SET_YEAR 2  // 设置年份
#define SYS_STATE_SET_MONTH 3 // 设置月份
#define SYS_STATE_SET_DAY  4  // 设置日期
#define SYS_STATE_SET_HOUR 5  // 设置小时
#define SYS_STATE_SET_MIN  6  // 设置分钟
#define SYS_STATE_SET_SEC  7  // 设置秒

// 当前系统状态
BYTE sysState = SYS_STATE_OFF;

// 按键和浇水控制相关变量
sbit KEY = P3^3;            // 按键连接到P3.3
bit keyPressed = 0;         // 按键按下标志
bit justEnteredSetMode = 0; // 标志是否刚刚进入设置模式
unsigned int xdata keyPressTime = 0; // 按键按下持续时间（以10ms为单位）
#define LONG_PRESS_TIME 100   // 长按时间阈值（约1秒，调整为合理值）

// 按键处理函数
void processKey() {
    if (KEY == 0 && !keyPressed) {
        keyPressed = 1;
        keyPressTime = 0;
    } 
    else if (KEY == 0 && keyPressed) {
        keyPressTime++;
        
        // 长按进入设置模式，从年份开始设置
        if (keyPressTime == LONG_PRESS_TIME && sysState == SYS_STATE_OFF) {
            sysState = SYS_STATE_SET_YEAR;
            PCA_SetTimeEditMode(YEAR_POS);
            PCA_SetDisplayMode(DISPLAY_DATE_MODE);  // 切换到日期显示模式
            PCA_ResetAutoToggle();  // 重置自动轮换计数器
            justEnteredSetMode = 1;
        }
    }
    else if (KEY == 1 && keyPressed) {
        if (keyPressTime < LONG_PRESS_TIME) {
            // 短按：根据当前状态增加对应的数值
            switch (sysState) {
                case SYS_STATE_OFF:
                    // 检查是否定时浇水正在运行
                    if(!timed_watering.enabled || !timed_watering.is_watering) {
                        sysState = SYS_STATE_WATERING;
                        
                        // 开始手动浇水记录 - 优化后无需传参
                        StartManualWateringRecord();
                        
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
                    
                    // 结束手动浇水记录 - 优化后无需传参
                    EndManualWateringRecord();
                    break;
                    
                case SYS_STATE_SET_YEAR:
                    PCA_IncreaseTimeValue(YEAR_POS);
                    break;
                    
                case SYS_STATE_SET_MONTH:
                    PCA_IncreaseTimeValue(MONTH_POS);
                    break;
                    
                case SYS_STATE_SET_DAY:
                    PCA_IncreaseTimeValue(DAY_POS);
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
            // 长按：切换到下一项设置
            if (justEnteredSetMode) {
                justEnteredSetMode = 0;
            }
            else if (sysState >= SYS_STATE_SET_YEAR && sysState <= SYS_STATE_SET_SEC) {
                sysState++;
                
                // 设置完所有参数后退出设置模式
                if (sysState > SYS_STATE_SET_SEC) {
                    sysState = SYS_STATE_OFF;
                    PCA_ExitTimeEditMode();
                    // 恢复到时间显示模式并重置自动轮换
                    PCA_SetDisplayMode(DISPLAY_TIME_MODE);
                    PCA_ResetAutoToggle();
                } else {
                    // 根据当前设置状态切换显示模式和编辑位置
                    switch(sysState) {
                        case SYS_STATE_SET_YEAR:
                            PCA_SetTimeEditMode(YEAR_POS);
                            PCA_SetDisplayMode(DISPLAY_DATE_MODE);
                            break;
                        case SYS_STATE_SET_MONTH:
                            PCA_SetTimeEditMode(MONTH_POS);
                            PCA_SetDisplayMode(DISPLAY_DATE_MODE);
                            break;
                        case SYS_STATE_SET_DAY:
                            PCA_SetTimeEditMode(DAY_POS);
                            PCA_SetDisplayMode(DISPLAY_DATE_MODE);
                            break;
                        case SYS_STATE_SET_HOUR:
                            PCA_SetTimeEditMode(HOUR_POS);
                            PCA_SetDisplayMode(DISPLAY_TIME_MODE);  // 切换到时间显示模式
                            break;
                        case SYS_STATE_SET_MIN:
                            PCA_SetTimeEditMode(MIN_POS);
                            PCA_SetDisplayMode(DISPLAY_TIME_MODE);
                            break;
                        case SYS_STATE_SET_SEC:
                            PCA_SetTimeEditMode(SEC_POS);
                            PCA_SetDisplayMode(DISPLAY_TIME_MODE);
                            break;
                    }
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
    
    
    
    PCA_Init();
    Relay_Init();
    WaveGen_Init();
    WaveGen_Start();
    UART_Init();
    I2C_Init();  
    FlowMeter_Init();
    KeyboardControl_Init();  // 初始化按键控制
    
    // 发送启动信息到串口
    UART_SendString("\r\nWatering System Started v4.2 (Full 8-Digit Display)\r\n");
    UART_SendString("Auto Display: Time<->Date every 5 seconds\r\n");
    UART_SendString("Date Format: YYYYMMDD (8-digit full display)\r\n");
    UART_SendString("Time Format: HH-MM-SS (8-digit full display)\r\n");
    UART_SendString("Flow Format: XXXXXXX10/11 (8-digit, 7-digit flow value)\r\n");
    UART_SendString("Auto Format: XXXXXXXA/B/c/d (8-digit param display)\r\n");  // 更新说明
    UART_SendString("P3.3 Key: Long press to set date/time\r\n");
    UART_SendString("Setting order: Year->Month->Day->Hour->Min->Sec\r\n");
    
    while (1) {
        processKey();
        KeyboardControl_Scan();
        CheckAndUpdateAutoDisplay();
        FlowMeter_UpdateDisplay();
        UART_ProcessCommand();
        
        PCA_ProcessTimeUpdate();
        PCA_ProcessDisplayUpdate();
        PCA_ProcessBlinkUpdate();

        delay_ms(10);
    }
}