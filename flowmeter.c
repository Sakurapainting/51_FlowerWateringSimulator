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
 * 
 * 【工作原理说明】
 * ================
 * 
 * 1. 方波发生器 (P1.0):
 *    - 输出5Hz标准方波信号
 *    - 每个脉冲代表1毫升水流
 *    - 连接到继电器常开触点的一端
 * 
 * 2. 继电器控制 (P1.1):
 *    - 低电平(0)时继电器吸合，水阀开启
 *    - 高电平(1)时继电器断开，水阀关闭
 *    - 吸合时P1.0信号传导到P3.2
 * 
 * 3. 流量计输入 (P3.2/INT0):
 *    - 外部中断0输入，下降沿触发
 *    - 连接到继电器常开触点的另一端
 *    - 继电器闭合时接收来自P1.0的脉冲
 * 
 * 4. 24C02存储器 (P2.0/P2.1):
 *    - P2.0: SDA数据线，双向通信
 *    - P2.1: SCL时钟线，控制通信时序
 *    - 存储累计流量数据，掉电不丢失
 * 
 * 【信号传输路径】
 * ================
 * 
 * 正常工作时：
 * P1.0(5Hz方波) → 继电器触点1 → 继电器触点2 → P3.2(INT0中断)
 *                      ↑
 *                 P1.1控制线(低电平吸合)
 * 
 * 停止工作时：
 * P1.0(5Hz方波) → 继电器触点1   X   继电器触点2 → P3.2(无信号)
 *                      ↑
 *                 P1.1控制线(高电平断开)
 */

/*
 * ========================================
 * 流量计显示功能详细说明
 * ========================================
 * 
 * 【数码管显示模式】
 * ==================
 * 
 * 浇水过程中，数码管会在两种模式间自动切换显示：
 * 
 * 1. 当前流量显示模式 (FLOW_MODE_CURR)：
 *    ┌─────────────────────────────────────┐
 *    │ 显示格式：XXXXX10                   │
 *    │ 含义：当前瞬时流量，单位毫升/秒      │
 *    │ 示例："000510" = 5毫升/秒           │
 *    │ 右侧"10"标识当前流量模式            │
 *    └─────────────────────────────────────┘
 * 
 * 2. 累计流量显示模式 (FLOW_MODE_TOTAL)：
 *    ┌─────────────────────────────────────┐
 *    │ 显示格式：XXXXX11                   │
 *    │ 含义：累计总流量，单位毫升           │
 *    │ 示例："012311" = 123毫升总流量      │
 *    │ 右侧"11"标识累计流量模式            │
 *    └─────────────────────────────────────┘
 * 
 * 【切换频率】
 * ============
 * - 每3秒自动切换一次显示模式
 * - 浇水期间持续轮流显示
 * - 停止浇水后关闭流量显示
 * 
 * 【定时浇水期间的显示】
 * ======================
 * 
 * 正在浇水时：
 * ┌─────────────────────────────────────┐
 * │ 显示：剩余毫升数 "XXXX05"            │
 * │ 示例："007505" = 还需浇75毫升       │
 * │ 实时减少，直到变为"000005"           │
 * └─────────────────────────────────────┘
 * 
 * 【流量计算原理】
 * ================
 * 
 * 脉冲计数：
 * - INT0下降沿触发中断
 * - 每个下降沿代表1毫升水流
 * - 5Hz频率 = 每秒5个脉冲 = 5毫升/秒
 * 
 * 累计流量：
 * - 每秒累加当前流量值
 * - 数据每10秒保存到24C02
 * - 断电后从24C02恢复数据
 * 
 * 【使用说明】
 * ============
 * 
 * 手动浇水时查看流量：
 * 1. 按P3.3开始手动浇水
 * 2. 观察数码管显示的当前流量和累计流量
 * 3. 再按P3.3停止浇水
 * 
 * 定时浇水时查看进度：
 * 1. 设置定时浇水并启动
 * 2. 浇水开始时观察剩余毫升数
 * 3. 数值逐渐减少到0时自动停止
 */

