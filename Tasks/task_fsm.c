#include "task_fsm.h"
#include "task_player.h"
#include "soft_timer.h"
#include "bsp_voice.h"
#include <stdio.h>

/*============ 内部变量 ============*/
static PlayState_t  s_state              = ST_IDLE;
static PlayState_t  s_prev_state         = ST_IDLE;   // 新增：用于检测状态变化
static uint32_t     s_voice_poll_tick    = 0;
static uint32_t     s_timeout_tick       = 0;
static uint8_t      s_hello_repeat_count = 0;
static uint8_t      s_last_voice_id      = 0x00;

#define VOICE_POLL_MS       100
#define TIMEOUT_MS          10000

/*============ 内部函数声明 ============*/
static StateEvent_t MapVoiceIdToEvent(uint8_t voice_id);
static void         FSM_Process(StateEvent_t event);
static void         FSM_OnEnter(PlayState_t new_state);  // 新增：状态入口动作

/*============ 调试输出 ============*/
static void Debug_Print(const char *msg)
{
    while (*msg) {
        while(DL_UART_isBusy(UART_INST));
        DL_UART_Main_transmitData(UART_INST, *msg++);
    }
}

static void Debug_PrintState(const char *prefix, PlayState_t st)
{
    char buf[48];
    sprintf(buf, "%s -> State=%d\r\n", prefix, (int)st);
    Debug_Print(buf);
}

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
    [ST_SAD_DEEP]        = "DeepSad",
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
    [ST_RES_MUSHROOM]    = "Mushrom",
    [ST_GAME_OVER]       = "GameEnd",
    [ST_GAME_HAPPY]      = "GameYay",
    [ST_GAME_SAD]        = "GameSad",
    [ST_CONFUSED]        = "Confused",
    [ST_EGG_1]           = "Easter1",
    [ST_EGG_2]           = "Easter2",
};

/*============ 状态切换（统一入口） ============*/
static void ChangeState(PlayState_t new_state)
{
    if (s_state != new_state) {
        Debug_PrintState("Leave", s_state);
        s_prev_state = s_state;
        s_state = new_state;
        Debug_PrintState("Enter", s_state);
        FSM_OnEnter(new_state);
    }
}

/*============ 初始化 ============*/
void TaskFSM_Init(void)
{
    s_state           = ST_IDLE;
    s_prev_state      = ST_IDLE;
    s_voice_poll_tick = GetTick();
    s_timeout_tick    = GetTick();
    s_last_voice_id   = 0x00;

    TaskPlayer_Init();
    Debug_Print("FSM Init OK\r\n");
}

/*============ 主运行函数 ============*/
void TaskFSM_Run(void)
{
    StateEvent_t event = EVENT_NONE;

    /* ---- 1. 播放器延时管理（每轮都要跑） ---- */
    TaskPlayer_Run();

    /* ---- 2. 检查延时完成事件 ---- */
    uint8_t pending_state;
    if (TaskPlayer_PopPendingState(&pending_state)) {
        Debug_Print("PlayerDone -> auto jump\r\n");
        ChangeState((PlayState_t)pending_state);
        // 入口动作已在 ChangeState -> FSM_OnEnter 中执行
        // 不需要再走下面的事件处理
        return;
    }

    /* ---- 3. 如果播放器正在延时，不接受新的语音输入 ---- */
    if (TaskPlayer_IsBusy()) {
        // 仍然要定期读取语音模块，防止IIC数据堆积
        if (IsTimeUp(&s_voice_poll_tick, VOICE_POLL_MS)) {
            Voice_Module_ReadID();  // 读取并丢弃
        }
        return;
    }

    /* ---- 4. 定时轮询语音模块 ---- */
    if (!IsTimeUp(&s_voice_poll_tick, VOICE_POLL_MS)) {
        return;
    }

    uint8_t cur_id = Voice_Module_ReadID();

    if (cur_id != 0x00 && cur_id != s_last_voice_id) {
        event = MapVoiceIdToEvent(cur_id);
        if (event != EVENT_NONE) {
            char buf[32];
            sprintf(buf, "Voice:0x%02X Evt:%d\r\n", cur_id, (int)event);
            Debug_Print(buf);
            s_timeout_tick = GetTick();
        }
    }
    else if (cur_id == 0x00) {
        if (s_state != ST_IDLE && s_state != ST_SILENT_GUARD) {
            if (HasElapsed(s_timeout_tick, TIMEOUT_MS)) {
                event = EVENT_TIMEOUT;
                s_timeout_tick = GetTick();
                Debug_Print("TIMEOUT\r\n");
            }
        }
    }

    s_last_voice_id = cur_id;

    /* ---- 5. 驱动状态机 ---- */
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
    return "???";
}

