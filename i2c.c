#include "i2c.h"
#include "intrins.h"

// I2C延时函数
static void I2C_Delay(void) {
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

// I2C初始化
void I2C_Init(void) {
    SDA = 1;
    SCL = 1;
}

// 发送起始信号
void I2C_Start(void) {
    SDA = 1;
    SCL = 1;
    I2C_Delay();
    SDA = 0;
    I2C_Delay();
    SCL = 0;
}

// 发送停止信号
void I2C_Stop(void) {
    SDA = 0;
    SCL = 0;
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    SDA = 1;
    I2C_Delay();
}

// 发送一个字节，返回ACK状态
bit I2C_SendByte(BYTE dat) {
    BYTE i;
    
    for(i = 0; i < 8; i++) {
        SCL = 0;
        I2C_Delay();
        SDA = (dat & 0x80) ? 1 : 0;
        dat <<= 1;
        I2C_Delay();
        SCL = 1;
        I2C_Delay();
    }
    
    SCL = 0;
    I2C_Delay();
    SDA = 1;
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    
    return SDA;  // 返回ACK状态，0表示ACK，1表示NACK
}

// 接收一个字节
BYTE I2C_ReceiveByte(bit ack) {
    BYTE i, dat = 0;
    
    SDA = 1;
    for(i = 0; i < 8; i++) {
        SCL = 0;
        I2C_Delay();
        SCL = 1;
        I2C_Delay();
        dat <<= 1;
        if(SDA) dat |= 0x01;
    }
    
    SCL = 0;
    I2C_Delay();
    SDA = ack ? 0 : 1;  // 发送ACK或NACK
    I2C_Delay();
    SCL = 1;
    I2C_Delay();
    SCL = 0;
    I2C_Delay();
    
    return dat;
}

// 写一个字节到24C02
void AT24C02_WriteByte(BYTE addr, BYTE dat) {
    I2C_Start();
    I2C_SendByte(AT24C02_ADDR);      // 发送器件地址+写命令
    I2C_SendByte(addr);              // 发送存储地址
    I2C_SendByte(dat);               // 发送数据
    I2C_Stop();
    
    // 等待写入完成
    delay_ms(10);
}

// 从24C02读一个字节
BYTE AT24C02_ReadByte(BYTE addr) {
    BYTE dat;
    
    I2C_Start();
    I2C_SendByte(AT24C02_ADDR);      // 发送器件地址+写命令
    I2C_SendByte(addr);              // 发送存储地址
    
    I2C_Start();                     // 重新开始
    I2C_SendByte(AT24C02_ADDR | 0x01); // 发送器件地址+读命令
    dat = I2C_ReceiveByte(0);        // 读取数据，发送NACK
    I2C_Stop();
    
    return dat;
}

// 写累计流量到24C02（4字节）
void AT24C02_WriteTotalFlow(unsigned long flow) {
    AT24C02_WriteByte(TOTAL_FLOW_ADDR_0, (BYTE)(flow & 0xFF));
    AT24C02_WriteByte(TOTAL_FLOW_ADDR_1, (BYTE)((flow >> 8) & 0xFF));
    AT24C02_WriteByte(TOTAL_FLOW_ADDR_2, (BYTE)((flow >> 16) & 0xFF));
    AT24C02_WriteByte(TOTAL_FLOW_ADDR_3, (BYTE)((flow >> 24) & 0xFF));
}

// 从24C02读累计流量（4字节）
unsigned long AT24C02_ReadTotalFlow(void) {
    unsigned long flow = 0;
    
    flow  = (unsigned long)AT24C02_ReadByte(TOTAL_FLOW_ADDR_0);
    flow |= (unsigned long)AT24C02_ReadByte(TOTAL_FLOW_ADDR_1) << 8;
    flow |= (unsigned long)AT24C02_ReadByte(TOTAL_FLOW_ADDR_2) << 16;
    flow |= (unsigned long)AT24C02_ReadByte(TOTAL_FLOW_ADDR_3) << 24;
    
    return flow;
}

// 移除定时浇水参数相关函数
// void AT24C02_WriteTimedWateringParams(TimedWatering *params) {
//     // 移除此函数
// }

// void AT24C02_ReadTimedWateringParams(TimedWatering *params) {
//     // 移除此函数
// }
