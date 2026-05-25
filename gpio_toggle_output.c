#include "ti_msp_dl_config.h"
#include "bsp_voice.h"

#define delay_ms(X)		delay_cycles((CPUCLK_FREQ/1000)*(X));

volatile unsigned int delay_times = 0;
volatile unsigned char uart_data = 0;

void uart_send_char(char ch);
void uart_send_string(char* str);
uint8_t Voice_Module_ReadID(void);

// 1. 定义播放器控制指令（十六进制数组）
uint8_t CMD_PLAY[10]  = {0x7E, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x01, 0xFE, 0xF7, 0xEF};
uint8_t CMD_PLAY2[10]  = {0x7E, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x02, 0xFE, 0xF6, 0xEF};
uint8_t CMD_PAUSE[10] = {0x7E, 0xFF, 0x06, 0x0E, 0x00, 0x00, 0x00, 0xFE, 0xED, 0xEF};

// 2. 专门向播放器发送 Hex 指令的函数
void uart_plr_send_hex(uint8_t *cmd, uint8_t len)
{
    for(uint8_t i = 0; i < len; i++)
    {
        // 等待 UART_PLR 发送缓冲区空闲
        while(DL_UART_isBusy(UART_PLR_INST) == true);
        // 通过指定的 UART_PLR 实例发送单个字节
        DL_UART_Main_transmitData(UART_PLR_INST, cmd[i]);
    }
}

// 3. 原有的调试串口字符串打印（保留用于在电脑端看日志）
void uart_debug_send_string(char* str)
{
    while(*str != '\0')
    {
        while(DL_UART_isBusy(UART_INST) == true);
        DL_UART_Main_transmitData(UART_INST, *str++);
    }
}

int main(void)
{
    // 初始化 MSPM0 的所有外设（SysConfig 里的配置在这里生效）
    SYSCFG_DL_init();

    uart_debug_send_string("MSPM0 Voice Control Player MVP Start...\r\n");

    uint8_t current_id = 0x00;
    uint8_t last_id = 0x00;

    while(1)
    {
        // 心跳灯，每 100ms 轮询一次，响应更灵敏
        DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
        delay_ms(100); 

        // 从 I2C 语音模块读取当前触发的 ID
        current_id = Voice_Module_ReadID();

        // 状态机：只有当识别到新有效指令，且与上一次不同时才触发（防止重复发送指令）
        if(current_id != 0x00 && current_id != last_id)
        {
						// 修改这里：直接打印原始字节的高 4 位和低 4 位
						uart_send_string("True Hardware ID: 0x");
						uint8_t true_high = (current_id >> 4) & 0x0F;
						uint8_t true_low = current_id & 0x0F;
						uart_send_char(true_high < 10 ? true_high + '0' : true_high - 10 + 'A');
						uart_send_char(true_low < 10 ? true_low + '0' : true_low - 10 + 'A');
						uart_send_string("\r\n");
            switch(current_id)
            {
                case 0x1C: // 假设 ID 0x01 对应关键词 “播放”
                    uart_debug_send_string("-> Action: PLAY\r\n");
                    // 核心动作：向播放器发送 10 字节的播放命令
                    uart_plr_send_hex(CMD_PLAY, 10); 
                    break;
								
								case 0x21: 
                    uart_debug_send_string("-> Action: PLAY\r\n");
                    // 核心动作：向播放器发送 10 字节的播放命令
                    uart_plr_send_hex(CMD_PLAY2, 10); 
                    break;

                case 0x09: // 假设 ID 0x02 对应关键词 “暂停”
                    uart_debug_send_string("-> Action: PAUSE\r\n");
                    // 核心动作：向播放器发送 10 字节的暂停命令
                    uart_plr_send_hex(CMD_PAUSE, 10); 
                    break;

                default:
                    uart_debug_send_string("-> Unknown Voice ID\r\n");
                    break;
            }
        }

        // 更新状态，松开语音后如果返回 0x00，last_id 也会清零，等待下一次唤醒
        last_id = current_id; 
    }
}

/******************************************************************
 * 函 数 名 称：Voice_Module_ReadID
 * 函 数 说 明：从语音识别模块读取当前识别到的关键词 ID
 * 函 数 返 回：0x00 代表未识别到；其余值（如0x01, 0x02）为识别到的ID
 ******************************************************************/
uint8_t Voice_Module_ReadID(void)
{
    uint8_t voice_id = 0x00;

    // ---- 步骤 1：向从机告知我们要读取的寄存器地址 ----
    IIC_Start();
    IIC_Write(VOICE_MODULE_ADDR << 1); // 发送从机写地址 (0x34 << 1 = 0x68)
    if(IIC_Wait_Ack() == 1) {
        return 0x00; // 模块无应答，直接返回
    }

    IIC_Write(REG_VOICE_RESULT);       // 发送识别结果寄存器地址 (0x64)
    if(IIC_Wait_Ack() == 1) {
        return 0x00;
    }

    // ---- 步骤 2：切换为读模式，获取 1 字节的数据 ----
    IIC_Start();                       // 重新发送起始信号 (Restart)
    IIC_Write((VOICE_MODULE_ADDR << 1) | 1); // 发送从机读地址 (0x69)
    if(IIC_Wait_Ack() == 1) {
        return 0x00;
    }

    voice_id = IIC_Read();             // 读取 1 个字节的识别结果
    IIC_Send_Ack(1);                   // 读完最后一个字节，给从机发送 NACK (非应答)
    IIC_Stop();                        // 结束通信

    return voice_id;
}

// 串口发送字符串辅助函数
void uart_send_string(char* str)
{
    while(*str != '\0')
    {
        while(DL_UART_isBusy(UART_INST) == true);
        DL_UART_Main_transmitData(UART_INST, *str++);
    }
}

// 串口发送单个字符（用于打印十六进制数）
void uart_send_char(char ch)
{
    while(DL_UART_isBusy(UART_INST) == true);
    DL_UART_Main_transmitData(UART_INST, ch);
}

//?????????
void UART_INST_IRQHandler(void)
{
    //?????????
    switch( DL_UART_getPendingInterrupt(UART_INST) )
    {
        case DL_UART_IIDX_RX://???????
            //??????????????
            uart_data = DL_UART_Main_receiveData(UART_INST);
            //???????????
            uart_send_char(uart_data);
            break;

        default://???????
            break;
    }
}