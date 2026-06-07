#ifndef __BSP_DS1302_H__
#define __BSP_DS1302_H__

#include "ti_msp_dl_config.h"

/*
 * Wiring from docs/DS1302 schematic:
 * DS1302 RST/CE -> PA25, DAT/I/O -> PA24, CLK/SCLK -> PA23
 */
#define DS1302_CE_PORT         (GPIOA)
#define DS1302_CE_PIN          (DL_GPIO_PIN_25)
#define DS1302_CE_IOMUX        (IOMUX_PINCM55)

#define DS1302_SCLK_PORT       (GPIOA)
#define DS1302_SCLK_PIN        (DL_GPIO_PIN_23)
#define DS1302_SCLK_IOMUX      (IOMUX_PINCM53)

#define DS1302_IO_PORT         (GPIOA)
#define DS1302_IO_PIN          (DL_GPIO_PIN_24)
#define DS1302_IO_IOMUX        (IOMUX_PINCM54)

typedef struct {
    uint16_t year;   /* 2000-2099 */
    uint8_t month;   /* 1-12 */
    uint8_t day;     /* 1-31 */
    uint8_t hour;    /* 0-23 */
    uint8_t minute;  /* 0-59 */
    uint8_t second;  /* 0-59 */
    uint8_t weekday; /* 1-7, user-defined */
} DS1302_DateTime_t;

typedef enum {
    DS1302_OK = 0,
    DS1302_ERR_NULL,
    DS1302_ERR_INVALID_TIME,
    DS1302_ERR_CLOCK_HALTED
} DS1302_Status_t;

void DS1302_Init(void);
DS1302_Status_t DS1302_GetDateTime(DS1302_DateTime_t *time);
DS1302_Status_t DS1302_SetDateTime(const DS1302_DateTime_t *time);
bool DS1302_IsClockHalted(void);
void DS1302_StartClock(void);

#endif /* __BSP_DS1302_H__ */
