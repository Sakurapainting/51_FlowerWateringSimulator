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
    
    /* 
     * ========================================
     * 智能浇花系统 - P3.3按键时间设置功能 (8位数码管版本)
     * ========================================
     * 
     * 【P3.3按键操作说明】
     * ==================
     * 
     * 🔘 短按功能：
     *    - 正常模式：启动/停止手动浇水
     *    - 设置模式：增加当前设置项的数值
     * 
     * 🔘 长按功能（约1秒）：
     *    - 正常模式：进入时间设置模式
     *    - 设置模式：切换到下一个设置项
     * 
     * 【时间设置流程】
     * ================
     * 
     * 1️⃣ 长按P3.3 → 进入年份设置
     *    显示：日期模式 (YYYYMMDD)，全部8位数码管显示
     *    操作：短按P3.3增加年份 (2000-2099)
     *    闪烁：左侧4位年份数字闪烁
     * 
     * 2️⃣ 长按P3.3 → 切换到月份设置
     *    显示：日期模式 (YYYYMMDD)，全部8位数码管显示
     *    操作：短按P3.3增加月份 (1-12)
     *    闪烁：中间2位月份数字闪烁
     * 
     * 3️⃣ 长按P3.3 → 切换到日期设置
     *    显示：日期模式 (YYYYMMDD)，全部8位数码管显示
     *    操作：短按P3.3增加日期 (1-31，自动适应月份)
     *    闪烁：右侧2位日期数字闪烁
     * 
     * 4️⃣ 长按P3.3 → 切换到小时设置
     *    显示：时间模式 (HHMMSS)，右侧6位数码管显示
     *    操作：短按P3.3增加小时 (0-23)
     *    闪烁：小时位闪烁，左侧2位关闭
     * 
     * 5️⃣ 长按P3.3 → 切换到分钟设置
     *    显示：时间模式 (HHMMSS)，右侧6位数码管显示
     *    操作：短按P3.3增加分钟 (0-59)
     *    闪烁：分钟位闪烁，左侧2位关闭
     * 
     * 6️⃣ 长按P3.3 → 切换到秒设置
     *    显示：时间模式 (HHMMSS)，右侧6位数码管显示
     *    操作：短按P3.3增加秒 (0-59)
     *    闪烁：秒位闪烁，左侧2位关闭
     * 
     * 7️⃣ 长按P3.3 → 完成设置，退出设置模式
     *    显示：返回正常时间显示模式，自动轮换
     * 
     * 【显示模式说明】
     * ================
     * 
     * 日期设置时 (步骤1-3)：
     * ┌─────────────────────────────────────┐
     * │ 数码管格式：YYYYMMDD (8位全显示)    │
     * │ 示例："20250527" = 2025年5月27日    │
     * │ 排列：2 0 2 5 0 5 2 7               │
     * │       ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑               │
     * │      千百十个月月日日                │
     * │       年年年年  份  期               │
     * │ 闪烁部分：当前正在设置的位          │
     * └─────────────────────────────────────┘
     * 
     * 时间设置时 (步骤4-6)：
     * ┌─────────────────────────────────────┐
     * │ 数码管格式：  HHMMSS (右侧6位)      │
     * │ 示例："  143025" = 14:30:25         │
     * │ 排列：⚫ ⚫ 1 4 3 0 2 5               │
     * │       ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑               │
     * │      关关时时分分秒秒                │
     * │      闭闭                           │
     * │ 闪烁部分：当前正在设置的位          │
     * └─────────────────────────────────────┘
     * 
     * 【正常运行显示】
     * ================
     * 
     * 自动轮换模式（每5秒切换）：
     * - 时间显示："  143025" (14:30:25)
     * - 日期显示："20250527" (2025年5月27日)
     * 
     * 【使用示例】
     * ============
     * 
     * 设置时间为 2025年12月25日 09:30:00：
     * 
     * 1. 长按P3.3进入设置 → 显示"20250101"，年份"2025"闪烁
     * 2. 短按P3.3将年份保持为2025 (或调整)
     * 3. 长按P3.3切换到月份 → 显示"20250101"，月份"01"闪烁
     * 4. 短按P3.3将月份调到12 → 显示"20251201"
     * 5. 长按P3.3切换到日期 → 显示"20251201"，日期"01"闪烁
     * 6. 短按P3.3将日期调到25 → 显示"20251225"
     * 7. 长按P3.3切换到小时 → 显示"  000000"，小时"00"闪烁
     * 8. 短按P3.3将小时调到09 → 显示"  090000"
     * 9. 长按P3.3切换到分钟 → 显示"  090000"，分钟"00"闪烁
     * 10. 短按P3.3将分钟调到30 → 显示"  093000"
     * 11. 长按P3.3切换到秒 → 显示"  093000"，秒"00"闪烁
     * 12. 保持秒为00
     * 13. 长按P3.3完成设置 → 退出设置模式，开始自动轮换显示
     * 
     * 【技术特点】
     * ============
     * 
     * ✓ 年份完整显示：显示完整4位年份 (2000-2099)
     * ✓ 8位数码管充分利用：日期显示使用全部8位
     * ✓ 时间显示节约：时间显示仅使用右侧6位，左侧2位关闭
     * ✓ 智能闪烁：编辑时相关位数闪烁，其他位保持显示
     * ✓ 自动轮换：正常运行时每5秒在时间和日期间切换
     * ✓ 闰年处理：2月份会自动适应28/29天
     * ✓ 日期校验：切换月份时会自动调整日期上限
     * 
     * 【注意事项】
     * ============
     * 
     * ⚠️ 显示区别：
     *   - 日期模式：8位全显示 (YYYYMMDD)
     *   - 时间模式：右6位显示 (  HHMMSS)
     * 
     * ⚠️ 闪烁提示：
     *   - 年份设置：左侧4位闪烁
     *   - 月份设置：中间2位闪烁 
     *   - 日期设置：右侧2位闪烁
     *   - 时分秒设置：对应2位闪烁
     * 
     * ⚠️ 年份范围：2000-2099，超出范围会循环
     * ⚠️ 设置连续性：必须按顺序设置，不可跳过
     */
    
    PCA_Init();
    Relay_Init();
    WaveGen_Init();
    WaveGen_Start();
    UART_Init();
    I2C_Init();  
    FlowMeter_Init();
    KeyboardControl_Init();  // 初始化按键控制
    
    // 发送启动信息到串口
    UART_SendString("\r\nWatering System Started v4.0 (8-Digit Display)\r\n");
    UART_SendString("Auto Display: Time<->Date every 5 seconds\r\n");
    UART_SendString("Date Format: YYYYMMDD (8-digit full display)\r\n");
    UART_SendString("Time Format:   HHMMSS (6-digit right display)\r\n");
    UART_SendString("P3.3 Key: Long press to set date/time\r\n");
    UART_SendString("Setting order: Year->Month->Day->Hour->Min->Sec\r\n");
    
    while (1) {
        processKey();
        KeyboardControl_Scan();
        CheckAndUpdateAutoDisplay();
        FlowMeter_UpdateDisplay();
        UART_ProcessCommand();
        
        delay_ms(10);
    }
}