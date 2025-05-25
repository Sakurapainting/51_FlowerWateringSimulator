#include <reg51.h>
#include <INTRINS.H>
#include "pca.h"

#define multiplier 1.085

void main(void) {
    EA = 1;            // 开启总中断
    P0 = 0xFF;
    
    // 验证初始时间是否合法
    if(!checktime(0, 0, 0)) {
        // 如果时间不合法

    }
    
    PCA_Init();        // 初始化PCA模块，包含时钟功能

    while (1) {
        // 主循环中不需要处理时钟显示
        // PCA中断会自动处理时钟和刷新显示
    }
}