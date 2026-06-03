#include "ti_msp_dl_config.h"
#include "bsp_voice.h"
#include "bsp_player.h"

#define delay_ms(X)        delay_cycles((CPUCLK_FREQ / 1000) * (X))

#define VOICE_TICK_MS                    (100U)
#define VOICE_SESSION_TIMEOUT_TICKS      (120U)     // 12 秒会话窗口
#define VOICE_AUTO_STEP_TICKS            (35U)      // 自动下一句之间保留约 3.5 秒
#define VOICE_REMINDER_TICKS             (27000U)   // 45 分钟休眠主动提醒
#define VOICE_MUSIC_LEADIN_MS            (1500U)

#define VOLUME_MIN                       (5U)
#define VOLUME_STEP                      (5U)
#define VOLUME_MEDIUM                    (20U)
#define VOLUME_MAX                       (30U)

#define VOICE_TRACK_VOLUME_MAX           (75U)
#define VOICE_TRACK_VOLUME_DOWN          (76U)
#define VOICE_TRACK_VOLUME_MEDIUM        (77U)
#define VOICE_TRACK_VOLUME_MIN           (78U)
#define VOICE_TRACK_STOP                 (79U)

#define MUSIC_NONE                       (0U)
#define MUSIC_CALM                       (1U)
#define MUSIC_HAPPY                      (2U)
#define MUSIC_COMFORT                    (3U)
#define MUSIC_FOCUS                      (4U)
#define MUSIC_PLAYFUL                    (5U)
#define MUSIC_SLEEP                      (6U)

#define EVENT_VOLUME_SUPER               (0x04U)
#define EVENT_VOLUME_DOWN                (0x05U)
#define EVENT_VOLUME_MAX                 (0x06U)
#define EVENT_VOLUME_MEDIUM              (0x07U)
#define EVENT_VOLUME_MIN                 (0x08U)

#define EV_WAKE                          (0x20U)
#define EV_TIME                          (0x21U)
#define EV_INTRO                         (0x22U)
#define EV_HELP                          (0x23U)
#define EV_GOODBYE                       (0x24U)
#define EV_STOP                          (0x25U)
#define EV_MOOD_ASK                      (0x26U)
#define EV_HAPPY                         (0x27U)
#define EV_TIRED                         (0x28U)
#define EV_SAD                           (0x29U)
#define EV_ANXIOUS                       (0x2AU)
#define EV_ANGRY                         (0x2BU)
#define EV_BORED                         (0x2CU)
#define EV_NOT_SURE                      (0x2DU)
#define EV_YES                           (0x2EU)
#define EV_NO                            (0x2FU)
#define EV_WORK                          (0x30U)
#define EV_STUDY                         (0x31U)
#define EV_SLEEP_BAD                     (0x32U)
#define EV_BODY_TIRED                    (0x33U)
#define EV_ACHIEVED                      (0x34U)
#define EV_PRAISED                       (0x35U)
#define EV_OUTING                        (0x36U)
#define EV_TALK                          (0x37U)
#define EV_HUG                           (0x38U)
#define EV_SMALL_STEP                    (0x39U)
#define EV_BREATHE                       (0x3AU)
#define EV_FOCUS                         (0x3BU)
#define EV_ENCOURAGE                     (0x3CU)
#define EV_WATER                         (0x3DU)
#define EV_JOKE                          (0x3EU)
#define EV_GAME                          (0x3FU)
#define EV_RIDDLE                        (0x40U)
#define EV_RED                           (0x41U)
#define EV_YELLOW                        (0x42U)
#define EV_BLUE                          (0x43U)
#define EV_FLOWER_JOKE                   (0x44U)
#define EV_TIME_JOKE                     (0x45U)
#define EV_LAMP_JOKE                     (0x46U)
#define EV_ANSWER_FLOWER                 (0x47U)
#define EV_WRONG                         (0x48U)
#define EV_MUSIC_MENU                    (0x49U)
#define EV_MUSIC_CALM                    (0x4AU)
#define EV_MUSIC_HAPPY                   (0x4BU)
#define EV_MUSIC_COMFORT                 (0x4CU)
#define EV_MUSIC_FOCUS                   (0x4DU)
#define EV_MUSIC_PLAYFUL                 (0x4EU)
#define EV_MUSIC_SLEEP                   (0x4FU)
#define EV_START_WORK                    (0x50U)
#define EV_END_WORK                      (0x51U)
#define EV_SUNLIGHT                      (0x52U)
#define EV_NIGHT                         (0x53U)
#define EV_THANKS                        (0x54U)
#define EV_NAME                          (0x55U)

