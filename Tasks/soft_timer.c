#include "soft_timer.h"

volatile uint32_t g_sys_tick_ms = 0;

/**
 * @brief  重新配置 SysTick 为 1ms 中断
 *         必须在 SYSCFG_DL_init() 之后调用，覆盖 sysconfig 的错误配置
 *         同时保证 delay_us 能正常工作（delay_us 读取 LOAD 和 VAL）
 */
void SysTick_Init(void)
{
    /* 先完全停止 */
    SysTick->CTRL = 0;

    /* LOAD = CPUCLK / 1000 - 1 = 31999，每1ms中断一次 */
    SysTick->LOAD = (CPUCLK_FREQ / 1000UL) - 1UL;
    SysTick->VAL  = 0UL;

    /* 启动：CPU时钟源 + 使能中断 + 使能计数器 */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk   |
                    SysTick_CTRL_ENABLE_Msk;

    g_sys_tick_ms = 0;
}

uint32_t GetTick(void)
{
    return g_sys_tick_ms;
}

void SysTick_Handler(void)
{
    g_sys_tick_ms++;
}
