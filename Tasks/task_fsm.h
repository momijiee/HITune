#ifndef __TASK_FSM_H
#define __TASK_FSM_H

#include "board.h"
#include "bsp_voice.h"
#include <stdbool.h>

/* ---- 状态枚举 ---- */
typedef enum {
    ST_IDLE = 0,
    ST_WAKE,
    ST_TIME,
    ST_BYE,

    ST_SAD_RECV,
    ST_PLAY_SAD_MUSIC,
    ST_SAD_TALK,
    ST_SAD_BETTER,
    ST_SAD_STILL,
    ST_SAD_REBEL,
    ST_SAD_DEEP,
    ST_SILENT_GUARD,
    ST_WAKE_UP_SUN,

    ST_HAPPY_RECV,
    ST_HAPPY_CELEBRATE,
    ST_HAPPY_SHY,
    ST_HAPPY_END,
    ST_HAPPY_QUIET,

    ST_JOKE_1,
    ST_JOKE_1_GOOD,
    ST_JOKE_1_BAD,
    ST_JOKE_2,
    ST_JOKE_2_GOOD,
    ST_JOKE_2_BAD,
    ST_JOKE_3,
    ST_JOKE_END,
    ST_JOKE_WIN,
    ST_JOKE_LOSE,

    ST_GAME_START,
    ST_GAME_Q1,
    ST_GAME_Q2_A,
    ST_GAME_Q2_B,
    ST_RES_SUNFLOWER,
    ST_RES_CACTUS,
    ST_RES_MIMOSA,
    ST_RES_MUSHROOM,
    ST_GAME_OVER,
    ST_GAME_HAPPY,
    ST_GAME_SAD,

    ST_CONFUSED,
    ST_EGG_1,
    ST_EGG_2,

    ST_COUNT  // 状态总数
} PlayState_t;

/* ---- 事件枚举 ---- */
typedef enum {
    EVENT_NONE = 0,
    EVENT_HELLO,
    EVENT_TIME,
    EVENT_SAD,
    EVENT_HAPPY,
    EVENT_JOKE,
    EVENT_GAME,
    EVENT_YES,
    EVENT_NO,
    EVENT_A,
    EVENT_B,
    EVENT_TIMEOUT,
    EVENT_PLAYER_DONE  // 新增：播放器延时完成事件
} StateEvent_t;

/* ---- API ---- */
void       TaskFSM_Init(void);
void       TaskFSM_Run(void);

PlayState_t TaskFSM_GetState(void);
const char* TaskFSM_GetStateName(void);  // 供OLED显示用

#endif
