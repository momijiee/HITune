#include "ti_msp_dl_config.h"
#include "bsp_voice.h"
#include "bsp_player.h"

// 毫秒延时宏定义
#define delay_ms(X)        delay_cycles((CPUCLK_FREQ/1000)*(X))

typedef enum {
    VOICE_STATE_SLEEP = 0,
    VOICE_STATE_AWAKE,
    VOICE_STATE_CHAT,
    VOICE_STATE_EMOTION_TIRED,
    VOICE_STATE_EMOTION_HAPPY,
    VOICE_STATE_MUSIC,
    VOICE_STATE_ACTION,
    VOICE_STATE_VOLUME,
    VOICE_STATE_GOODBYE,
    VOICE_STATE_REMINDER
} voice_state_t;

typedef enum {
    VOICE_EVENT_NONE = 0,
    VOICE_EVENT_REST,
    VOICE_EVENT_WAKE,
    VOICE_EVENT_VOLUME_MAX,
    VOICE_EVENT_VOLUME_DOWN,
    VOICE_EVENT_VOLUME_MEDIUM,
    VOICE_EVENT_VOLUME_MIN,
    VOICE_EVENT_STOP_OR_REPORT,
    VOICE_EVENT_HELLO,
    VOICE_EVENT_SELF_INTRO,
    VOICE_EVENT_TIRED,
    VOICE_EVENT_HAPPY,
    VOICE_EVENT_GOODBYE,
    VOICE_EVENT_LIGHT_ON,
    VOICE_EVENT_LIGHT_OFF,
    VOICE_EVENT_ACTION
} voice_event_t;

typedef struct {
    uint8_t id;
    voice_event_t event;
    const char* name;
} voice_event_map_t;

#define VOICE_TICK_MS                 (100U)
#define VOICE_SESSION_TIMEOUT_TICKS   (120U)    // 12 秒唤醒窗口
#define VOICE_REMINDER_TICKS          (3000U)   // 5 分钟主动提醒入口

#define VOICE_TRACK_GREETING          (1U)
#define VOICE_TRACK_SELF_INTRO        (2U)
#define VOICE_TRACK_TIRED             (3U)
#define VOICE_TRACK_HAPPY             (4U)
#define VOICE_TRACK_GOODBYE           (5U)
#define VOICE_TRACK_TIRED_MUSIC       (6U)
#define VOICE_TRACK_HAPPY_MUSIC       (7U)
#define VOICE_TRACK_ACTION_ACCEPTED   (8U)
#define VOICE_TRACK_LIGHT_ON          (9U)
#define VOICE_TRACK_LIGHT_OFF         (10U)
#define VOICE_TRACK_STOP              (11U)
#define VOICE_TRACK_VOLUME_MAX        (12U)
#define VOICE_TRACK_VOLUME_DOWN       (13U)
#define VOICE_TRACK_VOLUME_MEDIUM     (14U)
#define VOICE_TRACK_VOLUME_MIN        (15U)
#define VOICE_TRACK_WAKE_REQUIRED     (16U)
#define VOICE_TRACK_REMINDER_BASE     (17U)
#define VOICE_TRACK_REMINDER_COUNT    (4U)

#define MUSIC_TRACK_TIRED             (1U)
#define MUSIC_TRACK_HAPPY             (2U)

#define VOLUME_MIN                    (5U)
#define VOLUME_STEP                   (5U)
#define VOLUME_MEDIUM                 (20U)
#define VOLUME_MAX                    (30U)