typedef enum {
    S00_SLEEP = 0,
    S01_AWAKE_OPEN,
    S02_HOME_MENU,
    S03_TIME_REPLY,
    S04_IDENTITY_REPLY,
    S05_GOODBYE,
    S06_HELP_REPLY,
    S07_UNKNOWN_REPAIR,
    S10_MOOD_CHECK,
    S11_MOOD_HAPPY_ENTRY,
    S12_HAPPY_SHARE_ASK,
    S13_HAPPY_WORK,
    S14_HAPPY_PRAISE,
    S15_HAPPY_OUTING,
    S16_HAPPY_KEEP,
    S17_HAPPY_MUSIC_OFFER,
    S18_HAPPY_MUSIC_PLAY,
    S19_HAPPY_JOKE_OFFER,
    S20_TIRED_ENTRY,
    S21_TIRED_REASON_ASK,
    S22_TIRED_WORK,
    S23_TIRED_STUDY,
    S24_TIRED_SLEEP,
    S25_TIRED_BODY,
    S26_TIRED_RESET_STEP,
    S27_TIRED_MUSIC_OFFER,
    S28_TIRED_MUSIC_PLAY,
    S30_SAD_ENTRY,
    S31_SAD_COMPANION_ASK,
    S32_SAD_LISTEN,
    S33_SAD_HUG,
    S34_SAD_SMALL_STEP,
    S35_SAD_MUSIC_OFFER,
    S36_SAD_MUSIC_PLAY,
    S40_ANXIOUS_ENTRY,
    S41_BREATH_STEP_1,
    S42_BREATH_STEP_2,
    S43_BREATH_STEP_3,
    S44_ANXIOUS_NEXT_CHOICE,
    S45_FOCUS_MUSIC_PLAY,
    S46_ANGRY_ENTRY,
    S47_ANGRY_COUNTDOWN,
    S48_ANGRY_RELEASE,
    S49_ANGRY_MUSIC_OFFER,
    S50_BORED_ENTRY,
    S51_GAME_MENU,
    S52_PETAL_GAME_START,
    S53_PETAL_RED,
    S54_PETAL_YELLOW,
    S55_PETAL_BLUE,
    S56_PETAL_RESULT,
    S57_JOKE_MENU,
    S58_JOKE_FLOWER,
    S59_JOKE_TIME,
    S60_JOKE_LAMP,
    S61_JOKE_FEEDBACK,
    S62_RIDDLE_START,
    S63_RIDDLE_ANSWER_WAIT,
    S64_RIDDLE_RIGHT,
    S65_RIDDLE_WRONG,
    S70_WATER_REMINDER,
    S71_REST_REMINDER,
    S72_START_WORK,
    S73_END_WORK,
    S74_SUNLIGHT_CHAT,
    S75_NIGHT_CARE,
    S76_COMPLIMENT,
    S77_ENCOURAGE,
    S78_THANK_RESPONSE,
    S79_NAME_RESPONSE,
    S80_MUSIC_MENU,
    S81_MUSIC_CALM,
    S82_MUSIC_HAPPY,
    S83_MUSIC_COMFORT,
    S84_MUSIC_FOCUS,
    S85_MUSIC_PLAYFUL,
    S86_MUSIC_SLEEP,
    VOICE_STATE_COUNT
} voice_state_t;

typedef struct {
    const char* name;
    uint16_t voice_track;
    uint16_t music_track;
    voice_state_t auto_next;
} voice_state_config_t;

typedef struct {
    voice_state_t state;
    uint8_t event_id;
    voice_state_t next_state;
} voice_transition_t;

typedef struct {
    voice_state_t state;
    uint16_t session_ticks;
    uint16_t auto_ticks;
    voice_state_t auto_next;
    uint32_t reminder_ticks;
    uint8_t reminder_index;
    uint8_t current_volume;
    uint8_t heartbeat_divider;
} voice_context_t;

void uart_debug_send_string(const char* str);
void uart_debug_print_hex8(uint8_t val);

#define STATE(name, track, music, next) { name, track, music, next }

