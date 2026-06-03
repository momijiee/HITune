#ifndef __BSP_PLAYER_H__
#define __BSP_PLAYER_H__

#include "ti_msp_dl_config.h"

// ---- 播放器状态枚举 ----
typedef enum {
    PLAYER_STATUS_STOP    = 0x00, // 停止
    PLAYER_STATUS_PLAYING = 0x01, // 播放中
    PLAYER_STATUS_PAUSE   = 0x02, // 暂停
    PLAYER_STATUS_UNKNOWN = 0xFF  // 未知状态/超时
} PlayerStatus_t;

// ---- 核心业务接口 ----

/**
 * @brief  播放语音（指定 00 文件夹下的某首曲目）
 * @param  track_num: 曲目编号 (1 ~ 1000)
 */
void Player_PlayVoice(uint16_t track_num);

/**
 * @brief  播放歌曲（指定 01 文件夹下的某首曲目）
 * @param  track_num: 曲目编号 (1 ~ 1000)
 */
void Player_PlayMusic(uint16_t track_num);

/**
 * @brief  暂停当前播放
 */
void Player_Pause(void);

/**
 * @brief  设置音量
 * @param  volume: 音量大小 (0 ~ 30)
 */
void Player_SetVolume(uint8_t volume);

/**
 * @brief  查询音乐当前播放状态
 * @retval PLAYER_STATUS_PLAYING: 播放中, PLAYER_STATUS_PAUSE: 暂停, 等
 * @note   该函数会通过串口发送查询指令，并通过接收缓冲区判断返回
 */
PlayerStatus_t Player_QueryStatus(void);

#endif /* __BSP_PLAYER_H__ */