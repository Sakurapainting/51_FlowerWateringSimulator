#include "keyboard_control.h"
#include "relay.h"
#include "flowmeter.h"
#include "i2c.h"  

// 定时浇水配置 - 默认值：6:00:01开始，浇100毫升
TimedWatering xdata timed_watering = {0, 6, 0, 1, 100, 0, 0, 0, 0};

// 手动浇水记录
WateringRecord xdata manual_watering_record;

// 显示模式：0=时钟，1=自动浇水参数
BYTE auto_display_mode = DISPLAY_MODE_CLOCK;

// 参数设置模式：0=开始小时，1=开始分钟，2=开始秒，3=浇水毫升数
BYTE param_mode = PARAM_MODE_HOUR;

// 显示更新标志
bit display_update_flag = 0;

// 按键状态记录（用于消抖）
static BYTE xdata key_prev_state = 0;

// 按键延时消抖
static void KeyDelay(void) {
    BYTE i = 30;
    while(i--);
}

// 开始手动浇水记录 - 避免传参，直接写死类型
void StartManualWateringRecord(void) {
    // 记录开始时间 - 直接写死手动类型
    manual_watering_record.type = WATERING_TYPE_MANUAL;
    manual_watering_record.start_year = PCA_GetYear();
    manual_watering_record.start_month = PCA_GetMonth();
    manual_watering_record.start_day = PCA_GetDay();
    manual_watering_record.start_hour = PCA_GetHour();
    manual_watering_record.start_min = PCA_GetMin();
    manual_watering_record.start_sec = PCA_GetSec();
    
    // 记录开始时的累计流量
    manual_watering_record.total_flow = FlowMeter_GetTotalFlow();
}

// 开始自动浇水记录 - 避免传参，直接写死类型
void StartAutoWateringRecord(void) {
    // 记录开始时间 - 直接写死自动类型
    timed_watering.current_record.type = WATERING_TYPE_AUTO;
    timed_watering.current_record.start_year = PCA_GetYear();
    timed_watering.current_record.start_month = PCA_GetMonth();
    timed_watering.current_record.start_day = PCA_GetDay();
    timed_watering.current_record.start_hour = PCA_GetHour();
    timed_watering.current_record.start_min = PCA_GetMin();
    timed_watering.current_record.start_sec = PCA_GetSec();
    
    // 记录开始时的累计流量
    timed_watering.start_total_flow = FlowMeter_GetTotalFlow();
}

// 计算手动浇水持续时间 - 内联计算，避免传参
static void CalculateManualDuration(void) {
    unsigned int start_total_sec, end_total_sec, duration;
    
    // 将开始和结束时间转换为总秒数
    start_total_sec = manual_watering_record.start_hour * 3600 + 
                     manual_watering_record.start_min * 60 + 
                     manual_watering_record.start_sec;
    end_total_sec = manual_watering_record.end_hour * 3600 + 
                   manual_watering_record.end_min * 60 + 
                   manual_watering_record.end_sec;
    
    // 处理跨日期情况
    if(end_total_sec < start_total_sec) {
        end_total_sec += 24 * 3600;  // 加上一天的秒数
    }
    
    duration = end_total_sec - start_total_sec;
    manual_watering_record.duration_min = duration / 60;
    manual_watering_record.duration_sec = duration % 60;
}

// 计算自动浇水持续时间 - 内联计算，避免传参
static void CalculateAutoDuration(void) {
    unsigned int start_total_sec, end_total_sec, duration;
    
    // 将开始和结束时间转换为总秒数
    start_total_sec = timed_watering.current_record.start_hour * 3600 + 
                     timed_watering.current_record.start_min * 60 + 
                     timed_watering.current_record.start_sec;
    end_total_sec = timed_watering.current_record.end_hour * 3600 + 
                   timed_watering.current_record.end_min * 60 + 
                   timed_watering.current_record.end_sec;
    
    // 处理跨日期情况
    if(end_total_sec < start_total_sec) {
        end_total_sec += 24 * 3600;  // 加上一天的秒数
    }
    
    duration = end_total_sec - start_total_sec;
    timed_watering.current_record.duration_min = duration / 60;
    timed_watering.current_record.duration_sec = duration % 60;
}

