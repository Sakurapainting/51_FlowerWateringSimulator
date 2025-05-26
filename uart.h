#ifndef __UART_H__
#define __UART_H__

#include "reg51.h"
#include "pca.h"

// 串口命令处理相关定义 - 减少缓冲区大小
#define UART_BUF_SIZE 16    // 从32减少到16
#define CMD_SET_TIME 1      // 设置时间命令

// 函数声明
void UART_Init(void);                    // 初始化串口
void UART_SendByte(BYTE dat);            // 发送一个字节
void UART_SendString(char *s);           // 发送字符串
void UART_ProcessCommand(void);          // 处理串口命令

#endif /* __UART_H__ */
