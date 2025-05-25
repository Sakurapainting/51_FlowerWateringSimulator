#ifndef __WAVEGEN_H__
#define __WAVEGEN_H__

#include "reg51.h"
#include "pca.h"  // 包含BYTE类型定义

// 方波发生器相关定义
#define WAVE_PIN P1_0      // 方波输出引脚
#define WAVE_FREQ 5        // 方波频率 (Hz)

// 函数声明
void WaveGen_Init(void);   // 初始化方波发生器
void WaveGen_Start(void);  // 启动方波发生器
void WaveGen_Stop(void);   // 停止方波发生器
BYTE WaveGen_GetState(void); // 获取方波发生器状态

#endif /* __WAVEGEN_H__ */
