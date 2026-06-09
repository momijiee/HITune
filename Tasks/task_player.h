#ifndef __TASK_PLAYER_H
#define __TASK_PLAYER_H

#include "board.h"
#include "bsp_player.h"
#include <stdbool.h>

void TaskPlayer_Init(void);
void TaskPlayer_Run(void);

/**
 * @brief  播放语音 + 可选延时跳转
 * @param  voice_id    语音编号
 * @param  wait_ms     播放后等待时间（0=不等待不跳转）
 * @param  next_state  等待结束后跳转的目标状态
 */
void TaskPlayer_PlayVoice(uint16_t voice_id, uint32_t wait_ms, uint8_t next_state);

/**
 * @brief  播放音乐（不影响延时跳转任务）
 */
void TaskPlayer_PlayMusic(uint16_t track_num);

/**
 * @brief  查询并取出一个已完成的延时跳转
 * @return true = 有待处理的跳转
 */
bool TaskPlayer_PopPendingState(uint8_t *out_state);

/**
 * @brief  当前是否有延时任务正在运行
 */
bool TaskPlayer_IsBusy(void);

/**
 * @brief  强制取消当前延时任务（用于状态机被用户语音打断时）
 */
void TaskPlayer_Cancel(void);

#endif
