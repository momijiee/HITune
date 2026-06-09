#ifndef __TASK_CLOCK_H
#define __TASK_CLOCK_H

#include "board.h"

typedef struct {
    uint8_t hour, min, sec;
} Clock_t;

void    TaskClock_Init(void);
void    TaskClock_Run(void);
Clock_t TaskClock_GetTime(void);
void    TaskClock_SetTime(uint8_t h, uint8_t m, uint8_t s);

#endif