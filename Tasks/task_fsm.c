#include "task_fsm.h"
#include "task_player.h"
#include "soft_timer.h"
#include "bsp_voice.h"
#include <stdio.h>

/*============ 内部变量 ============*/
static PlayState_t  s_state              = ST_IDLE;
static uint32_t     s_voice_poll_tick    = 0;
static uint32_t     s_timeout_start      = 0;   // 改用绝对起始时刻
static uint8_t      s_hello_repeat_count = 0;
static uint8_t      s_last_voice_id      = 0x00;

#define VOICE_POLL_MS       100
#define TIMEOUT_MS          10000

/*============ 内部函数 ============*/
static StateEvent_t MapVoiceIdToEvent(uint8_t voice_id);
static void         FSM_Process(StateEvent_t event);
static void         FSM_EnterState(PlayState_t new_state);
static void         FSM_OnEnter(PlayState_t entered_state); 

/*============ 状态名称表 ============*/
static const char* s_state_names[] = {
    [ST_IDLE]            = "Sleeping",
    [ST_WAKE]            = "Awake",
    [ST_TIME]            = "Time",
    [ST_BYE]             = "Bye",
    [ST_SAD_RECV]        = "Sad?",
    [ST_PLAY_SAD_MUSIC]  = "Healing",
    [ST_SAD_TALK]        = "Comfort",
    [ST_SAD_BETTER]      = "Better?",
    [ST_SAD_STILL]       = "StillSad",
    [ST_SAD_REBEL]       = "Rebel",
    [ST_SAD_DEEP]        = "Deep",
    [ST_SILENT_GUARD]    = "Guard",
    [ST_WAKE_UP_SUN]     = "WakeUp",
    [ST_HAPPY_RECV]      = "Happy!",
    [ST_HAPPY_CELEBRATE] = "Party!",
    [ST_HAPPY_SHY]       = "Shy",
    [ST_HAPPY_END]       = "HappyEnd",
    [ST_HAPPY_QUIET]     = "Quiet",
    [ST_JOKE_1]          = "Joke1",
    [ST_JOKE_1_GOOD]     = "J1Good",
    [ST_JOKE_1_BAD]      = "J1Bad",
    [ST_JOKE_2]          = "Joke2",
    [ST_JOKE_2_GOOD]     = "J2Good",
    [ST_JOKE_2_BAD]      = "J2Bad",
    [ST_JOKE_3]          = "Joke3",
    [ST_JOKE_END]        = "JokeEnd",
    [ST_JOKE_WIN]        = "JokeWin",
    [ST_JOKE_LOSE]       = "JokeLose",
    [ST_GAME_START]      = "Game?",
    [ST_GAME_Q1]         = "Q1",
    [ST_GAME_Q2_A]       = "Q2-A",
    [ST_GAME_Q2_B]       = "Q2-B",
    [ST_RES_SUNFLOWER]   = "Sunflwr",
    [ST_RES_CACTUS]      = "Cactus",
    [ST_RES_MIMOSA]      = "Mimosa",
    [ST_RES_MUSHROOM]    = "Mushroom",
    [ST_GAME_OVER]       = "GameEnd",
    [ST_GAME_HAPPY]      = "GameYay",
    [ST_GAME_SAD]        = "GameSad",
    [ST_CONFUSED]        = "Confused",
    [ST_EGG_1]           = "Easter1",
    [ST_EGG_2]           = "Easter2",
};

/*============ 状态切换统一入口（带调试打印） ============*/
static void FSM_EnterState(PlayState_t new_state)
{
    if (s_state != new_state) {
        printf("FSM| %s -> %s\r\n",
               (s_state < ST_COUNT && s_state_names[s_state]) ? s_state_names[s_state] : "?",
               (new_state < ST_COUNT && s_state_names[new_state]) ? s_state_names[new_state] : "?");
        s_state = new_state;
        s_timeout_start = GetTick();  // 每次进入新状态重置超时计时
    }
}

/*============ 初始化 ============*/
void TaskFSM_Init(void)
{
    s_state           = ST_IDLE;
    s_voice_poll_tick = GetTick();
    s_timeout_start   = GetTick();
    s_last_voice_id   = 0x00;
    TaskPlayer_Init();
}

