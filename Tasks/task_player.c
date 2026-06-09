#include "task_player.h"
#include "soft_timer.h"

/*============ 结构体定义（仅本文件可见） ============*/
typedef struct {
    uint8_t  active;          // 1=延时进行中, 0=空闲（用uint8_t替代bool）
    uint32_t start_tick;
    uint32_t wait_ms;
    uint8_t  next_state;
    uint8_t  pending_done;    // 1=延时已完成待读取（用uint8_t替代bool）
} PlayerTask_t;

/*============ 静态变量（不用 {0} 整体初始化） ============*/
static PlayerTask_t s_task;

/*============ 初始化 ============*/
void TaskPlayer_Init(void)
{
    s_task.active       = 0;
    s_task.start_tick   = 0;
    s_task.wait_ms      = 0;
    s_task.next_state   = 0;
    s_task.pending_done = 0;

    Player_SetVolume(20);
}

/*============ 周期运行 ============*/
void TaskPlayer_Run(void)
{
    if (!s_task.active) return;

    if (HasElapsed(s_task.start_tick, s_task.wait_ms)) {
        s_task.active       = 0;
        s_task.pending_done = 1;
    }
}

/*============ 播放语音（可选延时跳转） ============*/
void TaskPlayer_PlayVoice(uint16_t voice_id, uint32_t wait_ms, uint8_t next_state)
{
    Player_PlayVoice(voice_id);

    if (wait_ms > 0) {
        s_task.active       = 1;
        s_task.start_tick   = GetTick();
        s_task.wait_ms      = wait_ms;
        s_task.next_state   = next_state;
        s_task.pending_done = 0;
    }
}

/*============ 播放音乐（不影响延时任务） ============*/
void TaskPlayer_PlayMusic(uint16_t track_num)
{
    Player_PlayMusic(track_num);
}

/*============ 取出已完成的跳转目标 ============*/
bool TaskPlayer_PopPendingState(uint8_t *out_state)
{
    if (s_task.pending_done) {
        *out_state = s_task.next_state;
        s_task.pending_done = 0;
        return true;
    }
    return false;
}

/*============ 查询是否忙碌 ============*/
bool TaskPlayer_IsBusy(void)
{
    return (s_task.active != 0);
}