/*
 * 数码管显示说明：
 * 浇水过程中数码管会轮流显示两种模式：当前流量和累计流量
 * 
 * 1. 当前流量显示模式（FLOW_MODE_CURR）:
 *    - 数码管从右到左依次为：位1-位6
 *    - 位1: 不显示
 *    - 位2: 显示数字"5"，表示当前为流量显示模式
 *    - 位3: 不显示
 *    - 位4-位6: 显示当前流量数值，单位为毫升/秒
 *      例如：显示"250 5 "表示当前流量为250毫升/秒
 * 
 * 2. 累计流量显示模式（FLOW_MODE_TOTAL）:
 *    - 数码管从右到左依次为：位1-位6
 *    - 位1: 显示数字"1"，表示当前为累计流量显示模式
 *    - 位2-位6: 显示累计流量数值，单位为毫升
 *      例如：显示"23501"表示累计流量为2350毫升
 * 
 * 注：系统会每3秒在这两种显示模式间自动切换
 */

// 流量计参数
static BYTE flowMode = FLOW_MODE_OFF;     // 流量显示模式
static WORD pulseCount = 0;               // 当前流量脉冲计数
static WORD currentFlow = 0;              // 当前流量值（毫升/秒）
// 移除本地totalFlow声明，使用i2c.c中的全局变量
extern unsigned long totalFlow;          // 累计流量值（毫升）- 使用i2c.c中的全局变量
static bit isRunning = 0;                 // 流量计运行状态
static bit needUpdateDisplay = 0;         // 显示更新标志
static BYTE initialDisplayDelay = 0;      // 初始显示延迟计数器

// 24C02存储控制变量
static BYTE saveCounter = 0;              // 定期保存计数器
static bit totalFlowChanged = 0;          // 累计流量变化标志
static unsigned long lastSavedFlow = 0;   // 上次保存的累计流量 - 移到xdata

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
    // 注意：这里不重置totalFlow，因为累计流量应该是永久累计的
    // 如果需要重置累计流量，应该提供单独的函数
}

// 新增：重置累计流量（同时清除24C02中的数据）
void FlowMeter_ResetTotal(void) {
    totalFlow = 0;
    AT24C02_WriteTotalFlow(0);
}

/*
 * ========================================
 * 流量计显示功能详细说明
 * ========================================
 * 
 * 【数码管显示模式】
 * ==================
 * 
 * 浇水过程中，数码管会在两种模式间自动切换显示：
 * 
 * 1. 当前流量显示模式 (FLOW_MODE_CURR)：
 *    ┌─────────────────────────────────────┐
 *    │ 显示格式：XXXXX10                   │
 *    │ 含义：当前瞬时流量，单位毫升/秒      │
 *    │ 示例："000510" = 5毫升/秒           │
 *    │ 右侧"10"标识当前流量模式            │
 *    └─────────────────────────────────────┘
 * 
 * 2. 累计流量显示模式 (FLOW_MODE_TOTAL)：
 *    ┌─────────────────────────────────────┐
 *    │ 显示格式：XXXXX11                   │
 *    │ 含义：累计总流量，单位毫升           │
 *    │ 示例："012311" = 123毫升总流量      │
 *    │ 右侧"11"标识累计流量模式            │
 *    └─────────────────────────────────────┘
 * 
 * 【切换频率】
 * ============
 * - 每3秒自动切换一次显示模式
 * - 浇水期间持续轮流显示
 * - 停止浇水后关闭流量显示
 * 
 * 【定时浇水期间的显示】
 * ======================
 * 
 * 正在浇水时：
 * ┌─────────────────────────────────────┐
 * │ 显示：剩余毫升数 "XXXX05"            │
 * │ 示例："007505" = 还需浇75毫升       │
 * │ 实时减少，直到变为"000005"           │
 * └─────────────────────────────────────┘
 * 
 * 【流量计算原理】
 * ================
 * 
 * 脉冲计数：
 * - INT0下降沿触发中断
 * - 每个下降沿代表1毫升水流
 * - 5Hz频率 = 每秒5个脉冲 = 5毫升/秒
 * 
 * 累计流量：
 * - 每秒累加当前流量值
 * - 数据每10秒保存到24C02
 * - 断电后从24C02恢复数据
 * 
 * 【使用说明】
 * ============
 * 
 * 手动浇水时查看流量：
 * 1. 按P3.3开始手动浇水
 * 2. 观察数码管显示的当前流量和累计流量
 * 3. 再按P3.3停止浇水
 * 
 * 定时浇水时查看进度：
 * 1. 设置定时浇水并启动
 * 2. 浇水开始时观察剩余毫升数
 * 3. 数值逐渐减少到0时自动停止
 */

