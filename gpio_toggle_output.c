#include "ti_msp_dl_config.h"
#include "bsp_voice.h"
#include "bsp_player.h"

// 毫秒延时宏定义
#define delay_ms(X)        delay_cycles((CPUCLK_FREQ/1000)*(X))

typedef enum {
    VOICE_STATE_IDLE = 0,
    VOICE_STATE_GREETING,
    VOICE_STATE_SELF_INTRO,
    VOICE_STATE_TIRED_RESPONSE,
    VOICE_STATE_HAPPY_RESPONSE,
    VOICE_STATE_GOODBYE
} voice_state_t;

typedef struct {
    uint8_t command_id;
    voice_state_t state;
    uint16_t track;
    char* state_name;
} voice_command_map_t;

#define VOICE_CMD_GREETING        (0x1A)
#define VOICE_CMD_SELF_INTRO      (0x1B)
#define VOICE_CMD_TIRED_RESPONSE  (0x1C)
#define VOICE_CMD_HAPPY_RESPONSE  (0x1D)
#define VOICE_CMD_GOODBYE         (0x1E)

static const voice_command_map_t voice_command_map[] = {
    {VOICE_CMD_GREETING,       VOICE_STATE_GREETING,       1, "GREETING"},
    {VOICE_CMD_SELF_INTRO,     VOICE_STATE_SELF_INTRO,     2, "SELF_INTRO"},
    {VOICE_CMD_TIRED_RESPONSE, VOICE_STATE_TIRED_RESPONSE, 3, "TIRED_RESPONSE"},
    {VOICE_CMD_HAPPY_RESPONSE, VOICE_STATE_HAPPY_RESPONSE, 4, "HAPPY_RESPONSE"},
    {VOICE_CMD_GOODBYE,        VOICE_STATE_GOODBYE,        5, "GOODBYE"},
};

// 调试串口辅助函数声明（用于在电脑串口助手看日志）
void uart_debug_send_string(char* str);
void uart_debug_print_hex8(uint8_t val);

static const voice_command_map_t* voice_find_command(uint8_t command_id)
{
    for(uint8_t i = 0; i < (sizeof(voice_command_map) / sizeof(voice_command_map[0])); i++)
    {
        if(voice_command_map[i].command_id == command_id)
        {
            return &voice_command_map[i];
        }
    }

    return 0;
}

static void voice_state_handle_command(uint8_t command_id, voice_state_t* current_state)
{
    const voice_command_map_t* command = voice_find_command(command_id);

    uart_debug_send_string("Voice Triggered! ID: 0x");
    uart_debug_print_hex8(command_id);
    uart_debug_send_string("\r\n");

    if(command == 0)
    {
        uart_debug_send_string("-> Warning: Unhandled Voice ID\r\n");
        *current_state = VOICE_STATE_IDLE;
        return;
    }

    *current_state = command->state;
    uart_debug_send_string("-> State: ");
    uart_debug_send_string(command->state_name);
    uart_debug_send_string("\r\n");

    Player_PlayVoice(command->track);

    *current_state = VOICE_STATE_IDLE;
    uart_debug_send_string("-> State: IDLE\r\n");
}

int main(void)
{
    // 1. 初始化 MSPM0 的所有外设（SysConfig 中的配置在此处生效）
    SYSCFG_DL_init();

    // 2. 上电打印提示信息
    uart_debug_send_string("===========================================\r\n");
    uart_debug_send_string("  MSPM0 Voice Conversation MVP Start...    \r\n");
    uart_debug_send_string("===========================================\r\n");

    // 3. 初始化播放器默认设置
    Player_SetVolume(20);
    uart_debug_send_string("-> Player Init: Volume set to 20\r\n");

    uint8_t current_id = 0x00;
    uint8_t last_id = 0x00;
    voice_state_t current_state = VOICE_STATE_IDLE;

    while(1)
    {
        // 心跳灯：每 100ms 闪烁一次，同时作为主循环的轮询周期
        DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
        delay_ms(100);

        // 从 I2C 语音模块读取当前触发的 ID
        current_id = Voice_Module_ReadID();

        // 只有当识别到新有效指令，且与上一次不同时才触发，避免重复播放
        if(current_id != 0x00 && current_id != last_id)
        {
            voice_state_handle_command(current_id, &current_state);
        }

        // 松开语音或未识别时，current_id 返回 0x00，last_id 随之清零，等待下一次唤醒
        last_id = current_id;
    }
}

/******************************************************************
 * 函 数 名 称：uart_debug_send_string
 * 函 数 说 明：向电脑调试串口打印字符串
 ******************************************************************/
void uart_debug_send_string(char* str)
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
