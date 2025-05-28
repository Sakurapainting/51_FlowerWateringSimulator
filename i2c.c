#include "i2c.h"
#include "intrins.h"

// 全局变量定义
unsigned long xdata totalFlow = 0;  // 从flowmeter.c移至此处

// 外部变量声明（如果需要访问闹钟时间）
// extern AlarmTime alarmTime;  // 如果项目中有闹钟功能，取消注释

/**
 * @brief I2C起始信号
 */
void I2C_Start() {
    SDA = 1; _nop_();
    SCL = 1; _nop_();
    SDA = 0; _nop_();
    SCL = 0; _nop_();
}

/**
 * @brief I2C停止信号
 */
void I2C_Stop() {
    SDA = 0; _nop_();
    SCL = 1; _nop_();
    SDA = 1; _nop_();
}

/**
 * @brief 发送应答信号
 * @param ack 0:应答 1:非应答
 */
void I2C_SendAck(bit ack) {
    SDA = ack; _nop_();
    SCL = 1; _nop_();
    SCL = 0; _nop_();
    SDA = 1; _nop_();
}

/**
 * @brief 等待应答信号
 * @return 0:应答成功 1:应答失败
 */
bit I2C_WaitAck() {
    SDA = 1; _nop_();
    SCL = 1; _nop_();
    if(SDA) { // 应答失败
        SCL = 0; _nop_();
        return 1;
    }
    SCL = 0; _nop_();
    return 0;
}

/**
 * @brief 写一个字节
 * @param dat 要写入的字节
 */
void I2C_WriteByte(unsigned char dat) {
    unsigned char i;
    for(i=0; i<8; i++) {
        SDA = (dat & 0x80) ? 1 : 0;
        SCL = 1; _nop_();
        SCL = 0; _nop_();
        dat <<= 1;
    }
    I2C_WaitAck();
}

/**
 * @brief 读一个字节
 * @param ack 是否发送应答信号 0:应答 1:非应答
 * @return 读取的字节
 */
unsigned char I2C_ReadByte(bit ack) {
    unsigned char i, dat = 0;
    SDA = 1;
    for(i=0; i<8; i++) {
        SCL = 1; _nop_();
        dat <<= 1;
        dat |= SDA;
        SCL = 0; _nop_();
    }
    I2C_SendAck(ack);
    return dat;
}


/**
 * @brief 向EEPROM写入unsigned long数据(4字节)
 * @param addr 起始地址
 * @param dat 要写入的数据
 */
void EEPROM_WriteULong(unsigned char addr, unsigned long dat) {
    I2C_Start();
    I2C_WriteByte(EEPROM_ADDR);    // 器件地址+写
    I2C_WriteByte(addr);           // 存储地址
    
    // 分4次写入，从低字节到高字节
    I2C_WriteByte((unsigned char)(dat & 0xFF));
    I2C_WriteByte((unsigned char)((dat >> 8) & 0xFF));
    I2C_WriteByte((unsigned char)((dat >> 16) & 0xFF));
    I2C_WriteByte((unsigned char)((dat >> 24) & 0xFF));
    
    I2C_Stop();
    delay_ms(20);                  // 写入时间稍长
}

/**
 * @brief 从EEPROM读取unsigned long数据(4字节)
 * @param addr 起始地址
 * @return 读取的数据
 */
unsigned long EEPROM_ReadULong(unsigned char addr) {
    unsigned long dat = 0;
    I2C_Start();
    I2C_WriteByte(EEPROM_ADDR);    // 器件地址+写
    I2C_WriteByte(addr);           // 存储地址

    I2C_Start();
    I2C_WriteByte(EEPROM_ADDR|1);  // 器件地址+读
    
    // 分4次读取，从低字节到高字节
    dat = I2C_ReadByte(0);         // 读取低字节(发送应答)
    dat |= (unsigned long)I2C_ReadByte(0) << 8;
    dat |= (unsigned long)I2C_ReadByte(0) << 16;
    dat |= (unsigned long)I2C_ReadByte(1) << 24; // 最后一个字节发送非应答
    
    I2C_Stop();
    return dat;
}

// ===== 兼容原有接口的函数实现 =====

// I2C初始化
void I2C_Init(void) {
    SDA = 1;
    SCL = 1;
}

// 写累计流量到24C02（兼容接口）
void AT24C02_WriteTotalFlow(unsigned long flow) {
    EEPROM_WriteULong(TOTAL_FLOW_ADDR_0, flow);
}

// 从24C02读累计流量（兼容接口）
unsigned long AT24C02_ReadTotalFlow(void) {
    return EEPROM_ReadULong(TOTAL_FLOW_ADDR_0);
}
