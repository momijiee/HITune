#ifndef __SOFT_TIMER_H
#define __SOFT_TIMER_H

#include "board.h"

extern volatile uint32_t g_sys_tick_ms;

void     SysTick_Init(void);
uint32_t GetTick(void);

/**
 * @brief 非阻塞时间判断，到期自动刷新基准
 */
static inline uint8_t IsTimeUp(uint32_t *last_tick, uint32_t interval_ms)
{
    uint32_t now = GetTick();
    if ((now - *last_tick) >= interval_ms) {
        *last_tick = now;
        return 1;
    }
    return 0;
}

/**
 * @brief 单次延时检测（不自动刷新，用于"等待一段时间后做某事"）
 */
static inline uint8_t HasElapsed(uint32_t start_tick, uint32_t duration_ms)
{
    return ((GetTick() - start_tick) >= duration_ms) ? 1 : 0;
}

#endif
