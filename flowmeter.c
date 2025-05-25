#include "flowmeter.h"
#include "intrins.h"

// 流量计参数
static BYTE flowMode = FLOW_MODE_OFF;     // 流量显示模式
static WORD pulseCount = 0;               // 当前流量脉冲计数
static WORD currentFlow = 0;              // 当前流量值（毫升/秒）- 修改单位为毫升/秒
static unsigned long totalFlow = 0;       // 累计流量值（毫升）
static bit isRunning = 0;                 // 流量计运行状态
static bit needUpdateDisplay = 0;         // 显示更新标志
static BYTE initialDisplayDelay = 0;      // 初始显示延迟计数器

// 流量计参数定义
#define PULSE_FACTOR 1                   // 降低脉冲因子，使每个脉冲代表更多水量（原为5）
#define FLOW_UPDATE_INTERVAL 1           // 更快地更新流量值（每秒更新）
#define FLOW_SPEED_MULTIPLIER 1          // 流量倍增因子

// 流量计初始化
void FlowMeter_Init(void) {
    IT0 = 1;                           // 设置INT0为边沿触发
    EX0 = 1;                           // 使能INT0中断
    pulseCount = 0;                    // 初始化脉冲计数
    currentFlow = 0;                   // 初始化当前流量
    totalFlow = 0;                     // 初始化总流量
    isRunning = 0;                     // 初始不运行
    flowMode = FLOW_MODE_OFF;          // 默认显示模式为关闭
    
    /* 流量计引脚连接说明：
     * - INT0 (P3.2) 作为脉冲信号输入，连接到继电器常开触点的一端
     * - 另一端连接到P1.0，当继电器闭合时接收来自方波发生器的脉冲
     * 
     * 浇水状态：
     * - 当浇水开始时，继电器闭合，P1.0的方波信号传导到P3.2
     * - INT0中断被触发并计数脉冲，模拟流量计测量水流
     * - 当浇水结束时，继电器断开，脉冲信号不再传导，INT0不再接收中断
     */
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
    totalFlow = 0;                     // 重置总流量
}

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
 *      例如：显示"047 5 "表示当前流量为47毫升/秒
 * 
 * 2. 累计流量显示模式（FLOW_MODE_TOTAL）:
 *    - 数码管从右到左依次为：位1-位6
 *    - 位1: 显示数字"1"，表示当前为累计流量显示模式
 *    - 位2-位6: 显示累计流量数值，单位为升
 *      例如：显示"00231"表示累计流量为23.1升
 * 
 * 注：系统会每3秒在这两种显示模式间自动切换
 */

// 计算流量（每秒调用一次，由中断触发）
void FlowMeter_CalcFlow(void) {
    static BYTE updateCounter = 0;
    
    if (initialDisplayDelay > 0) {
        initialDisplayDelay--;
        if (initialDisplayDelay == 0) {
            // 初始延迟结束后强制更新显示
            needUpdateDisplay = 1;
        }
    }
    
    // 每FLOW_UPDATE_INTERVAL秒更新一次当前流量
    if (++updateCounter >= FLOW_UPDATE_INTERVAL) {
        updateCounter = 0;
        
        if (isRunning) {
            // 计算当前流量（毫升/秒）- 添加乘数以增加流量
            if (pulseCount > 0) {
                // 将脉冲数转换为毫升/秒，添加FLOW_SPEED_MULTIPLIER增加流量
                currentFlow = (WORD)(pulseCount * 1000 * FLOW_SPEED_MULTIPLIER / (PULSE_FACTOR * FLOW_UPDATE_INTERVAL));
                
                // 确保有最小显示值
                if (currentFlow < 25) currentFlow = 25;  // 最小25毫升/秒
            } else {
                // 如果没有脉冲但系统仍在运行，保持最小流量显示
                currentFlow = 25;  // 显示25毫升/秒
            }
            
            // 更新累计流量（毫升），增加累积速度
            if (pulseCount > 0) {
                totalFlow += (unsigned long)pulseCount * 1000 * FLOW_SPEED_MULTIPLIER / PULSE_FACTOR;
            } else {
                totalFlow += 100;  // 每次增加100毫升
            }
        }
        
        // 重置脉冲计数，准备下次统计
        pulseCount = 0;
        
        // 设置显示需更新标志，但不在中断中更新
        needUpdateDisplay = 1;
    }
    
    // 轮流显示切换逻辑
    if (isRunning) {
        static BYTE displayToggle = 0;
        
        // 轮流显示当前流量和累计流量
        if (++displayToggle >= 3) {  
            displayToggle = 0;
            
            // 仅切换模式，不执行显示操作
            flowMode = (flowMode == FLOW_MODE_CURR) ? FLOW_MODE_TOTAL : FLOW_MODE_CURR;
            
            // 设置显示需更新标志
            needUpdateDisplay = 1;
        }
    }
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
    
    // 构造显示数字
    val1 = 10;  // 10，表示当前流量显示模式
    val2 = (currentFlow / 10000) % 10; // 万位（毫升/秒）
    val3 = (currentFlow / 1000) % 10; // 千位（毫升/秒）
    val4 = (currentFlow / 100) % 10;  // 百位（毫升/秒）
    val5 = (currentFlow / 10) % 10;  // 十位（毫升/秒）
    val6 = currentFlow % 10;     // 个位（毫升/秒）
    
    // 直接填充显示缓冲区
    FillCustomDispBuf(val6, val5, val4, val3, val2, val1);
}

// 更新累计流量显示
void UpdateTotalFlowDisplay(void) {
    BYTE val1, val2, val3, val4, val5, val6;
    
    // 提取总流量各位数字
    unsigned long flow_liter = totalFlow / 1000;  // 转换为升
    
    // 构造显示数字
    val1 = 11;  // 11,累计流量显示模式
    val2 = (BYTE)((flow_liter / 10000) % 10); // 万位（升）
    val3 = (BYTE)((flow_liter / 1000) % 10); // 千位（升）
    val4 = (BYTE)((flow_liter / 100) % 10);  // 百位（升）
    val5 = (BYTE)((flow_liter / 10) % 10);   // 十位（升）
    val6 = (BYTE)(flow_liter % 10);        // 个位（升）
    
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
    return currentFlow;
}

// 获取累计流量
unsigned long FlowMeter_GetTotalFlow(void) {
    return totalFlow;
}

// 外部中断0服务函数 - 用于脉冲计数
void INT0_ISR() interrupt 0 {
    pulseCount++;  // 每次中断增加脉冲计数
}