static const voice_state_config_t state_config[VOICE_STATE_COUNT] = {
    STATE("S00_SLEEP", 0, MUSIC_NONE, S00_SLEEP),
    STATE("S01_AWAKE_OPEN", 1, MUSIC_NONE, S02_HOME_MENU),
    STATE("S02_HOME_MENU", 0, MUSIC_NONE, S02_HOME_MENU),
    STATE("S03_TIME_REPLY", 2, MUSIC_NONE, S02_HOME_MENU),
    STATE("S04_IDENTITY_REPLY", 3, MUSIC_NONE, S02_HOME_MENU),
    STATE("S05_GOODBYE", 4, MUSIC_NONE, S00_SLEEP),
    STATE("S06_HELP_REPLY", 5, MUSIC_NONE, S02_HOME_MENU),
    STATE("S07_UNKNOWN_REPAIR", 6, MUSIC_NONE, S02_HOME_MENU),
    STATE("S10_MOOD_CHECK", 7, MUSIC_NONE, S10_MOOD_CHECK),
    STATE("S11_MOOD_HAPPY_ENTRY", 8, MUSIC_NONE, S12_HAPPY_SHARE_ASK),
    STATE("S12_HAPPY_SHARE_ASK", 9, MUSIC_NONE, S12_HAPPY_SHARE_ASK),
    STATE("S13_HAPPY_WORK", 10, MUSIC_NONE, S17_HAPPY_MUSIC_OFFER),
    STATE("S14_HAPPY_PRAISE", 11, MUSIC_NONE, S17_HAPPY_MUSIC_OFFER),
    STATE("S15_HAPPY_OUTING", 12, MUSIC_NONE, S17_HAPPY_MUSIC_OFFER),
    STATE("S16_HAPPY_KEEP", 13, MUSIC_NONE, S17_HAPPY_MUSIC_OFFER),
    STATE("S17_HAPPY_MUSIC_OFFER", 14, MUSIC_NONE, S17_HAPPY_MUSIC_OFFER),
    STATE("S18_HAPPY_MUSIC_PLAY", 15, MUSIC_HAPPY, S02_HOME_MENU),
    STATE("S19_HAPPY_JOKE_OFFER", 16, MUSIC_NONE, S19_HAPPY_JOKE_OFFER),
    STATE("S20_TIRED_ENTRY", 17, MUSIC_NONE, S21_TIRED_REASON_ASK),
    STATE("S21_TIRED_REASON_ASK", 18, MUSIC_NONE, S21_TIRED_REASON_ASK),
    STATE("S22_TIRED_WORK", 19, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S23_TIRED_STUDY", 20, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S24_TIRED_SLEEP", 21, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S25_TIRED_BODY", 22, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S26_TIRED_RESET_STEP", 23, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S27_TIRED_MUSIC_OFFER", 24, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S28_TIRED_MUSIC_PLAY", 25, MUSIC_CALM, S02_HOME_MENU),
    STATE("S30_SAD_ENTRY", 26, MUSIC_NONE, S31_SAD_COMPANION_ASK),
    STATE("S31_SAD_COMPANION_ASK", 27, MUSIC_NONE, S31_SAD_COMPANION_ASK),
    STATE("S32_SAD_LISTEN", 28, MUSIC_NONE, S32_SAD_LISTEN),
    STATE("S33_SAD_HUG", 29, MUSIC_NONE, S35_SAD_MUSIC_OFFER),
    STATE("S34_SAD_SMALL_STEP", 30, MUSIC_NONE, S35_SAD_MUSIC_OFFER),
    STATE("S35_SAD_MUSIC_OFFER", 31, MUSIC_NONE, S35_SAD_MUSIC_OFFER),
    STATE("S36_SAD_MUSIC_PLAY", 32, MUSIC_COMFORT, S02_HOME_MENU),
    STATE("S40_ANXIOUS_ENTRY", 33, MUSIC_NONE, S40_ANXIOUS_ENTRY),
    STATE("S41_BREATH_STEP_1", 34, MUSIC_NONE, S42_BREATH_STEP_2),
    STATE("S42_BREATH_STEP_2", 35, MUSIC_NONE, S43_BREATH_STEP_3),
    STATE("S43_BREATH_STEP_3", 36, MUSIC_NONE, S44_ANXIOUS_NEXT_CHOICE),
    STATE("S44_ANXIOUS_NEXT_CHOICE", 37, MUSIC_NONE, S44_ANXIOUS_NEXT_CHOICE),
    STATE("S45_FOCUS_MUSIC_PLAY", 38, MUSIC_FOCUS, S02_HOME_MENU),
    STATE("S46_ANGRY_ENTRY", 39, MUSIC_NONE, S47_ANGRY_COUNTDOWN),
    STATE("S47_ANGRY_COUNTDOWN", 40, MUSIC_NONE, S48_ANGRY_RELEASE),
    STATE("S48_ANGRY_RELEASE", 41, MUSIC_NONE, S48_ANGRY_RELEASE),
    STATE("S49_ANGRY_MUSIC_OFFER", 42, MUSIC_CALM, S02_HOME_MENU),
    STATE("S50_BORED_ENTRY", 43, MUSIC_NONE, S51_GAME_MENU),
    STATE("S51_GAME_MENU", 44, MUSIC_NONE, S51_GAME_MENU),
    STATE("S52_PETAL_GAME_START", 45, MUSIC_NONE, S52_PETAL_GAME_START),
    STATE("S53_PETAL_RED", 46, MUSIC_NONE, S56_PETAL_RESULT),
    STATE("S54_PETAL_YELLOW", 47, MUSIC_NONE, S56_PETAL_RESULT),
    STATE("S55_PETAL_BLUE", 48, MUSIC_NONE, S56_PETAL_RESULT),
    STATE("S56_PETAL_RESULT", 49, MUSIC_NONE, S56_PETAL_RESULT),
    STATE("S57_JOKE_MENU", 50, MUSIC_NONE, S57_JOKE_MENU),
    STATE("S58_JOKE_FLOWER", 51, MUSIC_NONE, S61_JOKE_FEEDBACK),
    STATE("S59_JOKE_TIME", 52, MUSIC_NONE, S61_JOKE_FEEDBACK),
    STATE("S60_JOKE_LAMP", 53, MUSIC_NONE, S61_JOKE_FEEDBACK),
    STATE("S61_JOKE_FEEDBACK", 54, MUSIC_NONE, S61_JOKE_FEEDBACK),
    STATE("S62_RIDDLE_START", 55, MUSIC_NONE, S63_RIDDLE_ANSWER_WAIT),
    STATE("S63_RIDDLE_ANSWER_WAIT", 0, MUSIC_NONE, S63_RIDDLE_ANSWER_WAIT),
    STATE("S64_RIDDLE_RIGHT", 56, MUSIC_NONE, S02_HOME_MENU),
    STATE("S65_RIDDLE_WRONG", 57, MUSIC_NONE, S02_HOME_MENU),
    STATE("S70_WATER_REMINDER", 58, MUSIC_NONE, S02_HOME_MENU),
    STATE("S71_REST_REMINDER", 59, MUSIC_NONE, S02_HOME_MENU),
    STATE("S72_START_WORK", 60, MUSIC_NONE, S45_FOCUS_MUSIC_PLAY),
    STATE("S73_END_WORK", 61, MUSIC_NONE, S27_TIRED_MUSIC_OFFER),
    STATE("S74_SUNLIGHT_CHAT", 62, MUSIC_NONE, S02_HOME_MENU),
    STATE("S75_NIGHT_CARE", 63, MUSIC_SLEEP, S00_SLEEP),
    STATE("S76_COMPLIMENT", 64, MUSIC_NONE, S02_HOME_MENU),
    STATE("S77_ENCOURAGE", 65, MUSIC_NONE, S02_HOME_MENU),
    STATE("S78_THANK_RESPONSE", 66, MUSIC_NONE, S02_HOME_MENU),
    STATE("S79_NAME_RESPONSE", 67, MUSIC_NONE, S02_HOME_MENU),
    STATE("S80_MUSIC_MENU", 68, MUSIC_NONE, S80_MUSIC_MENU),
    STATE("S81_MUSIC_CALM", 69, MUSIC_CALM, S02_HOME_MENU),
    STATE("S82_MUSIC_HAPPY", 70, MUSIC_HAPPY, S02_HOME_MENU),
    STATE("S83_MUSIC_COMFORT", 71, MUSIC_COMFORT, S02_HOME_MENU),
    STATE("S84_MUSIC_FOCUS", 72, MUSIC_FOCUS, S02_HOME_MENU),
    STATE("S85_MUSIC_PLAYFUL", 73, MUSIC_PLAYFUL, S02_HOME_MENU),
    STATE("S86_MUSIC_SLEEP", 74, MUSIC_SLEEP, S00_SLEEP)
};

