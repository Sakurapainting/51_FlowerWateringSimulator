#include "reg51.h"
#include "intrins.h"
#include "pca.h"     // 包含头文件
#include "flowmeter.h" // 添加flowmeter.h以获取FLOW_MODE_OFF定义
#include "keyboard_control.h" // 添加按键控制头文件

#define FOSC    11059200L
#define T100Hz  (FOSC / 12 / 100)
#define T1000Hz (FOSC / 12 / 1000)

// typedef unsigned char BYTE;
// typedef unsigned int WORD;

/*Declare SFR associated with the PCA */
sfr CCON        =   0xD8;           //PCA control register
sbit CCF0       =   CCON^0;         //PCA module-0 interrupt flag
sbit CCF1       =   CCON^1;         //PCA module-1 interrupt flag
sbit CR         =   CCON^6;         //PCA timer run control bit
sbit CF         =   CCON^7;         //PCA timer overflow flag
sfr CMOD        =   0xD9;           //PCA mode register
sfr CL          =   0xE9;           //PCA base timer LOW
sfr CH          =   0xF9;           //PCA base timer HIGH
sfr CCAPM0      =   0xDA;           //PCA module-0 mode register
sfr CCAP0L      =   0xEA;           //PCA module-0 capture register LOW
sfr CCAP0H      =   0xFA;           //PCA module-0 capture register HIGH
sfr CCAPM1      =   0xDB;           //PCA module-1 mode registers
sfr CCAP1L      =   0xEB;           //PCA module-1 capture register LOW
sfr CCAP1H      =   0xFB;           //PCA module-1 capture register HIGH
sfr PCAPWM0     =   0xf2;
sfr PCAPWM1     =   0xf3;

sbit PCA_LED    =   P1^0;           //PCA test LED

BYTE cnt;
WORD xdata value;
WORD xdata value1;

static bit time_update_flag = 0;
static bit display_update_needed = 0;
static bit watering_check_needed = 0;
static bit blink_update_needed = 0;

// 支持年月日的系统参数 - 初始化为2025年1月1日 00:00:00
SYS_PARAMS SysPara1 = {2025, 1, 1, 0, 0, 0};

// 显示相关引脚定义
sbit DATA = DISP_PORT^0;  // 串行数据输入
sbit SCK  = DISP_PORT^1;  // 移位时钟
sbit RCK  = DISP_PORT^2;  // 存储时钟
sbit OE   = DISP_PORT^3;  // 输出使能(低有效)

#define CURRENTFLOW_MODE 0x39 // 当前流量模式
#define TOTALFLOW_MODE 0X71   // 累计流量模式

/* 共阴极数码管段码定义 - 添加横线显示 */
static code const unsigned char LED[] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F,  // 9
    CURRENTFLOW_MODE,  // 10 - 当前流量模式
    TOTALFLOW_MODE     // 11 - 累计流量模式
};

// 显示缓冲区
unsigned char xdata dispbuff[8] = {SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF};

// 日期时间显示模式
BYTE datetime_display_mode = DISPLAY_TIME_MODE;  // 默认显示时间

// 实现显示相关函数
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 120; j++);
}

void Resetdispbuff() {
    unsigned char i;
    for(i = 0; i < 8; i++) dispbuff[i] = SEG_OFF;
}

void FillDispBuf(BYTE hour, BYTE min, BYTE sec) {
    Resetdispbuff();
    
    // 秒部分（右起0-1位）
    dispbuff[0] = LED[sec % 10];   // 秒个位
    dispbuff[1] = LED[sec / 10];   // 秒十位
    
    // 第一个横线（右起第2位）
    dispbuff[2] = 0x40;  // 显示横线 "-"
    
    // 分钟部分（右起3-4位）
    dispbuff[3] = LED[min % 10];   // 分个位
    dispbuff[4] = LED[min / 10];   // 分十位
    
    // 第二个横线（右起第5位）
    dispbuff[5] = 0x40;  // 显示横线 "-"
    
    // 小时部分（右起6-7位）
    dispbuff[6] = LED[hour % 10];  // 时个位
    dispbuff[7] = LED[hour / 10];  // 时十位
}