// 结束手动浇水记录 - 避免传参，直接访问全局变量
void EndManualWateringRecord(void) {
    unsigned long current_total_flow;
    
    // 记录结束时间
    manual_watering_record.end_year = PCA_GetYear();
    manual_watering_record.end_month = PCA_GetMonth();
    manual_watering_record.end_day = PCA_GetDay();
    manual_watering_record.end_hour = PCA_GetHour();
    manual_watering_record.end_min = PCA_GetMin();
    manual_watering_record.end_sec = PCA_GetSec();
    
    // 计算浇水量和累计流量
    current_total_flow = FlowMeter_GetTotalFlow();
    manual_watering_record.water_volume = current_total_flow - manual_watering_record.total_flow;
    manual_watering_record.total_flow = current_total_flow;
    
    // 计算持续时间
    CalculateManualDuration();
    
    // 发送浇水记录到串口
    UART_SendManualWateringRecord();
}

// 结束自动浇水记录 - 避免传参，直接访问全局变量
void EndAutoWateringRecord(void) {
    unsigned long current_total_flow;
    
    // 记录结束时间
    timed_watering.current_record.end_year = PCA_GetYear();
    timed_watering.current_record.end_month = PCA_GetMonth();
    timed_watering.current_record.end_day = PCA_GetDay();
    timed_watering.current_record.end_hour = PCA_GetHour();
    timed_watering.current_record.end_min = PCA_GetMin();
    timed_watering.current_record.end_sec = PCA_GetSec();
    
    // 计算浇水量和累计流量
    current_total_flow = FlowMeter_GetTotalFlow();
    timed_watering.current_record.water_volume = current_total_flow - timed_watering.start_total_flow;
    timed_watering.current_record.total_flow = current_total_flow;
    
    // 计算持续时间
    CalculateAutoDuration();
    
    // 发送浇水记录到串口
    UART_SendAutoWateringRecord();
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
    timed_watering.start_total_flow = 0;
    
    auto_display_mode = DISPLAY_MODE_CLOCK;
    param_mode = PARAM_MODE_HOUR;
}

// 按键扫描
void KeyboardControl_Scan(void) {
    BYTE current_keys = 0;
    BYTE key_pressed;
    
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
                    break;
                case PARAM_MODE_MIN:
                    timed_watering.start_min = (timed_watering.start_min + 1) % 60;
                    break;
                case PARAM_MODE_SEC:
                    timed_watering.start_sec = (timed_watering.start_sec + 1) % 60;
                    break;
                case PARAM_MODE_VOLUME:
                    if(timed_watering.water_volume_ml < 9950) {
                        timed_watering.water_volume_ml += 50;
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
                    break;
                case PARAM_MODE_MIN:
                    timed_watering.start_min = (timed_watering.start_min == 0) ? 59 : (timed_watering.start_min - 1);
                    break;
                case PARAM_MODE_SEC:
                    timed_watering.start_sec = (timed_watering.start_sec == 0) ? 59 : (timed_watering.start_sec - 1);
                    break;
                case PARAM_MODE_VOLUME:
                    if(timed_watering.water_volume_ml > 50) {
                        timed_watering.water_volume_ml -= 50;
                    }
                    break;
            }
            auto_display_mode = DISPLAY_MODE_AUTO;
            display_update_flag = 1;
        }
    }
    
    key_prev_state = current_keys;
}

// 启动定时浇水
void TimedWatering_Start(void) {
    timed_watering.enabled = 1;
    timed_watering.is_watering = 0;
    timed_watering.triggered_today = 0;  // 重置触发标志
    
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
    
    // 如果正在浇水，立即停止并记录
    if(timed_watering.is_watering) {
        timed_watering.is_watering = 0;
        Relay_Off();
        FlowMeter_Stop();
        FlowMeter_SetMode(FLOW_MODE_OFF);
        
        // 记录自动浇水结束
        EndAutoWateringRecord();
    }
}

