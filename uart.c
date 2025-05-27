#include "uart.h"
#include "flowmeter.h"
#include "keyboard_control.h"  // 添加此头文件以使用定时浇水功能
#include <string.h>

// 串口缓冲区及状态变量 - 减少缓冲区大小
static char uart_buffer[16];  // 减少缓冲区大小从32到16
static BYTE uart_count = 0;
static bit uart_complete = 0;

// 初始化串口
void UART_Init(void) {
    SCON = 0x50;    // 设置串口工作方式1，8位UART，可变波特率，REN=1允许接收
    PCON &= 0x7F;   // SMOD=0，波特率不加倍
    
    // 使用定时器1作为波特率发生器
    TMOD &= 0x0F;   // 清除T1的设置位
    TMOD |= 0x20;   // 设置T1为模式2（8位自动重载）
    
    // 9600波特率设置 (针对11.0592MHz晶振)
    TL1 = 0xFD;     // 设置初值
    TH1 = 0xFD;     // 设置重载值
    
    TR1 = 1;        // 启动定时器1
    ES = 1;         // 使能串口中断
    EA = 1;         // 使能总中断
    
    // 初始化缓冲区
    uart_count = 0;
    uart_complete = 0;
    memset(uart_buffer, 0, sizeof(uart_buffer));
    
    // 发送英文启动信息
    UART_SendString("\r\nSystem Ready\r\n");
    UART_SendString("TIME:HH:MM:SS\r\n");
    UART_SendString("A:HH:MM:SS:MMMM\r\n");
}

// 发送一个字节
void UART_SendByte(BYTE dat) {
    SBUF = dat;             // 将数据写入发送缓冲区
    while(!TI);             // 等待发送完成
    TI = 0;                 // 清除发送中断标志
}

// 发送字符串
void UART_SendString(char *s) {
    while(*s) {             // 发送字符串，直到遇到null结束符
        UART_SendByte(*s++);
    }
}

// 命令处理函数
static void UART_CommandHandler(void) {
    // 时间设置命令格式: "TIME:HH:MM:SS"
    if(strncmp(uart_buffer, "TIME:", 5) == 0) {
        BYTE hour = 0, min = 0, sec = 0;
        
        // 解析时间
        if(strlen(uart_buffer) >= 13) { // 确保格式正确且长度足够
            hour = (uart_buffer[5] - '0') * 10 + (uart_buffer[6] - '0');
            min = (uart_buffer[8] - '0') * 10 + (uart_buffer[9] - '0');
            sec = (uart_buffer[11] - '0') * 10 + (uart_buffer[12] - '0');
            
            // 检查时间值是否有效
            if(hour < 24 && min < 60 && sec < 60) {
                // 直接修改SysPara1中的时间值
                PCA_SetTime(hour, min, sec);
                
                UART_SendString("\r\nTime Set: ");
                // 发送时间字符串
                uart_buffer[7] = ':';
                uart_buffer[10] = ':';
                uart_buffer[13] = 0;
                UART_SendString(uart_buffer + 5);
                UART_SendString("\r\n");
            }
            else {
                UART_SendString("\r\nError: Invalid time\r\n");
                UART_SendString("Format: HH(0-23):MM(0-59):SS(0-59)\r\n");
            }
        }
        else {
            UART_SendString("\r\nError: Wrong format\r\n");
            UART_SendString("Format: TIME:HH:MM:SS\r\n");
            UART_SendString("Example: TIME:14:30:00\r\n");
        }
    }
    // 定时浇水设置命令格式: "A:HH:MM:SS:MMMM"
    else if(strncmp(uart_buffer, "A:", 2) == 0) {
        BYTE hour, min, sec;
        unsigned int volume;
        
        // 解析定时浇水参数 A:HH:MM:SS:MMMM
        if(strlen(uart_buffer) >= 15) { // 确保格式正确且长度足够
            hour = (uart_buffer[2] - '0') * 10 + (uart_buffer[3] - '0');
            min = (uart_buffer[5] - '0') * 10 + (uart_buffer[6] - '0');
            sec = (uart_buffer[8] - '0') * 10 + (uart_buffer[9] - '0');
            
            // 解析毫升数(最多4位)
            volume = 0;
            volume += (uart_buffer[11] - '0') * 1000;
            volume += (uart_buffer[12] - '0') * 100;
            volume += (uart_buffer[13] - '0') * 10;
            volume += (uart_buffer[14] - '0');
            
            // 检查参数有效性
            if(hour < 24 && min < 60 && sec < 60 && volume >= 50 && volume <= 9999) {
                // 设置定时浇水参数
                timed_watering.start_hour = hour;
                timed_watering.start_min = min;
                timed_watering.start_sec = sec;
                timed_watering.water_volume_ml = volume;
                timed_watering.enabled = 1;
                timed_watering.triggered_today = 0;
                
                UART_SendString("\r\nAuto Set OK\r\n");
                UART_SendString("Time: ");
                uart_buffer[4] = ':';
                uart_buffer[7] = ':';
                uart_buffer[10] = 0;
                UART_SendString(uart_buffer + 2);
                UART_SendString("\r\nVolume: ");
                uart_buffer[15] = 0;
                UART_SendString(uart_buffer + 11);
                UART_SendString("ml\r\n");
            }
            else {
                UART_SendString("\r\nError: Invalid params\r\n");
                UART_SendString("Time: HH(0-23):MM(0-59):SS(0-59)\r\n");
                UART_SendString("Volume: 50-9999ml\r\n");
            }
        }
        else {
            UART_SendString("\r\nError: Wrong format\r\n");
            UART_SendString("Format: A:HH:MM:SS:MMMM\r\n");
            UART_SendString("Example: A:06:00:01:0100\r\n");
        }
    }
    // 停止定时浇水命令: "STOP"
    else if(strncmp(uart_buffer, "STOP", 4) == 0) {
        TimedWatering_Stop();
        UART_SendString("\r\nAuto Stopped\r\n");
    }
    else {
        UART_SendString("\r\nError: Unknown cmd\r\n");
        UART_SendString("Commands:\r\n");
        UART_SendString("TIME:HH:MM:SS - Set time\r\n");
        UART_SendString("A:HH:MM:SS:MMMM - Set auto\r\n");
        UART_SendString("STOP - Stop auto\r\n");
    }
}

// 处理串口命令
void UART_ProcessCommand(void) {
    if(uart_complete) {
        UART_CommandHandler();
        uart_complete = 0;
        uart_count = 0;
        memset(uart_buffer, 0, sizeof(uart_buffer));
    }
}

// 串口中断服务函数
void UART_ISR() interrupt 4 {
    if(RI) {                // 接收中断
        RI = 0;             // 清除接收中断标志
        
        if(!uart_complete) { // 如果前一条命令还没处理完，则忽略当前接收的字符
            char ch = SBUF;  // 获取接收到的字符
            
            // 回显接收到的字符
            UART_SendByte(ch);
            
            if(ch == '\n' || ch == '\r') { // 接收到回车或换行
                uart_buffer[uart_count] = '\0';  // 字符串结束符
                if (uart_count > 0) {  // 非空命令才进行处理
                    uart_complete = 1;  // 设置接收完成标志
                }
            }
            else if(uart_count < UART_BUF_SIZE - 1) { // 缓冲区未满
                uart_buffer[uart_count++] = ch;  // 存储接收到的字符
            }
        }
    }
    
    if(TI) {                // 发送中断
        TI = 0;             // 清除发送中断标志
    }
}