/*============ 主运行函数 ============*/
void TaskFSM_Run(void)
{
    /*
     * 修复核心：一轮只处理一个事件
     * 优先级：PLAYER_DONE > 语音输入 > 超时
     */
    StateEvent_t event = EVENT_NONE;

    /* ---- 优先级1：播放器延时完成 ---- */
    uint8_t pending_state;
    if (TaskPlayer_PopPendingState(&pending_state)) {
        /*
         * 关键修复：不在这里直接改 s_state
         * 而是让 FSM_Process 在当前状态下处理 PLAYER_DONE 事件
         * 同时把目标状态传进去
         */
        FSM_EnterState((PlayState_t)pending_state);
        
        /*
         * 某些状态进入后需要立即播放语音（入口动作）
         * 在这里统一处理
         */
        FSM_OnEnter((PlayState_t)pending_state);
        return;  // 本轮只处理这一个事件
    }

    /* ---- 优先级2：定时轮询语音 ---- */
    if (!IsTimeUp(&s_voice_poll_tick, VOICE_POLL_MS)) {
        return;
    }

    /* 播放器忙时不接受新语音输入 */
    if (TaskPlayer_IsBusy()) {
        Voice_Module_ReadID();  // 读取并丢弃，防残留
        return;
    }

    uint8_t cur_id = Voice_Module_ReadID();

    if (cur_id != 0x00 && cur_id != s_last_voice_id) {
        event = MapVoiceIdToEvent(cur_id);
        if (event != EVENT_NONE) {
            s_timeout_start = GetTick();  // 重置超时
            printf("FSM| Voice:0x%02X -> Event:%d\r\n", cur_id, event);
        }
    }
    else if (cur_id == 0x00) {
        /* 超时检测 */
        if (s_state != ST_IDLE && s_state != ST_SILENT_GUARD) {
            if (HasElapsed(s_timeout_start, TIMEOUT_MS)) {
                event = EVENT_TIMEOUT;
                s_timeout_start = GetTick();
                printf("FSM| Timeout in state %s\r\n",
                       s_state_names[s_state] ? s_state_names[s_state] : "?");
            }
        }
    }

    s_last_voice_id = cur_id;

    /* ---- 驱动状态机 ---- */
    if (event != EVENT_NONE) {
        FSM_Process(event);
    }
}

PlayState_t TaskFSM_GetState(void)
{
    return s_state;
}

const char* TaskFSM_GetStateName(void)
{
    if (s_state < ST_COUNT && s_state_names[s_state] != NULL) {
        return s_state_names[s_state];
    }
    return "Unknown";
}

/*============ 语音ID映射（不变） ============*/
static StateEvent_t MapVoiceIdToEvent(uint8_t voice_id)
{
    switch(voice_id)
    {
        case 0x10: case 0x11: case 0x12: return EVENT_HELLO;
        case 0x20:                        return EVENT_TIME;
        case 0x30: case 0x31: case 0x32: case 0x33: return EVENT_SAD;
        case 0x40: case 0x41:             return EVENT_HAPPY;
        case 0x50: case 0x51:             return EVENT_JOKE;
        case 0x60:                        return EVENT_GAME;
        case 0x70: case 0x71: case 0x72: case 0x73: return EVENT_YES;
        case 0x80: case 0x81: case 0x82: case 0x83: return EVENT_NO;
        case 0xA0: case 0xA1:             return EVENT_A;
        case 0xB0: case 0xB1:             return EVENT_B;
        default:                           return EVENT_NONE;
    }
}

/*============================================================
 * 状态入口动作：当 TaskPlayer 延时跳转完成进入新状态时
 * 某些状态需要立即播放语音
 *============================================================*/
static void FSM_OnEnter(PlayState_t entered_state)
{
    switch (entered_state)
    {
        case ST_IDLE:
            /* 回到待机，无需播放 */
            break;

        case ST_WAKE:
            /* 从各种分支返回主菜单，无需额外播放 */
            break;

        case ST_JOKE_2:
            /* 从 JOKE_1_BAD 延时跳转过来，需要播放笑话2 */
            TaskPlayer_PlayVoice(34, 0, 0);
            break;

        case ST_JOKE_3:
            /* 从 JOKE_2_BAD 延时跳转过来，需要播放笑话3 */
            TaskPlayer_PlayVoice(37, 0, 0);
            break;

        case ST_GAME_START:
            /* 从 JOKE_LOSE 延时跳转过来 */
            TaskPlayer_PlayVoice(41, 0, 0);
            break;

        case ST_GAME_OVER:
            /* 从结果状态延时跳转过来，播放结算语音 */
            TaskPlayer_PlayVoice(49, 0, 0);
            break;

        case ST_SILENT_GUARD:
            /* 进入静默守护，无需播放 */
            break;

        default:
            break;
    }
}

/*============================================================
 * 核心状态机（简化清晰版）
 * 
 * 规则：
 * 1. 每个 case 只处理用户语音事件和超时事件
 * 2. PLAYER_DONE 导致的状态跳转由 FSM_OnEnter 处理
 * 3. 不再有递归调用
 * 4. 状态切换统一用 FSM_EnterState()
 *============================================================*/
