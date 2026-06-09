#include "task_display.h"
#include "task_clock.h"
#include "task_fsm.h"
#include "soft_timer.h"
#include "oled.h"
#include <stdio.h>

static uint32_t s_last_tick    = 0;
static uint8_t  s_colon_toggle = 1;
static uint8_t  s_last_state   = 0xFF;  // 强制首次刷新
static uint8_t  s_last_sec     = 0xFF;

#define DISPLAY_INTERVAL_MS  500

void TaskDisplay_Init(void)
{
    OLED_Init();
    OLED_Clear();
    s_last_tick = GetTick();
}

void TaskDisplay_Run(void)
{
    if (!IsTimeUp(&s_last_tick, DISPLAY_INTERVAL_MS)) return;

    Clock_t    time  = TaskClock_GetTime();
    PlayState_t state = TaskFSM_GetState();

    // ---- 脏检测：只有内容变化才重绘，减少SPI流量 ----
    uint8_t need_refresh = 0;

    // 第一行：时钟（每500ms闪烁冒号）
    {
        char buf[16];
        if (s_colon_toggle) {
            sprintf(buf, "%02d:%02d:%02d", time.hour, time.min, time.sec);
        } else {
            sprintf(buf, "%02d %02d %02d", time.hour, time.min, time.sec);
        }
        s_colon_toggle = !s_colon_toggle;

        // 时钟区域清除+重绘
        OLED_ShowString(32, 0, (u8 *)buf, 16, 1);
        need_refresh = 1;
    }

    // 第三行：状态机当前状态
    if ((uint8_t)state != s_last_state) {
        s_last_state = (uint8_t)state;

        // 清除状态行区域（用空格覆盖）
        OLED_ShowString(0, 40, (u8 *)"                ", 12, 1);
        
        // 显示状态名
        const char *name = TaskFSM_GetStateName();
        OLED_ShowString(0, 40, (u8 *)name, 12, 1);

        // 显示状态编号
        char state_num[8];
        sprintf(state_num, "[%02d]", (int)state);
        OLED_ShowString(90, 40, (u8 *)state_num, 12, 1);

        need_refresh = 1;
    }

    if (need_refresh) {
        OLED_Refresh();
    }
}