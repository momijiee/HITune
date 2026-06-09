#include "task_player.h"
#include "soft_timer.h"

typedef struct {
    bool     active;
    uint32_t start_tick;
    uint32_t wait_ms;
    uint8_t  next_state;
    bool     pending_done;
} PlayerTask_t;

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

    if (HasElapsed(s_task.start_tick, s_task.wait_ms)) {
        s_task.active       = false;
        s_task.pending_done = true;
    }
}

void TaskPlayer_PlayVoice(uint16_t voice_id, uint32_t wait_ms, uint8_t next_state)
{
    // 先发送播放指令
    Player_PlayVoice(voice_id);

    // 设置延时跳转（会覆盖之前未完成的任务——这是设计意图）
    if (wait_ms > 0) {
        s_task.active       = true;
        s_task.start_tick   = GetTick();
        s_task.wait_ms      = wait_ms;
        s_task.next_state   = next_state;
        s_task.pending_done = false;
    }
    // wait_ms == 0 时不设置延时任务，也不清除已有任务
}

void TaskPlayer_PlayMusic(uint16_t track_num)
{
    // 播放音乐，不影响延时跳转任务
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

void TaskPlayer_Cancel(void)
{
    s_task.active       = false;
    s_task.pending_done = false;
}
