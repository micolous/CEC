#include <libopencm3/cm3/systick.h>
#include <libopencm3/efm32/cmu.h>
#include <libopencm3/efm32/timer.h>

#include "Timer.h"

volatile uint32_t systick_millis = 0;

void timer_init(void) {
   /* Configure the system tick */
   systick_set_frequency(SYSTICK_FREQUENCY, AHB_FREQUENCY);
   systick_counter_enable();
   systick_interrupt_enable();

   /* set up a microsecond free running timer for ... things... */
   cmu_periph_clock_enable(CMU_TIMER2);
   /* ~7MHz counter */
   
   //timer_set_prescaler(TIM6, AHB_FREQUENCY / 1e6 - 1);
   //timer_set_period(TIM6, 0xffff);
   //timer_one_shot_mode(TIM6);
   timer_start(TIMER2);

}

void sys_tick_handler(void) {
   systick_millis++;
}

void delay(uint32_t millis) {
   uint32_t target = systick_millis + millis;
   while (target > systick_millis);
}

uint32_t millis(void) {
   return systick_millis;
}

uint32_t micros(void) {
   return TIMER0_CNT / (AHB_FREQUENCY / 1e6);
}

void delayMicroseconds(uint32_t time) {
   uint32_t dtime = micros() + time;
   while (micros() < dtime);
}

