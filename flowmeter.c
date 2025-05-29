#include "flowmeter.h"
#include "intrins.h"
#include "i2c.h"  
#include "uart.h"

/*
 * ========================================
 * 流量计系统 - 硬件连接说明
 * ========================================
 * 
 * 【引脚连接图】
 * ==============
 * 
 * 51单片机                   外设连接
 * ┌─────────────────────────────────────────────────┐
 * │                                                 │
 * │  P1.0 ───────────┐                            │
 * │                  │                            │
 * │                  ├──> 继电器常开触点1          │
 * │                  │                            │
 * │  P1.1 ───────────┼──> 继电器控制线(低电平吸合) │
 * │                  │                            │
 * │  P3.2 ───────────┴──> 继电器常开触点2          │
 * │  (INT0)                                        │
 * │                                                │
 * │  P2.0 ─────────────────> 24C02-SDA (数据线)   │
 * │  P2.1 ─────────────────> 24C02-SCL (时钟线)   │
 * │                                                │
 * │  P2.3~P2.7 ───────────> 数码管显示控制         │
 * │                                                │
 * └─────────────────────────────────────────────────┘
 */

// 流量计参数
static BYTE flowMode = FLOW_MODE_OFF;     // 流量显示模式
static WORD pulseCount = 0;               // 当前流量脉冲计数
static unsigned long currentFlow = 0;              // 当前流量值（毫升/秒）

static bit isRunning = 0;                 // 流量计运行状态
static bit needUpdateDisplay = 0;         // 显示更新标志
static BYTE initialDisplayDelay = 0;      // 初始显示延迟计数器

// 24C02存储控制变量
static BYTE saveCounter = 0;              // 定期保存计数器
static bit totalFlowChanged = 0;          // 累计流量变化标志
static unsigned long xdata lastSavedFlow = 0;   // 上次保存的累计流量

// 流量计参数定义
#define PULSE_FACTOR 1                   // 每个脉冲代表1毫升
#define FLOW_UPDATE_INTERVAL 1           // 每秒更新一次流量值
#define SAVE_INTERVAL 10                 // 每10秒保存一次到24C02
#define IMMEDIATE_SAVE_THRESHOLD 50      // 累计差值超过50ml立即保存

// 流量计初始化
void FlowMeter_Init(void) {
    /*
     * 硬件初始化说明：
     * - INT0 (P3.2): 设置为边沿触发，检测流量脉冲
     * - P1.0: 方波发生器输出，模拟水流传感器
     * - P1.1: 继电器控制，控制水阀开关
     * - P2.0/P2.1: I2C接口，连接24C02存储芯片
     */

    char flowStr[12];
    char temp[12];
    BYTE i = 0, j = 0;
    unsigned long tempFlow;

    IT0 = 1;                           // 设置INT0为边沿触发（下降沿触发）
    EX0 = 1;                           // 使能INT0中断
    pulseCount = 0;                    // 初始化脉冲计数
    currentFlow = 0;                   // 初始化当前流量
    
    // 从24C02读取累计流量数据
    totalFlow = AT24C02_ReadTotalFlow();

    lastSavedFlow = totalFlow;         // 立即保存原始值
    
    // 发送累计流量到串口进行调试
    UART_SendString("Total Flow: ");
    
    if(totalFlow == 0) {
        UART_SendString("0");
    } else {
        tempFlow = totalFlow;
        i = 0;  // 重置索引
        
        // 从低位到高位提取数字
        while(tempFlow > 0 && i < 10) {  // 添加边界检查
            temp[i++] = '0' + (tempFlow % 10);
            tempFlow /= 10;
        }
        
        // 确保至少有一个数字
        if(i == 0) {
            temp[i++] = '0';
        }
        
        // 反转字符串
        j = 0;
        while(i > 0 && j < 10) {  // 添加边界检查
            flowStr[j++] = temp[--i];
        }
        flowStr[j] = '\0';
        
        UART_SendString(flowStr);
    }
    
    UART_SendString(" ml\r\n");
    
    isRunning = 0;                     // 初始不运行
    flowMode = FLOW_MODE_OFF;          // 默认显示模式为关闭
    saveCounter = 0;                   // 重置保存计数器
    totalFlowChanged = 0;              // 重置变化标志
}

// 启动流量计
void FlowMeter_Start(void) {
    if (!isRunning) {
        pulseCount = 0;                // 重置脉冲计数
        EX0 = 1;                       // 使能INT0中断
        isRunning = 1;                 // 标记流量计开始运行
        
        // 设置初始非零流量值
        currentFlow = 0; 
        
        // 强制立即更新显示
        needUpdateDisplay = 1;
        initialDisplayDelay = 2;       // 设置初始显示延迟
    }
}

// 停止流量计
void FlowMeter_Stop(void) {
    isRunning = 0;                     // 标记流量计停止
    // 不关闭中断，以便继续统计总流量
}

// 复位流量计累计值
void FlowMeter_Reset(void) {
    pulseCount = 0;                    // 重置脉冲计数
    currentFlow = 0;                   // 重置当前流量

}

