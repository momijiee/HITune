#include "ti_msp_dl_config.h"
#include "stdio.h"
#include "board.h"
#include "bsp_servo.h"
#include "bsp_adc.h"

/*======================== 追踪参数配置 ========================*/
#define THRESHOLD_HIGH          100    // ADC差值阈值（开始调整）
#define THRESHOLD_LOW           30     // ADC差值阈值（停止调整/死区）
#define ANGLE_STEP              1      // 每次调整角度步长
#define ADJUST_INTERVAL_MS      50     // 调整间隔(ms)



/*======================== 主函数 ========================*/
int main(void)
{
    ADC_Result_t adc_result;
    int current_angle;

    SYSCFG_DL_init();

    /* 初始化BSP模块 */
    BSP_ADC_Init();
    Servo_PWM_Init();

    printf("===================================\r\n");
    printf("Servo + ADC Tracking Demo\r\n");
    printf("===================================\r\n");

    /* 舵机自检：0° -> 90° -> 180° */
    printf("Test: 0 deg\r\n");
    Servo_SetAngle(0);
    delay_ms(1000);

    printf("Test: 90 deg\r\n");
    Servo_SetAngle(90);
    delay_ms(1000);

    printf("Test: 180 deg\r\n");
    Servo_SetAngle(180);
    delay_ms(1000);

    /* 回到初始位置90° */
    printf("Init: 90 deg\r\n");
    Servo_SetAngle(SERVO_ANGLE_INIT);
    delay_ms(1000);

    printf("===================================\r\n");
    printf("ADC tracking started\r\n");
    printf("CH26 > CH27 -> angle decrease\r\n");
    printf("CH27 > CH26 -> angle increase\r\n");
    printf("===================================\r\n");

    while (1)
    {
        /* 读取ADC数据 */
        BSP_ADC_ReadResult(&adc_result);

        current_angle = Servo_GetCurrentAngle();

        /* 打印调试信息 */
        printf("CH27:%d(%d.%d%dV) CH26:%d(%d.%d%dV) d:%d a:%d\r\n",
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

        /* 根据差值调整舵机角度 */
        if (adc_result.diff > THRESHOLD_HIGH)
        {
            /* CH26 > CH27，角度递减 */
            if (current_angle > SERVO_ANGLE_MIN)
            {
                current_angle -= ANGLE_STEP;
                if (current_angle < SERVO_ANGLE_MIN)
                    current_angle = SERVO_ANGLE_MIN;
                Servo_SetAngle((uint16_t)current_angle);
            }
        }
        else if (adc_result.diff < -THRESHOLD_HIGH)
        {
            /* CH27 > CH26，角度递增 */
            if (current_angle < SERVO_ANGLE_MAX)
            {
                current_angle += ANGLE_STEP;
                if (current_angle > SERVO_ANGLE_MAX)
                    current_angle = SERVO_ANGLE_MAX;
                Servo_SetAngle((uint16_t)current_angle);
            }
        }
        /* else: 差值在死区内，保持不动 */

        delay_ms(ADJUST_INTERVAL_MS);
    }
}

/*======================== 中断服务函数 ========================*/
void ADC_INST_IRQHandler(void)
{
    BSP_ADC_IRQHandler();
}
