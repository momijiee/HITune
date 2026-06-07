#include "bsp_ds1302.h"

#define DS1302_CMD_SECONDS      (0x80)
#define DS1302_CMD_MINUTES      (0x82)
#define DS1302_CMD_HOURS        (0x84)
#define DS1302_CMD_DATE         (0x86)
#define DS1302_CMD_MONTH        (0x88)
#define DS1302_CMD_DAY          (0x8A)
#define DS1302_CMD_YEAR         (0x8C)
#define DS1302_CMD_CONTROL      (0x8E)
#define DS1302_CMD_CLOCK_BURST  (0xBE)

#define DS1302_CONTROL_WP       (0x80)
#define DS1302_SECONDS_CH       (0x80)

#define DS1302_DELAY_US         (2)
#define DS1302_DELAY()          delay_cycles((CPUCLK_FREQ / 1000000U) * DS1302_DELAY_US)

static void DS1302_CE(uint8_t level)
{
    if(level) {
        DL_GPIO_setPins(DS1302_CE_PORT, DS1302_CE_PIN);
    } else {
        DL_GPIO_clearPins(DS1302_CE_PORT, DS1302_CE_PIN);
    }
}

static void DS1302_SCLK(uint8_t level)
{
    if(level) {
        DL_GPIO_setPins(DS1302_SCLK_PORT, DS1302_SCLK_PIN);
    } else {
        DL_GPIO_clearPins(DS1302_SCLK_PORT, DS1302_SCLK_PIN);
    }
}

static void DS1302_IO(uint8_t level)
{
    if(level) {
        DL_GPIO_setPins(DS1302_IO_PORT, DS1302_IO_PIN);
    } else {
        DL_GPIO_clearPins(DS1302_IO_PORT, DS1302_IO_PIN);
    }
}

static void DS1302_IO_Output(void)
{
    DL_GPIO_initDigitalOutput(DS1302_IO_IOMUX);
    DL_GPIO_enableOutput(DS1302_IO_PORT, DS1302_IO_PIN);
}

static void DS1302_IO_Input(void)
{
    DL_GPIO_disableOutput(DS1302_IO_PORT, DS1302_IO_PIN);
    DL_GPIO_initDigitalInput(DS1302_IO_IOMUX);
}

static uint8_t DS1302_IO_Read(void)
{
    return (DL_GPIO_readPins(DS1302_IO_PORT, DS1302_IO_PIN) & DS1302_IO_PIN) ? 1U : 0U;
}

static uint8_t DS1302_ToBCD(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static uint8_t DS1302_FromBCD(uint8_t value)
{
    return (uint8_t)(((value >> 4U) * 10U) + (value & 0x0FU));
}

static void DS1302_WriteByteLSB(uint8_t value)
{
    uint8_t i;

    DS1302_IO_Output();
    for(i = 0; i < 8U; i++) {
        DS1302_IO(value & 0x01U);
        DS1302_DELAY();
        DS1302_SCLK(1);
        DS1302_DELAY();
        DS1302_SCLK(0);
        DS1302_DELAY();
        value >>= 1U;
    }
}

static uint8_t DS1302_ReadByteLSB(void)
{
    uint8_t i;
    uint8_t value = 0;

    DS1302_IO_Input();
    for(i = 0; i < 8U; i++) {
        if(DS1302_IO_Read()) {
            value |= (uint8_t)(1U << i);
        }
        DS1302_SCLK(1);
        DS1302_DELAY();
        DS1302_SCLK(0);
        DS1302_DELAY();
    }

    return value;
}

static void DS1302_WriteRegister(uint8_t command, uint8_t value)
{
    DS1302_CE(0);
    DS1302_SCLK(0);
    DS1302_DELAY();

    DS1302_CE(1);
    DS1302_WriteByteLSB(command & 0xFEU);
    DS1302_WriteByteLSB(value);
    DS1302_CE(0);
    DS1302_DELAY();
}

static uint8_t DS1302_ReadRegister(uint8_t command)
{
    uint8_t value;

    DS1302_CE(0);
    DS1302_SCLK(0);
    DS1302_DELAY();

    DS1302_CE(1);
    DS1302_WriteByteLSB(command | 0x01U);
    value = DS1302_ReadByteLSB();
    DS1302_CE(0);
    DS1302_IO_Output();
    DS1302_IO(0);
    DS1302_DELAY();

    return value;
}

static void DS1302_ReadClockBurst(uint8_t *buffer)
{
    uint8_t i;

    DS1302_CE(0);
    DS1302_SCLK(0);
    DS1302_DELAY();

    DS1302_CE(1);
    DS1302_WriteByteLSB(DS1302_CMD_CLOCK_BURST | 0x01U);
    for(i = 0; i < 8U; i++) {
        buffer[i] = DS1302_ReadByteLSB();
    }
    DS1302_CE(0);
    DS1302_IO_Output();
    DS1302_IO(0);
    DS1302_DELAY();
}

static bool DS1302_IsLeapYear(uint16_t year)
{
    return ((year % 4U) == 0U);
}

static uint8_t DS1302_DaysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if(month == 2U && DS1302_IsLeapYear(year)) {
        return 29;
    }

    return days[month - 1U];
}

