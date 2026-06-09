#include "task_tracker.h"
#include "soft_timer.h"
#include "bsp_servo.h"
#include "bsp_adc.h"
#include <stdio.h>

/*======================== 追踪参数配置 ========================*/
#define TRACKER_INTERVAL_MS     50      // 50ms调整一次 (20Hz)
#define THRESHOLD_HIGH          100     // ADC差值阈值：开始调整
#define THRESHOLD_LOW           30      // ADC差值阈值：死区（可选用于分级控速）
#define ANGLE_STEP_FAST         2       // 差值大时快速步进
#define ANGLE_STEP_SLOW         1       // 差值小时慢速步进
#define DEBUG_PRINT_INTERVAL_MS 500     // 串口调试打印间隔（避免刷屏）

/*======================== 内部变量 ========================*/
static uint32_t s_track_tick = 0;
static uint32_t s_debug_tick = 0;

/*======================== 函数实现 ========================*/

void TaskTracker_Init(void)
{
    BSP_ADC_Init();
    Servo_PWM_Init();

    s_track_tick = GetTick();
    s_debug_tick = GetTick();
}

void TaskTracker_Run(void)
{
    /* ---- 非阻塞定时：未到50ms直接返回 ---- */
    if (!IsTimeUp(&s_track_tick, TRACKER_INTERVAL_MS)) {
        return;
    }

    /* ---- 1. 读取ADC（内部有__WFE短暂等待，几十us级别） ---- */
    ADC_Result_t adc_result;
    BSP_ADC_ReadResult(&adc_result);

    /* ---- 2. 获取当前舵机角度 ---- */
    int current_angle = Servo_GetCurrentAngle();

    /* ---- 3. 根据差值控制舵机 ---- */
    /*
     * adc_result.diff = ch26_raw - ch27_raw
     *
     * diff > 0  →  CH26(光敏A)值更大  →  A侧更暗(电阻大电压高) 或 更亮(取决于电路)
     * diff < 0  →  CH27(光敏B)值更大
     *
     * 请根据你的实际硬件接线确认方向，以下假设：
     *   diff > THRESHOLD  →  角度递减（往CH27方向追光）
     *   diff < -THRESHOLD →  角度递增（往CH26方向追光）
     */

    int step = 0;

    if (adc_result.diff > THRESHOLD_HIGH) {
        /* CH26 > CH27，差值大 */
        step = (adc_result.diff > THRESHOLD_HIGH * 3) ? -ANGLE_STEP_FAST : -ANGLE_STEP_SLOW;
    }
    else if (adc_result.diff < -THRESHOLD_HIGH) {
        /* CH27 > CH26，差值大 */
        step = (adc_result.diff < -THRESHOLD_HIGH * 3) ? ANGLE_STEP_FAST : ANGLE_STEP_SLOW;
    }
    /* else: 在死区内，step = 0，不动 */

    if (step != 0) {
        current_angle += step;

        /* 边界钳位 */
        if (current_angle < SERVO_ANGLE_MIN) {
            current_angle = SERVO_ANGLE_MIN;
        } else if (current_angle > SERVO_ANGLE_MAX) {
            current_angle = SERVO_ANGLE_MAX;
        }

        Servo_SetAngle((uint16_t)current_angle);
    }

    /* ---- 4. 调试打印（低频，避免占用过多串口带宽） ---- */
    if (IsTimeUp(&s_debug_tick, DEBUG_PRINT_INTERVAL_MS)) {
        printf("TRK| CH27:%4d(%d.%d%dV) CH26:%4d(%d.%d%dV) diff:%4d angle:%3d\r\n",
               adc_result.ch27_raw,
               adc_result.ch27_voltage / 100,
               adc_result.ch27_voltage / 10 % 10,
               adc_result.ch27_voltage % 10,
               adc_result.ch26_raw,
               adc_result.ch26_voltage / 100,
               adc_result.ch26_voltage / 10 % 10,
               adc_result.ch26_voltage % 10,
               adc_result.diff,
               current_angle);
    }
}
