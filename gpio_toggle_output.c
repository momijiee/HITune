#include "board.h"
#include "oled.h"
#include "ti_msp_dl_config.h"
#include "bsp_voice.h"
#include "bsp_player.h"
#include <stdbool.h>
#include "stdio.h"

/*================================================================
 *  舵机参数
 *================================================================*/
#define SERVO_PWM_PERIOD        (80000 - 1)
#define SERVO_PULSE_MIN         2000
#define SERVO_PULSE_CENTER      6000
#define SERVO_PULSE_MAX         10000
#define SERVO_ANGLE_MIN         0
#define SERVO_ANGLE_MAX         180
#define SERVO_ANGLE_INIT        90

#define THRESHOLD_HIGH          100
#define THRESHOLD_LOW           30
#define ANGLE_STEP              1

/*================================================================
 *  任务调度周期(ms) — 由 SysTick 1ms 中断驱动
 *================================================================*/
#define TICK_SERVO              50      // 追光控制
#define TICK_STATEMACHINE       100     // 状态机轮询(语音ID读取)
#define TICK_OLED               200     // OLED刷新
#define TICK_DEBUG              500     // 串口调试

/*================================================================
 *  任务标志位（SysTick 中断中置位，主循环中清零并执行）
 *================================================================*/
volatile bool flag_servo  = false;
volatile bool flag_sm     = false;
volatile bool flag_oled   = false;
volatile bool flag_debug  = false;

volatile uint16_t cnt_servo = 0;
volatile uint16_t cnt_sm    = 0;
volatile uint16_t cnt_oled  = 0;
volatile uint16_t cnt_debug = 0;

/*================================================================
 *  延时相关（保留原有 delay_us / delay_ms 供底层驱动使用）
 *================================================================*/
volatile unsigned int delay_times = 0;

/*================================================================
 *  ADC 相关
 *================================================================*/
volatile bool gCheckADC = false;
unsigned int g_adc_ch27 = 0;
unsigned int g_adc_ch26 = 0;
int g_diff = 0;
int current_angle = SERVO_ANGLE_INIT;

/*================================================================
 *  UART 接收
 *================================================================*/
#define RE_0_BUFF_LEN_MAX  128
volatile uint8_t  recv0_buff[RE_0_BUFF_LEN_MAX] = {0};
volatile uint16_t recv0_length = 0;
volatile uint8_t  recv0_flag = 0;

/*================================================================
 *  状态机定义
 *================================================================*/
typedef enum {
    ST_IDLE = 0, ST_WAKE, ST_TIME, ST_BYE,
    ST_SAD_RECV, ST_PLAY_SAD_MUSIC, ST_SAD_TALK, ST_SAD_BETTER,
    ST_SAD_STILL, ST_SAD_REBEL, ST_SAD_DEEP, ST_SILENT_GUARD, ST_WAKE_UP_SUN,
    ST_HAPPY_RECV, ST_HAPPY_CELEBRATE, ST_HAPPY_SHY, ST_HAPPY_END, ST_HAPPY_QUIET,
    ST_JOKE_1, ST_JOKE_1_GOOD, ST_JOKE_1_BAD,
    ST_JOKE_2, ST_JOKE_2_GOOD, ST_JOKE_2_BAD,
    ST_JOKE_3, ST_JOKE_END, ST_JOKE_WIN, ST_JOKE_LOSE,
    ST_GAME_START, ST_GAME_Q1, ST_GAME_Q2_A, ST_GAME_Q2_B,
    ST_RES_SUNFLOWER, ST_RES_CACTUS, ST_RES_MIMOSA, ST_RES_MUSHROOM,
    ST_GAME_OVER, ST_GAME_HAPPY, ST_GAME_SAD,
    ST_CONFUSED, ST_EGG_1, ST_EGG_2
} PlayState_t;

typedef enum {
    EVENT_NONE = 0, EVENT_HELLO, EVENT_TIME, EVENT_SAD, EVENT_HAPPY,
    EVENT_JOKE, EVENT_GAME, EVENT_YES, EVENT_NO, EVENT_A, EVENT_B, EVENT_TIMEOUT
} StateEvent_t;