/*
 * 数码管显示说明：
 * 浇水过程中数码管会轮流显示两种模式：当前流量和累计流量
 * 
 * 1. 当前流量显示模式（FLOW_MODE_CURR）:
 *    - 数码管从右到左依次为：位1-位6
 *    - 位1: 不显示
 *    - 位2: 显示数字"5"，表示当前为流量显示模式
 *    - 位3: 不显示
 *    - 位4-位6: 显示当前流量数值，单位为毫升/秒
 *      例如：显示"250 5 "表示当前流量为250毫升/秒
 * 
 * 2. 累计流量显示模式（FLOW_MODE_TOTAL）:
 *    - 数码管从右到左依次为：位1-位6
 *    - 位1: 显示数字"1"，表示当前为累计流量显示模式
 *    - 位2-位6: 显示累计流量数值，单位为毫升
 *      例如：显示"23501"表示累计流量为2350毫升
 * 
 * 注：系统会每3秒在这两种显示模式间自动切换
 */

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

/*
 * ========================================
 * 24C02累计流量存储功能
 * ========================================
 * 
 * 【存储策略】
 * ============
 * 1. 定期保存：每10秒自动保存一次累计流量
 * 2. 即时保存：累计流量增加超过50ml时立即保存
 * 3. 智能保存：只有数据变化时才执行写入操作
 * 4. 掉电保护：断电重启后自动恢复累计流量数据
 * 
 * 【24C02地址分配】
 * ==================
 * 地址0x00-0x03: 累计流量数据 (4字节unsigned long)
 * 地址0x10-0x16: 定时浇水参数 (7字节)
 * 
 * 【数据格式】
 * ============
 * 累计流量按小端序存储：
 * - 0x00: 累计流量低字节  (totalFlow & 0xFF)
 * - 0x01: 累计流量第2字节 ((totalFlow >> 8) & 0xFF)  
 * - 0x02: 累计流量第3字节 ((totalFlow >> 16) & 0xFF)
 * - 0x03: 累计流量高字节  ((totalFlow >> 24) & 0xFF)
 */

// 保存累计流量到24C02
void SaveTotalFlowToEEPROM(void) {
    // 写入4字节累计流量数据到24C02
    AT24C02_WriteTotalFlow(totalFlow);
    
    // 更新保存状态
    lastSavedFlow = totalFlow;
    totalFlowChanged = 0;
    
    // 通过串口输出保存信息（调试用，可选）
    #ifdef DEBUG_FLOW_SAVE
    UART_SendString("累计流量已保存: ");
    // 这里可以添加数值转换和输出代码
    UART_SendString("毫升\r\n");
    #endif
}

// 从24C02读取累计流量
unsigned long LoadTotalFlowFromEEPROM(void) {
    unsigned long loadedFlow = AT24C02_ReadTotalFlow();
    
    // 数据有效性检查（防止读取到无效数据）
    if (loadedFlow > 999999) {  // 如果累计流量超过999升，认为数据异常
        loadedFlow = 0;         // 重置为0
        AT24C02_WriteTotalFlow(0); // 清零24C02中的数据
    }
    
    return loadedFlow;
}

