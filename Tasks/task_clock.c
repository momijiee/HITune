#include "task_clock.h"
#include "soft_timer.h"

static Clock_t  s_clock = {12, 0, 0};
static uint32_t s_tick_1s = 0;

void TaskClock_Init(void)
{
    s_clock.hour = 12; s_clock.min = 0; s_clock.sec = 0;
    s_tick_1s = GetTick();
}

void TaskClock_Run(void)
{
    if (!IsTimeUp(&s_tick_1s, 1000)) return;

    s_clock.sec++;
    if (s_clock.sec >= 60) { s_clock.sec = 0; s_clock.min++; }
    if (s_clock.min >= 60) { s_clock.min = 0; s_clock.hour++; }
    if (s_clock.hour >= 24) { s_clock.hour = 0; }
}

Clock_t TaskClock_GetTime(void) { return s_clock; }

void TaskClock_SetTime(uint8_t h, uint8_t m, uint8_t s)
{
    s_clock.hour = h % 24;
    s_clock.min  = m % 60;
    s_clock.sec  = s % 60;
}