PlayState_t g_current_state = ST_IDLE;
uint32_t g_timeout_counter = 0;
uint8_t g_hello_repeat_count = 0;

/*================================================================
 *  非阻塞延时（用于状态机中等待语音播放完成）
 *  在状态机中用 delay_ms 等待播放，此期间追光和OLED仍然靠
 *  中断标志位在 delay_us 内部的循环缝隙中被处理
 *================================================================*/

// 增强版delay_ms：等待期间仍执行追光和OLED
void Task_ServoControl(void);
void Task_OledUpdate(void);
void Task_DebugPrint(void);

void delay_ms_with_tasks(unsigned long ms)
{
    uint32_t start = cnt_debug; // 借用一个计数器做粗略计时
    volatile uint32_t elapsed = 0;

    // 用原始delay实现，但在间隙检查标志位
    while (elapsed < ms)
    {
        delay_ms(1);
        elapsed++;

        // 在等待期间仍然执行高优先级任务
        if (flag_servo)
        {
            flag_servo = false;
            Task_ServoControl();
        }
        if (flag_oled)
        {
            flag_oled = false;
            Task_OledUpdate();
        }
    }
}

/*================================================================
 *  函数声明
 *================================================================*/
void Servo_PWM_Init(void);
void Servo_SetAngle(uint16_t angle);
void adc_readBothChannels(unsigned int *ch27_val, unsigned int *ch26_val);
StateEvent_t MapVoiceIdToEvent(uint8_t voice_id);
void StateMachine_Process(StateEvent_t event);
void uart_debug_send_string(char* str);
void uart_debug_print_hex8(uint8_t val);


/*================================================================
 *  舵机控制
 *================================================================*/
uint32_t Servo_AngleToCompare(uint16_t angle)
{
    if (angle > 180) angle = 180;
    return SERVO_PULSE_MIN
         + (uint32_t)(SERVO_PULSE_MAX - SERVO_PULSE_MIN) * angle / 180;
}

void Servo_SetAngle(uint16_t angle)
{
    DL_TimerA_setCaptureCompareValue(PWM_INST,
        Servo_AngleToCompare(angle), DL_TIMER_CC_0_INDEX);
    current_angle = (int)angle;
}

void Servo_PWM_Init(void)
{
    DL_TimerA_stopCounter(PWM_INST);
    DL_TimerA_setLoadValue(PWM_INST, SERVO_PWM_PERIOD);
    DL_TimerA_setCaptureCompareValue(PWM_INST, SERVO_PULSE_CENTER, DL_TIMER_CC_0_INDEX);
    DL_TimerA_startCounter(PWM_INST);
}

/*================================================================
 *  ADC 读取
 *================================================================*/
void adc_readBothChannels(unsigned int *ch27_val, unsigned int *ch26_val)
{
    gCheckADC = false;
    DL_ADC12_startConversion(ADC_INST);

    uint32_t timeout = 100000;
    while (false == gCheckADC && timeout > 0) { timeout--; }

    if (gCheckADC)
    {
        *ch27_val = DL_ADC12_getMemResult(ADC_INST, DL_ADC12_MEM_IDX_0);
        *ch26_val = DL_ADC12_getMemResult(ADC_INST, DL_ADC12_MEM_IDX_1);
    }
    DL_ADC12_enableConversions(ADC_INST);
}

/*================================================================
 *  任务1：追光控制（每50ms）
 *================================================================*/