// 修改：填充日期显示缓冲区 (YYYYMMDD格式，使用全部8位数码管)
void FillDateBuf(WORD year, BYTE month, BYTE day) {
    Resetdispbuff();
    
    // 日期部分（右起0-1位）
    dispbuff[0] = LED[day % 10];   // 日个位
    dispbuff[1] = LED[day / 10];   // 日十位
    
    // 月份部分（右起2-3位）
    dispbuff[2] = LED[month % 10]; // 月个位
    dispbuff[3] = LED[month / 10]; // 月十位
    
    // 年份部分（右起4-7位，显示完整4位年份）
    dispbuff[4] = LED[year % 10];           // 年个位
    dispbuff[5] = LED[(year / 10) % 10];    // 年十位
    dispbuff[6] = LED[(year / 100) % 10];   // 年百位
    dispbuff[7] = LED[(year / 1000) % 10];  // 年千位
}

// 自定义显示缓冲区填充函数
void FillCustomDispBuf(BYTE val1, BYTE val2, BYTE val3, BYTE val4, BYTE val5, BYTE val6) {
    Resetdispbuff();
    
    // 填充6个数字位（右侧6位）
    dispbuff[0] = LED[val1];
    dispbuff[1] = LED[val2];
    dispbuff[2] = LED[val3];
    dispbuff[3] = LED[val4];
    dispbuff[4] = LED[val5];
    dispbuff[5] = LED[val6];
    
    // 左侧2位关闭
    dispbuff[6] = SEG_OFF;
    dispbuff[7] = SEG_OFF;
}

void disp(void) {
    unsigned char i;
    static unsigned char pos = 0;  // 当前扫描的位置
    static unsigned char mark = 0x01;  // 位选掩码
    unsigned char tmpdata;
    
    // 设置输出使能为高，准备数据传输
    OE = 1;
    
    // 准备位选数据（低电平有效）
    tmpdata = ~mark;
    
    // 发送位选数据到74HC595
    for(i = 0; i < 8; i++) {
        // 从最高位开始发送
        DATA = (tmpdata & 0x80) ? 1 : 0;
        SCK = 0;
        SCK = 1;
        tmpdata <<= 1;
    }
    
    // 发送段码数据到74HC595
    tmpdata = dispbuff[pos];
    for(i = 0; i < 8; i++) {
        // 从最高位开始发送
        DATA = (tmpdata & 0x80) ? 1 : 0;
        SCK = 0;
        SCK = 1;
        tmpdata <<= 1;
    }
    
    // 锁存数据并输出
    RCK = 0;
    RCK = 1;
    
    // 使能输出
    OE = 0;
    
    // 移动到下一位
    pos = (pos + 1) % 8;
    mark = (mark << 1) | (mark >> 7);  // 循环左移
    if(mark == 0) mark = 0x01;  // 确保非零
}

// 时间编辑相关变量
static BYTE timeEditMode = 0;  // 0: 正常显示, 1-6: 编辑年月日时分秒
static BYTE blinkState = 0;    // 闪烁状态: 0 显示, 1 不显示

// 新增：自动轮换显示相关变量
static BYTE xdata autoToggleCounter = 0;  // 自动切换计数器
#define AUTO_TOGGLE_INTERVAL 5       // 每5秒切换一次显示模式

// 设置时间编辑模式
void PCA_SetTimeEditMode(BYTE position) {
    timeEditMode = position;
    blinkState = 0;  // 开始时处于显示状态
}

// 退出时间编辑模式
void PCA_ExitTimeEditMode(void) {
    timeEditMode = 0;
    // 根据当前显示模式更新显示
    if(datetime_display_mode == DISPLAY_TIME_MODE) {
        FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
    } else {
        FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
    }
}

