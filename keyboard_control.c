#include "keyboard_control.h"
#include "relay.h"
#include "flowmeter.h"
#include "i2c.h"  // 添加I2C头文件

// 定时浇水配置 - 默认值：6:00:01开始，浇100毫升
TimedWatering timed_watering = {0, 6, 0, 1, 100, 0, 0, 0};

// 显示模式：0=时钟，1=自动浇水参数
BYTE auto_display_mode = DISPLAY_MODE_CLOCK;

// 参数设置模式：0=开始小时，1=开始分钟，2=开始秒，3=浇水毫升数
BYTE param_mode = PARAM_MODE_HOUR;  // 修正：使用PARAM_MODE_HOUR而不是PARAM_MODE_INTERVAL

// 显示更新标志
bit display_update_flag = 0;

// 按键状态记录（用于消抖）
static BYTE key_prev_state = 0;

// 按键延时消抖
static void KeyDelay(void) {
    BYTE i = 30;
    while(i--);
}

// 初始化按键控制
void KeyboardControl_Init(void) {
    // 设置按键引脚为输入（上拉）
    P1 |= 0xFC;  // P1.2-P1.7设为高电平（输入模式）
    
    // 初始化I2C和24C02
    I2C_Init();
    
    // 初始化定时浇水参数为默认值
    timed_watering.enabled = 0;
    timed_watering.start_hour = 6;      // 默认6点
    timed_watering.start_min = 0;       // 0分
    timed_watering.start_sec = 1;       // 1秒开始浇水
    timed_watering.water_volume_ml = 100; // 浇水100毫升
    timed_watering.is_watering = 0;
    timed_watering.watering_volume_left = 0;
    timed_watering.triggered_today = 0;
    
    // 💡 关键修改：启动时自动从24C02加载定时浇水参数
    TimedWatering_LoadParams();
    
    auto_display_mode = DISPLAY_MODE_CLOCK;
    param_mode = PARAM_MODE_HOUR;
}

/*
 * ========================================
 * 定时定量浇花功能详细使用说明
 * ========================================
 * 
 * 功能概述：
 * 本系统可以设置在指定时间点自动开始浇水，并按设定的毫升数定量浇水。
 * 累计流量数据保存在24C02中，掉电不丢失。
 * 
 * 【第一步：参数设置】
 * ==================
 * 
 * 1. 设置开始浇水时间：
 *    操作：按P1.7 (KEY_MODE) 切换到时间设置模式
 *    显示：数码管右侧显示模式标识符
 *    
 *    a) 设置小时：
 *       - 数码管显示格式："HH0003" (例如："060003"表示6点)
 *       - 按P1.3 (KEY_TIME_UP) 增加小时 (0-23)
 *       - 按P1.4 (KEY_TIME_DOWN) 减少小时
 *       - 右侧显示"03"表示当前在设置小时
 *    
 *    b) 设置分钟：
 *       - 再按P1.7切换到分钟设置
 *       - 数码管显示格式："MM0002" (例如："000002"表示0分)
 *       - 按P1.3/P1.4调节分钟数 (0-59)
 *       - 右侧显示"02"表示当前在设置分钟
 *    
 *    c) 设置秒：
 *       - 再按P1.7切换到秒设置
 *       - 数码管显示格式："SS0001" (例如："010001"表示1秒)
 *       - 按P1.3/P1.4调节秒数 (0-59)
 *       - 右侧显示"01"表示当前在设置秒
 * 
 * 2. 设置浇水量：
 *    - 再按P1.7切换到毫升设置
 *    - 数码管显示格式："MMMM05" (例如："010005"表示100毫升)
 *    - 按P1.3 (KEY_TIME_UP) 增加毫升数，每次+50ml
 *    - 按P1.4 (KEY_TIME_DOWN) 减少毫升数，每次-50ml
 *    - 范围：50-9999毫升
 *    - 右侧显示"05"表示当前在设置毫升数
 * 
 * 【第二步：启动定时浇水】
 * ====================
 * 
 * 操作：按P1.2 (KEY_AUTO) 启动定时浇水功能
 * 结果：系统进入定时浇水模式，等待设定时间到达
 * 显示：数码管显示当前设置的参数并闪烁
 * 
 * 【第三步：系统自动运行】
 * ====================
 * 
 * 1. 等待阶段：
 *    - 系统持续监控当前时间
 *    - 数码管轮流显示设置的参数
 *    - 当到达设定时间点时，自动开始浇水
 * 
 * 2. 浇水阶段：
 *    - 继电器自动闭合，开始浇水
 *    - 流量计开始计数脉冲
 *    - 数码管显示剩余毫升数："XXXX05"
 *    - 每个脉冲代表1毫升水流
 * 
 * 3. 停止阶段：
 *    - 当累计流量达到设定毫升数时
 *    - 继电器自动断开，停止浇水
 *    - 标记今天已完成浇水，明天同一时间再次触发
 * 
 * 【第四步：数据保存】
 * ==================
 * 
 * - 累计流量每10秒自动保存到24C02
 * - 断电重启后累计流量数据不丢失
 * - 定时设置保存在内存中，断电后需重新设置
 * 
 * 【使用示例】
 * ============
 * 
 * 目标：设置每天早上6:00:01自动浇水100毫升
 * 
 * 操作步骤：
 * 1. 按P1.7，显示"060003" → 用P1.3/P1.4设置为6点
 * 2. 按P1.7，显示"000002" → 用P1.3/P1.4设置为0分
 * 3. 按P1.7，显示"010001" → 用P1.3/P1.4设置为1秒
 * 4. 按P1.7，显示"010005" → 用P1.3/P1.4设置为100毫升
 * 5. 按P1.2启动定时浇水
 * 6. 系统将在每天6:00:01自动浇水100毫升
 * 
 * 【停止定时浇水】
 * ==============
 * 
 * 操作：再次按P1.2 (KEY_AUTO)
 * 结果：停止定时浇水功能，返回时钟显示模式
 * 
 * 【注意事项】
 * ============
 * 
 * 1. 每天只触发一次，避免重复浇水
 * 2. 如果当天已经浇过水，不会再次触发
 * 3. 过了午夜(00:00:00)会重置触发标志
 * 4. 手动浇水不影响定时浇水功能
 * 5. 定时浇水进行中时，手动按键无效
 */

