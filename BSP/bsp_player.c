#include "bsp_player.h"

// 基础帧格式定义说明
#define FRAME_START     0x7E
#define FRAME_VER       0xFF
#define FRAME_LEN       0x06
#define FRAME_FEEDBACK  0x00 // 0x00: 不需要应答，0x01: 需要应答
#define FRAME_END       0xEF

// 串口指令集定义
#define CMD_SET_VOLUME  0x06
#define CMD_PAUSE       0x0E
#define CMD_PLAY_FOLDER 0x14
#define CMD_QUERY_STAT  0x42

/**
 * @brief  内部函数：向播放器串口发送单字节
 */
static void Player_SendByte(uint8_t ch)
{
    while(DL_UART_isBusy(UART_PLR_INST) == true);
    DL_UART_Main_transmitData(UART_PLR_INST, ch);
}

/**
 * @brief  内部函数：组装标准 10 字节数据帧并发送（带自动校验计算）
 */
static void Player_SendCmd(uint8_t cmd, uint8_t para_high, uint8_t para_low)
{
    uint16_t checksum = 0;
    
    // 1. 计算校验和 (Version + Length + CMD + Feedback + ParaH + ParaL)
    checksum = FRAME_VER + FRAME_LEN + cmd + FRAME_FEEDBACK + para_high + para_low;
    checksum = 0 - checksum; // 求补码
    
    // 2. 顺序发送整帧数据
    Player_SendByte(FRAME_START);
    Player_SendByte(FRAME_VER);
    Player_SendByte(FRAME_LEN);
    Player_SendByte(cmd);
    Player_SendByte(FRAME_FEEDBACK);
    Player_SendByte(para_high);
    Player_SendByte(para_low);
    Player_SendByte((uint8_t)(checksum >> 8));   // 校验高字节
    Player_SendByte((uint8_t)(checksum & 0xFF));  // 校验低字节
    Player_SendByte(FRAME_END);
}

/**
 * @brief  播放语音（文件夹 00）
 */
void Player_PlayVoice(uint16_t track_num)
{
    // 高4位为文件夹(0x0)，低12位为曲目
    uint8_t para_high = (uint8_t)((track_num >> 8) & 0x0F); 
    uint8_t para_low  = (uint8_t)(track_num & 0xFF);
    
    Player_SendCmd(CMD_PLAY_FOLDER, para_high, para_low);
}

/**
 * @brief  播放歌曲（文件夹 01）
 */
void Player_PlayMusic(uint16_t track_num)
{
    // 高4位为文件夹(0x1)，包含进高字节中
    uint8_t para_high = (uint8_t)(((track_num >> 8) & 0x0F) | 0x10); 
    uint8_t para_low  = (uint8_t)(track_num & 0xFF);
    
    Player_SendCmd(CMD_PLAY_FOLDER, para_high, para_low);
}

/**
 * @brief  暂停当前播放
 */
void Player_Pause(void)
{
    Player_SendCmd(CMD_PAUSE, 0x00, 0x00);
}

/**
 * @brief  设置音量
 */
void Player_SetVolume(uint8_t volume)
{
    if(volume > 30) volume = 30; // 边界保护
    Player_SendCmd(CMD_SET_VOLUME, 0x00, volume);
}

/**
 * @brief  查询音乐播放状态
 */
PlayerStatus_t Player_QueryStatus(void)
{
    // 1. 清空当前可能存在的接收残留（非阻塞尝试读取）
    // 注意：假设你在 UART_PLR 的中断或者全域变量中维护了接收逻辑，这里发送查询指令
    Player_SendCmd(CMD_QUERY_STAT, 0x00, 0x00);
    
    // 2. 考虑芯片返回需要时间，此处作简单的非阻塞等待或后续在中断中异步解析更为稳妥。
    // 如果你当前需要“同步阻塞等待”返回结果，我们需要配合你的 UART_PLR 接收中断变量。
    // 这里先预留标准逻辑：
    
    /* // 伪代码参考：
    delay_cycles(CPUCLK_FREQ / 1000 * 20); // 稍微延时 20ms 等待串口回传
    if (Plr_Rx_Buffer_Ready) {
        // 通常返回格式类似：7E FF 06 42 00 00 01 xx xx EF (01代表播放，02代表暂停)
        return (PlayerStatus_t)Plr_Rx_Buffer[6]; 
    }
    */
    
    return PLAYER_STATUS_UNKNOWN;
}