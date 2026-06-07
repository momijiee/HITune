#include "ds1302.h"
#include "board.h"

// DAT 引脚设置为输出模式
static void DS1302_DAT_Output(void)
{
    DL_GPIO_initDigitalOutput(DS1302_DAT_IOMUX);
    DL_GPIO_enableOutput(DS1302_PORT, DS1302_DAT_PIN);
}

// DAT 引脚设置为输入模式
static void DS1302_DAT_Input(void)
{
    DL_GPIO_initDigitalInput(DS1302_DAT_IOMUX);
    DL_GPIO_disableOutput(DS1302_PORT, DS1302_DAT_PIN);
}

// BCD 转 十进制
static uint8_t BCD2DEC(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 十进制 转 BCD
static uint8_t DEC2BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

// 向 DS1302 写入一个字节
static void DS1302_WriteOneByte(uint8_t data)
{
    uint8_t i;
    DS1302_DAT_Output();

    for (i = 0; i < 8; i++)
    {
        DS1302_CLK_LOW();
        delay_us(2);

        if (data & 0x01)
            DS1302_DAT_HIGH();
        else
            DS1302_DAT_LOW();

        data >>= 1;
        delay_us(2);
        DS1302_CLK_HIGH();
        delay_us(2);
    }
}

// 从 DS1302 读取一个字节
static uint8_t DS1302_ReadOneByte(void)
{
    uint8_t i, data = 0;
    DS1302_DAT_Input();

    for (i = 0; i < 8; i++)
    {
        DS1302_CLK_LOW();
        delay_us(2);

        if (DS1302_DAT_READ())
            data |= (1 << i);

        DS1302_CLK_HIGH();
        delay_us(2);
    }

    return data;
}

// 向指定寄存器写入数据
void DS1302_WriteByte(uint8_t addr, uint8_t data)
{
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    delay_us(2);
    DS1302_RST_HIGH();
    delay_us(2);

    DS1302_WriteOneByte(addr);
    DS1302_WriteOneByte(data);

    DS1302_RST_LOW();
    delay_us(2);
}

// 从指定寄存器读取数据
uint8_t DS1302_ReadByte(uint8_t addr)
{
    uint8_t data;

    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    delay_us(2);
    DS1302_RST_HIGH();
    delay_us(2);

    DS1302_WriteOneByte(addr | 0x01);  // 读地址 = 写地址 | 0x01
    data = DS1302_ReadOneByte();

    DS1302_RST_LOW();
    delay_us(2);

    return data;
}

// DS1302 初始化
void DS1302_Init(void)
{
    DS1302_RST_LOW();
    DS1302_CLK_LOW();
    delay_ms(10);

    // 关闭写保护
    DS1302_WriteByte(DS1302_CTRL_REG, 0x00);
}

// 设置时间
void DS1302_SetTime(DS1302_Time_t *time)
{
    // 关闭写保护
    DS1302_WriteByte(DS1302_CTRL_REG, 0x00);

    DS1302_WriteByte(DS1302_YEAR_REG,  DEC2BCD(time->year));
    DS1302_WriteByte(DS1302_MONTH_REG, DEC2BCD(time->month));
    DS1302_WriteByte(DS1302_DATE_REG,  DEC2BCD(time->date));
    DS1302_WriteByte(DS1302_DAY_REG,   DEC2BCD(time->day));
    DS1302_WriteByte(DS1302_HOUR_REG,  DEC2BCD(time->hour));
    DS1302_WriteByte(DS1302_MIN_REG,   DEC2BCD(time->min));
    DS1302_WriteByte(DS1302_SEC_REG,   DEC2BCD(time->sec));

    // 开启写保护
    DS1302_WriteByte(DS1302_CTRL_REG, 0x80);
}

// 读取时间
void DS1302_GetTime(DS1302_Time_t *time)
{
    time->year  = BCD2DEC(DS1302_ReadByte(DS1302_YEAR_REG));
    time->month = BCD2DEC(DS1302_ReadByte(DS1302_MONTH_REG));
    time->date  = BCD2DEC(DS1302_ReadByte(DS1302_DATE_REG));
    time->day   = BCD2DEC(DS1302_ReadByte(DS1302_DAY_REG));
    time->hour  = BCD2DEC(DS1302_ReadByte(DS1302_HOUR_REG));
    time->min   = BCD2DEC(DS1302_ReadByte(DS1302_MIN_REG));
    time->sec   = BCD2DEC(DS1302_ReadByte(DS1302_SEC_REG));
}