/*============ 语音ID映射 ============*/
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

/*================================================================
 * 状态入口动作
 * 
 * 核心修复：当延时跳转到达目标状态时，自动执行该状态的"入口语音"
 * 这样就不需要在源状态的 EVENT_PLAYER_DONE 分支中处理了
 *================================================================*/
static void FSM_OnEnter(PlayState_t new_state)
{
    switch (new_state)
    {
        /* -- 延时跳转后需要立即播放语音的状态 -- */

        case ST_IDLE:
            // 回到待机，不播语音
            Debug_Print("[Idle] Sleeping...\r\n");
            break;

        case ST_WAKE:
            // 从各种路径回到 WAKE，不需要额外语音
            // （唤醒语音在 FSM_Process 的 ST_IDLE->ST_WAKE 中已播放）
            Debug_Print("[Wake] Ready for command\r\n");
            break;

        case ST_JOKE_2:
            // 从 ST_JOKE_1_BAD 延时跳转过来，需要播放笑话2
            TaskPlayer_PlayVoice(34, 0, 0);
            Debug_Print("[Joke2] Playing joke 2\r\n");
            break;

        case ST_JOKE_3:
            // 从 ST_JOKE_2_BAD 延时跳转过来
            TaskPlayer_PlayVoice(37, 0, 0);
            Debug_Print("[Joke3] Playing joke 3\r\n");
            break;

        case ST_JOKE_END:
            // 从 ST_JOKE_3 或其他路径到达
            TaskPlayer_PlayVoice(38, 0, 0);
            Debug_Print("[JokeEnd] Summary\r\n");
            break;

        case ST_GAME_START:
            // 从 ST_JOKE_LOSE 延时跳转过来
            TaskPlayer_PlayVoice(41, 0, 0);
            Debug_Print("[GameStart] Wanna play?\r\n");
            break;

        case ST_GAME_OVER:
            // 从 ST_RES_xxx 延时跳转过来
            TaskPlayer_PlayVoice(49, 0, 0);
            Debug_Print("[GameOver] Result\r\n");
            break;

        case ST_SILENT_GUARD:
            // 从 ST_SAD_DEEP 延时跳转过来
            Debug_Print("[Guard] Silent watching...\r\n");
            break;

        default:
            // 其他状态不需要入口动作
            break;
    }
}

/*================================================================
 * 核心状态机（纯事件驱动，不含延时，不含 PLAYER_DONE 处理）
 * 
 * 规则：
 *   1. 需要"播放后等一段时间再跳转"的：
 *      TaskPlayer_PlayVoice(voice_id, wait_ms, target_state)
 *      → 延时到期后 ChangeState(target_state) → FSM_OnEnter()自动播下一段
 *   
 *   2. 需要"播放后立即等待用户输入"的：
 *      TaskPlayer_PlayVoice(voice_id, 0, 0)
 *      → 不设延时，等用户语音触发下一个事件
 *   
 *   3. 状态切换统一走 ChangeState()
 *================================================================*/
