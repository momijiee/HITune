#include "bsp_adc.h"

/*======================== 内部变量 ========================*/
static volatile bool s_adc_conv_done = false;

/*======================== 函数实现 ========================*/

void BSP_ADC_Init(void)
{
    NVIC_EnableIRQ(ADC_INST_INT_IRQN);
}

void BSP_ADC_ReadBothChannels(unsigned int *ch27_val, unsigned int *ch26_val)
{
    s_adc_conv_done = false;

    DL_ADC12_startConversion(ADC_INST);

    while (false == s_adc_conv_done)
    {
        __WFE();
    }

    /* MEM_IDX_0 -> CH0 -> PA27 */
    *ch27_val = DL_ADC12_getMemResult(ADC_INST, DL_ADC12_MEM_IDX_0);
    /* MEM_IDX_1 -> CH1 -> PA26 */
    *ch26_val = DL_ADC12_getMemResult(ADC_INST, DL_ADC12_MEM_IDX_1);

    DL_ADC12_enableConversions(ADC_INST);
}

unsigned int BSP_ADC_RawToVoltage100(unsigned int raw)
{
    /* voltage_100 = raw / 4095.0 * 3.3 * 100 = raw * 330 / 4095 */
    return (unsigned int)((uint32_t)raw * 330 / 4095);
}

void BSP_ADC_ReadResult(ADC_Result_t *result)
{
    BSP_ADC_ReadBothChannels(&result->ch27_raw, &result->ch26_raw);

    result->diff = (int)result->ch26_raw - (int)result->ch27_raw;
    result->ch27_voltage = BSP_ADC_RawToVoltage100(result->ch27_raw);
    result->ch26_voltage = BSP_ADC_RawToVoltage100(result->ch26_raw);
}

void BSP_ADC_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC_INST))
    {
        case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
            s_adc_conv_done = true;
            break;
        default:
            break;
    }
}
