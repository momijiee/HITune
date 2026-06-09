#include "soft_timer.h"

volatile uint32_t g_sys_tick_ms = 0;

void SysTick_Init(void)
{
    SysTick->LOAD  = (32000000UL / 1000UL) - 1UL;
    SysTick->VAL   = 0UL;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
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