// 计算流量（每秒调用一次，由中断触发）
void FlowMeter_CalcFlow(void) {
    static BYTE updateCounter = 0;
    
    if (initialDisplayDelay > 0) {
        initialDisplayDelay--;
        if (initialDisplayDelay == 0) {
            needUpdateDisplay = 1;
        }
    }
    
    // 每FLOW_UPDATE_INTERVAL秒更新一次当前流量
    if (++updateCounter >= FLOW_UPDATE_INTERVAL) {
        updateCounter = 0;
        
        if (isRunning) {
            currentFlow = pulseCount;
            
            // 更新累计流量（毫升）
            if (currentFlow > 0) {
                unsigned long oldTotalFlow = totalFlow;
                totalFlow += currentFlow;
                
                // 标记累计流量已改变
                if (totalFlow != oldTotalFlow) {
                    totalFlowChanged = 1;
                }
                
                // 检查是否需要立即保存（防止大量数据丢失）
                if (totalFlow - lastSavedFlow >= IMMEDIATE_SAVE_THRESHOLD) {
                    SaveTotalFlowToEEPROM();
                }
            }
        }
        
        // 定期保存累计流量到24C02
        if (++saveCounter >= SAVE_INTERVAL) {
            saveCounter = 0;
            
            // 只有当累计流量发生变化时才保存
            if (totalFlowChanged) {
                SaveTotalFlowToEEPROM();
            }
        }
        
        // 重置脉冲计数，准备下次统计
        pulseCount = 0;
        needUpdateDisplay = 1;
    }
    
    // 轮流显示切换逻辑
    if (isRunning) {
        static BYTE displayToggle = 0;
        
        if (++displayToggle >= 3) {  
            displayToggle = 0;
            flowMode = (flowMode == FLOW_MODE_CURR) ? FLOW_MODE_TOTAL : FLOW_MODE_CURR;
            needUpdateDisplay = 1;
        }
    }
}


// 保存累计流量到24C02
void SaveTotalFlowToEEPROM(void) {
    // 写入4字节累计流量数据到24C02
    AT24C02_WriteTotalFlow(totalFlow);
    
    // 更新保存状态
    lastSavedFlow = totalFlow;
    totalFlowChanged = 0;

}

void FlowMeter_UpdateDisplay(void) {
    if (needUpdateDisplay) {
        needUpdateDisplay = 0;  // 清除标志
        
        // 根据当前模式更新显示
        if (flowMode == FLOW_MODE_CURR) {
            UpdateCurrentFlowDisplay();
        } else if (flowMode == FLOW_MODE_TOTAL) {
            UpdateTotalFlowDisplay();
        }
    }
}

void UpdateCurrentFlowDisplay(void) {
    BYTE val1, val2, val3, val4, val5, val6, val7, val8;
    
    // 提取当前流量各位数字（毫升/秒）- 支持最大9999999毫升/秒
    unsigned long flow_display = (unsigned long)currentFlow;
    
    // 构造8位显示数字 - 格式：XXXXXXX10（前7位流量值，最后1位模式标识10）
    val1 = 10;  // 模式标识保持不变 - "10"表示当前流量
    val2 = (BYTE)(flow_display % 10);                    // 个位（毫升/秒）
    val3 = (BYTE)((flow_display / 10) % 10);             // 十位（毫升/秒）
    val4 = (BYTE)((flow_display / 100) % 10);            // 百位（毫升/秒）
    val5 = (BYTE)((flow_display / 1000) % 10);           // 千位（毫升/秒）
    val6 = (BYTE)((flow_display / 10000) % 10);          // 万位（毫升/秒）
    val7 = (BYTE)((flow_display / 100000) % 10);         // 十万位（毫升/秒）
    val8 = (BYTE)((flow_display / 1000000) % 10);        // 百万位（毫升/秒）
    
    // 使用8位显示缓冲区
    FillCustomDispBuf8(val1, val2, val3, val4, val5, val6, val7, val8);
}

// 更新累计流量显示 - 扩展到8位数码管，支持7位流量值
void UpdateTotalFlowDisplay(void) {
    BYTE val1, val2, val3, val4, val5, val6, val7, val8;
    
    // 提取总流量各位数字（毫升）- 支持最大9999999毫升累计
    unsigned long flow_ml = totalFlow;
    
    // 构造8位显示数字 - 格式：XXXXXXX11（前7位流量值，最后1位模式标识11）
    val1 = 11;  // "11"表示累计流量
    val2 = (BYTE)(flow_ml % 10);                 // 个位（毫升）
    val3 = (BYTE)((flow_ml / 10) % 10);          // 十位（毫升）
    val4 = (BYTE)((flow_ml / 100) % 10);         // 百位（毫升）
    val5 = (BYTE)((flow_ml / 1000) % 10);        // 千位（毫升）
    val6 = (BYTE)((flow_ml / 10000) % 10);       // 万位（毫升）
    val7 = (BYTE)((flow_ml / 100000) % 10);      // 十万位（毫升）
    val8 = (BYTE)((flow_ml / 1000000) % 10);     // 百万位（毫升）
    
    // 使用8位显示缓冲区
    FillCustomDispBuf8(val1, val2, val3, val4, val5, val6, val7, val8);
}

// 设置流量显示模式
void FlowMeter_SetMode(BYTE mode) {
    flowMode = mode;

    needUpdateDisplay = 1;
}

// 获取当前流量显示模式
BYTE FlowMeter_GetMode(void) {
    return flowMode;
}

// 获取累计流量
unsigned long FlowMeter_GetTotalFlow(void) {
    return totalFlow;
}

// 外部中断0服务函数 - 用于脉冲计数
void INT0_ISR() interrupt 0 {
    pulseCount++;  // 每次中断增加脉冲计数
}