// 增加时间值
void PCA_IncreaseTimeValue(BYTE position) {
    switch (position) {
        case YEAR_POS:
            SysPara1.year++;
            if(SysPara1.year > 2099) SysPara1.year = 2000;  // 年份范围2000-2099
            break;
        case MONTH_POS:
            SysPara1.month++;
            if(SysPara1.month > 12) SysPara1.month = 1;
            // 检查日期是否超出当月最大天数
            if(SysPara1.day > PCA_GetDaysInMonth(SysPara1.year, SysPara1.month)) {
                SysPara1.day = PCA_GetDaysInMonth(SysPara1.year, SysPara1.month);
            }
            break;
        case DAY_POS:
            SysPara1.day++;
            if(SysPara1.day > PCA_GetDaysInMonth(SysPara1.year, SysPara1.month)) {
                SysPara1.day = 1;
            }
            break;
        case HOUR_POS:
            SysPara1.hour = (SysPara1.hour + 1) % 24;
            break;
        case MIN_POS:
            SysPara1.min = (SysPara1.min + 1) % 60;
            break;
        case SEC_POS:
            SysPara1.sec = (SysPara1.sec + 1) % 60;
            break;
    }
    
    // 更新显示
    if(datetime_display_mode == DISPLAY_TIME_MODE) {
        FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
    } else {
        FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
    }
}

// 设置时分秒
void PCA_SetTime(BYTE hour, BYTE min, BYTE sec) {
    // 验证输入时间是否有效
    if(hour < 24 && min < 60 && sec < 60) {
        SysPara1.hour = hour;
        SysPara1.min = min;
        SysPara1.sec = sec;
        
        // 时间修改后重新计算今日触发标志
        if(timed_watering.enabled) {
            // 重新判断今天的浇水时间是否已过
            if(SysPara1.hour > timed_watering.start_hour || 
               (SysPara1.hour == timed_watering.start_hour && SysPara1.min > timed_watering.start_min) ||
               (SysPara1.hour == timed_watering.start_hour && SysPara1.min == timed_watering.start_min && SysPara1.sec > timed_watering.start_sec)) {
                timed_watering.triggered_today = 1;  // 今天的浇水时间已过
            } else {
                timed_watering.triggered_today = 0;  // 今天的浇水时间还没到，可以触发
            }
        }
        
        // 如果当前是时钟显示模式，更新显示
        if(FlowMeter_GetMode() == FLOW_MODE_OFF && timeEditMode == 0) {
            if(datetime_display_mode == DISPLAY_TIME_MODE) {
                FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
            }
        }
    }
}

// 新增：设置年月日
void PCA_SetDate(WORD year, BYTE month, BYTE day) {
    // 验证输入日期是否有效
    if(year >= 2000 && year <= 2099 && month >= 1 && month <= 12 && 
       day >= 1 && day <= PCA_GetDaysInMonth(year, month)) {
        SysPara1.year = year;
        SysPara1.month = month;
        SysPara1.day = day;
        
        // 如果当前是日期显示模式，更新显示
        if(FlowMeter_GetMode() == FLOW_MODE_OFF && timeEditMode == 0) {
            if(datetime_display_mode == DISPLAY_DATE_MODE) {
                FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
            }
        }
    }
}

// 新增：获取时间相关函数
WORD PCA_GetYear(void) { return SysPara1.year; }
BYTE PCA_GetMonth(void) { return SysPara1.month; }
BYTE PCA_GetDay(void) { return SysPara1.day; }
BYTE PCA_GetHour(void) { return SysPara1.hour; }
BYTE PCA_GetMin(void) { return SysPara1.min; }
BYTE PCA_GetSec(void) { return SysPara1.sec; }

// 新增：显示模式控制函数
void PCA_SetDisplayMode(BYTE mode) {
    datetime_display_mode = mode;
    if(timeEditMode == 0) {  // 非编辑模式下立即更新显示
        if(mode == DISPLAY_TIME_MODE) {
            FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
        } else {
            FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
        }
    }
}

