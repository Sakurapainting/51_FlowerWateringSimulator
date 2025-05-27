#ifndef __I2C_H__
#define __I2C_H__

#include "reg51.h"
#include "pca.h"

// I2C引脚定义 - 保持原有引脚不变
sbit SDA = P2^5;  // I2C数据线
sbit SCL = P2^6;  // I2C时钟线

// 24C02参数定义
#define AT24C02_ADDR 0xA0  // 24C02器件地址
#define EEPROM_ADDR 0xA0   // 兼容24C02.c中的定义

// 累计流量存储地址定义
#define TOTAL_FLOW_ADDR_0 0x00  // 累计流量低字节
#define TOTAL_FLOW_ADDR_1 0x01  // 累计流量第2字节
#define TOTAL_FLOW_ADDR_2 0x02  // 累计流量第3字节
#define TOTAL_FLOW_ADDR_3 0x03  // 累计流量高字节

// 24C02.c中的地址定义
#define EEPROM_HOUR_ADR 0x10    // 闹钟小时存储地址
#define EEPROM_MIN_ADR 0x11     // 闹钟分钟存储地址
#define EEPROM_SEC_ADR 0x12     // 闹钟秒存储地址
#define EEPROM_WATER_ADR 0x04   // 浇水量存储起始地址(4字节)
#define INIT_FLAG_ADDR 0x20     // 初始化标志地址
#define INIT_FLAG_VALUE 0x55    // 初始化标志值

// 24C02.c中的基础I2C函数声明
void I2C_Start(void);                          // 发送起始信号
void I2C_Stop(void);                           // 发送停止信号
void I2C_SendAck(bit ack);                     // 发送应答信号
bit I2C_WaitAck(void);                         // 等待应答信号
void I2C_WriteByte(unsigned char dat);         // 写一个字节
unsigned char I2C_ReadByte(bit ack);           // 读一个字节

// 24C02操作函数声明
void EEPROM_Write(unsigned char addr, unsigned char dat);  // 向24C02写数据
unsigned char EEPROM_Read(unsigned char addr); // 从24C02读数据
void EEPROM_WriteULong(unsigned char addr, unsigned long dat); // 写unsigned long数据
unsigned long EEPROM_ReadULong(unsigned char addr);       // 读unsigned long数据
bit IsFirstPowerOn(void);                      // 检测是否为第一次上电
void SetInitializedFlag(void);                 // 标记已初始化

// 兼容原有接口的函数声明
void I2C_Init(void);                           // I2C初始化
bit I2C_SendByte(BYTE dat);                    // 发送一个字节(兼容接口)
BYTE I2C_ReceiveByte(bit ack);                 // 接收一个字节(兼容接口)
void AT24C02_WriteByte(BYTE addr, BYTE dat);   // 写一个字节到24C02
BYTE AT24C02_ReadByte(BYTE addr);              // 从24C02读一个字节
void AT24C02_WriteTotalFlow(unsigned long flow); // 写累计流量到24C02
unsigned long AT24C02_ReadTotalFlow(void);    // 从24C02读累计流量

// 24C02.c中的应用层函数声明
void SaveAlarmToEEPROM(void);                  // 保存闹钟时间到EEPROM
void ReadAlarmFromEEPROM(void);                // 从EEPROM读取闹钟时间
void SaveWateringToEEPROM(void);               // 保存浇水量到EEPROM
void ReadWateringFromEEPROM(void);             // 从EEPROM读取浇水量

// 外部变量声明
extern unsigned long xdata totalFlow;

#endif /* __I2C_H__ */
