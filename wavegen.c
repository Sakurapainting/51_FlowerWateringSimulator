#include "wavegen.h"
#include "intrins.h"

// 方波输出引脚定义
sbit WAVE_OUT = P1^0;  // 方波输出引脚 - 模拟流量计脉冲输出，通过继电器连接到INT0

// 标记方波发生器状态
static bit isRunning = 0;

// 计算T0初值
// 方波周期 = 1/5Hz = 200ms
// 半个周期 = 100ms, 这是高/低电平的持续时间
// 11.0592MHz晶振，12分频，每毫秒大约是921.6个机器周期
// 100ms需要92160个机器周期
// 65536 - 92160 = -26624 (需要转为16位无符号数)
// 所以初值设为65536 - 92160 = 39376 = 0x99C0
#define T0_INIT_VAL 0x99C0

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
     * - P1.0 (WAVE_OUT) 输出5Hz方波信号，模拟流量计脉冲
     * - 该信号通过继电器的常开触点连接到INT0 (P3.2)
     * - 当继电器闭合（浇水开始）时，脉冲信号传导到INT0，模拟水流产生的脉冲
     * - 每个脉冲代表特定体积的水流过，根据配置可以计算流量
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

// 停止方波发生器
void WaveGen_Stop(void) {
    TR0 = 0;               // 停止T0
    WAVE_OUT = 0;          // 输出低电平
    isRunning = 0;
}

// 获取方波发生器状态
BYTE WaveGen_GetState(void) {
    return isRunning;
}

// T0中断服务函数
void T0_ISR() interrupt 1 {
    // 重新加载计数初值
    TH0 = T0_INIT_VAL >> 8;
    TL0 = T0_INIT_VAL & 0xFF;
    
    // 翻转输出电平
    WAVE_OUT = !WAVE_OUT;
}
