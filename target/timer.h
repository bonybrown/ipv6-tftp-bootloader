
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

void timer_init();

/* delay for a number of milliseconds. This is blocking */
void timer_delay_ms( uint16_t milliseconds );

#endif