void Task_ServoControl(void)
{
    adc_readBothChannels(&g_adc_ch27, &g_adc_ch26);
    g_diff = (int)g_adc_ch26 - (int)g_adc_ch27;

    if (g_diff > THRESHOLD_HIGH)
    {
        if (current_angle > SERVO_ANGLE_MIN)
        {
            current_angle -= ANGLE_STEP;
            if (current_angle < SERVO_ANGLE_MIN)
                current_angle = SERVO_ANGLE_MIN;
            Servo_SetAngle((uint16_t)current_angle);
        }
    }
    else if (g_diff < -THRESHOLD_HIGH)
    {
        if (current_angle < SERVO_ANGLE_MAX)
        {
            current_angle += ANGLE_STEP;
            if (current_angle > SERVO_ANGLE_MAX)
                current_angle = SERVO_ANGLE_MAX;
            Servo_SetAngle((uint16_t)current_angle);
        }
    }
}

/*================================================================
 *  任务2：OLED 显示（每200ms）
 *================================================================*/

// 获取状态名称字符串
const char* GetStateName(PlayState_t state)
{
    switch (state)
    {
        case ST_IDLE:           return "IDLE    ";
        case ST_WAKE:           return "WAKE    ";
        case ST_TIME:           return "TIME    ";
        case ST_BYE:            return "BYE     ";
        case ST_SAD_RECV:       return "SAD     ";
        case ST_PLAY_SAD_MUSIC: return "SAD MUS ";
        case ST_SAD_TALK:       return "SAD TALK";
        case ST_SAD_BETTER:     return "BETTER  ";
        case ST_HAPPY_RECV:     return "HAPPY   ";
        case ST_HAPPY_CELEBRATE:return "CELEBR  ";
        case ST_JOKE_1:         return "JOKE 1  ";
        case ST_JOKE_2:         return "JOKE 2  ";
        case ST_JOKE_3:         return "JOKE 3  ";
        case ST_GAME_START:     return "GAME    ";
        case ST_GAME_Q1:        return "GAME Q1 ";
        case ST_GAME_Q2_A:      return "GAME Q2A";
        case ST_GAME_Q2_B:      return "GAME Q2B";
        case ST_SILENT_GUARD:   return "SILENT  ";
        case ST_EGG_1:          return "EGG!    ";
        default:                return "OTHER   ";
    }
}

// 整数转字符串（简易版，用于OLED显示）
void IntToStr(int val, char *buf, int len)
{
    int i;
    bool neg = false;

    if (val < 0) {
        neg = true;
        val = -val;
    }

    for (i = len - 1; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }

    if (neg && len > 0) buf[0] = '-';
    buf[len] = '\0';
}

void Task_OledUpdate(void)
{
    char line_buf[22];  // OLED每行最多约21个字符(128/6)
    unsigned int v27, v26;

    OLED_Clear();

    // 第1行：状态显示 (y=0, 12号字体)
    // "ST:WAKE    "
    sprintf(line_buf, "ST:%s", GetStateName(g_current_state));
    OLED_ShowString(0, 0, (uint8_t *)line_buf, 12, 1);

    // 第2行：舵机角度 (y=12)
    sprintf(line_buf, "Angle: %3d deg", current_angle);
    OLED_ShowString(0, 12, (uint8_t *)line_buf, 12, 1);

    // 第3行：ADC值 (y=24)
    sprintf(line_buf, "L:%4d  R:%4d", g_adc_ch27, g_adc_ch26);
    OLED_ShowString(0, 24, (uint8_t *)line_buf, 12, 1);

    // 第4行：电压值 (y=36)
    v27 = (unsigned int)((g_adc_ch27 / 4095.0 * 3.3) * 100);
    v26 = (unsigned int)((g_adc_ch26 / 4095.0 * 3.3) * 100);
    sprintf(line_buf, "%d.%d%dV    %d.%d%dV",
            v27/100, v27/10%10, v27%10,
            v26/100, v26/10%10, v26%10);
    OLED_ShowString(0, 36, (uint8_t *)line_buf, 12, 1);

    // 第5行：差值与方向指示 (y=48)
    if (g_diff > THRESHOLD_HIGH)
        sprintf(line_buf, "d:%4d  <<< LEFT", g_diff);
    else if (g_diff < -THRESHOLD_HIGH)
        sprintf(line_buf, "d:%4d  RIGHT>>>", g_diff);
    else
        sprintf(line_buf, "d:%4d  =CENTER=", g_diff);
    OLED_ShowString(0, 48, (uint8_t *)line_buf, 12, 1);

    OLED_Refresh();
}