static void FSM_Process(StateEvent_t event)
{
    /* ---- 彩蛋检测 ---- */
    if (event == EVENT_HELLO && s_state != ST_IDLE && s_state != ST_SILENT_GUARD) {
        s_hello_repeat_count++;
        if (s_hello_repeat_count >= 3) {
            FSM_EnterState(ST_EGG_1);
            TaskPlayer_PlayVoice(53, 5000, ST_WAKE);
            s_hello_repeat_count = 0;
            return;
        }
    } else if (event != EVENT_TIMEOUT) {
        s_hello_repeat_count = 0;
    }

    switch (s_state)
    {
        /* ===== 基础 ===== */
        case ST_IDLE:
            if (event == EVENT_HELLO) {
                FSM_EnterState(ST_WAKE);
                TaskPlayer_PlayVoice(2, 0, 0);
            } else if (event == EVENT_SAD) {
                FSM_EnterState(ST_SAD_RECV);
                TaskPlayer_PlayVoice(11, 0, 0);
            }
            break;

        case ST_WAKE:
            if (event == EVENT_TIME) {
                FSM_EnterState(ST_TIME);
                TaskPlayer_PlayVoice(3, 5000, ST_WAKE);
            } else if (event == EVENT_SAD) {
                FSM_EnterState(ST_SAD_RECV);
                TaskPlayer_PlayVoice(11, 0, 0);
            } else if (event == EVENT_HAPPY) {
                FSM_EnterState(ST_HAPPY_RECV);
                TaskPlayer_PlayVoice(21, 0, 0);
            } else if (event == EVENT_JOKE) {
                FSM_EnterState(ST_JOKE_1);
                TaskPlayer_PlayVoice(31, 0, 0);
            } else if (event == EVENT_GAME) {
                FSM_EnterState(ST_GAME_START);
                TaskPlayer_PlayVoice(41, 0, 0);
            } else if (event == EVENT_TIMEOUT || event == EVENT_NO) {
                FSM_EnterState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_TIME:
            /* 延时跳转回 ST_WAKE，此处无需处理 */
            break;

        /* ===== 负面情绪 ===== */
        case ST_SAD_RECV:
            if (event == EVENT_YES || event == EVENT_SAD) {
                FSM_EnterState(ST_PLAY_SAD_MUSIC);
                TaskPlayer_PlayMusic(1);
                TaskPlayer_PlayVoice(12, 0, 0);
            } else if (event == EVENT_NO) {
                FSM_EnterState(ST_SAD_TALK);
                TaskPlayer_PlayVoice(13, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_PLAY_SAD_MUSIC:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                FSM_EnterState(ST_SAD_BETTER);
                TaskPlayer_PlayVoice(14, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_SAD) {
                FSM_EnterState(ST_SAD_STILL);
                TaskPlayer_PlayVoice(15, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_SAD_BETTER);
                TaskPlayer_PlayVoice(14, 0, 0);
            }
            break;

        case ST_SAD_TALK:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_SAD_BETTER);
                TaskPlayer_PlayVoice(14, 0, 0);
            } else if (event == EVENT_NO) {
                FSM_EnterState(ST_SAD_REBEL);
                TaskPlayer_PlayVoice(16, 3000, ST_IDLE);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_SAD_BETTER:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_JOKE_1);
                TaskPlayer_PlayVoice(31, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_SAD_STILL:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            } else if (event == EVENT_NO) {
                FSM_EnterState(ST_SAD_DEEP);
                TaskPlayer_PlayVoice(17, 6000, ST_SILENT_GUARD);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_SILENT_GUARD);
                TaskPlayer_PlayVoice(17, 0, 0);
            }
            break;

        case ST_SILENT_GUARD:
            if (event == EVENT_HELLO) {
                FSM_EnterState(ST_WAKE_UP_SUN);
                TaskPlayer_PlayVoice(19, 3000, ST_WAKE);
            }
            break;

        /* ===== 正面情绪 ===== */
        case ST_HAPPY_RECV:
            if (event == EVENT_HAPPY || event == EVENT_YES) {
                FSM_EnterState(ST_HAPPY_CELEBRATE);
                TaskPlayer_PlayMusic(2);
                TaskPlayer_PlayVoice(22, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_HAPPY_SHY);
                TaskPlayer_PlayVoice(23, 4000, ST_WAKE);
            }
            break;

        case ST_HAPPY_CELEBRATE:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                FSM_EnterState(ST_HAPPY_END);
                TaskPlayer_PlayVoice(24, 3000, ST_IDLE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_HAPPY_QUIET);
                TaskPlayer_PlayVoice(25, 3000, ST_WAKE);
            }
            break;

        /* ===== 冷笑话 ===== */
        case ST_JOKE_1:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                FSM_EnterState(ST_JOKE_1_GOOD);
                TaskPlayer_PlayVoice(32, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_JOKE_1_BAD);
                TaskPlayer_PlayVoice(33, 3000, ST_JOKE_2);
            }
            break;

        case ST_JOKE_1_GOOD:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_JOKE_2);
                TaskPlayer_PlayVoice(34, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_WAKE);
            }
            break;

        /* ST_JOKE_1_BAD: 延时自动跳 ST_JOKE_2，FSM_OnEnter 播放 voice 34 */

        case ST_JOKE_2:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                FSM_EnterState(ST_JOKE_2_GOOD);
                TaskPlayer_PlayVoice(35, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_JOKE_2_BAD);
                TaskPlayer_PlayVoice(36, 3000, ST_JOKE_3);
            }
            break;

        case ST_JOKE_2_GOOD:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_JOKE_3);
                TaskPlayer_PlayVoice(37, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_WAKE);
            }
            break;

        /* ST_JOKE_2_BAD: 延时自动跳 ST_JOKE_3，FSM_OnEnter 播放 voice 37 */

        case ST_JOKE_3:
            /* 笑话3播完后等一会儿自动进入结算 */
            if (event == EVENT_YES || event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_JOKE_END);
                TaskPlayer_PlayVoice(38, 0, 0);
            }
            break;

        case ST_JOKE_END:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_JOKE_WIN);
                TaskPlayer_PlayVoice(39, 3000, ST_WAKE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_JOKE_LOSE);
                TaskPlayer_PlayVoice(40, 4000, ST_GAME_START);
            }
            break;

        /* ST_JOKE_WIN/LOSE: 延时自动跳转，FSM_OnEnter 处理 */

        /* ===== 游戏 ===== */
        case ST_GAME_START:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_GAME_Q1);
                TaskPlayer_PlayVoice(42, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_WAKE);
            }
            break;

        case ST_GAME_Q1:
            if (event == EVENT_A) {
                FSM_EnterState(ST_GAME_Q2_A);
                TaskPlayer_PlayVoice(43, 0, 0);
            } else if (event == EVENT_B) {
                FSM_EnterState(ST_GAME_Q2_B);
                TaskPlayer_PlayVoice(44, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_WAKE);
            }
            break;

        case ST_GAME_Q2_A:
            if (event == EVENT_A) {
                FSM_EnterState(ST_RES_SUNFLOWER);
                TaskPlayer_PlayVoice(45, 8000, ST_GAME_OVER);
            } else if (event == EVENT_B) {
                FSM_EnterState(ST_RES_CACTUS);
                TaskPlayer_PlayVoice(46, 8000, ST_GAME_OVER);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_WAKE);
            }
            break;

        case ST_GAME_Q2_B:
            if (event == EVENT_A) {
                FSM_EnterState(ST_RES_MIMOSA);
                TaskPlayer_PlayVoice(47, 8000, ST_GAME_OVER);
            } else if (event == EVENT_B) {
                FSM_EnterState(ST_RES_MUSHROOM);
                TaskPlayer_PlayVoice(48, 8000, ST_GAME_OVER);
            } else if (event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_WAKE);
            }
            break;

        /* ST_RES_xxx: 延时自动跳 ST_GAME_OVER，FSM_OnEnter 播放 voice 49 */
        case ST_RES_SUNFLOWER:
        case ST_RES_CACTUS:
        case ST_RES_MIMOSA:
        case ST_RES_MUSHROOM:
            /* 正在播放结果，等延时跳转即可 */
            break;

        case ST_GAME_OVER:
            if (event == EVENT_YES) {
                FSM_EnterState(ST_GAME_HAPPY);
                TaskPlayer_PlayVoice(50, 3000, ST_WAKE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                FSM_EnterState(ST_GAME_SAD);
                TaskPlayer_PlayVoice(51, 3000, ST_WAKE);
            }
            break;

        /* ST_GAME_HAPPY/SAD: 延时自动跳 ST_WAKE */
        case ST_GAME_HAPPY:
        case ST_GAME_SAD:
        case ST_JOKE_WIN:
        case ST_JOKE_LOSE:
        case ST_JOKE_1_BAD:
        case ST_JOKE_2_BAD:
        case ST_BYE:
        case ST_SAD_REBEL:
        case ST_SAD_DEEP:
        case ST_WAKE_UP_SUN:
        case ST_HAPPY_SHY:
        case ST_HAPPY_END:
        case ST_HAPPY_QUIET:
        case ST_EGG_1:
            /* 这些状态都在等待 TaskPlayer 延时跳转，不响应用户输入 */
            break;

        default:
            FSM_EnterState(ST_WAKE);
            break;
    }
}