static const voice_transition_t state_transitions[] = {
    {S02_HOME_MENU, EV_INTRO, S04_IDENTITY_REPLY},
    {S02_HOME_MENU, EV_MOOD_ASK, S10_MOOD_CHECK},
    {S02_HOME_MENU, EV_HAPPY, S11_MOOD_HAPPY_ENTRY},
    {S02_HOME_MENU, EV_TIRED, S20_TIRED_ENTRY},
    {S02_HOME_MENU, EV_SAD, S30_SAD_ENTRY},
    {S02_HOME_MENU, EV_ANXIOUS, S40_ANXIOUS_ENTRY},
    {S02_HOME_MENU, EV_ANGRY, S46_ANGRY_ENTRY},
    {S02_HOME_MENU, EV_BORED, S50_BORED_ENTRY},
    {S02_HOME_MENU, EV_JOKE, S57_JOKE_MENU},
    {S02_HOME_MENU, EV_GAME, S51_GAME_MENU},
    {S02_HOME_MENU, EV_RIDDLE, S62_RIDDLE_START},
    {S02_HOME_MENU, EV_START_WORK, S72_START_WORK},
    {S02_HOME_MENU, EV_END_WORK, S73_END_WORK},
    {S02_HOME_MENU, EV_SUNLIGHT, S74_SUNLIGHT_CHAT},
    {S02_HOME_MENU, EV_NIGHT, S75_NIGHT_CARE},
    {S02_HOME_MENU, EV_THANKS, S78_THANK_RESPONSE},
    {S02_HOME_MENU, EV_NAME, S79_NAME_RESPONSE},
    {S02_HOME_MENU, EV_WATER, S70_WATER_REMINDER},
    {S02_HOME_MENU, EV_FOCUS, S45_FOCUS_MUSIC_PLAY},
    {S02_HOME_MENU, EV_ENCOURAGE, S77_ENCOURAGE},

    {S10_MOOD_CHECK, EV_HAPPY, S11_MOOD_HAPPY_ENTRY},
    {S10_MOOD_CHECK, EV_TIRED, S20_TIRED_ENTRY},
    {S10_MOOD_CHECK, EV_SAD, S30_SAD_ENTRY},
    {S10_MOOD_CHECK, EV_ANXIOUS, S40_ANXIOUS_ENTRY},
    {S10_MOOD_CHECK, EV_ANGRY, S46_ANGRY_ENTRY},
    {S10_MOOD_CHECK, EV_BORED, S50_BORED_ENTRY},
    {S10_MOOD_CHECK, EV_NOT_SURE, S76_COMPLIMENT},

    {S12_HAPPY_SHARE_ASK, EV_ACHIEVED, S13_HAPPY_WORK},
    {S12_HAPPY_SHARE_ASK, EV_PRAISED, S14_HAPPY_PRAISE},
    {S12_HAPPY_SHARE_ASK, EV_OUTING, S15_HAPPY_OUTING},
    {S12_HAPPY_SHARE_ASK, EV_NOT_SURE, S16_HAPPY_KEEP},
    {S17_HAPPY_MUSIC_OFFER, EV_YES, S18_HAPPY_MUSIC_PLAY},
    {S17_HAPPY_MUSIC_OFFER, EV_NO, S19_HAPPY_JOKE_OFFER},
    {S19_HAPPY_JOKE_OFFER, EV_YES, S57_JOKE_MENU},
    {S19_HAPPY_JOKE_OFFER, EV_NO, S02_HOME_MENU},

    {S21_TIRED_REASON_ASK, EV_WORK, S22_TIRED_WORK},
    {S21_TIRED_REASON_ASK, EV_STUDY, S23_TIRED_STUDY},
    {S21_TIRED_REASON_ASK, EV_SLEEP_BAD, S24_TIRED_SLEEP},
    {S21_TIRED_REASON_ASK, EV_BODY_TIRED, S25_TIRED_BODY},
    {S21_TIRED_REASON_ASK, EV_NOT_SURE, S26_TIRED_RESET_STEP},
    {S27_TIRED_MUSIC_OFFER, EV_YES, S28_TIRED_MUSIC_PLAY},
    {S27_TIRED_MUSIC_OFFER, EV_NO, S77_ENCOURAGE},

    {S31_SAD_COMPANION_ASK, EV_TALK, S32_SAD_LISTEN},
    {S31_SAD_COMPANION_ASK, EV_HUG, S33_SAD_HUG},
    {S31_SAD_COMPANION_ASK, EV_SMALL_STEP, S34_SAD_SMALL_STEP},
    {S31_SAD_COMPANION_ASK, EV_MUSIC_MENU, S35_SAD_MUSIC_OFFER},
    {S32_SAD_LISTEN, EV_SAD, S35_SAD_MUSIC_OFFER},
    {S32_SAD_LISTEN, EV_NO, S02_HOME_MENU},
    {S35_SAD_MUSIC_OFFER, EV_YES, S36_SAD_MUSIC_PLAY},
    {S35_SAD_MUSIC_OFFER, EV_NO, S02_HOME_MENU},

    {S40_ANXIOUS_ENTRY, EV_YES, S41_BREATH_STEP_1},
    {S40_ANXIOUS_ENTRY, EV_BREATHE, S41_BREATH_STEP_1},
    {S40_ANXIOUS_ENTRY, EV_NO, S44_ANXIOUS_NEXT_CHOICE},
    {S44_ANXIOUS_NEXT_CHOICE, EV_FOCUS, S45_FOCUS_MUSIC_PLAY},
    {S44_ANXIOUS_NEXT_CHOICE, EV_ENCOURAGE, S77_ENCOURAGE},
    {S46_ANGRY_ENTRY, EV_YES, S47_ANGRY_COUNTDOWN},
    {S48_ANGRY_RELEASE, EV_TALK, S32_SAD_LISTEN},
    {S48_ANGRY_RELEASE, EV_WATER, S70_WATER_REMINDER},
    {S48_ANGRY_RELEASE, EV_YES, S49_ANGRY_MUSIC_OFFER},
    {S48_ANGRY_RELEASE, EV_MUSIC_MENU, S49_ANGRY_MUSIC_OFFER},

    {S51_GAME_MENU, EV_JOKE, S57_JOKE_MENU},
    {S51_GAME_MENU, EV_RIDDLE, S62_RIDDLE_START},
    {S51_GAME_MENU, EV_GAME, S52_PETAL_GAME_START},
    {S52_PETAL_GAME_START, EV_RED, S53_PETAL_RED},
    {S52_PETAL_GAME_START, EV_YELLOW, S54_PETAL_YELLOW},
    {S52_PETAL_GAME_START, EV_BLUE, S55_PETAL_BLUE},
    {S56_PETAL_RESULT, EV_YES, S57_JOKE_MENU},
    {S56_PETAL_RESULT, EV_NO, S02_HOME_MENU},
    {S57_JOKE_MENU, EV_FLOWER_JOKE, S58_JOKE_FLOWER},
    {S57_JOKE_MENU, EV_TIME_JOKE, S59_JOKE_TIME},
    {S57_JOKE_MENU, EV_LAMP_JOKE, S60_JOKE_LAMP},
    {S61_JOKE_FEEDBACK, EV_YES, S57_JOKE_MENU},
    {S61_JOKE_FEEDBACK, EV_NO, S02_HOME_MENU},
    {S63_RIDDLE_ANSWER_WAIT, EV_ANSWER_FLOWER, S64_RIDDLE_RIGHT},
    {S63_RIDDLE_ANSWER_WAIT, EV_WRONG, S65_RIDDLE_WRONG},
    {S63_RIDDLE_ANSWER_WAIT, EV_NOT_SURE, S65_RIDDLE_WRONG},

    {S80_MUSIC_MENU, EV_MUSIC_CALM, S81_MUSIC_CALM},
    {S80_MUSIC_MENU, EV_MUSIC_HAPPY, S82_MUSIC_HAPPY},
    {S80_MUSIC_MENU, EV_MUSIC_COMFORT, S83_MUSIC_COMFORT},
    {S80_MUSIC_MENU, EV_MUSIC_FOCUS, S84_MUSIC_FOCUS},
    {S80_MUSIC_MENU, EV_MUSIC_PLAYFUL, S85_MUSIC_PLAYFUL},
    {S80_MUSIC_MENU, EV_MUSIC_SLEEP, S86_MUSIC_SLEEP}
};