// 新增：日期计算辅助函数
bit PCA_IsLeapYear(WORD year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

BYTE PCA_GetDaysInMonth(WORD year, BYTE month) {
    static code BYTE daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if(month == 2 && PCA_IsLeapYear(year)) {
        return 29;  // 闰年2月有29天
    }
    return daysInMonth[month - 1];
}

// 新增：更新日期时间（处理日期跨越）
void PCA_UpdateDateTime(void) {
    // 秒进位
    SysPara1.sec++;
    if(SysPara1.sec >= 60) {
        SysPara1.sec = 0;
        
        // 分钟进位
        SysPara1.min++;
        if(SysPara1.min >= 60) {
            SysPara1.min = 0;
            
            // 小时进位
            SysPara1.hour++;
            if(SysPara1.hour >= 24) {
                SysPara1.hour = 0;
                
                // 日期进位
                SysPara1.day++;
                if(SysPara1.day > PCA_GetDaysInMonth(SysPara1.year, SysPara1.month)) {
                    SysPara1.day = 1;
                    
                    // 月份进位
                    SysPara1.month++;
                    if(SysPara1.month > 12) {
                        SysPara1.month = 1;
                        
                        // 年份进位
                        SysPara1.year++;
                        if(SysPara1.year > 2099) {
                            SysPara1.year = 2000; // 年份超限后回到2000年
                        }
                    }
                }
            }
        }
    }
}

void PCA_isr() interrupt 7
{
    if(CCF1){
        CCF1 = 0;
        CCAP1L = value1;
        CCAP1H = value1 >> 8;
        value1 += T1000Hz;
        disp();  // 只保留简单的显示扫描函数
    }

    if(CCF0){
        CCF0 = 0;
        CCAP0L = value;
        CCAP0H = value >> 8;
        value += T100Hz;
        cnt++;
        
        if(cnt >= 100) {
            cnt = 0;
            PCA_LED = !PCA_LED;
            
            // 设置标志，在主循环中处理时间更新
            time_update_flag = 1;
            
            // 时间编辑模式闪烁控制 - 只改变闪烁状态，不更新显示
            if (timeEditMode > 0) {
                blinkState = !blinkState;
                blink_update_needed = 1;  // 设置闪烁更新标志
            }
            // 正常显示模式的逻辑
            else if (FlowMeter_GetMode() == FLOW_MODE_OFF) {
                // 时钟显示模式：实现自动轮换显示
                if(auto_display_mode == DISPLAY_MODE_CLOCK) {
                    // 自动轮换计数器递增
                    autoToggleCounter++;
                    
                    // 每AUTO_TOGGLE_INTERVAL秒切换一次显示模式
                    if(autoToggleCounter >= AUTO_TOGGLE_INTERVAL) {
                        autoToggleCounter = 0;
                        datetime_display_mode = (datetime_display_mode == DISPLAY_TIME_MODE) ? 
                                               DISPLAY_DATE_MODE : DISPLAY_TIME_MODE;
                    }
                    
                    // 设置显示更新标志
                    display_update_needed = 1;
                }
                // 自动浇水参数显示模式：设置更新标志
                else if(auto_display_mode == DISPLAY_MODE_AUTO) {
                    display_update_flag = 1;  // 设置标志，在主循环中更新显示
                }
            }
        }
        else if(cnt % 50 == 0 && timeEditMode > 0) {
            // 0.5秒闪烁一次 - 只改变状态，不更新显示
            blinkState = !blinkState;
            blink_update_needed = 1;  // 设置闪烁更新标志
        }
    }
}

void PCA_Init(void)
{
    CCON = 0;                       // Initial PCA control register
                                    // PCA timer stop running
                                    // Clear CF flag
                                    // Clear all module interrupt flag
    CL = 0;                         // Reset PCA base timer
    CH = 0;
    CMOD = 0x00;                    // Set PCA timer clock source as Fosc/12
                                    // Disable PCA timer overflow interrupt
    
    // 初始化PCA模块1 (1000Hz)
    value1 = T1000Hz;
    CCAP1L = value1;
    CCAP1H = value1 >> 8;           // Initial PCA module-1
    value1 += T1000Hz;
    CCAPM1 = 0x49;                  // PCA module-1 work in 16-bit timer mode
                                    // and enable PCA interrupt
    
    // 初始化PCA模块0 (100Hz)
    value = T100Hz;
    CCAP0L = value;
    CCAP0H = value >> 8;            // Initial PCA module-0
    value += T100Hz;
    CCAPM0 = 0x49;                  // PCA module-0 work in 16-bit timer mode
                                    // and enable PCA interrupt
    
    CR = 1;                         // PCA timer start run
    EA = 1;                         // Enable global interrupt
    cnt = 0;

    // 初始化显示 - 根据默认显示模式
    datetime_display_mode = DISPLAY_TIME_MODE;  // 默认显示时间
    autoToggleCounter = 0;          // 初始化自动轮换计数器
    
    if(datetime_display_mode == DISPLAY_TIME_MODE) {
        FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
    } else {
        FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
    }
}

// 新增：重置自动轮换计数器（在进入设置模式时调用）
void PCA_ResetAutoToggle(void) {
    autoToggleCounter = 0;
}

void PCA_ProcessBlinkUpdate(void) {
    if(blink_update_needed) {
        blink_update_needed = 0;
        
        // 根据编辑的是日期还是时间来更新显示
        if(timeEditMode <= DAY_POS) {
            // 编辑日期 (年月日) - 使用完整8位显示
            FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
            if (blinkState) {
                switch (timeEditMode) {
                    case YEAR_POS:
                        // 年份闪烁 - 4位全部闪烁
                        dispbuff[4] = SEG_OFF;  // 年个位
                        dispbuff[5] = SEG_OFF;  // 年十位
                        dispbuff[6] = SEG_OFF;  // 年百位
                        dispbuff[7] = SEG_OFF;  // 年千位
                        break;
                    case MONTH_POS:
                        // 月份闪烁
                        dispbuff[2] = SEG_OFF;  // 月个位
                        dispbuff[3] = SEG_OFF;  // 月十位
                        break;
                    case DAY_POS:
                        // 日期闪烁
                        dispbuff[0] = SEG_OFF;  // 日个位
                        dispbuff[1] = SEG_OFF;  // 日十位
                        break;
                }
            }
        } else {
            // 编辑时间 (时分秒) - 现在使用全部8位显示 HH-MM-SS
            FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
            if (blinkState) {
                switch (timeEditMode) {
                    case HOUR_POS:
                        // 小时闪烁 (位置6-7)
                        dispbuff[6] = SEG_OFF;  // 小时个位
                        dispbuff[7] = SEG_OFF;  // 小时十位
                        break;
                    case MIN_POS:
                        // 分钟闪烁 (位置3-4)
                        dispbuff[3] = SEG_OFF;  // 分钟个位
                        dispbuff[4] = SEG_OFF;  // 分钟十位
                        break;
                    case SEC_POS:
                        // 秒闪烁 (位置0-1)
                        dispbuff[0] = SEG_OFF;  // 秒个位
                        dispbuff[1] = SEG_OFF;  // 秒十位
                        break;
                }
            }
        }
    }
}

void PCA_ProcessTimeUpdate(void) {
    if(time_update_flag) {
        time_update_flag = 0;
        
        // 使用新的日期时间更新函数
        PCA_UpdateDateTime();
        
        // 更新定时浇水状态 - 确保在时钟更新后立即调用
        TimedWatering_Update();
        
        // 确保每1秒调用一次流量计算
        FlowMeter_CalcFlow();  // 每秒调用一次，统计过去1秒的脉冲数
    }
}

void PCA_ProcessDisplayUpdate(void) {
    if(display_update_needed) {
        display_update_needed = 0;
        
        // 根据当前显示模式更新显示
        if(datetime_display_mode == DISPLAY_TIME_MODE) {
            FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
        } else {
            FillDateBuf(SysPara1.year, SysPara1.month, SysPara1.day);
        }
    }
}
