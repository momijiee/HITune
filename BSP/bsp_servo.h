#ifndef BSP_SERVO_H
#define BSP_SERVO_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

/*======================== 修正后的PWM参数 ========================*/
#define SERVO_PWM_PERIOD        (80000 - 1)   // 50Hz: 4MHz / 50 = 80000

#define SERVO_PULSE_MIN         4500           // 4000/4MHz = 1.0ms   → 0°
#define SERVO_PULSE_CENTER      7000           // 6000/4MHz = 1.5ms   → 90°
#define SERVO_PULSE_MAX         9500           // 8000/4MHz = 2.0ms   → 180°

#define SERVO_ANGLE_MIN         0
#define SERVO_ANGLE_MAX         180
#define SERVO_ANGLE_INIT        90

/*======================== 函数声明 ========================*/

/**
 * @brief  初始化舵机PWM，设置50Hz周期，默认90°
 */
void Servo_PWM_Init(void);

/**
 * @brief  设置舵机角度
 * @param  angle: 目标角度 (0 ~ 180)
 */
void Servo_SetAngle(uint16_t angle);

/**
 * @brief  将角度值转换为PWM比较值
 * @param  angle: 角度 (0 ~ 180)
 * @return 对应的PWM比较寄存器值
 */
uint32_t Servo_AngleToCompare(uint16_t angle);

/**
 * @brief  获取当前舵机角度
 * @return 当前角度值
 */
int Servo_GetCurrentAngle(void);

/**
 * @brief  设置当前角度记录值（不驱动舵机）
 * @param  angle: 角度值
 */
void Servo_SetCurrentAngle(int angle);

#endif /* BSP_SERVO_H */
