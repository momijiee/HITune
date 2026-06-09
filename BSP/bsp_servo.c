#include "bsp_servo.h"

/*======================== 内部变量 ========================*/
static int s_current_angle = SERVO_ANGLE_INIT;

/*======================== 函数实现 ========================*/

uint32_t Servo_AngleToCompare(uint16_t angle)
{
    if (angle > 180) {
        angle = 180;
    }
    uint32_t compare = SERVO_PULSE_MIN
                     + (uint32_t)(SERVO_PULSE_MAX - SERVO_PULSE_MIN) * angle / 180;
    return compare;
}

void Servo_SetAngle(uint16_t angle)
{
    uint32_t compareValue;

    if (angle > SERVO_ANGLE_MAX) {
        angle = SERVO_ANGLE_MAX;
    }

    compareValue = Servo_AngleToCompare(angle);
    DL_TimerA_setCaptureCompareValue(PWM_INST, compareValue, DL_TIMER_CC_0_INDEX);

    s_current_angle = (int)angle;
}

void Servo_PWM_Init(void)
{
    DL_TimerA_stopCounter(PWM_INST);
    DL_TimerA_setLoadValue(PWM_INST, SERVO_PWM_PERIOD);
    DL_TimerA_setCaptureCompareValue(PWM_INST, SERVO_PULSE_CENTER, DL_TIMER_CC_0_INDEX);
    DL_TimerA_startCounter(PWM_INST);

    s_current_angle = SERVO_ANGLE_INIT;
}

int Servo_GetCurrentAngle(void)
{
    return s_current_angle;
}

void Servo_SetCurrentAngle(int angle)
{
    if (angle < SERVO_ANGLE_MIN) {
        angle = SERVO_ANGLE_MIN;
    } else if (angle > SERVO_ANGLE_MAX) {
        angle = SERVO_ANGLE_MAX;
    }
    s_current_angle = angle;
}
