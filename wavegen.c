#include "wavegen.h"
#include "intrins.h"

// 方波输出引脚定义
sbit WAVE_OUT = P1^0;  // 方波输出引脚 - 模拟流量计脉冲输出，通过继电器连接到INT0

// 最小化状态变量
static bit isRunning = 0;  // 只保留一个必要的状态位

#define T0_INIT_VAL 0x4C00  // 50ms半周期，软件2分频得到5Hz

// 方波发生器初始化
void WaveGen_Init(void) {
    TMOD &= 0xF0;          // 清除T0的设置位
    TMOD |= 0x01;          // 设置T0为模式1（16位定时器）
    TH0 = T0_INIT_VAL >> 8;  // 设置T0初值高字节
    TL0 = T0_INIT_VAL & 0xFF; // 设置T0初值低字节
    ET0 = 1;               // 允许T0中断
    WAVE_OUT = 0;          // 初始输出低电平
    isRunning = 0;         // 初始状态为停止
    TR0 = 0;               // T0不开始计时
}

// 启动方波发生器
void WaveGen_Start(void) {
    if (!isRunning) {
        TH0 = T0_INIT_VAL >> 8;
        TL0 = T0_INIT_VAL & 0xFF;
        TR0 = 1;           // 启动T0
        isRunning = 1;
    }
}


// T0中断服务函数
void T0_ISR() interrupt 1 {
    static bit divider = 0;
    
    // 重新加载计数初值
    TH0 = T0_INIT_VAL >> 8;
    TL0 = T0_INIT_VAL & 0xFF;
    
    // 软件2分频
    divider = !divider;
    if(!divider) {
        WAVE_OUT = !WAVE_OUT;
    }
}
