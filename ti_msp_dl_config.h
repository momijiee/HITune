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





/* Port definition for Pin Group LED1 */
#define LED1_PORT                                                        (GPIOA)

/* Defines for PIN_22: GPIOA.22 with pinCMx 47 on package pin 18 */
#define LED1_PIN_22_PIN                                         (DL_GPIO_PIN_22)
#define LED1_PIN_22_IOMUX                                        (IOMUX_PINCM47)
/* Port definition for Pin Group I2C */
#define I2C_PORT                                                         (GPIOA)

/* Defines for SCL: GPIOA.0 with pinCMx 1 on package pin 33 */
#define I2C_SCL_PIN                                              (DL_GPIO_PIN_0)
#define I2C_SCL_IOMUX                                             (IOMUX_PINCM1)
/* Defines for SDA: GPIOA.1 with pinCMx 2 on package pin 34 */
#define I2C_SDA_PIN                                              (DL_GPIO_PIN_1)
#define I2C_SDA_IOMUX                                             (IOMUX_PINCM2)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_UART_init(void);
void SYSCFG_DL_UART_PLR_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
