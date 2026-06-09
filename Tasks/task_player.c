#include "task_player.h"
#include "soft_timer.h"

static PlayerTask_t s_task = {0};

void TaskPlayer_Init(void)
{
    s_task.active       = false;
    s_task.pending_done = false;
    Player_SetVolume(20);
}

void TaskPlayer_Run(void)
{
    if (!s_task.active) return;

    // 检查延时是否到期
    if (HasElapsed(s_task.start_tick, s_task.wait_ms)) {
        s_task.active       = false;
        s_task.pending_done = true;  // 标记完成，等状态机来取
    }
}

void TaskPlayer_PlayVoice(uint16_t voice_id, uint32_t wait_ms, uint8_t next_state)
{
    Player_PlayVoice(voice_id);

    if (wait_ms > 0) {
        s_task.active       = true;
        s_task.start_tick   = GetTick();
        s_task.wait_ms      = wait_ms;
        s_task.next_state   = next_state;
        s_task.pending_done = false;
    }
}

void TaskPlayer_PlayMusic(uint16_t track_num)
{
    Player_PlayMusic(track_num);
}

bool TaskPlayer_PopPendingState(uint8_t *out_state)
{
    if (s_task.pending_done) {
        *out_state = s_task.next_state;
        s_task.pending_done = false;
        return true;
    }
    return false;
}

bool TaskPlayer_IsBusy(void)
{
    return s_task.active;
}
