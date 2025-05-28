#include "wavegen.h"
#include "intrins.h"

// 方波输出引脚定义
sbit WAVE_OUT = P1^0;  // 方波输出引脚 - 模拟流量计脉冲输出，通过继电器连接到INT0

// 最小化状态变量
static bit isRunning = 0;  // 只保留一个必要的状态位

#define T0_INIT_VAL 0x4C00  // 50ms半周期，10Hz基频，软件2分频得到5Hz

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

    /* 方波发生器引脚连接说明：
     * - P1.0 (WAVE_OUT) 输出精确5Hz方波信号，模拟流量计脉冲
     * - 每个完整周期（高→低→高）为200ms，每个半周期100ms
     * - INT0设置为边沿触发，每个上升沿或下降沿产生一次中断
     * - 所以5Hz方波会产生10次中断/秒（每个周期2次中断）
     * - 但流量计算应该按完整周期计算，即5个脉冲/秒
     * - 该信号通过继电器的常开触点连接到INT0 (P3.2)
     * - 当继电器闭合（浇水开始）时，脉冲信号传导到INT0，模拟水流产生的脉冲
     * - 每个脉冲代表1毫升的水流过，5Hz频率意味着每秒5毫升的基础流量
     */
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


// T0中断服务函数 - 最简化版本
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
