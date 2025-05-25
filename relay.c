#include "relay.h"
#include "pca.h"  // 添加pca.h以使用BYTE类型定义

// 继电器控制引脚定义
sbit RELAY_CTRL = P1^1;  // 继电器控制引脚 - 连接继电器的控制端，高电平时继电器闭合
sbit RELAY_NODE1 = P1^0; // 继电器常开节点连接的第一个引脚 - 连接到方波发生器输出，模拟流量计信号输出
sbit RELAY_NODE2 = P3^2; // 继电器常开节点连接的第二个引脚 - 连接到INT0，用于捕获流量计脉冲

// 继电器控制函数实现
void Relay_Init(void) {
    RELAY_CTRL = 0;      // 初始状态下继电器关闭，即水阀关闭
    RELAY_NODE1 = 1;     // 将P1.0设为高电平（作为输入时的上拉）
    RELAY_NODE2 = 1;     // 将P3.2设为高电平（作为输入时的上拉）
    
    /* 浇水系统引脚连接说明：
     * 1. P1.1 (RELAY_CTRL) - 输出：连接继电器控制端，高电平时继电器通电，水阀打开
     * 2. P1.0 (RELAY_NODE1) - 输出：方波发生器输出，模拟流量计信号源
     * 3. P3.2 (RELAY_NODE2) - 输入：INT0外部中断输入，当继电器闭合时接收P1.0的方波信号
     * 
     * 工作原理：
     * - 当P1.1输出高电平，继电器通电，常开触点闭合
     * - 此时P1.0输出的方波信号会传导到P3.2
     * - P3.2的INT0中断会捕获这些脉冲并进行计数，实现流量计功能
     * - 当P1.1输出低电平，继电器断电，常开触点断开，信号传输中断
     */
}

void Relay_On(void) {
    RELAY_CTRL = 1;      // 开启继电器
}

void Relay_Off(void) {
    RELAY_CTRL = 0;      // 关闭继电器
}

BYTE Relay_GetState(void) {
    return RELAY_CTRL;   // 返回继电器当前状态
}
