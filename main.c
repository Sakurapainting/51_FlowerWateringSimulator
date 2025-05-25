#include <reg51.h>
#include <INTRINS.H>
#include "pca.h"
#include "relay.h"  // 包含继电器控制头文件

#define multiplier 1.085

void main(void) {
    EA = 1;            // 开启总中断
    P0 = 0xFF;
    
    PCA_Init();        // 初始化PCA模块，包含时钟功能
    Relay_Init();      // 初始化继电器

    while (1) {
        // // 10s切换一次继电器状态
        // if((SysPara1.sec % 10) == 0 && SysPara1.sec != 0) {
        //     // 获取当前继电器状态并切换
        //     if(Relay_GetState()) {
        //         Relay_Off();  // 如果继电器开着，就关闭它
        //     } else {
        //         Relay_On();   // 如果继电器关着，就开启它
        //     }
            
        //     // 短暂延时，避免在同一秒内多次切换
        //     delay_ms(500);
        // }
    }
}