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
 * @brief 向24C02写数据
 * @param addr 要写入数据的地址
 * @param dat 要写入的数据
 */
void EEPROM_Write(unsigned char addr, unsigned char dat) {
    I2C_Start();
    I2C_WriteByte(EEPROM_ADDR);    // 器件地址+写
    I2C_WriteByte(addr);           // 存储地址
    I2C_WriteByte(dat);            // 要写入的数据
    I2C_Stop();
    delay_ms(10);                  // 等待写入完成
}

/**
 * @brief 从24C02读数据
 * @param addr 要从EEPROM读取的数据的地址
 * @return 读取的数据
 */
unsigned char EEPROM_Read(unsigned char addr) {
    unsigned char dat;
    I2C_Start();
    I2C_WriteByte(EEPROM_ADDR);    // 器件地址+写
    I2C_WriteByte(addr);           // 存储地址

    I2C_Start();
    I2C_WriteByte(EEPROM_ADDR|1);  // 器件地址+读
    dat = I2C_ReadByte(1);         // 读取数据(发送非应答)
    I2C_Stop();
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

/**
 * @brief 检测是否为第一次上电
 * @return 是否是第一次上电
 */
bit IsFirstPowerOn() {
    return (EEPROM_Read(INIT_FLAG_ADDR) != INIT_FLAG_VALUE);
}

/**
 * @brief 标记已初始化
 */
void SetInitializedFlag() {
    EEPROM_Write(INIT_FLAG_ADDR, INIT_FLAG_VALUE);
}

/**
 * @brief 将闹钟时间写入24C02，保证掉电不丢失
 */
void SaveAlarmToEEPROM() {
    // 如果项目中有闹钟功能，取消下面的注释并确保alarmTime变量可用
    /*
    EEPROM_Write(EEPROM_HOUR_ADR, alarmTime.hour);
    EEPROM_Write(EEPROM_MIN_ADR, alarmTime.min);
    EEPROM_Write(EEPROM_SEC_ADR, alarmTime.sec);
    */
}

/**
 * @brief 从24C02读取闹钟时间
 */
void ReadAlarmFromEEPROM(){
    // 如果项目中有闹钟功能，取消下面的注释并确保alarmTime变量可用
    /*
    alarmTime.hour = EEPROM_Read(EEPROM_HOUR_ADR);
    delay_ms(10);
    alarmTime.min = EEPROM_Read(EEPROM_MIN_ADR);
    delay_ms(10);
    alarmTime.sec = EEPROM_Read(EEPROM_SEC_ADR);
    delay_ms(10);
    alarmTime.alarmTriggered = 0;
    */
}

/**
 * @brief 保存浇水量上限到EEPROM
 */
void SaveWateringToEEPROM() {
    EEPROM_WriteULong(EEPROM_WATER_ADR, totalFlow);
}

/**
 * @brief 从EEPROM读取浇水量上限
 */
void ReadWateringFromEEPROM() {
    totalFlow = EEPROM_ReadULong(EEPROM_WATER_ADR);
}

// ===== 兼容原有接口的函数实现 =====

// I2C初始化
void I2C_Init(void) {
    SDA = 1;
    SCL = 1;
}

// 兼容原有的发送字节接口（返回ACK状态）
bit I2C_SendByte(BYTE dat) {
    I2C_WriteByte(dat);
    return 0;  // 假设总是成功，实际状态已在I2C_WriteByte中处理
}

// 兼容原有的接收字节接口
BYTE I2C_ReceiveByte(bit ack) {
    return I2C_ReadByte(ack);
}

// 写一个字节到24C02（兼容接口）
void AT24C02_WriteByte(BYTE addr, BYTE dat) {
    EEPROM_Write(addr, dat);
}

// 从24C02读一个字节（兼容接口）
BYTE AT24C02_ReadByte(BYTE addr) {
    return EEPROM_Read(addr);
}

// 写累计流量到24C02（兼容接口）
void AT24C02_WriteTotalFlow(unsigned long flow) {
    EEPROM_WriteULong(TOTAL_FLOW_ADDR_0, flow);
}

// 从24C02读累计流量（兼容接口）
unsigned long AT24C02_ReadTotalFlow(void) {
    return EEPROM_ReadULong(TOTAL_FLOW_ADDR_0);
}
