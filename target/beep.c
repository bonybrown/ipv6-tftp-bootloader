#include "beep.h"
#include "timer.h"
#include <xc.h>


static void beep_set_pitch( uint16_t pitch ){
  while( TMR3 ){
    //wait
  }
  OC1RS = pitch / 2;
  PR3 = pitch;
  T3CONbits.TON = 1; // Start Timer
}

void beep( uint16_t pitch, uint16_t duration_ms ){
  uint16_t now = timer_now_ms();
  now += duration_ms;
  beep_set_pitch( pitch );
  while( now != timer_now_ms() ){
    //do nothing
  }
  beep_set_pitch(0);
}

void beep_init(){

  // Fosc = 80Mhz, Fcy = 40Mhz,  prescale of 1:64 = Fin = 625kHz
  //   division by the reload counter gives different notes
  T3CONbits.TON = 0; // Disable Timer
  T3CONbits.TCS = 0; // Select internal instruction cycle clock
  T3CONbits.TGATE = 0; // Disable Gated Timer mode
  T3CONbits.TCKPS = 0b10; // Select 1:64 Prescaler
  
  //configure output compare module
  __builtin_write_OSCCONL(OSCCON & ~(1<<6)); 
  RPOR12bits.RP25R = 18; //assign OC1 module to pin RP25 (RC 9 )
  //__builtin_write_OSCCONL(OSCCON | (1<<6));

  OC1CONbits.OCM = 0 ; //module disable
  OC1CONbits.OCTSEL =1; //source is timer 3
  OC1RS = PR3 / 2;
  OC1CONbits.OCM = 6 ; //PWM without fault protection  
  TMR3 = 0;
   
  beep_set_pitch( 0 );
  
}


