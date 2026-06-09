#include "board.h"
#include "soft_timer.h"
#include "task_fsm.h"
#include "task_player.h"
#include "task_tracker.h"
#include "task_clock.h"
#include "task_display.h"
#include "bsp_adc.h"
#include "oled.h"
#include <stdio.h>

int main(void)
{
    board_init();
    SysTick_Init();
    
    printf("=== PLAYER DEBUG ===\r\n");
    
    /* ---- 测试A：最原始的串口发送，绕过所有封装 ---- */
    printf("TestA: Raw UART send...\r\n");
    
    // 手动发一帧 SetVolume=20 的完整指令
    // 帧格式: 7E FF 06 06 00 00 14 xx xx EF
    uint8_t frame[] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x14, 0xFF, 0xE6, 0xEF};
    for (int i = 0; i < 10; i++) {
        while(DL_UART_isBusy(UART_PLR_INST) == true);
        DL_UART_Main_transmitData(UART_PLR_INST, frame[i]);
    }
    delay_ms(500);
    
    // 手动发一帧 PlayVoice(2): CMD=0x14, Para=0x0002
    // Checksum = 0 - (0xFF+0x06+0x14+0x00+0x00+0x02) = 0 - 0x11B = 0xFEE5
    uint8_t play[] = {0x7E, 0xFF, 0x06, 0x14, 0x00, 0x00, 0x02, 0xFE, 0xE5, 0xEF};
    for (int i = 0; i < 10; i++) {
        while(DL_UART_isBusy(UART_PLR_INST) == true);
        DL_UART_Main_transmitData(UART_PLR_INST, play[i]);
    }
    
    printf("TestA: Sent. Wait 5s...\r\n");
    delay_ms(5000);
    
    /* ---- 测试B：确认UART_PLR_INST是否正确 ---- */
    printf("TestB: Ping on PLR UART TX pin...\r\n");
    // 连续发0x55（方便示波器/逻辑分析仪看波形）
    for (int i = 0; i < 20; i++) {
        while(DL_UART_isBusy(UART_PLR_INST) == true);
        DL_UART_Main_transmitData(UART_PLR_INST, 0x55);
    }
    printf("TestB: Done.\r\n");
    delay_ms(1000);

    /* ---- 测试C：用你的封装函数 ---- */
    printf("TestC: Player_SetVolume + Player_PlayVoice...\r\n");
    Player_SetVolume(25);
    delay_ms(200);
    Player_PlayVoice(2);
    printf("TestC: Sent. Wait 5s...\r\n");
    delay_ms(5000);
    
    printf("=== ALL TESTS DONE ===\r\n");
    while(1) {}
}

/* ======== 中断 ======== */
void ADC_INST_IRQHandler(void)
{
    BSP_ADC_IRQHandler();
}
