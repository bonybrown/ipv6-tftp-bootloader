
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

void timer_init();

/* delay for a number of milliseconds. This is blocking */
void timer_delay_ms( uint16_t milliseconds );

/* return the number of mS the timer has counted.
 * This value will overflow to 0 */
uint16_t timer_now_ms();

#endif