// 强制保存当前累计流量（外部调用）
void FlowMeter_ForceSave(void) {
    SaveTotalFlowToEEPROM();
}

// 获取上次保存时间点的累计流量
unsigned long FlowMeter_GetLastSavedFlow(void) {
    return lastSavedFlow;
}

// 检查累计流量是否有未保存的变化
bit FlowMeter_HasUnsavedChanges(void) {
    return totalFlowChanged;
}

// 检查并更新显示（在主循环中调用，不在中断中）
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

// 更新当前流量显示
void UpdateCurrentFlowDisplay(void) {
    BYTE val1, val2, val3, val4, val5, val6;
    
    // 提取当前流量各位数字（毫升/秒）
    WORD flow_display = currentFlow;  // currentFlow已经是毫升/秒的值
    
    // 构造显示数字 - 格式：XXXX毫升/秒
    val1 = 10;  // 10，表示当前流量显示模式
    val2 = (flow_display / 10000) % 10; // 万位（毫升/秒）
    val3 = (flow_display / 1000) % 10;  // 千位（毫升/秒）
    val4 = (flow_display / 100) % 10;   // 百位（毫升/秒）
    val5 = (flow_display / 10) % 10;    // 十位（毫升/秒）
    val6 = flow_display % 10;           // 个位（毫升/秒）
    
    // 直接填充显示缓冲区 - 显示格式如：250毫升/秒显示为"25010"
    FillCustomDispBuf(val6, val5, val4, val3, val2, val1);
}

// 更新累计流量显示
void UpdateTotalFlowDisplay(void) {
    BYTE val1, val2, val3, val4, val5, val6;
    
    // 提取总流量各位数字（毫升）
    unsigned long flow_ml = totalFlow;  // 直接使用毫升值
    
    // 构造显示数字
    val1 = 11;  // 11,累计流量显示模式
    val2 = (BYTE)((flow_ml / 10000) % 10); // 万位（毫升）
    val3 = (BYTE)((flow_ml / 1000) % 10);  // 千位（毫升）
    val4 = (BYTE)((flow_ml / 100) % 10);   // 百位（毫升）
    val5 = (BYTE)((flow_ml / 10) % 10);    // 十位（毫升）
    val6 = (BYTE)(flow_ml % 10);           // 个位（毫升）
    
    // 直接填充显示缓冲区
    FillCustomDispBuf(val6, val5, val4, val3, val2, val1);
}

// 显示当前流量（这个函数仅供外部调用，不在内部调用链中使用）
void FlowMeter_DisplayCurrent(void) {
    UpdateCurrentFlowDisplay();
}

// 显示累计流量（这个函数仅供外部调用，不在内部调用链中使用）
void FlowMeter_DisplayTotal(void) {
    UpdateTotalFlowDisplay();
}

// 设置流量显示模式
void FlowMeter_SetMode(BYTE mode) {
    flowMode = mode;
    
    // 仅设置更新标志，实际显示在主循环中执行
    needUpdateDisplay = 1;
}

// 获取当前流量显示模式
BYTE FlowMeter_GetMode(void) {
    return flowMode;
}

// 获取当前流量
WORD FlowMeter_GetCurrentFlow(void) {
    // 返回实际的毫升/秒值，用于定时浇水计算
    return currentFlow;  // 直接返回毫升/秒值
}

// 获取累计流量
unsigned long FlowMeter_GetTotalFlow(void) {
    return totalFlow;
}

// 外部中断0服务函数 - 用于脉冲计数
void INT0_ISR() interrupt 0 {
    // 简单计数，每次下降沿中断增加计数
    // 如果这里被调用太频繁，可能是：
    // 1. T0频率过高
    // 2. 继电器触点抖动
    // 3. 信号噪声
    pulseCount++;  // 每次中断增加脉冲计数
}
