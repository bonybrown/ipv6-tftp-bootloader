#ifndef __BEEP_H__
#define __BEEP_H__

#include <stdint.h>

#define BEEP_PITCH_HIGH 0x0C7
#define BEEP_PITCH_LOW  0x2C7


void beep_init(void);

void beep( uint16_t pitch, uint16_t duration_ms );

#endif