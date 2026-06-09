#include "board.h"
#include "stdio.h"

#define RE_0_BUFF_LEN_MAX	128

volatile uint8_t  recv0_buff[RE_0_BUFF_LEN_MAX] = {0};
volatile uint16_t recv0_length = 0;
volatile uint8_t  recv0_flag = 0;

void board_init(void)
{
	// SYSCFG???
	SYSCFG_DL_init();
	// ????????
	NVIC_ClearPendingIRQ(UART_INST_INT_IRQN);
	// ??????
	NVIC_EnableIRQ(UART_INST_INT_IRQN);
	
	printf("Board Init [[ ** LCKFB ** ]]\r\n");
}

void delay_us(unsigned long __us)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 38;

    ticks = __us * (CPUCLK_FREQ / 1000000UL);  // ← 用宏替代硬编码

    told = SysTick->VAL;

    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += SysTick->LOAD - tnow + told;
            told = tnow;
            if (tcnt >= ticks)
                break;
        }
    }
}


// ????????????ms??
void delay_ms(unsigned long ms) 
{
	delay_us( ms * 1000 );
}

void delay_1us(unsigned long __us){ delay_us(__us); }
void delay_1ms(unsigned long ms){ delay_ms(ms); }

// ????????
void uart0_send_char(char ch)
{
	while( DL_UART_isBusy(UART_INST) == true );
	DL_UART_Main_transmitData(UART_INST, ch);
}

// ???????
void uart0_send_string(char* str)
{
	while(*str!=0&&str!=0)
	{
		uart0_send_char(*str++);
	}
}


#if !defined(__MICROLIB)
#if (__ARMCLIB_VERSION <= 6000000)
struct __FILE
{
	int handle;
};
#endif

FILE __stdout;

void _sys_exit(int x)
{
	x = x;
}
#endif


// printf?????
int fputc(int ch, FILE *stream)
{
	while( DL_UART_isBusy(UART_INST) == true );
	DL_UART_Main_transmitData(UART_INST, ch);
	return ch;
}

// ?????????
void UART_INST_IRQHandler(void)
{
	uint8_t receivedData = 0;
	
	switch( DL_UART_getPendingInterrupt(UART_INST) )
	{
		case DL_UART_IIDX_RX:
			
			receivedData = DL_UART_Main_receiveData(UART_INST);

			if (recv0_length < RE_0_BUFF_LEN_MAX - 1)
			{
				recv0_buff[recv0_length++] = receivedData;
				uart0_send_char(receivedData);
			}
			else
			{
				recv0_length = 0;
			}

			recv0_flag = 1;
		
			break;
		
		default:
			break;
	}
}
