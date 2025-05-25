#include "relay.h"
#include "pca.h"  // 添加pca.h以使用BYTE类型定义

// 继电器控制引脚定义
sbit RELAY_CTRL = P1^1;  // 继电器控制引脚
sbit RELAY_NODE1 = P1^0; // 继电器常开节点连接的第一个引脚
sbit RELAY_NODE2 = P3^2; // 继电器常开节点连接的第二个引脚

// 继电器控制函数实现
void Relay_Init(void) {
    RELAY_CTRL = 0;      // 初始状态下继电器关闭
    RELAY_NODE1 = 1;     // 将P1.0设为高电平（作为输入时的上拉）
    RELAY_NODE2 = 1;     // 将P3.2设为高电平（作为输入时的上拉）
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
