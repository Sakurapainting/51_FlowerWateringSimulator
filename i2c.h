#ifndef __I2C_H__
#define __I2C_H__

#include "reg51.h"
#include "pca.h"

// I2C引脚定义
sbit SDA = P2^0;  // I2C数据线
sbit SCL = P2^1;  // I2C时钟线

// 24C02参数定义
#define AT24C02_ADDR 0xA0  // 24C02器件地址

// 累计流量存储地址定义
#define TOTAL_FLOW_ADDR_0 0x00  // 累计流量低字节
#define TOTAL_FLOW_ADDR_1 0x01  // 累计流量第2字节
#define TOTAL_FLOW_ADDR_2 0x02  // 累计流量第3字节
#define TOTAL_FLOW_ADDR_3 0x03  // 累计流量高字节

// 函数声明
void I2C_Init(void);                           // I2C初始化
void I2C_Start(void);                          // 发送起始信号
void I2C_Stop(void);                           // 发送停止信号
bit I2C_SendByte(BYTE dat);                    // 发送一个字节
BYTE I2C_ReceiveByte(bit ack);                 // 接收一个字节

void AT24C02_WriteByte(BYTE addr, BYTE dat);   // 写一个字节到24C02
BYTE AT24C02_ReadByte(BYTE addr);              // 从24C02读一个字节
void AT24C02_WriteTotalFlow(unsigned long flow); // 写累计流量到24C02
unsigned long AT24C02_ReadTotalFlow(void);    // 从24C02读累计流量

#endif /* __I2C_H__ */
