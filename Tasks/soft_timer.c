#include "soft_timer.h"

volatile uint32_t g_sys_tick_ms = 0;

void SysTick_Init(void)
{
    /* 1. 完全停止 SysTick（清除 ENABLE + TICKINT） */
    SysTick->CTRL = 0;
    
    /* 2. 清除当前计数值 */
    SysTick->VAL = 0UL;
    
    /* 3. 设置正确的重装载值：1ms = CPUCLK / 1000 - 1 */
    SysTick->LOAD = (CPUCLK_FREQ / 1000UL) - 1UL;  // = 31999
    
    /* 4. 清零计数器（写任意值到VAL会清零） */
    SysTick->VAL = 0UL;
    
    /* 5. 清除可能残留的 SysTick 挂起中断 */
    SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
    
    /* 6. 重置计数变量 */
    g_sys_tick_ms = 0;
    
    /* 7. 最后才启动（CPU时钟源 + 中断使能 + 计数器使能） */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;
}

uint32_t GetTick(void)
{
    return g_sys_tick_ms;
}

void SysTick_Handler(void)
{
    g_sys_tick_ms++;
}