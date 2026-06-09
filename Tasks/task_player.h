#ifndef __TASK_PLAYER_H
#define __TASK_PLAYER_H

#include "board.h"
#include "bsp_player.h"
#include <stdbool.h>

void TaskPlayer_Init(void);
void TaskPlayer_Run(void);

void TaskPlayer_PlayVoice(uint16_t voice_id, uint32_t wait_ms, uint8_t next_state);
void TaskPlayer_PlayMusic(uint16_t track_num);

bool TaskPlayer_PopPendingState(uint8_t *out_state);
bool TaskPlayer_IsBusy(void);

#endif
