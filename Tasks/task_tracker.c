#include "task_tracker.h"
#include "soft_timer.h"

#define TRACKER_INTERVAL_MS  50
#define ADC_DEADZONE         50
#define SERVO_STEP           1

static uint32_t s_last_tick = 0;

static uint16_t Read_ADC_Left(void)
{
    DL_ADC12_startConversion(ADC12_0_INST);
    while (DL_ADC12_isBusy(ADC12_0_INST));
    uint16_t val = DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_0);
    DL_ADC12_enableConversions(ADC12_0_INST);
    return val;
}

static uint16_t Read_ADC_Right(void)
{
    DL_ADC12_startConversion(ADC12_0_INST);
    while (DL_ADC12_isBusy(ADC12_0_INST));
    uint16_t val = DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_1);
    DL_ADC12_enableConversions(ADC12_0_INST);
    return val;
}

void TaskTracker_Init(void)
{
    Servo_PWM_Init();
    s_last_tick = GetTick();
}

void TaskTracker_Run(void)
{
    if (!IsTimeUp(&s_last_tick, TRACKER_INTERVAL_MS)) return;

    uint16_t left  = Read_ADC_Left();
    uint16_t right = Read_ADC_Right();
    int diff = (int)left - (int)right;
    int angle = Servo_GetCurrentAngle();

    if (diff > ADC_DEADZONE)       angle -= SERVO_STEP;
    else if (diff < -ADC_DEADZONE) angle += SERVO_STEP;

    if (angle < SERVO_ANGLE_MIN) angle = SERVO_ANGLE_MIN;
    if (angle > SERVO_ANGLE_MAX) angle = SERVO_ANGLE_MAX;

    Servo_SetAngle((uint16_t)angle);
}