#include "task_fsm.h"
#include "task_player.h"
#include "soft_timer.h"
#include "bsp_voice.h"

/*============ 内部变量 ============*/
static PlayState_t  s_state              = ST_IDLE;
static uint32_t     s_voice_poll_tick    = 0;
static uint32_t     s_timeout_tick       = 0;
static uint8_t      s_hello_repeat_count = 0;
static uint8_t      s_last_voice_id      = 0x00;

#define VOICE_POLL_MS       100     // 语音模块轮询周期
#define TIMEOUT_MS          10000   // 10秒无操作超时

/*============ 内部函数声明 ============*/
static StateEvent_t MapVoiceIdToEvent(uint8_t voice_id);
static void         FSM_Process(StateEvent_t event);

/*============ 状态名称表（供OLED显示） ============*/
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
    [ST_SILENT_GUARD]    = "Guard",
    [ST_HAPPY_RECV]      = "Happy!",
    [ST_HAPPY_CELEBRATE] = "Party!",
    [ST_JOKE_1]          = "Joke1",
    [ST_JOKE_2]          = "Joke2",
    [ST_JOKE_3]          = "Joke3",
    [ST_JOKE_END]        = "JokeEnd",
    [ST_GAME_START]      = "Game?",
    [ST_GAME_Q1]         = "Q1",
    [ST_GAME_Q2_A]       = "Q2-A",
    [ST_GAME_Q2_B]       = "Q2-B",
    [ST_RES_SUNFLOWER]   = "Sunflwr",
    [ST_RES_CACTUS]      = "Cactus",
    [ST_RES_MIMOSA]      = "Mimosa",
    [ST_RES_MUSHROOM]    = "Mushrom",
    [ST_GAME_OVER]       = "GameEnd",
    [ST_EGG_1]           = "Easter1",
    [ST_EGG_2]           = "Easter2",
};

/*============ 初始化 ============*/
void TaskFSM_Init(void)
{
    s_state           = ST_IDLE;
    s_voice_poll_tick = GetTick();
    s_timeout_tick    = GetTick();
    s_last_voice_id   = 0x00;
    
    TaskPlayer_Init();
}

