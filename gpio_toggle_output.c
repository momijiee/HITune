#include "board.h"
#include "oled.h"
#include "ti_msp_dl_config.h"
#include "bsp_voice.h"
#include "bsp_player.h"
#include <stdbool.h>


// ---- 状态枚举定义 (共54个状态，此处精简列出核心及代表性状态) ----
typedef enum {
    ST_IDLE = 0,
    ST_WAKE,
    ST_TIME,
    ST_BYE,
    
    // 负面情绪模块
    ST_SAD_RECV,
    ST_PLAY_SAD_MUSIC,
    ST_SAD_TALK,
    ST_SAD_BETTER,
    ST_SAD_STILL,
    ST_SAD_REBEL,
    ST_SAD_DEEP,
    ST_SILENT_GUARD,
    ST_WAKE_UP_SUN,
    
    // 正面情绪模块
    ST_HAPPY_RECV,
    ST_HAPPY_CELEBRATE,
    ST_HAPPY_SHY,
    ST_HAPPY_END,
    ST_HAPPY_QUIET,
    
    // 冷笑话模块
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
    
    // 游戏模块
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
    
    // 特殊/异常状态
    ST_CONFUSED,
    ST_EGG_1,
    ST_EGG_2
} PlayState_t;

// ---- 抽象出的状态机逻辑输入事件 ----
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
    EVENT_TIMEOUT
} StateEvent_t;

// 全局状态变量
PlayState_t g_current_state = ST_IDLE;
uint32_t g_timeout_counter = 0;
uint8_t g_hello_repeat_count = 0; // 连续调戏计数器

// 函数声明
StateEvent_t MapVoiceIdToEvent(uint8_t voice_id);
void StateMachine_Process(StateEvent_t event);
void uart_debug_send_string(char* str);
void uart_debug_print_hex8(uint8_t val);

int main(void)
{
		board_init();
    OLED_Init();    
    OLED_Clear();
    SYSCFG_DL_init();
    Player_SetVolume(20);
    
    uint8_t current_id = 0x00;
    uint8_t last_id = 0x00;
    
    uart_debug_send_string("Flower Companion State Machine Ready.\r\n");

    while(1)
    {
				OLED_ShowString(0,0,(uint8_t *)"ABC",8,1);//6*8 “ABC”
        OLED_ShowString(0,8,(uint8_t *)"ABC",12,1);//6*12 “ABC”
        OLED_ShowString(0,20,(uint8_t *)"ABC",16,1);//8*16 “ABC”
        OLED_ShowString(0,36,(uint8_t *)"ABC",24,1);//12*24 “ABC”
        OLED_Refresh();	
			
        delay_ms(100); 

        current_id = Voice_Module_ReadID();
        StateEvent_t cur_event = EVENT_NONE;
			
        // 1. 硬件层去重与事件映射
        if(current_id != 0x00 && current_id != last_id)
        {
            uart_debug_send_string("HW ID Triggered: 0x");
            uart_debug_print_hex8(current_id);
            uart_debug_send_string("\r\n");
            
            cur_event = MapVoiceIdToEvent(current_id);
            g_timeout_counter = 0; // 收到任意有效语音，重置超时计数
        }
        else if(current_id == 0x00)
        {
            // 2. 超时检测机制（非休眠状态下，约 10 秒无人说话触发超时事件）
            if(g_current_state != ST_IDLE && g_current_state != ST_SILENT_GUARD)
            {
                g_timeout_counter++;
                if(g_timeout_counter >= 250) // 100 * 100ms = 10s
                {
                    cur_event = EVENT_TIMEOUT;
                    g_timeout_counter = 0;
                    uart_debug_send_string("Event: Timeout Triggered\r\n");
                }
            }
        }

        // 3. 驱动状态机核心
        if(cur_event != EVENT_NONE)
        {
            StateMachine_Process(cur_event);
        }

        last_id = current_id; 
    }
}

/******************************************************************
 * 映射层：将单点拆分后的硬件 ID 转换为状态机输入事件 (严格一词一 ID)
 ******************************************************************/
StateEvent_t MapVoiceIdToEvent(uint8_t voice_id)
{
    switch(voice_id)
    {
        // 1. 唤醒词聚合
        case 0x10: 
        case 0x11: 
        case 0x12: return EVENT_HELLO;
        
        // 2. 问时间
        case 0x20: return EVENT_TIME;
        
        // 3. 负面情绪词包聚合
        case 0x30: 
        case 0x31: 
        case 0x32: 
        case 0x33: return EVENT_SAD;
        
        // 4. 正面情绪词包聚合
        case 0x40: 
        case 0x41: return EVENT_HAPPY;
        
        // 5. 娱乐诉求聚合
        case 0x50: 
        case 0x51: return EVENT_JOKE;
        case 0x60: return EVENT_GAME;
        
        // 6. 肯定词包聚合 (对应 0x70 ~ 0x73)
        case 0x70: // 是的
        case 0x71: // 好啊
        case 0x72: // 喜欢
        case 0x73: // 要听
            return EVENT_YES;
            
        // 7. 否定词包聚合 (对应 0x80 ~ 0x83)
        case 0x80: // 不要
        case 0x81: // 不是
        case 0x82: // 算了
        case 0x83: // 再见
            return EVENT_NO;
            
        // 8. 游戏选项 A 聚合
        case 0xA0: // 选项A
        case 0xA1: // 第一个
            return EVENT_A;
            
        // 9. 游戏选项 B 聚合
        case 0xB0: // 选项B
        case 0xB1: // 第二个
            return EVENT_B;
            
        // 兜底：未匹配到有效指令
        default: return EVENT_NONE;
    }
}

/******************************************************************
 * 核心状态机引擎 (按模块深度嵌套运行)
 ******************************************************************/
