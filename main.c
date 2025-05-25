#include <reg51.h>
#include <INTRINS.H>

#define multiplier 1.085

// 显示相关定义
#define DISP_PORT P2  // 八位数码管连接端口

// 引脚定义
sbit DATA = DISP_PORT^0;  // 串行数据输入
sbit SCK  = DISP_PORT^1;  // 移位时钟
sbit RCK  = DISP_PORT^2;  // 存储时钟
sbit OE   = DISP_PORT^3;  // 输出使能(低有效)

/* 共阴极数码管段码定义 */
#define SEG_OFF 0x00      // 字段全灭


// 替换原来的字符映射表为LED数组形式
static code const unsigned char LED[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};

// 更新显示缓冲区初始化
unsigned char dispbuff[8] = {SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF,SEG_OFF}; // 显示缓冲区

// 时钟变量
unsigned char hour = 23, minute = 59, second = 55; // 初始时间
unsigned int t0_count = 0; // 50ms计数

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 120; j++);
}

void SendTo595(unsigned char data_seg, unsigned char data_bit) {
    unsigned char i;
    OE = 1;
    
    for(i = 0; i < 8; i++) {
        DATA = (data_bit & 0x80) ? 1 : 0;
        SCK = 0; SCK = 1;
        data_bit <<= 1;
    }
    
    for(i = 0; i < 8; i++) {
        DATA = (data_seg & 0x80) ? 1 : 0;
        SCK = 0; SCK = 1;
        data_seg <<= 1;
    }
    
    RCK = 0; RCK = 1;
    OE = 0;
    delay_ms(1);
}

void display_from_right(unsigned char target[]) {
    unsigned char i;
    static unsigned char mark = 0x01;
    
    for(i = 0; i < 8; i++) {
        SendTo595(target[i], ~(mark << i));
    }
    SendTo595(SEG_OFF, 0xFF);
    mark = 0x01;
}

void Resetdispbuff() {
    unsigned char i;
    for(i = 0; i < 8; i++) dispbuff[i] = SEG_OFF;
}

void FillTimeBuff() {
    Resetdispbuff();
    // 秒部分（右起0-1位）
    dispbuff[0] = LED[second % 10]; // 秒个位
    dispbuff[1] = LED[second / 10]; // 秒十位
    
    // 分钟部分（右起2-3位）
    dispbuff[2] = LED[minute % 10];
    dispbuff[3] = LED[minute / 10];
    
    // 小时部分（右起4-5位）
    dispbuff[4] = LED[hour % 10];
    dispbuff[5] = LED[hour / 10];
}

void Timer0_Init() {
    TMOD &= 0xF0;     // 清除T0配置
    TMOD |= 0x01;     // 定时器0，模式1（16位）
    TH0 = 0x4B;       // 50ms初值
    TL0 = 0xFD;
    ET0 = 1;          // 允许中断
    TR0 = 1;          // 启动定时器
}

void main(void) {
    EA = 1;           // 开启总中断
    Timer0_Init();     // 初始化定时器
    P0 = 0xFF;
    
    while (1) {
        FillTimeBuff();          // 更新时间显示缓冲区
        display_from_right(dispbuff); // 刷新显示
    }
}

// 定时器0中断服务函数
void Timer0_ISR() interrupt 1 {
    TH0 = 0x4B;       // 重装初值
    TL0 = 0xFD;
    t0_count++;
    
    if(t0_count >= 20) { // 1秒到
        t0_count = 0;
        second++;
        
        if(second >= 60) {
            second = 0;
            minute++;
            if(minute >= 60) {
                minute = 0;
                hour++;
                if(hour >= 24) hour = 0;
            }
        }
    }
}