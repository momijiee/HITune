#include "ti_msp_dl_config.h"
#include "board.h"
#include "stdio.h"

int main(void)
{
    board_init();

    DL_GPIO_initDigitalOutput(DS1302_SCLK_IOMUX);
    DL_GPIO_initDigitalOutput(DS1302_CE_IOMUX);
    DL_GPIO_initDigitalOutput(DS1302_IO_IOMUX);
    DL_GPIO_enableOutput(DS1302_PORT, DS1302_SCLK_PIN | DS1302_IO_PIN | DS1302_CE_PIN);
    DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN | DS1302_IO_PIN | DS1302_CE_PIN);
    delay_ms(100);

    printf("\r\n=== DS1302 PIN SWAP TEST ===\r\n");
    printf("If all fail, try swapping CLK and DAT wires\r\n\r\n");

    // ??????
    printf("--- Normal: CLK=PA23, DAT=PA24, RST=PA25 ---\r\n");

    // ????
    // CE=1
    DL_GPIO_setPins(DS1302_PORT, DS1302_CE_PIN);
    delay_us(10);

    // ????? 0x8E (??????), ?? 0x00 (????)
    // ????: 0x8E = 10001110, LSB first = 01110001
    uint8_t cmd = 0x8E;
    uint8_t dat = 0x00;
    uint8_t i;

    // ?????
    DL_GPIO_initDigitalOutput(DS1302_IO_IOMUX);
    DL_GPIO_enableOutput(DS1302_PORT, DS1302_IO_PIN);
    for(i = 0; i < 8; i++) {
        DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
        if(cmd & 0x01)
            DL_GPIO_setPins(DS1302_PORT, DS1302_IO_PIN);
        else
            DL_GPIO_clearPins(DS1302_PORT, DS1302_IO_PIN);
        cmd >>= 1;
        delay_us(10);
        DL_GPIO_setPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
    }
    // ?????
    for(i = 0; i < 8; i++) {
        DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
        if(dat & 0x01)
            DL_GPIO_setPins(DS1302_PORT, DS1302_IO_PIN);
        else
            DL_GPIO_clearPins(DS1302_PORT, DS1302_IO_PIN);
        dat >>= 1;
        delay_us(10);
        DL_GPIO_setPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
    }
    DL_GPIO_clearPins(DS1302_PORT, DS1302_CE_PIN);
    delay_us(10);

    // ????? 0x80, ?? 0x30
    DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
    delay_us(10);
    DL_GPIO_setPins(DS1302_PORT, DS1302_CE_PIN);
    delay_us(10);
    cmd = 0x80;
    dat = 0x30;
    DL_GPIO_initDigitalOutput(DS1302_IO_IOMUX);
    DL_GPIO_enableOutput(DS1302_PORT, DS1302_IO_PIN);
    for(i = 0; i < 8; i++) {
        DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
        if(cmd & 0x01)
            DL_GPIO_setPins(DS1302_PORT, DS1302_IO_PIN);
        else
            DL_GPIO_clearPins(DS1302_PORT, DS1302_IO_PIN);
        cmd >>= 1;
        delay_us(10);
        DL_GPIO_setPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
    }
    for(i = 0; i < 8; i++) {
        DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
        if(dat & 0x01)
            DL_GPIO_setPins(DS1302_PORT, DS1302_IO_PIN);
        else
            DL_GPIO_clearPins(DS1302_PORT, DS1302_IO_PIN);
        dat >>= 1;
        delay_us(10);
        DL_GPIO_setPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
    }
    DL_GPIO_clearPins(DS1302_PORT, DS1302_CE_PIN);
    delay_us(10);

    // ????? 0x81
    DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
    delay_us(10);
    DL_GPIO_setPins(DS1302_PORT, DS1302_CE_PIN);
    delay_us(10);
    cmd = 0x81;  // ??
    DL_GPIO_initDigitalOutput(DS1302_IO_IOMUX);
    DL_GPIO_enableOutput(DS1302_PORT, DS1302_IO_PIN);
    for(i = 0; i < 8; i++) {
        DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
        if(cmd & 0x01)
            DL_GPIO_setPins(DS1302_PORT, DS1302_IO_PIN);
        else
            DL_GPIO_clearPins(DS1302_PORT, DS1302_IO_PIN);
        cmd >>= 1;
        delay_us(10);
        DL_GPIO_setPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
    }

    // ??IO??????
    DL_GPIO_disableOutput(DS1302_PORT, DS1302_IO_PIN);
    DL_GPIO_initDigitalInput(DS1302_IO_IOMUX);
    delay_us(10);

    uint8_t result = 0;
    for(i = 0; i < 8; i++) {
        DL_GPIO_clearPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
        if(DL_GPIO_readPins(DS1302_PORT, DS1302_IO_PIN) & DS1302_IO_PIN)
            result |= (1 << i);
        delay_us(10);
        DL_GPIO_setPins(DS1302_PORT, DS1302_SCLK_PIN);
        delay_us(10);
    }
    DL_GPIO_clearPins(DS1302_PORT, DS1302_CE_PIN);

    printf("Write 0x30 to SEC, Read back: 0x%02X\r\n", result);

    if(result == 0x00)
        printf("\r\n>>> RESULT: 0x00 = DS1302 not responding\r\n");
    else if(result == 0xFF)
        printf("\r\n>>> RESULT: 0xFF = IO line stuck high, check wiring\r\n");
    else
        printf("\r\n>>> RESULT: DS1302 is responding!\r\n");

    printf("\r\nTry: 1) Insert battery  2) Try 5V  3) Swap CLK<->DAT wires\r\n");

    while(1) { delay_ms(1000); }
}
