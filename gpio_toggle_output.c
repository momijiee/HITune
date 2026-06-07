#include "ti_msp_dl_config.h"
#include "board.h"
#include "ds1302.h"
#include "oled.h"
#include "stdio.h"

DS1302_Time_t time_now;
char time_str[20];

int main(void)
{
    board_init();
    OLED_Init();
    DS1302_Init();

    // 首次设置时间: 2025-06-07 星期六 15:30:00
    DS1302_Time_t set_time = {
        .year  = 25,
        .month = 6,
        .date  = 7,
        .day   = 6,
        .hour  = 15,
        .min   = 30,
        .sec   = 0
    };
    DS1302_SetTime(&set_time);

    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t *)"DS1302 Clock", 16, 1);

    while (1)
    {
        DS1302_GetTime(&time_now);

        // 第一行显示日期
        sprintf(time_str, "20%02d-%02d-%02d",
                time_now.year, time_now.month, time_now.date);
        OLED_ShowString(0, 2, (uint8_t *)time_str, 16, 1);

        // 第二行显示时间
        sprintf(time_str, "%02d:%02d:%02d",
                time_now.hour, time_now.min, time_now.sec);
        OLED_ShowString(0, 4, (uint8_t *)time_str, 16, 1);

        // 串口也输出
        printf("20%02d-%02d-%02d %02d:%02d:%02d\r\n",
               time_now.year, time_now.month, time_now.date,
               time_now.hour, time_now.min, time_now.sec);

        delay_ms(500);
    }
}