/*================================================================
 *  任务3：状态机轮询（每100ms）
 *================================================================*/
uint8_t g_last_voice_id = 0x00;

void Task_StateMachine(void)
{
    uint8_t current_id = Voice_Module_ReadID();
    StateEvent_t cur_event = EVENT_NONE;

    if (current_id != 0x00 && current_id != g_last_voice_id)
    {
        uart_debug_send_string("VID:0x");
        uart_debug_print_hex8(current_id);
        uart_debug_send_string("\r\n");

        cur_event = MapVoiceIdToEvent(current_id);
        g_timeout_counter = 0;
    }
    else if (current_id == 0x00)
    {
        if (g_current_state != ST_IDLE && g_current_state != ST_SILENT_GUARD)
        {
            g_timeout_counter++;
            if (g_timeout_counter >= 100)  // 100 * 100ms = 10s
            {
                cur_event = EVENT_TIMEOUT;
                g_timeout_counter = 0;
                uart_debug_send_string("TIMEOUT\r\n");
            }
        }
    }

    if (cur_event != EVENT_NONE)
    {
        StateMachine_Process(cur_event);
    }

    g_last_voice_id = current_id;
}

/*================================================================
 *  任务4：串口调试（每500ms）
 *================================================================*/
void Task_DebugPrint(void)
{
    unsigned int v27 = (unsigned int)((g_adc_ch27 / 4095.0 * 3.3) * 100);
    unsigned int v26 = (unsigned int)((g_adc_ch26 / 4095.0 * 3.3) * 100);

    printf("[%s] CH27:%d(%d.%d%dV) CH26:%d(%d.%d%dV) d:%d a:%d\r\n",
           GetStateName(g_current_state),
           g_adc_ch27, v27/100, v27/10%10, v27%10,
           g_adc_ch26, v26/100, v26/10%10, v26%10,
           g_diff, current_angle);
}

/*================================================================
 *  主函数
 *================================================================*/
int main(void)
{
    board_init();
    OLED_Init();
    OLED_Clear();

    // ADC中断使能
    NVIC_EnableIRQ(ADC_INST_INT_IRQN);

    // 舵机初始化
    Servo_PWM_Init();
    Servo_SetAngle(SERVO_ANGLE_INIT);

    // DFPlayer 音量
    Player_SetVolume(20);

    // 开机画面
    OLED_ShowString(0, 0,  (uint8_t *)"================", 12, 1);
    OLED_ShowString(0, 16, (uint8_t *)" Flower Companion", 12, 1);
    OLED_ShowString(0, 32, (uint8_t *)"  System  Ready", 12, 1);
    OLED_ShowString(0, 48, (uint8_t *)"================", 12, 1);
    OLED_Refresh();

    uart_debug_send_string("=== Flower Companion System ===\r\n");
    uart_debug_send_string("Servo + ADC + StateMachine + OLED\r\n");

    delay_ms(2000);

    /*================================================================
     *  主循环：标志位驱动的任务调度
     *
     *  SysTick 每1ms中断一次，递增计数器，到期置标志
     *  主循环检查标志，执行对应任务后清除标志
     *
     *  优先级：舵机 > 状态机 > OLED > 调试打印
     *================================================================*/
    while (1)
    {
        // 最高优先级：追光舵机控制（50ms）
        if (flag_servo)
        {
            flag_servo = false;
            Task_ServoControl();
        }

        // 次优先级：状态机轮询（100ms）
        if (flag_sm)
        {
            flag_sm = false;
            Task_StateMachine();
        }

        // OLED刷新（200ms）
        if (flag_oled)
        {
            flag_oled = false;
            Task_OledUpdate();
        }

        // 调试打印（500ms）
        if (flag_debug)
        {
            flag_debug = false;
            Task_DebugPrint();
        }
    }
}

/*================================================================
 *  事件映射（与原代码完全一致）
 *================================================================*/
