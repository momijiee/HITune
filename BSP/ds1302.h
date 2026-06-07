#ifndef __DS1302_H
#define __DS1302_H

#include "ti_msp_dl_config.h"

// 引脚操作宏定义 - 对应你的 SysConfig 配置
#define DS1302_RST_HIGH()   DL_GPIO_setPins(DS1302_PORT, DS1302_RST_PIN)
#define DS1302_RST_LOW()    DL_GPIO_clearPins(DS1302_PORT, DS1302_RST_PIN)

#define DS1302_CLK_HIGH()   DL_GPIO_setPins(DS1302_PORT, DS1302_CLK_PIN)
#define DS1302_CLK_LOW()    DL_GPIO_clearPins(DS1302_PORT, DS1302_CLK_PIN)

#define DS1302_DAT_HIGH()   DL_GPIO_setPins(DS1302_PORT, DS1302_DAT_PIN)
#define DS1302_DAT_LOW()    DL_GPIO_clearPins(DS1302_PORT, DS1302_DAT_PIN)

#define DS1302_DAT_READ()   DL_GPIO_readPins(DS1302_PORT, DS1302_DAT_PIN)

// DS1302 寄存器地址
#define DS1302_SEC_REG      0x80
#define DS1302_MIN_REG      0x82
#define DS1302_HOUR_REG     0x84
#define DS1302_DATE_REG     0x86
#define DS1302_MONTH_REG    0x88
#define DS1302_DAY_REG      0x8A
#define DS1302_YEAR_REG     0x8C
#define DS1302_CTRL_REG     0x8E
#define DS1302_CHARGE_REG   0x90

// 时间结构体
typedef struct {
    uint8_t year;    // 0-99
    uint8_t month;   // 1-12
    uint8_t date;    // 1-31
    uint8_t day;     // 1-7 (星期)
    uint8_t hour;    // 0-23
    uint8_t min;     // 0-59
    uint8_t sec;     // 0-59
} DS1302_Time_t;

// 函数声明
void DS1302_Init(void);
void DS1302_WriteByte(uint8_t addr, uint8_t data);
uint8_t DS1302_ReadByte(uint8_t addr);
void DS1302_SetTime(DS1302_Time_t *time);
void DS1302_GetTime(DS1302_Time_t *time);

#endif