static uint8_t voice_is_volume_event(uint8_t event_id)
{
    return event_id == EVENT_VOLUME_SUPER ||
           event_id == EVENT_VOLUME_MAX ||
           event_id == EVENT_VOLUME_DOWN ||
           event_id == EVENT_VOLUME_MEDIUM ||
           event_id == EVENT_VOLUME_MIN;
}

static uint8_t voice_is_known_dialog_event(uint8_t event_id)
{
    return event_id >= EV_WAKE && event_id <= EV_NAME;
}

static uint8_t voice_is_waiting_state(voice_state_t state)
{
    return state_config[state].auto_next == state;
}

static void voice_refresh_session(voice_context_t* ctx)
{
    ctx->session_ticks = VOICE_SESSION_TIMEOUT_TICKS;
}

static void voice_log_state(voice_state_t from, voice_state_t to)
{
    if(from != to)
    {
        uart_debug_send_string("-> State: ");
        uart_debug_send_string(state_config[from].name);
        uart_debug_send_string(" -> ");
        uart_debug_send_string(state_config[to].name);
        uart_debug_send_string("\r\n");
    }
}

static void voice_enter_state(voice_context_t* ctx, voice_state_t next_state)
{
    const voice_state_config_t* config = &state_config[next_state];

    voice_log_state(ctx->state, next_state);
    ctx->state = next_state;
    ctx->auto_ticks = 0;
    ctx->auto_next = next_state;

    if(next_state != S00_SLEEP)
    {
        voice_refresh_session(ctx);
    }
    else
    {
        ctx->session_ticks = 0;
    }

    if(config->voice_track != 0)
    {
        Player_PlayVoice(config->voice_track);
        uart_debug_send_string("-> Voice track: 0x");
        uart_debug_print_hex8((uint8_t)config->voice_track);
        uart_debug_send_string("\r\n");
    }

    if(config->music_track != MUSIC_NONE)
    {
        if(config->voice_track != 0)
        {
            delay_ms(VOICE_MUSIC_LEADIN_MS);
        }
        Player_PlayMusic(config->music_track);
        uart_debug_send_string("-> Music track: 0x");
        uart_debug_print_hex8((uint8_t)config->music_track);
        uart_debug_send_string("\r\n");
    }

    if(config->auto_next != next_state)
    {
        ctx->auto_next = config->auto_next;
        ctx->auto_ticks = VOICE_AUTO_STEP_TICKS;
    }
}