void StateMachine_Process(StateEvent_t event)
{
    // 防刷策略：非初始状态下连续打招呼触发彩蛋
    if (event == EVENT_HELLO && g_current_state != ST_IDLE) {
        g_hello_repeat_count++;
        if (g_hello_repeat_count >= 3) {
            g_current_state = ST_EGG_1;
            Player_PlayVoice(53); // 触发彩蛋1语音
            g_hello_repeat_count = 0;
            return;
        }
    } else if (event != EVENT_NONE) {
        g_hello_repeat_count = 0;
    }

    switch(g_current_state)
    {
        /* --- 1. 基础服务与唤醒 --- */
        case ST_IDLE:
            if(event == EVENT_HELLO) {
                // 此处可以加入时间段判断逻辑，进入深夜彩蛋 ST_EGG_2，此处默认正常唤醒
                g_current_state = ST_WAKE;
                Player_PlayVoice(2); // 播放唤醒问候
            } else if(event == EVENT_SAD) {
                g_current_state = ST_SAD_RECV;
                Player_PlayVoice(11); // 直达安慰
            }
            break;

        case ST_WAKE:
            if(event == EVENT_TIME) {
                g_current_state = ST_TIME;
                Player_PlayVoice(3); // 播报时间
                delay_ms(5000);      // 等待播放完自动切回
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
                delay_ms(3000);
                g_current_state = ST_IDLE;
            }
            break;

        /* --- 2. 负面情绪陪伴模块 --- */
        case ST_SAD_RECV:
            if(event == EVENT_YES || event == EVENT_SAD) {
                g_current_state = ST_PLAY_SAD_MUSIC;
                Player_PlayMusic(1); // 播放 TF 卡歌曲 1: 治愈音乐
                Player_PlayVoice(12); // 同时叠加语音：闭上眼听歌
            } else if(event == EVENT_NO) {
                g_current_state = ST_SAD_TALK;
                Player_PlayVoice(13); // 纯言语安慰
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
                delay_ms(3000);
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
                delay_ms(3000);
                g_current_state = ST_IDLE;
            }
            break;

        case ST_SAD_STILL:
            if(event == EVENT_YES) {
                g_current_state = ST_BYE;
                Player_PlayVoice(4);
                delay_ms(3000);
                g_current_state = ST_IDLE;
            } else if(event == EVENT_NO) {
                g_current_state = ST_SAD_DEEP;
                Player_PlayVoice(17);
                delay_ms(6000);
                g_current_state = ST_SILENT_GUARD;
            }
            break;

        case ST_SILENT_GUARD:
            if(event == EVENT_HELLO) {
                g_current_state = ST_WAKE_UP_SUN;
                Player_PlayVoice(19);
                delay_ms(3000);
                g_current_state = ST_WAKE;
            }
            break;

        /* --- 3. 正面情绪共享模块 --- */
        case ST_HAPPY_RECV:
            if(event == EVENT_HAPPY || event == EVENT_YES) {
                g_current_state = ST_HAPPY_CELEBRATE;
                Player_PlayMusic(2); // 播放 TF 卡歌曲 2: 快乐蹦迪曲
                Player_PlayVoice(22);
            } else if(event == EVENT_NO) {
                g_current_state = ST_HAPPY_SHY;
                Player_PlayVoice(23);
                delay_ms(4000);
                g_current_state = ST_WAKE;
            }
            break;

        case ST_HAPPY_CELEBRATE:
            if(event == EVENT_YES || event == EVENT_HAPPY) {
                g_current_state = ST_HAPPY_END;
                Player_PlayVoice(24);
                delay_ms(3000);
                g_current_state = ST_IDLE;
            } else if(event == EVENT_NO) {
                g_current_state = ST_HAPPY_QUIET;
                Player_PlayVoice(25);
                delay_ms(3000);
                g_current_state = ST_WAKE;
            }
            break;

        /* --- 4. 连续冷笑话模块 --- */
        case ST_JOKE_1:
            if(event == EVENT_YES || event == EVENT_HAPPY) {
                g_current_state = ST_JOKE_1_GOOD;
                Player_PlayVoice(32);
            } else {
                g_current_state = ST_JOKE_1_BAD;
                Player_PlayVoice(33);
                delay_ms(3000);
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
                delay_ms(3000);
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
            // 讲完笑话3后自动切到终点判断
            g_current_state = ST_JOKE_END;
            Player_PlayVoice(38);
            break;

        case ST_JOKE_END:
            if(event == EVENT_YES) {
                g_current_state = ST_JOKE_WIN;
                Player_PlayVoice(39);
                delay_ms(3000);
                g_current_state = ST_WAKE;
            } else {
                g_current_state = ST_JOKE_LOSE;
                Player_PlayVoice(40);
                delay_ms(4000);
                g_current_state = ST_GAME_START;
                Player_PlayVoice(41);
            }
            break;

        /* --- 5. 互动小游戏模块 --- */
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
            delay_ms(8000);
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
            delay_ms(8000);
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
            delay_ms(3000);
            g_current_state = ST_WAKE;
            break;

        /* --- 6. 兜底与异常处理 --- */
        default:
            g_current_state = ST_WAKE;
            break;
    }
}

void uart_debug_send_string(char* str) {
    while(*str != '\0')
    {
        while(DL_UART_isBusy(UART_INST) == true);
        DL_UART_Main_transmitData(UART_INST, *str++);
    }
}

void uart_debug_print_hex8(uint8_t val) {
    char high = (char)((val >> 4) & 0x0F);
    char low  = (char)(val & 0x0F);

    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, high < 10 ? high + '0' : high - 10 + 'A');

    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, low < 10 ? low + '0' : low - 10 + 'A');
}