StateEvent_t MapVoiceIdToEvent(uint8_t voice_id)
{
    switch(voice_id)
    {
        case 0x10: case 0x11: case 0x12: return EVENT_HELLO;
        case 0x20: return EVENT_TIME;
        case 0x30: case 0x31: case 0x32: case 0x33: return EVENT_SAD;
        case 0x40: case 0x41: return EVENT_HAPPY;
        case 0x50: case 0x51: return EVENT_JOKE;
        case 0x60: return EVENT_GAME;
        case 0x70: case 0x71: case 0x72: case 0x73: return EVENT_YES;
        case 0x80: case 0x81: case 0x82: case 0x83: return EVENT_NO;
        case 0xA0: case 0xA1: return EVENT_A;
        case 0xB0: case 0xB1: return EVENT_B;
        default: return EVENT_NONE;
    }
}

/*================================================================
 *  状态机引擎
 *  关键修改：所有 delay_ms 替换为 delay_ms_with_tasks
 *  这样在等待语音播放期间，追光和OLED仍然持续工作
 *================================================================*/
void StateMachine_Process(StateEvent_t event)
{
    if (event == EVENT_HELLO && g_current_state != ST_IDLE) {
        g_hello_repeat_count++;
        if (g_hello_repeat_count >= 3) {
            g_current_state = ST_EGG_1;
            Player_PlayVoice(53);
            g_hello_repeat_count = 0;
            return;
        }
    } else if (event != EVENT_NONE) {
        g_hello_repeat_count = 0;
    }

    switch(g_current_state)
    {
        /* --- 基础服务与唤醒 --- */
        case ST_IDLE:
            if(event == EVENT_HELLO) {
                g_current_state = ST_WAKE;
                Player_PlayVoice(2);
            } else if(event == EVENT_SAD) {
                g_current_state = ST_SAD_RECV;
                Player_PlayVoice(11);
            }
            break;

        case ST_WAKE:
            if(event == EVENT_TIME) {
                g_current_state = ST_TIME;
                Player_PlayVoice(3);
                delay_ms_with_tasks(5000);
                g_current_state = ST_WAKE;
            } else if(event == EVENT_SAD) {
                g_current_state = ST_SAD_RECV;
                Player_PlayVoice(11);
            } else if(event == EVENT_HAPPY) {
                g_current_state = ST_HAPPY_RECV;
                Player_PlayVoice(21);
            } else if(event == EVENT_JOKE) {
                g_current_state = ST_JOKE_1;
                Player_PlayVoice(31);
            } else if(event == EVENT_GAME) {
                g_current_state = ST_GAME_START;
                Player_PlayVoice(41);
            } else if(event == EVENT_TIMEOUT || event == EVENT_NO) {
                g_current_state = ST_BYE;
                Player_PlayVoice(4);
                delay_ms_with_tasks(3000);
                g_current_state = ST_IDLE;
            }
            break;

        /* --- 负面情绪陪伴模块 --- */
        case ST_SAD_RECV:
            if(event == EVENT_YES || event == EVENT_SAD) {
                g_current_state = ST_PLAY_SAD_MUSIC;
                Player_PlayMusic(1);
                Player_PlayVoice(12);
            } else if(event == EVENT_NO) {
                g_current_state = ST_SAD_TALK;
                Player_PlayVoice(13);
            }
            break;

        case ST_PLAY_SAD_MUSIC:
            if(event == EVENT_YES || event == EVENT_HAPPY) {
                g_current_state = ST_SAD_BETTER;
                Player_PlayVoice(14);
            } else if(event == EVENT_NO || event == EVENT_SAD) {
                g_current_state = ST_SAD_STILL;
                Player_PlayVoice(15);
            }
            break;

        case ST_SAD_TALK:
            if(event == EVENT_YES) {
                g_current_state = ST_SAD_BETTER;
                Player_PlayVoice(14);
            } else if(event == EVENT_NO) {
                g_current_state = ST_SAD_REBEL;
                Player_PlayVoice(16);
                delay_ms_with_tasks(3000);
                g_current_state = ST_IDLE;
            }
            break;

        case ST_SAD_BETTER:
            if(event == EVENT_YES) {
                g_current_state = ST_JOKE_1;
                Player_PlayVoice(31);
            } else {
                g_current_state = ST_BYE;
                Player_PlayVoice(4);
                delay_ms_with_tasks(3000);
                g_current_state = ST_IDLE;
            }
            break;

        case ST_SAD_STILL:
            if(event == EVENT_YES) {
                g_current_state = ST_BYE;
                Player_PlayVoice(4);
                delay_ms_with_tasks(3000);
                g_current_state = ST_IDLE;
            } else if(event == EVENT_NO) {
                g_current_state = ST_SAD_DEEP;
                Player_PlayVoice(17);
                delay_ms_with_tasks(6000);
                g_current_state = ST_SILENT_GUARD;
            }
            break;

        case ST_SILENT_GUARD:
            if(event == EVENT_HELLO) {
                g_current_state = ST_WAKE_UP_SUN;
                Player_PlayVoice(19);
                delay_ms_with_tasks(3000);
                g_current_state = ST_WAKE;
            }
            break;

        /* --- 正面情绪共享模块 --- */
        case ST_HAPPY_RECV:
            if(event == EVENT_HAPPY || event == EVENT_YES) {
                g_current_state = ST_HAPPY_CELEBRATE;
                Player_PlayMusic(2);
                Player_PlayVoice(22);
            } else if(event == EVENT_NO) {
                g_current_state = ST_HAPPY_SHY;
                Player_PlayVoice(23);
                delay_ms_with_tasks(4000);
                g_current_state = ST_WAKE;
            }
            break;

        case ST_HAPPY_CELEBRATE:
            if(event == EVENT_YES || event == EVENT_HAPPY) {
                g_current_state = ST_HAPPY_END;
                Player_PlayVoice(24);
                delay_ms_with_tasks(3000);
                g_current_state = ST_IDLE;
            } else if(event == EVENT_NO) {
                g_current_state = ST_HAPPY_QUIET;
                Player_PlayVoice(25);
                delay_ms_with_tasks(3000);
                g_current_state = ST_WAKE;
            }
            break;

        /* --- 连续冷笑话模块 --- */
        case ST_JOKE_1:
            if(event == EVENT_YES || event == EVENT_HAPPY) {
                g_current_state = ST_JOKE_1_GOOD;
                Player_PlayVoice(32);
            } else {
                g_current_state = ST_JOKE_1_BAD;
                Player_PlayVoice(33);
                delay_ms_with_tasks(3000);
                g_current_state = ST_JOKE_2;
                Player_PlayVoice(34);
            }
            break;

        case ST_JOKE_1_GOOD:
            if(event == EVENT_YES) {
                g_current_state = ST_JOKE_2;
                Player_PlayVoice(34);
            } else {
                g_current_state = ST_WAKE;
            }
            break;

        case ST_JOKE_2:
            if(event == EVENT_YES) {
                g_current_state = ST_JOKE_2_GOOD;
                Player_PlayVoice(35);
            } else {
                g_current_state = ST_JOKE_2_BAD;
                Player_PlayVoice(36);
                delay_ms_with_tasks(3000);
                g_current_state = ST_JOKE_3;
                Player_PlayVoice(37);
            }
            break;

        case ST_JOKE_2_GOOD:
            if(event == EVENT_YES) {
                g_current_state = ST_JOKE_3;
                Player_PlayVoice(37);
            } else {
                g_current_state = ST_WAKE;
            }
            break;

        case ST_JOKE_3:
            g_current_state = ST_JOKE_END;
            Player_PlayVoice(38);
            break;

        case ST_JOKE_END:
            if(event == EVENT_YES) {
                g_current_state = ST_JOKE_WIN;
                Player_PlayVoice(39);
                delay_ms_with_tasks(3000);
                g_current_state = ST_WAKE;
            } else {
                g_current_state = ST_JOKE_LOSE;
                Player_PlayVoice(40);
                delay_ms_with_tasks(4000);
                g_current_state = ST_GAME_START;
                Player_PlayVoice(41);
            }
            break;

        /* --- 互动小游戏模块 --- */
        case ST_GAME_START:
            if(event == EVENT_YES) {
                g_current_state = ST_GAME_Q1;
                Player_PlayVoice(42);
            } else {
                g_current_state = ST_WAKE;
            }
            break;

        case ST_GAME_Q1:
            if(event == EVENT_A) {
                g_current_state = ST_GAME_Q2_A;
                Player_PlayVoice(43);
            } else if(event == EVENT_B) {
                g_current_state = ST_GAME_Q2_B;
                Player_PlayVoice(44);
            }
            break;

        case ST_GAME_Q2_A:
            if(event == EVENT_A) {
                g_current_state = ST_RES_SUNFLOWER;
                Player_PlayVoice(45);
            } else if(event == EVENT_B) {
                g_current_state = ST_RES_CACTUS;
                Player_PlayVoice(46);
            }
            delay_ms_with_tasks(8000);
            g_current_state = ST_GAME_OVER;
            Player_PlayVoice(49);
            break;

        case ST_GAME_Q2_B:
            if(event == EVENT_A) {
                g_current_state = ST_RES_MIMOSA;
                Player_PlayVoice(47);
            } else if(event == EVENT_B) {
                g_current_state = ST_RES_MUSHROOM;
                Player_PlayVoice(48);
            }
            delay_ms_with_tasks(8000);
            g_current_state = ST_GAME_OVER;
            Player_PlayVoice(49);
            break;

        case ST_GAME_OVER:
            if(event == EVENT_YES) {
                g_current_state = ST_GAME_HAPPY;
                Player_PlayVoice(50);
            } else {
                g_current_state = ST_GAME_SAD;
                Player_PlayVoice(51);
            }
            delay_ms_with_tasks(3000);
            g_current_state = ST_WAKE;
            break;

        default:
            g_current_state = ST_WAKE;
            break;
    }
}