static void voice_handle_volume(uint8_t event_id, voice_context_t* ctx)
{
    uint16_t reply_track = 0;

    switch(event_id)
    {
        case EVENT_VOLUME_SUPER:
        case EVENT_VOLUME_MAX:
            ctx->current_volume = VOLUME_MAX;
            reply_track = VOICE_TRACK_VOLUME_MAX;
            break;

        case EVENT_VOLUME_DOWN:
            if(ctx->current_volume > (VOLUME_MIN + VOLUME_STEP))
            {
                ctx->current_volume = ctx->current_volume - VOLUME_STEP;
            }
            else
            {
                ctx->current_volume = VOLUME_MIN;
            }
            reply_track = VOICE_TRACK_VOLUME_DOWN;
            break;

        case EVENT_VOLUME_MEDIUM:
            ctx->current_volume = VOLUME_MEDIUM;
            reply_track = VOICE_TRACK_VOLUME_MEDIUM;
            break;

        case EVENT_VOLUME_MIN:
            ctx->current_volume = VOLUME_MIN;
            reply_track = VOICE_TRACK_VOLUME_MIN;
            break;

        default:
            return;
    }

    Player_SetVolume(ctx->current_volume);
    Player_PlayVoice(reply_track);
    uart_debug_send_string("-> Volume set: 0x");
    uart_debug_print_hex8(ctx->current_volume);
    uart_debug_send_string("\r\n");
}