static void FSM_Process(StateEvent_t event)
{
    /* ---- 彩蛋检测 ---- */
    if (event == EVENT_HELLO && s_state != ST_IDLE) {
        s_hello_repeat_count++;
        if (s_hello_repeat_count >= 3) {
            ChangeState(ST_EGG_1);
            TaskPlayer_PlayVoice(53, 5000, ST_WAKE);
            s_hello_repeat_count = 0;
            return;
        }
    } else {
        s_hello_repeat_count = 0;
    }

    switch (s_state)
    {
        /* ==================== 1. 基础唤醒 ==================== */
        case ST_IDLE:
            if (event == EVENT_HELLO) {
                ChangeState(ST_WAKE);
                TaskPlayer_PlayVoice(2, 0, 0);
            } else if (event == EVENT_SAD) {
                ChangeState(ST_SAD_RECV);
                TaskPlayer_PlayVoice(11, 0, 0);
            }
            // 其他事件在IDLE状态下忽略
            break;

        case ST_WAKE:
            if (event == EVENT_TIME) {
                ChangeState(ST_TIME);
                TaskPlayer_PlayVoice(3, 5000, ST_WAKE);
            } else if (event == EVENT_SAD) {
                ChangeState(ST_SAD_RECV);
                TaskPlayer_PlayVoice(11, 0, 0);
            } else if (event == EVENT_HAPPY) {
                ChangeState(ST_HAPPY_RECV);
                TaskPlayer_PlayVoice(21, 0, 0);
            } else if (event == EVENT_JOKE) {
                ChangeState(ST_JOKE_1);
                TaskPlayer_PlayVoice(31, 0, 0);
            } else if (event == EVENT_GAME) {
                ChangeState(ST_GAME_START);
                TaskPlayer_PlayVoice(41, 0, 0);
            } else if (event == EVENT_TIMEOUT || event == EVENT_NO) {
                ChangeState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_TIME:
            // 延时自动回 ST_WAKE
            // 用户提前打断：直接切回 WAKE 让 WAKE 处理
            if (event == EVENT_SAD || event == EVENT_HAPPY ||
                event == EVENT_JOKE || event == EVENT_GAME) {
                ChangeState(ST_WAKE);
                FSM_Process(event);
            }
            break;

        /* ==================== 2. 负面情绪 ==================== */
        case ST_SAD_RECV:
            if (event == EVENT_YES || event == EVENT_SAD) {
                ChangeState(ST_PLAY_SAD_MUSIC);
                TaskPlayer_PlayMusic(1);
                TaskPlayer_PlayVoice(12, 0, 0);
            } else if (event == EVENT_NO) {
                ChangeState(ST_SAD_TALK);
                TaskPlayer_PlayVoice(13, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_PLAY_SAD_MUSIC:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                ChangeState(ST_SAD_BETTER);
                TaskPlayer_PlayVoice(14, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_SAD) {
                ChangeState(ST_SAD_STILL);
                TaskPlayer_PlayVoice(15, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_SAD_BETTER);
                TaskPlayer_PlayVoice(14, 0, 0);
            }
            break;

        case ST_SAD_TALK:
            if (event == EVENT_YES) {
                ChangeState(ST_SAD_BETTER);
                TaskPlayer_PlayVoice(14, 0, 0);
            } else if (event == EVENT_NO) {
                ChangeState(ST_SAD_REBEL);
                TaskPlayer_PlayVoice(16, 3000, ST_IDLE);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_SAD_BETTER:
            if (event == EVENT_YES) {
                ChangeState(ST_JOKE_1);
                TaskPlayer_PlayVoice(31, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_SAD_STILL:
            if (event == EVENT_YES) {
                ChangeState(ST_BYE);
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            } else if (event == EVENT_NO) {
                ChangeState(ST_SAD_DEEP);
                TaskPlayer_PlayVoice(17, 6000, ST_SILENT_GUARD);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_SILENT_GUARD);
            }
            break;

        case ST_SAD_REBEL:
            // 延时自动回 ST_IDLE
            break;

        case ST_SAD_DEEP:
            // 延时自动跳 ST_SILENT_GUARD (由 FSM_OnEnter 处理)
            break;

        case ST_SILENT_GUARD:
            if (event == EVENT_HELLO) {
                ChangeState(ST_WAKE_UP_SUN);
                TaskPlayer_PlayVoice(19, 3000, ST_WAKE);
            }
            break;

        case ST_WAKE_UP_SUN:
            // 延时自动跳 ST_WAKE
            break;

        /* ==================== 3. 正面情绪 ==================== */
        case ST_HAPPY_RECV:
            if (event == EVENT_HAPPY || event == EVENT_YES) {
                ChangeState(ST_HAPPY_CELEBRATE);
                TaskPlayer_PlayMusic(2);
                TaskPlayer_PlayVoice(22, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_HAPPY_SHY);
                TaskPlayer_PlayVoice(23, 4000, ST_WAKE);
            }
            break;

        case ST_HAPPY_CELEBRATE:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                ChangeState(ST_HAPPY_END);
                TaskPlayer_PlayVoice(24, 3000, ST_IDLE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_HAPPY_QUIET);
                TaskPlayer_PlayVoice(25, 3000, ST_WAKE);
            }
            break;

        case ST_HAPPY_SHY:
        case ST_HAPPY_END:
        case ST_HAPPY_QUIET:
            // 延时自动跳转，不响应事件
            break;

        /* ==================== 4. 冷笑话模块 ==================== */
        case ST_JOKE_1:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                ChangeState(ST_JOKE_1_GOOD);
                TaskPlayer_PlayVoice(32, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_JOKE_1_BAD);
                TaskPlayer_PlayVoice(33, 3000, ST_JOKE_2);
                // 3秒后自动跳 ST_JOKE_2 → FSM_OnEnter 播放笑话2
            }
            break;

        case ST_JOKE_1_GOOD:
            if (event == EVENT_YES) {
                ChangeState(ST_JOKE_2);
                TaskPlayer_PlayVoice(34, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_WAKE);
            }
            break;

        case ST_JOKE_1_BAD:
            // 延时自动跳 ST_JOKE_2 → OnEnter播放34
            break;

        case ST_JOKE_2:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                ChangeState(ST_JOKE_2_GOOD);
                TaskPlayer_PlayVoice(35, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_JOKE_2_BAD);
                TaskPlayer_PlayVoice(36, 3000, ST_JOKE_3);
            }
            break;

        case ST_JOKE_2_GOOD:
            if (event == EVENT_YES) {
                ChangeState(ST_JOKE_3);
                // OnEnter 会自动播放37
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_WAKE);
            }
            break;

        case ST_JOKE_2_BAD:
            // 延时自动跳 ST_JOKE_3 → OnEnter播放37
            break;

        case ST_JOKE_3:
            // 笑话3播完后，任何事件都推进到结算
            if (event == EVENT_YES || event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_JOKE_END);
                // OnEnter 会自动播放38
            }
            break;

        case ST_JOKE_END:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                ChangeState(ST_JOKE_WIN);
                TaskPlayer_PlayVoice(39, 3000, ST_WAKE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_JOKE_LOSE);
                TaskPlayer_PlayVoice(40, 4000, ST_GAME_START);
                // 4秒后跳 ST_GAME_START → OnEnter播放41
            }
            break;

        case ST_JOKE_WIN:
        case ST_JOKE_LOSE:
            // 延时自动跳转
            break;

        /* ==================== 5. 互动游戏 ==================== */
        case ST_GAME_START:
            if (event == EVENT_YES) {
                ChangeState(ST_GAME_Q1);
                TaskPlayer_PlayVoice(42, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_WAKE);
            }
            break;

        case ST_GAME_Q1:
            if (event == EVENT_A) {
                ChangeState(ST_GAME_Q2_A);
                TaskPlayer_PlayVoice(43, 0, 0);
            } else if (event == EVENT_B) {
                ChangeState(ST_GAME_Q2_B);
                TaskPlayer_PlayVoice(44, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_WAKE);
            }
            break;

        case ST_GAME_Q2_A:
            if (event == EVENT_A) {
                ChangeState(ST_RES_SUNFLOWER);
                TaskPlayer_PlayVoice(45, 8000, ST_GAME_OVER);
            } else if (event == EVENT_B) {
                ChangeState(ST_RES_CACTUS);
                TaskPlayer_PlayVoice(46, 8000, ST_GAME_OVER);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_WAKE);
            }
            break;

        case ST_GAME_Q2_B:
            if (event == EVENT_A) {
                ChangeState(ST_RES_MIMOSA);
                TaskPlayer_PlayVoice(47, 8000, ST_GAME_OVER);
            } else if (event == EVENT_B) {
                ChangeState(ST_RES_MUSHROOM);
                TaskPlayer_PlayVoice(48, 8000, ST_GAME_OVER);
            } else if (event == EVENT_TIMEOUT) {
                ChangeState(ST_WAKE);
            }
            break;

        case ST_RES_SUNFLOWER:
        case ST_RES_CACTUS:
        case ST_RES_MIMOSA:
        case ST_RES_MUSHROOM:
            // 正在播放结果，延时自动跳 ST_GAME_OVER → OnEnter播放49
            break;

        case ST_GAME_OVER:
            if (event == EVENT_YES) {
                ChangeState(ST_GAME_HAPPY);
                TaskPlayer_PlayVoice(50, 3000, ST_WAKE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                ChangeState(ST_GAME_SAD);
                TaskPlayer_PlayVoice(51, 3000, ST_WAKE);
            }
            break;

        case ST_GAME_HAPPY:
        case ST_GAME_SAD:
            // 延时自动跳 ST_WAKE
            break;

        /* ==================== 6. 彩蛋 ==================== */
        case ST_EGG_1:
            // 延时自动跳 ST_WAKE
            break;

        /* ==================== 7. 兜底 ==================== */
        default:
            Debug_Print("[FSM] Unknown state, reset to WAKE\r\n");
            ChangeState(ST_WAKE);
            break;
    }
}
