#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

/*======================== ADC通道定义 ========================*/
/* MEM_IDX_0 -> CH0 -> PA27 */
/* MEM_IDX_1 -> CH1 -> PA26 */

/*======================== 数据结构 ========================*/
typedef struct {
    unsigned int ch27_raw;       // PA27 ADC原始值
    unsigned int ch26_raw;       // PA26 ADC原始值
    int          diff;           // ch26 - ch27 差值
    unsigned int ch27_voltage;   // PA27 电压值 (×100，如 330 = 3.30V)
    unsigned int ch26_voltage;   // PA26 电压值 (×100，如 330 = 3.30V)
} ADC_Result_t;

/*======================== 函数声明 ========================*/

/**
 * @brief  初始化ADC模块（使能中断）
 */
void BSP_ADC_Init(void);

/**
 * @brief  读取两个ADC通道的原始值（阻塞等待转换完成）
 * @param  ch27_val: 指向存储PA27结果的变量
 * @param  ch26_val: 指向存储PA26结果的变量
 */
void BSP_ADC_ReadBothChannels(unsigned int *ch27_val, unsigned int *ch26_val);

/**
 * @brief  读取两个通道并计算电压、差值，填充结果结构体
 * @param  result: 指向结果结构体的指针
 */
void BSP_ADC_ReadResult(ADC_Result_t *result);

/**
 * @brief  将ADC原始值转换为电压值（×100）
 * @param  raw: ADC原始值 (0 ~ 4095)
 * @return 电压值×100，如 330 表示 3.30V
 */
unsigned int BSP_ADC_RawToVoltage100(unsigned int raw);

/**
 * @brief  ADC中断回调，需在ADC ISR中调用
 */
void BSP_ADC_IRQHandler(void);

#endif /* BSP_ADC_H */