static const voice_event_map_t voice_event_map[] = {
    {0x02, VOICE_EVENT_REST,           "REST"},
    {0x03, VOICE_EVENT_WAKE,           "WAKE"},
    {0x04, VOICE_EVENT_VOLUME_MAX,     "VOLUME_SUPER"},
    {0x05, VOICE_EVENT_VOLUME_DOWN,    "VOLUME_DOWN"},
    {0x06, VOICE_EVENT_VOLUME_MAX,     "VOLUME_MAX"},
    {0x07, VOICE_EVENT_VOLUME_MEDIUM,  "VOLUME_MEDIUM"},
    {0x08, VOICE_EVENT_VOLUME_MIN,     "VOLUME_MIN"},
    {0x09, VOICE_EVENT_STOP_OR_REPORT, "STOP_OR_REPORT"},
    {0x12, VOICE_EVENT_LIGHT_ON,       "LIGHT_ON"},
    {0x13, VOICE_EVENT_LIGHT_OFF,      "LIGHT_OFF"},
    {0x1A, VOICE_EVENT_HELLO,          "HELLO"},
    {0x1B, VOICE_EVENT_SELF_INTRO,     "SELF_INTRO"},
    {0x1C, VOICE_EVENT_TIRED,          "TIRED"},
    {0x1D, VOICE_EVENT_HAPPY,          "HAPPY"},
    {0x1E, VOICE_EVENT_GOODBYE,        "GOODBYE"},
    {0x01, VOICE_EVENT_ACTION,         "MOVE_FORWARD"},
    {0x0A, VOICE_EVENT_ACTION,         "STAND"},
    {0x0B, VOICE_EVENT_ACTION,         "LIE_DOWN"},
    {0x0C, VOICE_EVENT_ACTION,         "SIT_DOWN"},
    {0x0D, VOICE_EVENT_ACTION,         "SPEED_UP"},
    {0x0E, VOICE_EVENT_ACTION,         "SLOW_DOWN"},
};

// 调试串口辅助函数声明（用于在电脑串口助手看日志）
void uart_debug_send_string(const char* str);
void uart_debug_print_hex8(uint8_t val);

static const char* voice_state_name(voice_state_t state)
{
    switch(state)
    {
        case VOICE_STATE_SLEEP:         return "SLEEP";
        case VOICE_STATE_AWAKE:         return "AWAKE";
        case VOICE_STATE_CHAT:          return "CHAT";
        case VOICE_STATE_EMOTION_TIRED: return "EMOTION_TIRED";
        case VOICE_STATE_EMOTION_HAPPY: return "EMOTION_HAPPY";
        case VOICE_STATE_MUSIC:         return "MUSIC";
        case VOICE_STATE_ACTION:        return "ACTION";
        case VOICE_STATE_VOLUME:        return "VOLUME";
        case VOICE_STATE_GOODBYE:       return "GOODBYE";
        case VOICE_STATE_REMINDER:      return "REMINDER";
        default:                        return "UNKNOWN";
    }
}

static const voice_event_map_t* voice_find_event(uint8_t command_id)
{
    uint8_t i;

    for(i = 0; i < (sizeof(voice_event_map) / sizeof(voice_event_map[0])); i++)
    {
        if(voice_event_map[i].id == command_id)
        {
            return &voice_event_map[i];
        }
    }

    return 0;
}

static void voice_set_state(voice_state_t* current_state, voice_state_t next_state)
{
    if(*current_state != next_state)
    {
        uart_debug_send_string("-> State: ");
        uart_debug_send_string(voice_state_name(*current_state));
        uart_debug_send_string(" -> ");
        uart_debug_send_string(voice_state_name(next_state));
        uart_debug_send_string("\r\n");
        *current_state = next_state;
    }
}

static void voice_refresh_session(uint16_t* session_ticks)
{
    *session_ticks = VOICE_SESSION_TIMEOUT_TICKS;
}

static void voice_enter_awake(voice_state_t* current_state, uint16_t* session_ticks)
{
    voice_set_state(current_state, VOICE_STATE_AWAKE);
    voice_refresh_session(session_ticks);
    Player_PlayVoice(VOICE_TRACK_GREETING);
    uart_debug_send_string("-> Reply: Track 1 greeting\r\n");
}

