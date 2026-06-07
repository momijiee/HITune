/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for PWM */
#define PWM_INST                                                           TIMA1
#define PWM_INST_IRQHandler                                     TIMA1_IRQHandler
#define PWM_INST_INT_IRQN                                       (TIMA1_INT_IRQn)
#define PWM_INST_CLK_FREQ                                               32000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_C0_PORT                                                   GPIOA
#define GPIO_PWM_C0_PIN                                           DL_GPIO_PIN_15
#define GPIO_PWM_C0_IOMUX                                        (IOMUX_PINCM37)
#define GPIO_PWM_C0_IOMUX_FUNC                       IOMUX_PINCM37_PF_TIMA1_CCP0
#define GPIO_PWM_C0_IDX                                      DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_C1_PORT                                                   GPIOA
#define GPIO_PWM_C1_PIN                                           DL_GPIO_PIN_16
#define GPIO_PWM_C1_IOMUX                                        (IOMUX_PINCM38)
#define GPIO_PWM_C1_IOMUX_FUNC                       IOMUX_PINCM38_PF_TIMA1_CCP1
#define GPIO_PWM_C1_IDX                                      DL_TIMER_CC_1_INDEX



/* Defines for UART */
#define UART_INST                                                          UART0
#define UART_INST_FREQUENCY                                              4000000
#define UART_INST_IRQHandler                                    UART0_IRQHandler
#define UART_INST_INT_IRQN                                        UART0_INT_IRQn
#define GPIO_UART_RX_PORT                                                  GPIOA
#define GPIO_UART_TX_PORT                                                  GPIOA
#define GPIO_UART_RX_PIN                                          DL_GPIO_PIN_11
#define GPIO_UART_TX_PIN                                          DL_GPIO_PIN_10
#define GPIO_UART_IOMUX_RX                                       (IOMUX_PINCM22)
#define GPIO_UART_IOMUX_TX                                       (IOMUX_PINCM21)
#define GPIO_UART_IOMUX_RX_FUNC                        IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_IOMUX_TX_FUNC                        IOMUX_PINCM21_PF_UART0_TX
#define UART_BAUD_RATE                                                    (9600)
#define UART_IBRD_4_MHZ_9600_BAUD                                           (26)
#define UART_FBRD_4_MHZ_9600_BAUD                                            (3)
/* Defines for UART_PLR */
#define UART_PLR_INST                                                      UART1
#define UART_PLR_INST_FREQUENCY                                          4000000
#define UART_PLR_INST_IRQHandler                                UART1_IRQHandler
#define UART_PLR_INST_INT_IRQN                                    UART1_INT_IRQn
#define GPIO_UART_PLR_RX_PORT                                              GPIOB
#define GPIO_UART_PLR_TX_PORT                                              GPIOB
#define GPIO_UART_PLR_RX_PIN                                       DL_GPIO_PIN_7
#define GPIO_UART_PLR_TX_PIN                                       DL_GPIO_PIN_6
#define GPIO_UART_PLR_IOMUX_RX                                   (IOMUX_PINCM24)
#define GPIO_UART_PLR_IOMUX_TX                                   (IOMUX_PINCM23)
#define GPIO_UART_PLR_IOMUX_RX_FUNC                    IOMUX_PINCM24_PF_UART1_RX
#define GPIO_UART_PLR_IOMUX_TX_FUNC                    IOMUX_PINCM23_PF_UART1_TX
#define UART_PLR_BAUD_RATE                                                (9600)
#define UART_PLR_IBRD_4_MHZ_9600_BAUD                                       (26)
#define UART_PLR_FBRD_4_MHZ_9600_BAUD                                        (3)





/* Defines for ADC12_0 */
#define ADC12_0_INST                                                        ADC0
#define ADC12_0_INST_IRQHandler                                  ADC0_IRQHandler
#define ADC12_0_INST_INT_IRQN                                    (ADC0_INT_IRQn)
#define ADC12_0_ADCMEM_0                                      DL_ADC12_MEM_IDX_0
#define ADC12_0_ADCMEM_0_REF                     DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_0_REF_VOLTAGE_V                                       3.3
#define GPIO_ADC12_0_C0_PORT                                               GPIOA
#define GPIO_ADC12_0_C0_PIN                                       DL_GPIO_PIN_27
#define GPIO_ADC12_0_IOMUX_C0                                    (IOMUX_PINCM60)
#define GPIO_ADC12_0_IOMUX_C0_FUNC                (IOMUX_PINCM60_PF_UNCONNECTED)



/* Port definition for Pin Group I2C */
#define I2C_PORT                                                         (GPIOA)

/* Defines for SCL: GPIOA.0 with pinCMx 1 on package pin 33 */
#define I2C_SCL_PIN                                              (DL_GPIO_PIN_0)
#define I2C_SCL_IOMUX                                             (IOMUX_PINCM1)
/* Defines for SDA: GPIOA.1 with pinCMx 2 on package pin 34 */
#define I2C_SDA_PIN                                              (DL_GPIO_PIN_1)
#define I2C_SDA_IOMUX                                             (IOMUX_PINCM2)
/* Port definition for Pin Group OLED */
#define OLED_PORT                                                        (GPIOA)

/* Defines for SCL_OLED: GPIOA.12 with pinCMx 34 on package pin 5 */
#define OLED_SCL_OLED_PIN                                       (DL_GPIO_PIN_12)
#define OLED_SCL_OLED_IOMUX                                      (IOMUX_PINCM34)
/* Defines for SDA_OLED: GPIOA.14 with pinCMx 36 on package pin 7 */
#define OLED_SDA_OLED_PIN                                       (DL_GPIO_PIN_14)
#define OLED_SDA_OLED_IOMUX                                      (IOMUX_PINCM36)
/* Defines for RES_OLED: GPIOA.21 with pinCMx 46 on package pin 17 */
#define OLED_RES_OLED_PIN                                       (DL_GPIO_PIN_21)
#define OLED_RES_OLED_IOMUX                                      (IOMUX_PINCM46)
/* Defines for DC_OLED: GPIOA.22 with pinCMx 47 on package pin 18 */
#define OLED_DC_OLED_PIN                                        (DL_GPIO_PIN_22)
#define OLED_DC_OLED_IOMUX                                       (IOMUX_PINCM47)
/* Defines for CS_OLED: GPIOA.2 with pinCMx 7 on package pin 42 */
#define OLED_CS_OLED_PIN                                         (DL_GPIO_PIN_2)
#define OLED_CS_OLED_IOMUX                                        (IOMUX_PINCM7)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_init(void);
void SYSCFG_DL_UART_init(void);
void SYSCFG_DL_UART_PLR_init(void);
void SYSCFG_DL_ADC12_0_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
