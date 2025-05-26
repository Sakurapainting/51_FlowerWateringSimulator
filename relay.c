#include "relay.h"
#include "pca.h"  // 添加pca.h以使用BYTE类型定义

// 继电器控制引脚定义
sbit RELAY_CTRL = P1^1;  // 继电器控制引脚 - 低电平时继电器闭合
sbit RELAY_NODE1 = P1^0; // 继电器常开节点连接的第一个引脚 - 连接到方波发生器输出，模拟流量计信号输出
sbit RELAY_NODE2 = P3^2; // 继电器常开节点连接的第二个引脚 - 连接到INT0，用于捕获流量计脉冲

// 继电器控制函数实现
void Relay_Init(void) {
    RELAY_CTRL = 1;      // 初始状态下继电器断开（高电平），即水阀关闭
    RELAY_NODE1 = 1;     // 将P1.0设为高电平（作为输入时的上拉）
    RELAY_NODE2 = 1;     // 将P3.2设为高电平（作为输入时的上拉）
    
    /* 低电平触发说明：
     * - P1.1 (RELAY_CTRL) 输出低电平时继电器吸合，水阀打开
     * - 输出高电平时继电器断开，水阀关闭
     */
}

void Relay_On(void) {
    RELAY_CTRL = 0;      // 低电平，开启继电器（吸合，水阀打开）
}

void Relay_Off(void) {
    RELAY_CTRL = 1;      // 高电平，关闭继电器（断开，水阀关闭）
}

BYTE Relay_GetState(void) {
    // 低电平为开，返回0表示开，1表示关
    return (RELAY_CTRL == 0) ? 1 : 0;
}