// 按键扫描
void KeyboardControl_Scan(void) {
    BYTE current_keys = 0;
    BYTE key_pressed;
    bit param_changed = 0;  // 参数修改标志
    
    // 读取当前按键状态
    if(KEY_AUTO == 0) current_keys |= 0x01;
    if(KEY_TIME_UP == 0) current_keys |= 0x02;
    if(KEY_TIME_DOWN == 0) current_keys |= 0x04;
    if(KEY_VOL_UP == 0) current_keys |= 0x08;
    if(KEY_VOL_DOWN == 0) current_keys |= 0x10;
    if(KEY_MODE == 0) current_keys |= 0x20;
    
    // 检测按键按下（下降沿）
    key_pressed = (~key_prev_state) & current_keys;
    
    if(key_pressed & 0x01) {  // KEY_AUTO按下
        KeyDelay();
        if(KEY_AUTO == 0) {
            if(timed_watering.enabled) {
                TimedWatering_Stop();
                auto_display_mode = DISPLAY_MODE_CLOCK;
                FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
                display_update_flag = 1;
            } else {
                TimedWatering_Start();
            }
        }
    }
    
    if(key_pressed & 0x20) {  // KEY_MODE按下
        KeyDelay();
        if(KEY_MODE == 0) {
            param_mode = (param_mode + 1) % 4;  // 4个参数模式
            auto_display_mode = DISPLAY_MODE_AUTO;
            display_update_flag = 1;
        }
    }
    
    if(key_pressed & 0x02) {  // KEY_TIME_UP按下
        KeyDelay();
        if(KEY_TIME_UP == 0) {
            switch(param_mode) {
                case PARAM_MODE_HOUR:
                    timed_watering.start_hour = (timed_watering.start_hour + 1) % 24;
                    param_changed = 1;
                    break;
                case PARAM_MODE_MIN:
                    timed_watering.start_min = (timed_watering.start_min + 1) % 60;
                    param_changed = 1;
                    break;
                case PARAM_MODE_SEC:
                    timed_watering.start_sec = (timed_watering.start_sec + 1) % 60;
                    param_changed = 1;
                    break;
                case PARAM_MODE_VOLUME:
                    if(timed_watering.water_volume_ml < 9950) {
                        timed_watering.water_volume_ml += 50;
                        param_changed = 1;
                    }
                    break;
            }
            auto_display_mode = DISPLAY_MODE_AUTO;
            display_update_flag = 1;
        }
    }
    
    if(key_pressed & 0x04) {  // KEY_TIME_DOWN按下
        KeyDelay();
        if(KEY_TIME_DOWN == 0) {
            switch(param_mode) {
                case PARAM_MODE_HOUR:
                    timed_watering.start_hour = (timed_watering.start_hour == 0) ? 23 : (timed_watering.start_hour - 1);
                    param_changed = 1;
                    break;
                case PARAM_MODE_MIN:
                    timed_watering.start_min = (timed_watering.start_min == 0) ? 59 : (timed_watering.start_min - 1);
                    param_changed = 1;
                    break;
                case PARAM_MODE_SEC:
                    timed_watering.start_sec = (timed_watering.start_sec == 0) ? 59 : (timed_watering.start_sec - 1);
                    param_changed = 1;
                    break;
                case PARAM_MODE_VOLUME:
                    if(timed_watering.water_volume_ml > 50) {
                        timed_watering.water_volume_ml -= 50;
                        param_changed = 1;
                    }
                    break;
            }
            auto_display_mode = DISPLAY_MODE_AUTO;
            display_update_flag = 1;
        }
    }
    
    // 💡 关键新增：参数修改后立即保存
    if(param_changed) {
        TimedWatering_SaveParams();
    }
    
    key_prev_state = current_keys;
}

