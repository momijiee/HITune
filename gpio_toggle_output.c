#include "board.h"
#include "soft_timer.h"
#include "task_fsm.h"
#include "task_player.h"
#include "task_tracker.h"
#include "task_clock.h"
#include "task_display.h"
#include "bsp_adc.h"
#include <stdio.h>

int main(void)
{
    /* ======== 1. 硬件初始化 ======== */
    board_init();       // 内部 SYSCFG_DL_init() → SysTick LOAD 被设成31

    /* ======== 2. 立刻重配 SysTick（覆盖 sysconfig 的配置） ======== */
    SysTick_Init();     // LOAD = 31999, 1ms中断

    /* ======== 3. 验证 ======== */
    printf("SysTick LOAD = %lu (expect 31999)\r\n", (unsigned long)SysTick->LOAD);

    {
        uint32_t t0 = GetTick();
        delay_ms(1000);
        uint32_t t1 = GetTick();
        printf("1s test: tick diff = %lu (expect ~1000)\r\n",
               (unsigned long)(t1 - t0));
    }

    /* ======== 4. 各任务初始化 ======== */
    TaskDisplay_Init();     // OLED初始化（现在delay_ms正常了）
    TaskFSM_Init();
    TaskTracker_Init();
    TaskClock_Init();

    printf("=== Flower Companion v2.2 ===\r\n");

    /* ======== 5. 主循环 ======== */
    while (1)
    {
        TaskPlayer_Run();
        TaskFSM_Run();
        TaskTracker_Run();
        TaskClock_Run();
        TaskDisplay_Run();
    }
}

/* ======== 中断 ======== */
void ADC_INST_IRQHandler(void)
{
    BSP_ADC_IRQHandler();
}