static const voice_transition_t* voice_find_transition(voice_state_t state, uint8_t event_id)
{
    uint16_t i;

    for(i = 0; i < (sizeof(state_transitions) / sizeof(state_transitions[0])); i++)
    {
        if(state_transitions[i].state == state &&
           state_transitions[i].event_id == event_id)
        {
            return &state_transitions[i];
        }
    }

    return 0;
}

static void voice_route_from_home_or_repair(voice_context_t* ctx, uint8_t event_id)
{
    const voice_transition_t* home_transition = voice_find_transition(S02_HOME_MENU, event_id);

    if(home_transition != 0)
    {
        voice_enter_state(ctx, home_transition->next_state);
    }
    else
    {
        voice_enter_state(ctx, S07_UNKNOWN_REPAIR);
    }
}

static void voice_handle_event(uint8_t event_id, voice_context_t* ctx)
{
    const voice_transition_t* transition;

    uart_debug_send_string("Voice Triggered! ID: 0x");
    uart_debug_print_hex8(event_id);
    uart_debug_send_string("\r\n");

    if(voice_is_volume_event(event_id))
    {
        voice_handle_volume(event_id, ctx);
        return;
    }

    if(event_id == EV_STOP)
    {
        Player_Pause();
        Player_PlayVoice(VOICE_TRACK_STOP);
        ctx->auto_ticks = 0;
        ctx->auto_next = S02_HOME_MENU;
        voice_enter_state(ctx, S02_HOME_MENU);
        return;
    }

    if(event_id == EV_WAKE)
    {
        voice_enter_state(ctx, S01_AWAKE_OPEN);
        return;
    }

    if(event_id == EV_TIME)
    {
        voice_enter_state(ctx, S03_TIME_REPLY);
        return;
    }

    if(event_id == EV_HELP)
    {
        voice_enter_state(ctx, S06_HELP_REPLY);
        return;
    }

    if(event_id == EV_GOODBYE)
    {
        voice_enter_state(ctx, S05_GOODBYE);
        return;
    }

    transition = voice_find_transition(ctx->state, event_id);
    if(transition != 0)
    {
        voice_enter_state(ctx, transition->next_state);
        return;
    }

    if(event_id == EV_MUSIC_MENU)
    {
        voice_enter_state(ctx, S80_MUSIC_MENU);
        return;
    }

    if(ctx->state == S00_SLEEP && voice_is_known_dialog_event(event_id))
    {
        voice_route_from_home_or_repair(ctx, event_id);
        return;
    }

    if(voice_is_known_dialog_event(event_id))
    {
        voice_enter_state(ctx, S07_UNKNOWN_REPAIR);
    }
    else
    {
        uart_debug_send_string("-> Warning: Unknown voice ID\r\n");
    }
}