// 启动定时浇水
void TimedWatering_Start(void) {
    timed_watering.enabled = 1;
    timed_watering.is_watering = 0;
    timed_watering.triggered_today = 0;  // 重置触发标志
    
    // 💡 立即保存参数到24C02
    TimedWatering_SaveParams();
    
    // 启动后立即返回时钟显示模式，而不是显示参数
    auto_display_mode = DISPLAY_MODE_CLOCK;
    display_update_flag = 1;  // 设置标志立即更新显示
    
    // 强制更新时钟显示
    FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
}

// 停止定时浇水
void TimedWatering_Stop(void) {
    timed_watering.enabled = 0;
    timed_watering.triggered_today = 0;
    
    // 如果正在浇水，立即停止
    if(timed_watering.is_watering) {
        timed_watering.is_watering = 0;
        Relay_Off();
        FlowMeter_Stop();
        FlowMeter_SetMode(FLOW_MODE_OFF);
    }
    
    // 💡 立即保存参数到24C02
    TimedWatering_SaveParams();
}

// 更新定时浇水状态（每秒调用一次）
void TimedWatering_Update(void) {
    unsigned long current_total_flow;
    static unsigned long start_total_flow = 0;
    unsigned long watered_volume;
    
    if(!timed_watering.enabled) return;
    
    if(timed_watering.is_watering) {
        // 正在浇水，检查累计流量是否达到目标
        current_total_flow = FlowMeter_GetTotalFlow();
        watered_volume = current_total_flow - start_total_flow;
        
        if(watered_volume >= timed_watering.water_volume_ml) {
            // 达到目标毫升数，停止浇水
            timed_watering.is_watering = 0;
            timed_watering.triggered_today = 1;  // 标记今天已触发
            
            Relay_Off();
            FlowMeter_Stop();
            FlowMeter_SetMode(FLOW_MODE_OFF);
            
            // 保存累计流量到24C02
            AT24C02_WriteTotalFlow(current_total_flow);
            
            // 浇水完成后返回时钟显示
            auto_display_mode = DISPLAY_MODE_CLOCK;
            FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
            display_update_flag = 1;
        } else {
            // 更新剩余毫升数显示
            timed_watering.watering_volume_left = timed_watering.water_volume_ml - watered_volume;
            if(auto_display_mode != DISPLAY_MODE_AUTO) {
                auto_display_mode = DISPLAY_MODE_AUTO;
                display_update_flag = 1;
            }
        }
    } else {
        // 💡 核心逻辑：每天检查是否到达设定时间点
        if(!timed_watering.triggered_today &&
           SysPara1.hour == timed_watering.start_hour &&
           SysPara1.min == timed_watering.start_min &&
           SysPara1.sec == timed_watering.start_sec) {
            
            // 开始浇水 - 每天在设定时间自动触发
            timed_watering.is_watering = 1;
            timed_watering.watering_volume_left = timed_watering.water_volume_ml;
            start_total_flow = FlowMeter_GetTotalFlow();
            
            Relay_On();
            FlowMeter_Start();
            FlowMeter_SetMode(FLOW_MODE_CURR);
            
            auto_display_mode = DISPLAY_MODE_AUTO;
            display_update_flag = 1;
        }
        
        // 💡 关键机制：午夜重置，确保每天都能触发
        // 当时钟走到00:00:00时，重置今日触发标志
        // 这样明天同一时间又可以触发浇水了
        if(timed_watering.triggered_today && 
           SysPara1.hour == 0 && SysPara1.min == 0 && SysPara1.sec == 0) {
            timed_watering.triggered_today = 0;  // 重置标志，准备明天的触发
        }
    }
}