static uint16_t voice_volume_track(voice_event_t event)
{
    switch(event)
    {
        case VOICE_EVENT_VOLUME_MAX:    return VOICE_TRACK_VOLUME_MAX;
        case VOICE_EVENT_VOLUME_DOWN:   return VOICE_TRACK_VOLUME_DOWN;
        case VOICE_EVENT_VOLUME_MEDIUM: return VOICE_TRACK_VOLUME_MEDIUM;
        case VOICE_EVENT_VOLUME_MIN:    return VOICE_TRACK_VOLUME_MIN;
        default:                        return 0;
    }
}

static void voice_handle_volume(voice_event_t event, uint8_t* current_volume)
{
    uint16_t reply_track = voice_volume_track(event);

    switch(event)
    {
        case VOICE_EVENT_VOLUME_MAX:
            *current_volume = VOLUME_MAX;
            break;
        case VOICE_EVENT_VOLUME_DOWN:
            if(*current_volume > (VOLUME_MIN + VOLUME_STEP))
            {
                *current_volume = *current_volume - VOLUME_STEP;
            }
            else
            {
                *current_volume = VOLUME_MIN;
            }
            break;
        case VOICE_EVENT_VOLUME_MEDIUM:
            *current_volume = VOLUME_MEDIUM;
            break;
        case VOICE_EVENT_VOLUME_MIN:
            *current_volume = VOLUME_MIN;
            break;
        default:
            return;
    }

    Player_SetVolume(*current_volume);
    uart_debug_send_string("-> Action: Set volume 0x");
    uart_debug_print_hex8(*current_volume);
    uart_debug_send_string("\r\n");

    if(reply_track != 0)
    {
        Player_PlayVoice(reply_track);
    }
}

static void voice_handle_action(voice_event_t event, uint8_t* light_manual_mode)
{
    switch(event)
    {
        case VOICE_EVENT_LIGHT_ON:
            DL_GPIO_setPins(LED1_PORT, LED1_PIN_22_PIN);
            *light_manual_mode = 1;
            Player_PlayVoice(VOICE_TRACK_LIGHT_ON);
            uart_debug_send_string("-> Action: Light on\r\n");
            break;
        case VOICE_EVENT_LIGHT_OFF:
            DL_GPIO_clearPins(LED1_PORT, LED1_PIN_22_PIN);
            *light_manual_mode = 1;
            Player_PlayVoice(VOICE_TRACK_LIGHT_OFF);
            uart_debug_send_string("-> Action: Light off\r\n");
            break;
        case VOICE_EVENT_STOP_OR_REPORT:
            Player_Pause();
            Player_PlayVoice(VOICE_TRACK_STOP);
            uart_debug_send_string("-> Action: Pause player / stop action\r\n");
            break;
        default:
            Player_PlayVoice(VOICE_TRACK_ACTION_ACCEPTED);
            uart_debug_send_string("-> Action: Command accepted, driver pending\r\n");
            break;
    }
}

