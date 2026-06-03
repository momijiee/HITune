#include "ti_msp_dl_config.h"
#include "bsp_voice.h"
#include "bsp_player.h"

// 毫秒延时宏定义
#define delay_ms(X)        delay_cycles((CPUCLK_FREQ/1000)*(X))

// 调试串口辅助函数声明（用于在电脑串口助手看日志）
void uart_debug_send_string(char* str);
void uart_debug_print_hex8(uint8_t val);

int main(void)
{
    // 1. 初始化 MSPM0 的所有外设（SysConfig 中的配置在此处生效）
    SYSCFG_DL_init();

    // 2. 上电打印提示信息
    uart_debug_send_string("===========================================\r\n");
    uart_debug_send_string("  MSPM0 Smart Voice-Player Demo Start...   \r\n");
    uart_debug_send_string("===========================================\r\n");

    // 3. 初始化播放器默认设置（比如上电先初始化音量为 20 级）
    Player_SetVolume(20);
    uart_debug_send_string("-> Player Init: Volume set to 20\r\n");

    uint8_t current_id = 0x00;
    uint8_t last_id = 0x00;

    while(1)
    {
        // 心跳灯：每 100ms 闪烁一次，同时作为主循环的轮询周期
        DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
        delay_ms(100); 

        // 从 I2C 语音模块读取当前触发的 ID
        current_id = Voice_Module_ReadID();

        // 状态机：只有当识别到新有效指令，且与上一次不同时才触发（防止持续按住或不松口时重复发送串口指令）
        if(current_id != 0x00 && current_id != last_id)
        {
            // 打印当前捕获到的真实硬件语音 ID
            uart_debug_send_string("Voice Triggered! ID: 0x");
            uart_debug_print_hex8(current_id);
            uart_debug_send_string("\r\n");

            // 根据不同的语音 ID 映射对应的播放器动作
            switch(current_id)
            {
                case 0x1C: // 露一手
                    uart_debug_send_string("-> Action: Play Voice (Folder 00, Track 1)\r\n");
                    Player_PlayVoice(1); // 播放 00 文件夹下的第 0001 首曲目
                    break;
								
                case 0x21: // 战斗模式
                    uart_debug_send_string("-> Action: Play Music (Folder 01, Track 1)\r\n");
                    Player_PlayMusic(1); // 播放 01 文件夹下的第 0005 首歌曲
                    break;

                case 0x09: // 停止
                    uart_debug_send_string("-> Action: PAUSE\r\n");
                    Player_Pause();      // 暂停当前音频
                    break;

                case 0x01: // 前进
                    uart_debug_send_string("-> Action: Set Volume to 20\r\n");
                    Player_SetVolume(20); 
                    break;

                case 0x02: // 前进
                    uart_debug_send_string("-> Action: Set Volume to 10\r\n");
                    Player_SetVolume(10); 
                    break;

                default:
                    uart_debug_send_string("-> Warning: Unhandled Voice ID\r\n");
                    break;
            }
        }

        // 更新状态：松开语音或未识别时，current_id 返回 0x00，last_id 随之清零，等待下一次唤醒
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