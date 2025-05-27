#ifndef __UART_H__
#define __UART_H__

#include "reg51.h"
#include "pca.h"

/*
 * ========================================
 * UART Commands Usage Guide (Updated)
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
 * 2. Set Date Command:
 *    Format: DATE:YYYY:MM:DD
 *    Example: DATE:2025:05:27
 *    Description: Set system date to 2025-05-27
 * 
 * 3. Set DateTime Command:
 *    Format: DATETIME:YYYY:MM:DD:HH:MM:SS
 *    Example: DATETIME:2025:05:27:14:30:00
 *    Description: Set complete date and time
 * 
 * 4. Set Auto Watering Command:
 *    Format: A:HH:MM:SS:MMMM
 *    Example: A:06:00:01:0100
 *    Description: Set auto watering at 6:00:01 for 100ml
 *    Parameters: HH(0-23), MM(0-59), SS(0-59), MMMM(50-9999ml)
 * 
 * 5. Stop Auto Watering Command:
 *    Format: STOP
 *    Description: Stop current auto watering function
 * 
 * 6. Display Mode Commands:
 *    Format: DISPTIME or DISPDATE
 *    Description: Switch display between time and date mode
 * 
 * Usage Examples:
 * ===============
 * 
 * Set current date and time:
 * Send: DATETIME:2025:05:27:15:30:00
 * Reply: DateTime Set: 2025-05-27 15:30:00
 * 
 * Set only date:
 * Send: DATE:2025:12:25
 * Reply: Date Set: 2025-12-25
 * 
 * Set only time:
 * Send: TIME:15:30:00
 * Reply: Time Set: 15:30:00
 * 
 * Switch to date display:
 * Send: DISPDATE
 * Reply: Display Mode: Date
 * 
 * Set morning watering 200ml at 7:00:
 * Send: A:07:00:00:0200
 * Reply: Auto Set OK
 * 
 * Stop auto watering:
 * Send: STOP
 * Reply: Auto Stopped
 */

// 串口命令处理相关定义
#define UART_BUF_SIZE 32    // 增加缓冲区大小以支持更长的日期时间命令
#define CMD_SET_TIME 1      // 设置时间命令
#define CMD_SET_DATE 2      // 设置日期命令
#define CMD_SET_DATETIME 3  // 设置日期时间命令

// 函数声明
void UART_Init(void);                    // 初始化串口
void UART_SendByte(BYTE dat);            // 发送一个字节
void UART_SendString(char *s);           // 发送字符串
void UART_ProcessCommand(void);          // 处理串口命令

#endif /* __UART_H__ */