// 显示自动浇水参数
void DisplayAutoWateringParams(void) {
    BYTE val1, val2, val3, val4, val5, val6;
    
    if(timed_watering.is_watering) {
        // 显示剩余毫升数
        val1 = timed_watering.watering_volume_left % 10;
        val2 = (timed_watering.watering_volume_left / 10) % 10;
        val3 = (timed_watering.watering_volume_left / 100) % 10;
        val4 = (timed_watering.watering_volume_left / 1000) % 10;
        val5 = 0;  // 显示0
        val6 = 5;  // 显示5（剩余毫升标识）
    } else {
        // 移除闪烁显示，直接显示当前参数
        switch(param_mode) {
            case PARAM_MODE_HOUR:
                // 显示开始小时 "小时数03"
                val1 = timed_watering.start_hour % 10;
                val2 = (timed_watering.start_hour / 10) % 10;
                val3 = 0;
                val4 = 0;
                val5 = 0;
                val6 = 3;  // 显示3（小时标识）
                break;
                
            case PARAM_MODE_MIN:
                // 显示开始分钟 "分钟数02"
                val1 = timed_watering.start_min % 10;
                val2 = (timed_watering.start_min / 10) % 10;
                val3 = 0;
                val4 = 0;
                val5 = 0;
                val6 = 2;  // 显示2（分钟标识）
                break;
                
            case PARAM_MODE_SEC:
                // 显示开始秒 "秒数01"
                val1 = timed_watering.start_sec % 10;
                val2 = (timed_watering.start_sec / 10) % 10;
                val3 = 0;
                val4 = 0;
                val5 = 0;
                val6 = 1;  // 显示1（秒标识）
                break;
                
            case PARAM_MODE_VOLUME:
                // 显示浇水毫升数 "毫升数05"
                val1 = timed_watering.water_volume_ml % 10;
                val2 = (timed_watering.water_volume_ml / 10) % 10;
                val3 = (timed_watering.water_volume_ml / 100) % 10;
                val4 = (timed_watering.water_volume_ml / 1000) % 10;
                val5 = 0;
                val6 = 5;  // 显示5（毫升标识）
                break;
                
            default:
                val1 = val2 = val3 = val4 = val5 = val6 = 0;
                break;
        }
        
        // 移除闪烁逻辑，始终显示参数值
        // if(display_toggle >= 4) {
        //     val1 = val2 = val3 = val4 = 0;  // 数值部分熄灭
        // }
    }
    
    FillCustomDispBuf(val1, val2, val3, val4, val5, val6);
}

// 添加缺失的函数：检查并更新自动显示
void CheckAndUpdateAutoDisplay(void) {
    if(display_update_flag) {
        display_update_flag = 0;  // 清除标志
        
        if(auto_display_mode == DISPLAY_MODE_AUTO) {
            DisplayAutoWateringParams();
        }
    }
}

// 保存定时浇水参数到24C02
void TimedWatering_SaveParams(void) {
    AT24C02_WriteTimedWateringParams(&timed_watering);
}

// 从24C02加载定时浇水参数
void TimedWatering_LoadParams(void) {
    AT24C02_ReadTimedWateringParams(&timed_watering);
    
    // 💡 关键修改：保持加载的enabled状态，但重置运行时状态
    // 如果之前启用了定时浇水，加载后依然保持启用
    // timed_watering.enabled = timed_watering.enabled;  // 保持原值
    
    // 重置运行时状态
    timed_watering.is_watering = 0;
    timed_watering.watering_volume_left = 0;
    
    // 💡 重要：检查是否需要重置今日触发标志
    // 如果系统重启，应该允许今天再次触发（除非今天已经过了设定时间）
    if(timed_watering.enabled) {
        // 如果当前时间已经超过了设定时间，标记为今日已触发
        if(SysPara1.hour > timed_watering.start_hour || 
           (SysPara1.hour == timed_watering.start_hour && SysPara1.min > timed_watering.start_min) ||
           (SysPara1.hour == timed_watering.start_hour && SysPara1.min == timed_watering.start_min && SysPara1.sec > timed_watering.start_sec)) {
            timed_watering.triggered_today = 1;  // 今天的浇水时间已过
        } else {
            timed_watering.triggered_today = 0;  // 今天的浇水时间还没到
        }
    }
}
