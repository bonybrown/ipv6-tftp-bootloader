#include "button.h"
#include "beep.h"
#include "timer.h"

#include <xc.h>


#define RESET_BTN_INPUT (PORTAbits.RA7)
#define ONE_SECOND (1000)
#define SHORT_BEEP_MS 100
#define LONG_BEEP_MS 600
/*
 * return the number of beeps that the reset button is held for
 */
uint8_t button_hold_count(){
  uint16_t last = timer_now_ms();
  uint16_t next = last + ONE_SECOND;
  uint16_t now = last;
  uint8_t hold_count = 0;
  uint8_t debounce = 10;
  while( debounce > 0 ){
    now = timer_now_ms();
    if( last != now ){
      last = now;
      if( RESET_BTN_INPUT != 0 ){
        debounce--;
      }
      if( now > next ){
        hold_count++;
        next = now + ONE_SECOND;
        if( hold_count == BUTTON_MAX_COUNT ){
          beep( BEEP_PITCH_HIGH, LONG_BEEP_MS );
          return hold_count;
        }
        else{
          beep( BEEP_PITCH_HIGH, SHORT_BEEP_MS );
        }
      }
    }
  }
  return hold_count;
}