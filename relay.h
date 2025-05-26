#ifndef __RELAY_H__
#define __RELAY_H__

#include "reg51.h"

// 继电器控制引脚定义（低电平触发）
#define RELAY_PIN P1_1   // 继电器控制引脚（低电平吸合）
#define RELAY_IN1  P1_0  // 继电器常开节点连接的第一个引脚
#define RELAY_IN2  P3_2  // 继电器常开节点连接的第二个引脚

// 函数声明
void Relay_Init(void);                    // 继电器初始化
void Relay_On(void);                      // 开启继电器（低电平吸合）
void Relay_Off(void);                     // 关闭继电器（高电平断开）
unsigned char Relay_GetState(void);       // 获取继电器状态，0=开(吸合)，1=关(断开)

#endif /* __RELAY_H__ */
