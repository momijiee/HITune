#ifndef __TASK_PLAYER_H
#define __TASK_PLAYER_H

#include "board.h"
#include "bsp_player.h"
#include <stdbool.h>

/**
 * @brief 播放请求结构体
 *        播放一条语音/音乐，等待指定毫秒后，自动将状态机跳转到 next_state
 */
typedef struct {
    bool     active;           // 当前是否有延时任务在跑
    uint32_t start_tick;       // 开始时刻
    uint32_t wait_ms;          // 需要等待的毫秒数
    uint8_t  next_state;       // 等待结束后要跳转的状态 (PlayState_t)
    bool     pending_done;     // 标记：延时已到，等待状态机读取
} PlayerTask_t;

void TaskPlayer_Init(void);
void TaskPlayer_Run(void);

/**
 * @brief  发起一次"播放语音 + 延时后自动跳转"请求
 * @param  voice_id    要播放的语音编号
 * @param  wait_ms     播放后等待的毫秒数（0 = 不等待，不自动跳转）
 * @param  next_state  等待结束后自动跳转的目标状态（仅 wait_ms > 0 时有效）
 */
void TaskPlayer_PlayVoice(uint16_t voice_id, uint32_t wait_ms, uint8_t next_state);

/**
 * @brief  发起一次"播放音乐"请求（不带延时跳转）
 */
void TaskPlayer_PlayMusic(uint16_t track_num);

/**
 * @brief  查询是否有延时跳转已完成
 * @param  out_state  输出：目标状态
 * @return true = 有一个跳转待处理
 */
bool TaskPlayer_PopPendingState(uint8_t *out_state);

/**
 * @brief  当前是否正在等待延时
 */
bool TaskPlayer_IsBusy(void);

#endif