/*================================================================
 *  调试工具函数
 *================================================================*/
void uart_debug_send_string(char* str)
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
    DL_UART_Main_transmitData(UART_INST, high < 10 ? high+'0' : high-10+'A');
    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, low < 10 ? low+'0' : low-10+'A');
}

/*================================================================
 *  中断处理函数
 *================================================================*/

// SysTick: 每1ms中断，驱动所有任务调度
void SysTick_Handler(void)
{
    // 保留原有delay功能
    if (delay_times != 0) {
        delay_times--;
    }

    // 舵机任务 50ms
    cnt_servo++;
    if (cnt_servo >= TICK_SERVO) {
        cnt_servo = 0;
        flag_servo = true;
    }

    // 状态机任务 100ms
    cnt_sm++;
    if (cnt_sm >= TICK_STATEMACHINE) {
        cnt_sm = 0;
        flag_sm = true;
    }

    // OLED任务 200ms
    cnt_oled++;
    if (cnt_oled >= TICK_OLED) {
        cnt_oled = 0;
        flag_oled = true;
    }

    // 调试打印任务 500ms
    cnt_debug++;
    if (cnt_debug >= TICK_DEBUG) {
        cnt_debug = 0;
        flag_debug = true;
    }
}

// ADC中断
void ADC_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC_INST))
    {
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            gCheckADC = true;
            break;
        default:
            break;
    }
}

// UART接收中断
void UART_INST_IRQHandler(void)
{
    uint8_t receivedData = 0;
    switch(DL_UART_getPendingInterrupt(UART_INST))
    {
        case DL_UART_IIDX_RX:
            receivedData = DL_UART_Main_receiveData(UART_INST);
            if (recv0_length < RE_0_BUFF_LEN_MAX - 1)
                recv0_buff[recv0_length++] = receivedData;
            else
                recv0_length = 0;
            recv0_flag = 1;
            break;
        default:
            break;
    }
}