/*============ 主运行函数（非阻塞） ============*/
void TaskFSM_Run(void)
{
    StateEvent_t event = EVENT_NONE;

    /* ---- 1. 检查播放器延时完成事件（最高优先级） ---- */
    uint8_t pending_state;
    if (TaskPlayer_PopPendingState(&pending_state)) {
        s_state = (PlayState_t)pending_state;
        event = EVENT_PLAYER_DONE;
        // 某些状态跳转后需要立即播放下一段语音
        // 在 FSM_Process 的对应 case 中处理
    }

    /* ---- 2. 定时轮询语音模块 ---- */
    if (event == EVENT_NONE && IsTimeUp(&s_voice_poll_tick, VOICE_POLL_MS)) {

        // 如果播放器正在延时等待中，不处理新语音输入（防止打断）
        if (TaskPlayer_IsBusy()) {
            // 但仍然读取并丢弃，防止语音ID残留
            Voice_Module_ReadID();
            return;
        }

        uint8_t cur_id = Voice_Module_ReadID();

        if (cur_id != 0x00 && cur_id != s_last_voice_id) {
            event = MapVoiceIdToEvent(cur_id);
            s_timeout_tick = GetTick();  // 重置超时
        }
        else if (cur_id == 0x00) {
            // 超时检测
            if (s_state != ST_IDLE && s_state != ST_SILENT_GUARD) {
                if (HasElapsed(s_timeout_tick, TIMEOUT_MS)) {
                    event = EVENT_TIMEOUT;
                    s_timeout_tick = GetTick();
                }
            }
        }

        s_last_voice_id = cur_id;
    }

    /* ---- 3. 驱动状态机 ---- */
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

/*============ 语音ID -> 事件映射（不变） ============*/
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
 * 核心状态机（完全非阻塞重构）
 * 
 * 改造原则：
 *   原来的  Player_PlayVoice(x); delay_ms(N); g_current_state = ST_XXX;
 *   改为    TaskPlayer_PlayVoice(x, N, ST_XXX);  // 非阻塞
 *   
 *   延时到期后 TaskPlayer 会设置 pending_done，
 *   下一轮 TaskFSM_Run() 会自动跳转到 ST_XXX 并触发 EVENT_PLAYER_DONE
 *============================================================*/
static void FSM_Process(StateEvent_t event)
{
    // ---- 彩蛋：连续打招呼检测 ----
    if (event == EVENT_HELLO && s_state != ST_IDLE) {
        s_hello_repeat_count++;
        if (s_hello_repeat_count >= 3) {
            s_state = ST_EGG_1;
            TaskPlayer_PlayVoice(53, 5000, ST_WAKE);
            s_hello_repeat_count = 0;
            return;
        }
    } else if (event != EVENT_NONE && event != EVENT_PLAYER_DONE) {
        s_hello_repeat_count = 0;
    }

    switch(s_state)
    {
        /* ========== 1. 基础服务与唤醒 ========== */
        case ST_IDLE:
            if (event == EVENT_HELLO) {
                s_state = ST_WAKE;
                TaskPlayer_PlayVoice(2, 0, 0);  // 播放唤醒问候，不延时跳转
            } else if (event == EVENT_SAD) {
                s_state = ST_SAD_RECV;
                TaskPlayer_PlayVoice(11, 0, 0);
            }
            break;

        case ST_WAKE:
            if (event == EVENT_TIME) {
                s_state = ST_TIME;
                // 播放时间语音，5秒后自动返回 ST_WAKE
                TaskPlayer_PlayVoice(3, 5000, ST_WAKE);
            } else if (event == EVENT_SAD) {
                s_state = ST_SAD_RECV;
                TaskPlayer_PlayVoice(11, 0, 0);
            } else if (event == EVENT_HAPPY) {
                s_state = ST_HAPPY_RECV;
                TaskPlayer_PlayVoice(21, 0, 0);
            } else if (event == EVENT_JOKE) {
                s_state = ST_JOKE_1;
                TaskPlayer_PlayVoice(31, 0, 0);
            } else if (event == EVENT_GAME) {
                s_state = ST_GAME_START;
                TaskPlayer_PlayVoice(41, 0, 0);
            } else if (event == EVENT_TIMEOUT || event == EVENT_NO) {
                s_state = ST_BYE;
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            // EVENT_PLAYER_DONE 到达 ST_WAKE 时无需额外动作
            break;

        case ST_TIME:
            // 此状态由 TaskPlayer 延时自动跳转回 ST_WAKE，无需手动处理
            // 但如果用户提前说话，也可以响应
            if (event == EVENT_HELLO || event == EVENT_SAD || event == EVENT_HAPPY) {
                // 提前打断，直接跳到 ST_WAKE 再处理
                s_state = ST_WAKE;
                FSM_Process(event);  // 递归一次处理
            }
            break;

        /* ========== 2. 负面情绪陪伴模块 ========== */
        case ST_SAD_RECV:
            if (event == EVENT_YES || event == EVENT_SAD) {
                s_state = ST_PLAY_SAD_MUSIC;
                TaskPlayer_PlayMusic(1);
                TaskPlayer_PlayVoice(12, 0, 0);
            } else if (event == EVENT_NO) {
                s_state = ST_SAD_TALK;
                TaskPlayer_PlayVoice(13, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_BYE;
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_PLAY_SAD_MUSIC:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                s_state = ST_SAD_BETTER;
                TaskPlayer_PlayVoice(14, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_SAD) {
                s_state = ST_SAD_STILL;
                TaskPlayer_PlayVoice(15, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                // 音乐放着没人搭理，温柔提醒
                s_state = ST_SAD_BETTER;
                TaskPlayer_PlayVoice(14, 0, 0);
            }
            break;

        case ST_SAD_TALK:
            if (event == EVENT_YES) {
                s_state = ST_SAD_BETTER;
                TaskPlayer_PlayVoice(14, 0, 0);
            } else if (event == EVENT_NO) {
                s_state = ST_SAD_REBEL;
                TaskPlayer_PlayVoice(16, 3000, ST_IDLE);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_BYE;
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_SAD_BETTER:
            if (event == EVENT_YES) {
                s_state = ST_JOKE_1;
                TaskPlayer_PlayVoice(31, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_BYE;
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            }
            break;

        case ST_SAD_STILL:
            if (event == EVENT_YES) {
                s_state = ST_BYE;
                TaskPlayer_PlayVoice(4, 3000, ST_IDLE);
            } else if (event == EVENT_NO) {
                s_state = ST_SAD_DEEP;
                TaskPlayer_PlayVoice(17, 6000, ST_SILENT_GUARD);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_SILENT_GUARD;
                TaskPlayer_PlayVoice(17, 0, 0);
            }
            break;

        case ST_SILENT_GUARD:
            // 静默守护状态：只响应唤醒词
            if (event == EVENT_HELLO) {
                s_state = ST_WAKE_UP_SUN;
                TaskPlayer_PlayVoice(19, 3000, ST_WAKE);
            }
            // 不响应 TIMEOUT（一直守护到用户说话）
            break;

        case ST_WAKE_UP_SUN:
            // 由 TaskPlayer 自动跳转到 ST_WAKE
            break;

        /* ========== 3. 正面情绪共享模块 ========== */
        case ST_HAPPY_RECV:
            if (event == EVENT_HAPPY || event == EVENT_YES) {
                s_state = ST_HAPPY_CELEBRATE;
                TaskPlayer_PlayMusic(2);
                TaskPlayer_PlayVoice(22, 0, 0);
            } else if (event == EVENT_NO) {
                s_state = ST_HAPPY_SHY;
                TaskPlayer_PlayVoice(23, 4000, ST_WAKE);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_HAPPY_SHY;
                TaskPlayer_PlayVoice(23, 4000, ST_WAKE);
            }
            break;

        case ST_HAPPY_CELEBRATE:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                s_state = ST_HAPPY_END;
                TaskPlayer_PlayVoice(24, 3000, ST_IDLE);
            } else if (event == EVENT_NO) {
                s_state = ST_HAPPY_QUIET;
                TaskPlayer_PlayVoice(25, 3000, ST_WAKE);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_HAPPY_END;
                TaskPlayer_PlayVoice(24, 3000, ST_IDLE);
            }
            break;

        /* ========== 4. 冷笑话模块 ========== */
        case ST_JOKE_1:
            if (event == EVENT_YES || event == EVENT_HAPPY) {
                s_state = ST_JOKE_1_GOOD;
                TaskPlayer_PlayVoice(32, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_JOKE_1_BAD;
                // 播放吐槽，3秒后自动进入第二个笑话
                TaskPlayer_PlayVoice(33, 3000, ST_JOKE_2);
            }
            break;

        case ST_JOKE_1_GOOD:
            if (event == EVENT_YES) {
                s_state = ST_JOKE_2;
                TaskPlayer_PlayVoice(34, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_WAKE;
            }
            break;

        case ST_JOKE_1_BAD:
            // 由 TaskPlayer 延时自动跳转到 ST_JOKE_2
            if (event == EVENT_PLAYER_DONE) {
                // 已经跳转到 ST_JOKE_2 了，播放笑话2
                TaskPlayer_PlayVoice(34, 0, 0);
            }
            break;

        case ST_JOKE_2:
            if (event == EVENT_PLAYER_DONE) {
                // 笑话2刚播完进入此状态，等待用户反应
                // 不做额外操作
            } else if (event == EVENT_YES) {
                s_state = ST_JOKE_2_GOOD;
                TaskPlayer_PlayVoice(35, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_JOKE_2_BAD;
                TaskPlayer_PlayVoice(36, 3000, ST_JOKE_3);
            }
            break;

        case ST_JOKE_2_GOOD:
            if (event == EVENT_YES) {
                s_state = ST_JOKE_3;
                TaskPlayer_PlayVoice(37, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_WAKE;
            }
            break;

        case ST_JOKE_2_BAD:
            if (event == EVENT_PLAYER_DONE) {
                TaskPlayer_PlayVoice(37, 0, 0);
            }
            break;

        case ST_JOKE_3:
            if (event == EVENT_PLAYER_DONE) {
                // 笑话3播完，自动进入结算
                s_state = ST_JOKE_END;
                TaskPlayer_PlayVoice(38, 0, 0);
            } else {
                // 任何用户事件都推进到结算
                s_state = ST_JOKE_END;
                TaskPlayer_PlayVoice(38, 0, 0);
            }
            break;

        case ST_JOKE_END:
            if (event == EVENT_YES) {
                s_state = ST_JOKE_WIN;
                TaskPlayer_PlayVoice(39, 3000, ST_WAKE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_JOKE_LOSE;
                TaskPlayer_PlayVoice(40, 4000, ST_GAME_START);
            }
            break;

        case ST_JOKE_WIN:
            // 延时自动跳 ST_WAKE
            if (event == EVENT_PLAYER_DONE) {
                // 已在 ST_WAKE
            }
            break;

        case ST_JOKE_LOSE:
            if (event == EVENT_PLAYER_DONE) {
                // 已跳转到 ST_GAME_START
                TaskPlayer_PlayVoice(41, 0, 0);
            }
            break;

        /* ========== 5. 互动小游戏模块 ========== */
        case ST_GAME_START:
            if (event == EVENT_PLAYER_DONE) {
                // 从笑话模块跳过来的，语音已播放
            } else if (event == EVENT_YES) {
                s_state = ST_GAME_Q1;
                TaskPlayer_PlayVoice(42, 0, 0);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_WAKE;
            }
            break;

        case ST_GAME_Q1:
            if (event == EVENT_A) {
                s_state = ST_GAME_Q2_A;
                TaskPlayer_PlayVoice(43, 0, 0);
            } else if (event == EVENT_B) {
                s_state = ST_GAME_Q2_B;
                TaskPlayer_PlayVoice(44, 0, 0);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_WAKE;
            }
            break;

        case ST_GAME_Q2_A:
            if (event == EVENT_A) {
                s_state = ST_RES_SUNFLOWER;
                // 播放结果，8秒后进入 GAME_OVER
                TaskPlayer_PlayVoice(45, 8000, ST_GAME_OVER);
            } else if (event == EVENT_B) {
                s_state = ST_RES_CACTUS;
                TaskPlayer_PlayVoice(46, 8000, ST_GAME_OVER);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_WAKE;
            }
            break;

        case ST_GAME_Q2_B:
            if (event == EVENT_A) {
                s_state = ST_RES_MIMOSA;
                TaskPlayer_PlayVoice(47, 8000, ST_GAME_OVER);
            } else if (event == EVENT_B) {
                s_state = ST_RES_MUSHROOM;
                TaskPlayer_PlayVoice(48, 8000, ST_GAME_OVER);
            } else if (event == EVENT_TIMEOUT) {
                s_state = ST_WAKE;
            }
            break;

        case ST_RES_SUNFLOWER:
        case ST_RES_CACTUS:
        case ST_RES_MIMOSA:
        case ST_RES_MUSHROOM:
            // 正在播放结果语音，延时后自动跳 ST_GAME_OVER
            // 此处不响应用户输入（播放中）
            break;

        case ST_GAME_OVER:
            if (event == EVENT_PLAYER_DONE) {
                // 刚跳转过来，播放结算语音
                TaskPlayer_PlayVoice(49, 0, 0);
            } else if (event == EVENT_YES) {
                s_state = ST_GAME_HAPPY;
                TaskPlayer_PlayVoice(50, 3000, ST_WAKE);
            } else if (event == EVENT_NO || event == EVENT_TIMEOUT) {
                s_state = ST_GAME_SAD;
                TaskPlayer_PlayVoice(51, 3000, ST_WAKE);
            }
            break;

        case ST_GAME_HAPPY:
        case ST_GAME_SAD:
            // 延时自动跳 ST_WAKE
            break;

        /* ========== 6. 彩蛋状态 ========== */
        case ST_EGG_1:
            // 延时自动跳 ST_WAKE (在上面彩蛋检测中已设置)
            break;

        /* ========== 7. 兜底 ========== */
        default:
            s_state = ST_WAKE;
            break;
    }
}