static void voice_handle_event(uint8_t command_id,
                               voice_state_t* current_state,
                               uint16_t* session_ticks,
                               uint8_t* current_volume,
                               uint8_t* light_manual_mode)
{
    const voice_event_map_t* event_item = voice_find_event(command_id);
    voice_event_t event = VOICE_EVENT_NONE;

    uart_debug_send_string("Voice Triggered! ID: 0x");
    uart_debug_print_hex8(command_id);
    uart_debug_send_string("\r\n");

    if(event_item == 0)
    {
        uart_debug_send_string("-> Warning: Unhandled Voice ID\r\n");
        return;
    }

    event = event_item->event;
    uart_debug_send_string("-> Event: ");
    uart_debug_send_string(event_item->name);
    uart_debug_send_string("\r\n");

    if(event == VOICE_EVENT_VOLUME_MAX ||
       event == VOICE_EVENT_VOLUME_DOWN ||
       event == VOICE_EVENT_VOLUME_MEDIUM ||
       event == VOICE_EVENT_VOLUME_MIN)
    {
        voice_state_t before_volume = *current_state;
        voice_set_state(current_state, VOICE_STATE_VOLUME);
        voice_handle_volume(event, current_volume);
        voice_set_state(current_state, before_volume);
        return;
    }

    if(event == VOICE_EVENT_WAKE || event == VOICE_EVENT_HELLO)
    {
        voice_enter_awake(current_state, session_ticks);
        return;
    }

    if(event == VOICE_EVENT_REST || event == VOICE_EVENT_GOODBYE)
    {
        voice_set_state(current_state, VOICE_STATE_GOODBYE);
        Player_PlayVoice(VOICE_TRACK_GOODBYE);
        uart_debug_send_string("-> Reply: Track 5 goodbye\r\n");
        voice_set_state(current_state, VOICE_STATE_SLEEP);
        *session_ticks = 0;
        return;
    }

    if(*current_state == VOICE_STATE_SLEEP &&
       (event == VOICE_EVENT_LIGHT_ON ||
        event == VOICE_EVENT_LIGHT_OFF ||
        event == VOICE_EVENT_STOP_OR_REPORT ||
        event == VOICE_EVENT_ACTION))
    {
        Player_PlayVoice(VOICE_TRACK_WAKE_REQUIRED);
        uart_debug_send_string("-> Ignored: wake word required before action\r\n");
        return;
    }

    if(*current_state == VOICE_STATE_SLEEP)
    {
        uart_debug_send_string("-> Auto wake before handling command\r\n");
        voice_set_state(current_state, VOICE_STATE_AWAKE);
    }

    voice_refresh_session(session_ticks);

    switch(event)
    {
        case VOICE_EVENT_SELF_INTRO:
            voice_set_state(current_state, VOICE_STATE_CHAT);
            Player_PlayVoice(VOICE_TRACK_SELF_INTRO);
            uart_debug_send_string("-> Reply: Track 2 self intro\r\n");
            voice_set_state(current_state, VOICE_STATE_AWAKE);
            break;

        case VOICE_EVENT_TIRED:
            voice_set_state(current_state, VOICE_STATE_EMOTION_TIRED);
            Player_PlayVoice(VOICE_TRACK_TIRED);
            uart_debug_send_string("-> Reply: Track 3 tired comfort\r\n");
            delay_ms(1200);
            Player_PlayVoice(VOICE_TRACK_TIRED_MUSIC);
            uart_debug_send_string("-> Reply: Track 6 tired music prompt\r\n");
            delay_ms(1000);
            voice_set_state(current_state, VOICE_STATE_MUSIC);
            Player_PlayMusic(MUSIC_TRACK_TIRED);
            uart_debug_send_string("-> Music: Track 1 tired mood\r\n");
            voice_set_state(current_state, VOICE_STATE_AWAKE);
            break;

        case VOICE_EVENT_HAPPY:
            voice_set_state(current_state, VOICE_STATE_EMOTION_HAPPY);
            Player_PlayVoice(VOICE_TRACK_HAPPY);
            uart_debug_send_string("-> Reply: Track 4 happy response\r\n");
            delay_ms(1200);
            Player_PlayVoice(VOICE_TRACK_HAPPY_MUSIC);
            uart_debug_send_string("-> Reply: Track 7 happy music prompt\r\n");
            delay_ms(1000);
            voice_set_state(current_state, VOICE_STATE_MUSIC);
            Player_PlayMusic(MUSIC_TRACK_HAPPY);
            uart_debug_send_string("-> Music: Track 2 happy mood\r\n");
            voice_set_state(current_state, VOICE_STATE_AWAKE);
            break;

        case VOICE_EVENT_LIGHT_ON:
        case VOICE_EVENT_LIGHT_OFF:
        case VOICE_EVENT_STOP_OR_REPORT:
        case VOICE_EVENT_ACTION:
            voice_set_state(current_state, VOICE_STATE_ACTION);
            voice_handle_action(event, light_manual_mode);
            voice_set_state(current_state, VOICE_STATE_AWAKE);
            break;

        default:
            uart_debug_send_string("-> Warning: Event has no state branch\r\n");
            break;
    }
}

