#include "reg51.h"
#include "intrins.h"
#include "pca.h"     // 包含头文件
#include "flowmeter.h" // 添加flowmeter.h以获取FLOW_MODE_OFF定义

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
WORD value;
WORD value1;



SYS_PARAMS SysPara1 = {0, 0, 0};    // 初始化系统参数

// 显示相关引脚定义
sbit DATA = DISP_PORT^0;  // 串行数据输入
sbit SCK  = DISP_PORT^1;  // 移位时钟
sbit RCK  = DISP_PORT^2;  // 存储时钟
sbit OE   = DISP_PORT^3;  // 输出使能(低有效)

#define CURRENTFLOW_MODE 0x39 // 当前流量模式
#define TOTALFLOW_MODE 0X71   // 累计流量模式

/* 共阴极数码管段码定义 */
static code const unsigned char LED[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,CURRENTFLOW_MODE,TOTALFLOW_MODE};

// 显示缓冲区
unsigned char dispbuff[8] = {SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF};

// 实现显示相关函数
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 120; j++);
}

// void SendTo595(unsigned char data_seg, unsigned char data_bit) {
//     unsigned char i;
//     OE = 1;
    
//     for(i = 0; i < 8; i++) {
//         DATA = (data_bit & 0x80) ? 1 : 0;
//         SCK = 0; SCK = 1;
//         data_bit <<= 1;
//     }
    
//     for(i = 0; i < 8; i++) {
//         DATA = (data_seg & 0x80) ? 1 : 0;
//         SCK = 0; SCK = 1;
//         data_seg <<= 1;
//     }
    
//     RCK = 0; RCK = 1;
//     OE = 0;
//     delay_ms(1);
// }

void Resetdispbuff() {
    unsigned char i;
    for(i = 0; i < 8; i++) dispbuff[i] = SEG_OFF;
}

void FillDispBuf(BYTE hour, BYTE min, BYTE sec) {
    Resetdispbuff();
    // 秒部分（右起0-1位）
    dispbuff[0] = LED[sec % 10]; // 秒个位
    dispbuff[1] = LED[sec / 10]; // 秒十位
    
    // 分钟部分（右起2-3位）
    dispbuff[2] = LED[min % 10];
    dispbuff[3] = LED[min / 10];
    
    // 小时部分（右起4-5位）
    dispbuff[4] = LED[hour % 10];
    dispbuff[5] = LED[hour / 10];
}

// 自定义显示缓冲区填充函数
void FillCustomDispBuf(BYTE val1, BYTE val2, BYTE val3, BYTE val4, BYTE val5, BYTE val6) {
    Resetdispbuff();
    
    // 填充6个数字位
    dispbuff[0] = LED[val1];
    dispbuff[1] = LED[val2];
    dispbuff[2] = LED[val3];
    dispbuff[3] = LED[val4];
    dispbuff[4] = LED[val5];
    dispbuff[5] = LED[val6];
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
static BYTE timeEditMode = 0;  // 0: 正常显示, 1: 编辑小时, 2: 编辑分钟, 3: 编辑秒
static BYTE blinkState = 0;    // 闪烁状态: 0 显示, 1 不显示

// 设置时间编辑模式
void PCA_SetTimeEditMode(BYTE position) {
    timeEditMode = position;
    blinkState = 0;  // 开始时处于显示状态
}

// 退出时间编辑模式
void PCA_ExitTimeEditMode(void) {
    timeEditMode = 0;
    FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
}

// 增加时间值
void PCA_IncreaseTimeValue(BYTE position) {
    switch (position) {
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
    FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
}

// 修改PCA中断服务函数，加入闪烁处理逻辑
void PCA_isr() interrupt 7
{
    if(CCF1){
        CCF1 = 0;                       // Clear interrupt flag
        CCAP1L = value1;
        CCAP1H = value1 >> 8;           // Update compare value
        value1 += T1000Hz;
        disp();                         // 刷新显示
    }

    if(CCF0){
        CCF0 = 0;                       // Clear interrupt flag
        CCAP0L = value;
        CCAP0H = value >> 8;            // Update compare value
        value += T100Hz;
        cnt++;
        
        if(cnt >= 100) {                // Count 100 times
            cnt = 0;                    // 1秒计时
            PCA_LED = !PCA_LED;         // 闪烁LED指示灯
            
            // 调用流量计计算函数（不更新显示），改为更频繁调用
            FlowMeter_CalcFlow();
            
            // 更新时钟，但不直接更新显示
            SysPara1.sec++;
            if(SysPara1.sec >= 60) {
                SysPara1.sec = 0;
                SysPara1.min++;
                if(SysPara1.min >= 60) {
                    SysPara1.min = 0;
                    SysPara1.hour++;
                    if(SysPara1.hour >= 24)
                        SysPara1.hour = 0;
                }
            }
            
            // 时间编辑模式闪烁控制 (每秒切换一次)
            if (timeEditMode > 0) {
                blinkState = !blinkState;
                FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
                
                // 根据编辑模式和闪烁状态，设置相应位为不显示
                if (blinkState) {
                    switch (timeEditMode) {
                        case HOUR_POS:  // 编辑小时
                            dispbuff[4] = SEG_OFF;
                            dispbuff[5] = SEG_OFF;
                            break;
                        case MIN_POS:  // 编辑分钟
                            dispbuff[2] = SEG_OFF;
                            dispbuff[3] = SEG_OFF;
                            break;
                        case SEC_POS:  // 编辑秒
                            dispbuff[0] = SEG_OFF;
                            dispbuff[1] = SEG_OFF;
                            break;
                    }
                }
            }
            
            // 只有在流量计模式为关闭且不在时间编辑模式时才更新时钟显示的缓冲区
            else if (FlowMeter_GetMode() == FLOW_MODE_OFF) {
                FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
            }
        }
        else if(cnt % 20 == 0 && FlowMeter_GetMode() != FLOW_MODE_OFF) {
            // 每0.2秒也调用一次流量计算，以减少刷新延迟
            FlowMeter_CalcFlow();
        }
        else if(cnt % 50 == 0 && timeEditMode > 0) {
            // 在时间编辑模式下，每0.5秒切换一次闪烁状态
            blinkState = !blinkState;
            FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
            
            // 根据编辑模式和闪烁状态，设置相应位为不显示
            if (blinkState) {
                switch (timeEditMode) {
                    case HOUR_POS:  // 编辑小时
                        dispbuff[4] = SEG_OFF;
                        dispbuff[5] = SEG_OFF;
                        break;
                    case MIN_POS:  // 编辑分钟
                        dispbuff[2] = SEG_OFF;
                        dispbuff[3] = SEG_OFF;
                        break;
                    case SEC_POS:  // 编辑秒
                        dispbuff[0] = SEG_OFF;
                        dispbuff[1] = SEG_OFF;
                        break;
                }
            }
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

    // 初始化时钟显示
    FillDispBuf(SysPara1.hour, SysPara1.min, SysPara1.sec);
}