static void voice_play_sleep_reminder(voice_context_t* ctx)
{
    uint16_t track = 58U;

    if(ctx->reminder_index != 0)
    {
        track = 59U;
    }

    ctx->reminder_index = (uint8_t)((ctx->reminder_index + 1U) % 2U);
    Player_PlayVoice(track);
    uart_debug_send_string("-> Sleep reminder track: 0x");
    uart_debug_print_hex8((uint8_t)track);
    uart_debug_send_string("\r\n");
}

static void voice_state_tick(voice_context_t* ctx)
{
    if(ctx->auto_ticks > 0)
    {
        ctx->auto_ticks--;
        if(ctx->auto_ticks == 0)
        {
            voice_enter_state(ctx, ctx->auto_next);
        }
    }

    if(ctx->state != S00_SLEEP && ctx->auto_ticks == 0 && voice_is_waiting_state(ctx->state))
    {
        if(ctx->session_ticks > 0)
        {
            ctx->session_ticks--;
            if(ctx->session_ticks == 0)
            {
                uart_debug_send_string("-> Session timeout\r\n");
                voice_enter_state(ctx, S00_SLEEP);
            }
        }
    }

    if(ctx->state == S00_SLEEP)
    {
        ctx->reminder_ticks++;
        if(ctx->reminder_ticks >= VOICE_REMINDER_TICKS)
        {
            ctx->reminder_ticks = 0;
            voice_play_sleep_reminder(ctx);
        }
    }
    else
    {
        ctx->reminder_ticks = 0;
    }
}

static void voice_update_heartbeat(voice_context_t* ctx)
{
    if(ctx->state == S00_SLEEP)
    {
        ctx->heartbeat_divider++;
        if(ctx->heartbeat_divider >= 10U)
        {
            ctx->heartbeat_divider = 0;
            DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
        }
    }
    else
    {
        ctx->heartbeat_divider = 0;
        DL_GPIO_setPins(LED1_PORT, LED1_PIN_22_PIN);
    }
}

int main(void)
{
    uint8_t current_id = 0x00;
    uint8_t last_id = 0x00;
    voice_context_t voice_ctx = {
        S00_SLEEP,
        0,
        0,
        S00_SLEEP,
        0,
        0,
        VOLUME_MEDIUM,
        0
    };

    SYSCFG_DL_init();

    uart_debug_send_string("===========================================\r\n");
    uart_debug_send_string("  HITune Companion State Machine Start     \r\n");
    uart_debug_send_string("===========================================\r\n");

    Player_SetVolume(voice_ctx.current_volume);
    uart_debug_send_string("-> Player Init: Volume set to 20\r\n");
    uart_debug_send_string("-> State: S00_SLEEP\r\n");

    while(1)
    {
        delay_ms(VOICE_TICK_MS);
        voice_state_tick(&voice_ctx);
        voice_update_heartbeat(&voice_ctx);

        current_id = Voice_Module_ReadID();
        if(current_id != 0x00 && current_id != last_id)
        {
            voice_handle_event(current_id, &voice_ctx);
        }

        last_id = current_id;
    }
}

void uart_debug_send_string(const char* str)
{
    while(*str != '\0')
    {
        while(DL_UART_isBusy(UART_INST) == true);
        DL_UART_Main_transmitData(UART_INST, *str++);
    }
}

void uart_debug_print_hex8(uint8_t val)
{
    char high = (char)((val >> 4) & 0x0F);
    char low  = (char)(val & 0x0F);

    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, high < 10 ? high + '0' : high - 10 + 'A');

    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, low < 10 ? low + '0' : low - 10 + 'A');
}
