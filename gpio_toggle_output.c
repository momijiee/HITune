#include "board.h"
#include "soft_timer.h"
#include "task_fsm.h"
#include "task_player.h"
#include "task_tracker.h"
#include "task_clock.h"
#include "task_display.h"
#include <stdio.h>

int main(void)
{
    /* ======== 1. 硬件初始化 ======== */
    board_init();       // GPIO, UART, etc.
    SysTick_Init();     // 1ms系统节拍

    /* ======== 2. 各任务初始化 ======== */
    TaskDisplay_Init(); // OLED (内含 OLED_Init)
    TaskFSM_Init();     // 语音状态机 + 播放器
    TaskTracker_Init(); // 追光舵机
    TaskClock_Init();   // 电子时钟

    printf("=== Flower Companion v2.0 (Non-Blocking) ===\r\n");

    /* ======== 3. 主循环 ======== */
    while (1)
    {
        TaskPlayer_Run();    // [最高] 播放器延时管理
        TaskFSM_Run();       // [高]   语音轮询 + 状态机
        TaskTracker_Run();   // [中]   追光ADC + 舵机
        TaskClock_Run();     // [低]   秒自增
        TaskDisplay_Run();   // [低]   OLED刷新
    }
}
