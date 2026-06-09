#ifndef __SOFT_TIMER_H
#define __SOFT_TIMER_H

#include "board.h"

#ifndef CPUCLK_FREQ
#define CPUCLK_FREQ     32000000UL
#endif

extern volatile uint32_t g_sys_tick_ms;

void     SysTick_Init(void);
uint32_t GetTick(void);

static inline uint8_t IsTimeUp(uint32_t *last_tick, uint32_t interval_ms)
{
    uint32_t now = GetTick();
    if ((now - *last_tick) >= interval_ms) {
        *last_tick = now;
        return 1;
    }
    return 0;
}

static inline uint8_t HasElapsed(uint32_t start_tick, uint32_t duration_ms)
{
    return ((GetTick() - start_tick) >= duration_ms) ? 1 : 0;
}

#endif
