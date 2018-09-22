#ifndef TIMER_H__
#define TIMER_H__
#include <stdint.h>

/* Systick interrupt frequency, Hz */
#define SYSTICK_FREQUENCY 1000

/* Default AHB (core clock) frequency of Tomu board */
#define AHB_FREQUENCY 14000000

#ifdef __cplusplus
extern "C" {
#endif

void timer_init(void);
void sys_tick_handler(void);
void delay(uint32_t millis);
uint32_t millis(void);
uint32_t micros(void);
void delayMicroseconds(uint32_t time);

#ifdef __cplusplus
}
#endif

#endif // TIMER_H__