static bool DS1302_IsValidDateTime(const DS1302_DateTime_t *time)
{
    if(time == NULL) {
        return false;
    }

    if(time->year < 2000U || time->year > 2099U) {
        return false;
    }
    if(time->month < 1U || time->month > 12U) {
        return false;
    }
    if(time->day < 1U || time->day > DS1302_DaysInMonth(time->year, time->month)) {
        return false;
    }
    if(time->hour > 23U || time->minute > 59U || time->second > 59U) {
        return false;
    }
    if(time->weekday < 1U || time->weekday > 7U) {
        return false;
    }

    return true;
}

void DS1302_Init(void)
{
    DL_GPIO_initDigitalOutput(DS1302_CE_IOMUX);
    DL_GPIO_initDigitalOutput(DS1302_SCLK_IOMUX);
    DL_GPIO_initDigitalOutput(DS1302_IO_IOMUX);

    DL_GPIO_clearPins(DS1302_CE_PORT, DS1302_CE_PIN);
    DL_GPIO_clearPins(DS1302_SCLK_PORT, DS1302_SCLK_PIN);
    DL_GPIO_clearPins(DS1302_IO_PORT, DS1302_IO_PIN);

    DL_GPIO_enableOutput(DS1302_CE_PORT, DS1302_CE_PIN);
    DL_GPIO_enableOutput(DS1302_SCLK_PORT, DS1302_SCLK_PIN);
    DL_GPIO_enableOutput(DS1302_IO_PORT, DS1302_IO_PIN);
}

DS1302_Status_t DS1302_GetDateTime(DS1302_DateTime_t *time)
{
    uint8_t raw[8];

    if(time == NULL) {
        return DS1302_ERR_NULL;
    }

    DS1302_ReadClockBurst(raw);
    if(raw[0] & DS1302_SECONDS_CH) {
        return DS1302_ERR_CLOCK_HALTED;
    }
    if(raw[2] & 0x80U) {
        return DS1302_ERR_INVALID_TIME;
    }

    time->second = DS1302_FromBCD(raw[0] & 0x7FU);
    time->minute = DS1302_FromBCD(raw[1] & 0x7FU);
    time->hour = DS1302_FromBCD(raw[2] & 0x3FU);
    time->day = DS1302_FromBCD(raw[3] & 0x3FU);
    time->month = DS1302_FromBCD(raw[4] & 0x1FU);
    time->weekday = DS1302_FromBCD(raw[5] & 0x07U);
    time->year = (uint16_t)(2000U + DS1302_FromBCD(raw[6]));

    if(!DS1302_IsValidDateTime(time)) {
        return DS1302_ERR_INVALID_TIME;
    }

    return DS1302_OK;
}

DS1302_Status_t DS1302_SetDateTime(const DS1302_DateTime_t *time)
{
    if(time == NULL) {
        return DS1302_ERR_NULL;
    }

    if(!DS1302_IsValidDateTime(time)) {
        return DS1302_ERR_INVALID_TIME;
    }

    DS1302_WriteRegister(DS1302_CMD_CONTROL, 0x00);
    DS1302_WriteRegister(DS1302_CMD_SECONDS, DS1302_ToBCD(time->second) & ~DS1302_SECONDS_CH);
    DS1302_WriteRegister(DS1302_CMD_MINUTES, DS1302_ToBCD(time->minute));
    DS1302_WriteRegister(DS1302_CMD_HOURS, DS1302_ToBCD(time->hour));
    DS1302_WriteRegister(DS1302_CMD_DATE, DS1302_ToBCD(time->day));
    DS1302_WriteRegister(DS1302_CMD_MONTH, DS1302_ToBCD(time->month));
    DS1302_WriteRegister(DS1302_CMD_DAY, DS1302_ToBCD(time->weekday));
    DS1302_WriteRegister(DS1302_CMD_YEAR, DS1302_ToBCD((uint8_t)(time->year - 2000U)));
    DS1302_WriteRegister(DS1302_CMD_CONTROL, DS1302_CONTROL_WP);

    return DS1302_OK;
}

bool DS1302_IsClockHalted(void)
{
    return (DS1302_ReadRegister(DS1302_CMD_SECONDS) & DS1302_SECONDS_CH) ? true : false;
}

void DS1302_StartClock(void)
{
    uint8_t seconds = DS1302_ReadRegister(DS1302_CMD_SECONDS);

    DS1302_WriteRegister(DS1302_CMD_CONTROL, 0x00);
    DS1302_WriteRegister(DS1302_CMD_SECONDS, seconds & ~DS1302_SECONDS_CH);
    DS1302_WriteRegister(DS1302_CMD_CONTROL, DS1302_CONTROL_WP);
}
