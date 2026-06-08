#include "ti_msp_dl_config.h"
#include "stdio.h"
#include "board.h"

/*======================== ??PWM?? ========================*/
#define SERVO_PWM_PERIOD        (80000 - 1)   // 50Hz: 4MHz / 50 = 80000
#define SERVO_PULSE_MIN         2000           // 0.5ms  ? 0°
#define SERVO_PULSE_CENTER      6000           // 1.5ms  ? 90°
#define SERVO_PULSE_MAX         10000          // 2.5ms  ? 180°

#define SERVO_ANGLE_MIN         0
#define SERVO_ANGLE_MAX         180
#define SERVO_ANGLE_INIT        90

/*======================== ???? ========================*/
#define THRESHOLD_HIGH          100    // ADC?????????
#define THRESHOLD_LOW           30     // ADC??????????(??)
#define ANGLE_STEP              1      // ??????
#define ADJUST_INTERVAL_MS      50     // ????

/*======================== ???? ========================*/
volatile unsigned int delay_times = 0;
volatile bool gCheckADC = false;
int current_angle = SERVO_ANGLE_INIT;

/*======================== ???? ========================*/
void Servo_PWM_Init(void);
void Servo_SetAngle(uint16_t angle);
uint32_t Servo_AngleToCompare(uint16_t angle);
void adc_readBothChannels(unsigned int *ch27_val, unsigned int *ch26_val);


/*======================== ???? ========================*/
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
    uint32_t compareValue = Servo_AngleToCompare(angle);
    DL_TimerA_setCaptureCompareValue(PWM_INST, compareValue, DL_TIMER_CC_0_INDEX);

    if (angle > SERVO_ANGLE_MAX)
        current_angle = SERVO_ANGLE_MAX;
    else
        current_angle = (int)angle;
}

void Servo_PWM_Init(void)
{
    DL_TimerA_stopCounter(PWM_INST);
    DL_TimerA_setLoadValue(PWM_INST, SERVO_PWM_PERIOD);
    DL_TimerA_setCaptureCompareValue(PWM_INST, SERVO_PULSE_CENTER, DL_TIMER_CC_0_INDEX);
    DL_TimerA_startCounter(PWM_INST);
}

/*======================== ADC?? ========================*/
void adc_readBothChannels(unsigned int *ch27_val, unsigned int *ch26_val)
{
    gCheckADC = false;

    DL_ADC12_startConversion(ADC_INST);

    while (false == gCheckADC)
    {
        __WFE();
    }

    // MEM_IDX_0 -> CH0 -> PA27
    *ch27_val = DL_ADC12_getMemResult(ADC_INST, DL_ADC12_MEM_IDX_0);
    // MEM_IDX_1 -> CH1 -> PA26
    *ch26_val = DL_ADC12_getMemResult(ADC_INST, DL_ADC12_MEM_IDX_1);

    DL_ADC12_enableConversions(ADC_INST);
}

/*======================== ??? ========================*/
int main(void)
{
    unsigned int adc_ch27 = 0;
    unsigned int adc_ch26 = 0;
    int diff = 0;
    unsigned int voltage_ch27 = 0;
    unsigned int voltage_ch26 = 0;

    SYSCFG_DL_init();

    // ??ADC??
    NVIC_EnableIRQ(ADC_INST_INT_IRQN);

    // ?????PWM?50Hz,????90?
    Servo_PWM_Init();

    printf("===================================\r\n");
    printf("Servo + ADC Tracking Demo\r\n");
    printf("===================================\r\n");

    // ?????????
    printf("Test: 0 deg\r\n");
    Servo_SetAngle(0);
    delay_ms(1000);

    printf("Test: 90 deg\r\n");
    Servo_SetAngle(90);
    delay_ms(1000);

    printf("Test: 180 deg\r\n");
    Servo_SetAngle(180);
    delay_ms(1000);

    // ????90?
    printf("Init: 90 deg\r\n");
    Servo_SetAngle(SERVO_ANGLE_INIT);
    current_angle = SERVO_ANGLE_INIT;
    delay_ms(1000);

    printf("===================================\r\n");
    printf("ADC tracking started\r\n");
    printf("CH26 > CH27 -> angle decrease\r\n");
    printf("CH27 > CH26 -> angle increase\r\n");
    printf("===================================\r\n");

    while (1)
    {
        // ????ADC??
        adc_readBothChannels(&adc_ch27, &adc_ch26);

        // ?? = CH26 - CH27
        diff = (int)adc_ch26 - (int)adc_ch27;

        // ????(??100?)
        voltage_ch27 = (unsigned int)((adc_ch27 / 4095.0 * 3.3) * 100);
        voltage_ch26 = (unsigned int)((adc_ch26 / 4095.0 * 3.3) * 100);

        // ??????
        printf("CH27:%d(%d.%d%dV) CH26:%d(%d.%d%dV) d:%d a:%d\r\n",
               adc_ch27,
               voltage_ch27 / 100, voltage_ch27 / 10 % 10, voltage_ch27 % 10,
               adc_ch26,
               voltage_ch26 / 100, voltage_ch26 / 10 % 10, voltage_ch26 % 10,
               diff,
               current_angle);

        /*=============== ????????? ===============
         *
         *   diff > +THRESHOLD_HIGH  ? CH26?? ? ????
         *   diff < -THRESHOLD_HIGH  ? CH27?? ? ????
         *   |diff| < THRESHOLD_LOW  ? ??,??
         *   ???? ? ???,??????
         *
         *=================================================*/

        if (diff > THRESHOLD_HIGH)
        {
            // CH26 > CH27,??????
            if (current_angle > SERVO_ANGLE_MIN)
            {
                current_angle -= ANGLE_STEP;
                if (current_angle < SERVO_ANGLE_MIN)
                    current_angle = SERVO_ANGLE_MIN;
                Servo_SetAngle((uint16_t)current_angle);
            }
        }
        else if (diff < -THRESHOLD_HIGH)
        {
            // CH27 > CH26,??????
            if (current_angle < SERVO_ANGLE_MAX)
            {
                current_angle += ANGLE_STEP;
                if (current_angle > SERVO_ANGLE_MAX)
                    current_angle = SERVO_ANGLE_MAX;
                Servo_SetAngle((uint16_t)current_angle);
            }
        }
        // else: ????????,?????

        delay_ms(ADJUST_INTERVAL_MS);
    }
}


void ADC_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC_INST))
    {
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            gCheckADC = true;
            break;
        default:
            break;
    }
}