static void voice_state_tick(voice_state_t* current_state,
                             uint16_t* session_ticks,
                             uint32_t* reminder_ticks,
                             uint8_t* reminder_index)
{
    if(*current_state == VOICE_STATE_AWAKE && *session_ticks > 0)
    {
        (*session_ticks)--;
        if(*session_ticks == 0)
        {
            uart_debug_send_string("-> Session timeout\r\n");
            voice_set_state(current_state, VOICE_STATE_SLEEP);
        }
    }

    if(*current_state == VOICE_STATE_SLEEP)
    {
        (*reminder_ticks)++;
        if(*reminder_ticks >= VOICE_REMINDER_TICKS)
        {
            uint16_t reminder_track = VOICE_TRACK_REMINDER_BASE + *reminder_index;

            *reminder_ticks = 0;
            *reminder_index = (uint8_t)((*reminder_index + 1U) % VOICE_TRACK_REMINDER_COUNT);

            voice_set_state(current_state, VOICE_STATE_REMINDER);
            Player_PlayVoice(reminder_track);
            uart_debug_send_string("-> Reminder: Track 0x");
            uart_debug_print_hex8((uint8_t)reminder_track);
            uart_debug_send_string("\r\n");
            voice_set_state(current_state, VOICE_STATE_SLEEP);
        }
    }
    else
    {
        *reminder_ticks = 0;
    }
}

int main(void)
{
    uint8_t current_volume = VOLUME_MEDIUM;
    uint8_t current_id = 0x00;
    uint8_t last_id = 0x00;
    uint16_t session_ticks = 0;
    uint32_t reminder_ticks = 0;
    uint8_t reminder_index = 0;
    uint8_t light_manual_mode = 0;
    voice_state_t current_state = VOICE_STATE_SLEEP;

    // 1. 初始化 MSPM0 的所有外设（SysConfig 中的配置在此处生效）
    SYSCFG_DL_init();

    // 2. 上电打印提示信息
    uart_debug_send_string("===========================================\r\n");
    uart_debug_send_string("  MSPM0 Voice State Machine Start...       \r\n");
    uart_debug_send_string("===========================================\r\n");

    // 3. 初始化播放器默认设置
    Player_SetVolume(current_volume);
    uart_debug_send_string("-> Player Init: Volume set to 20\r\n");

    uart_debug_send_string("-> State: SLEEP\r\n");

    while(1)
    {
        // 心跳灯：每 100ms 闪烁一次，同时作为主循环的轮询周期
        if(light_manual_mode == 0)
        {
            DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
        }
        delay_ms(VOICE_TICK_MS);
        voice_state_tick(&current_state, &session_ticks, &reminder_ticks, &reminder_index);

        // 从 I2C 语音模块读取当前触发的 ID
        current_id = Voice_Module_ReadID();

        // 只有当识别到新有效指令，且与上一次不同时才触发，避免重复播放
        if(current_id != 0x00 && current_id != last_id)
        {
            voice_handle_event(current_id,
                               &current_state,
                               &session_ticks,
                               &current_volume,
                               &light_manual_mode);
        }

        // 松开语音或未识别时，current_id 返回 0x00，last_id 随之清零，等待下一次唤醒
        last_id = current_id;
    }
}

/******************************************************************
 * 函 数 名 称：uart_debug_send_string
 * 函 数 说 明：向电脑调试串口打印字符串
 ******************************************************************/
void uart_debug_send_string(const char* str)
{
    while(*str != '\0')
    {
        while(DL_UART_isBusy(UART_INST) == true);
        DL_UART_Main_transmitData(UART_INST, *str++);
    }
}

/******************************************************************
 * 函 数 名 称：uart_debug_print_hex8
 * 函 数 说 明：以十六进制格式打印一个字节（例如 0x1C 打印出 "1C"）
 ******************************************************************/
void uart_debug_print_hex8(uint8_t val)
{
    char high = (val >> 4) & 0x0F;
    char low  = val & 0x0F;

    // 发送高 4 位
    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, high < 10 ? high + '0' : high - 10 + 'A');

    // 发送低 4 位
    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, low < 10 ? low + '0' : low - 10 + 'A');
}
