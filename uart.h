#ifndef __UART_H__
#define __UART_H__

#include "reg51.h"
#include "pca.h"

/*
 * ========================================
 * UART Commands Usage Guide
 * ========================================
 * 
 * Supported Commands:
 * ===================
 * 
 * 1. Set Time Command:
 *    Format: TIME:HH:MM:SS
 *    Example: TIME:14:30:00
 *    Description: Set system time to 14:30:00
 * 
 * 2. Set Auto Watering Command:
 *    Format: A:HH:MM:SS:MMMM
 *    Example: A:06:00:01:0100
 *    Description: Set auto watering at 6:00:01 for 100ml
 *    Parameters: HH(0-23), MM(0-59), SS(0-59), MMMM(50-9999ml)
 * 
 * 3. Stop Auto Watering Command:
 *    Format: STOP
 *    Description: Stop current auto watering function
 * 
 * Usage Examples:
 * ===============
 * 
 * Set morning watering 200ml at 7:00:
 * Send: A:07:00:00:0200
 * Reply: Auto Set OK
 * 
 * Set current time:
 * Send: TIME:15:30:00
 * Reply: Time Set: 15:30:00
 * 
 * Stop auto watering:
 * Send: STOP
 * Reply: Auto Stopped
 */

// 串口命令处理相关定义 - 减少缓冲区大小
#define UART_BUF_SIZE 16    // 从32减少到16
#define CMD_SET_TIME 1      // 设置时间命令

// 函数声明
void UART_Init(void);                    // 初始化串口
void UART_SendByte(BYTE dat);            // 发送一个字节
void UART_SendString(char *s);           // 发送字符串
void UART_ProcessCommand(void);          // 处理串口命令

#endif /* __UART_H__ */