// 更新定时浇水状态（每秒调用一次）
void TimedWatering_Update(void) {
    unsigned long current_total_flow;
    unsigned long watered_volume;
    
    if(!timed_watering.enabled) return;
    
    if(timed_watering.is_watering) {
        // 正在浇水，检查累计流量是否达到目标
        current_total_flow = FlowMeter_GetTotalFlow();
        watered_volume = current_total_flow - timed_watering.start_total_flow;
        
        if(watered_volume >= timed_watering.water_volume_ml) {
            // 达到目标毫升数，停止浇水
            timed_watering.is_watering = 0;
            timed_watering.triggered_today = 1;  // 标记今天已触发
            
            Relay_Off();
            FlowMeter_Stop();
            FlowMeter_SetMode(FLOW_MODE_OFF);
            
            // 保存累计流量到24C02
            AT24C02_WriteTotalFlow(current_total_flow);
            
            // 记录自动浇水结束
            EndAutoWateringRecord();
            
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
        // 每天检查是否到达设定时间点
        if(!timed_watering.triggered_today &&
           SysPara1.hour == timed_watering.start_hour &&
           SysPara1.min == timed_watering.start_min &&
           SysPara1.sec == timed_watering.start_sec) {
            
            // 开始浇水 - 每天在设定时间自动触发
            timed_watering.is_watering = 1;
            timed_watering.watering_volume_left = timed_watering.water_volume_ml;
            timed_watering.start_total_flow = FlowMeter_GetTotalFlow();
            
            // 记录自动浇水开始
            StartAutoWateringRecord();
            
            Relay_On();
            FlowMeter_Start();
            FlowMeter_SetMode(FLOW_MODE_CURR);
            
            auto_display_mode = DISPLAY_MODE_AUTO;
            display_update_flag = 1;
        }
        
        // 午夜重置，确保每天都能触发
        if(timed_watering.triggered_today && 
           SysPara1.hour == 0 && SysPara1.min == 0 && SysPara1.sec == 0) {
            timed_watering.triggered_today = 0;  // 重置标志，准备明天的触发
        }
    }
}

// 显示自动浇水参数
void DisplayAutoWateringParams(void) {
    BYTE val1, val2, val3, val4, val5, val6, val7, val8;
    
    if(timed_watering.is_watering) {
        // 显示剩余毫升数 - 格式：XXXXXXXd（前7位剩余量，最后1位标识d）
        unsigned int remaining = timed_watering.watering_volume_left;
        val1 = 12;  // 模式标识 "d" (使用LED数组索引12)
        val2 = remaining % 10;
        val3 = (remaining / 10) % 10;
        val4 = (remaining / 100) % 10;
        val5 = (remaining / 1000) % 10;
        val6 = (remaining / 10000) % 10;
        val7 = 0;  // 十万位通常为0
        val8 = 0;  // 百万位通常为0
    } else {
        // 显示当前参数设置
        switch(param_mode) {
            case PARAM_MODE_HOUR:
                // 显示开始小时 - 格式：000000Hc（前6位填零，H为小时值，最后1位标识c）
                val1 = 15;  // 模式标识 "c" (使用LED数组索引15)
                val2 = timed_watering.start_hour % 10;           // 小时个位
                val3 = (timed_watering.start_hour / 10) % 10;    // 小时十位
                val4 = val5 = val6 = val7 = val8 = 0;           // 其余位填零
                break;
                
            case PARAM_MODE_MIN:
                // 显示开始分钟 - 格式：000000MB（前6位填零，M为分钟值，最后1位标识B）
                val1 = 14;  // 模式标识 "B" (使用LED数组索引14)
                val2 = timed_watering.start_min % 10;           // 分钟个位
                val3 = (timed_watering.start_min / 10) % 10;    // 分钟十位
                val4 = val5 = val6 = val7 = val8 = 0;          // 其余位填零
                break;
                
            case PARAM_MODE_SEC:
                // 显示开始秒 - 格式：000000SA（前6位填零，S为秒值，最后1位标识A）
                val1 = 13;  // 模式标识 "A" (使用LED数组索引13)
                val2 = timed_watering.start_sec % 10;           // 秒个位
                val3 = (timed_watering.start_sec / 10) % 10;    // 秒十位
                val4 = val5 = val6 = val7 = val8 = 0;          // 其余位填零
                break;
                
            case PARAM_MODE_VOLUME:
                {  // 添加大括号创建新作用域
                    // 显示浇水毫升数 - 格式：XXXXXXXd（前7位毫升数，最后1位标识d）
                    unsigned int volume = timed_watering.water_volume_ml;
                    val1 = 12;  // 模式标识 "d" (使用LED数组索引12)
                    val2 = volume % 10;
                    val3 = (volume / 10) % 10;
                    val4 = (volume / 100) % 10;
                    val5 = (volume / 1000) % 10;
                    val6 = (volume / 10000) % 10;
                    val7 = 0;  // 十万位通常为0
                    val8 = 0;  // 百万位通常为0
                }  // 结束大括号
                break;
                
            default:
                val1 = val2 = val3 = val4 = val5 = val6 = val7 = val8 = 0;
                break;
        }
    }
    
    // 使用8位显示缓冲区
    FillCustomDispBuf8(val1, val2, val3, val4, val5, val6, val7, val8);
}

// 检查并更新自动显示
void CheckAndUpdateAutoDisplay(void) {
    if(display_update_flag) {
        display_update_flag = 0;  // 清除标志
        
        if(auto_display_mode == DISPLAY_MODE_AUTO) {
            DisplayAutoWateringParams();
        }
    }
}
