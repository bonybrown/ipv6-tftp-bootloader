#include "timer.h"
#include <p33Fxxxx.h>

static unsigned int timer_value;

void __attribute__((__interrupt__, no_auto_psv)) _AltT2Interrupt(void)
{
	IFS0bits.T2IF = 0;  // Clear Timer1 Interrupt Flag
	timer_value++;
  //PORT B 5 output toggle
  LATBbits.LATB5 = !PORTBbits.RB5;
}

/*
 * Sets up timer 2 for 1ms operation
 */
void timer_init(void) {

  timer_value =0;
  // Fosc = 80Mhz, Fcy = 40Mhz,  prescale of 1:64 = Fin = 625kHz = 1.6uSec
  //   reload register 625 = 625kHz/625 = 1kHz =  1ms
  T2CONbits.TON = 0; // Disable Timer
  T2CONbits.TCS = 0; // Select internal instruction cycle clock
  T2CONbits.TGATE = 0; // Disable Gated Timer mode
  //bit 5-4 TCKPS<1:0>: Timerx Input Clock Prescale Select bits
  //    11 = 1:256 prescale value
  //    10 = 1:64 prescale value
  //    01 = 1:8 prescale value
  //    00 = 1:1 prescale value
  T2CONbits.TCKPS = 0b10; // Select 1:64 Prescaler

  TMR2 = 0x00;  // Clear timer register
  PR2 = 625;  // Load the period value
  IPC1bits.T2IP = 0x01; // Set Timer2 Interrupt Priority Level
  IFS0bits.T2IF = 0; // Clear Timer2 Interrupt Flag
  IEC0bits.T2IE = 1; // Enable Timer2 interrupt
  T2CONbits.TON = 1; // Start Timer

  TRISBbits.TRISB5 = 0; //PORT B 5 output
}

/* delay for a number of milliseconds. This is blocking */
void timer_delay_ms( uint16_t milliseconds ){
    uint16_t stop_value = timer_value + milliseconds;
    while( timer_value != stop_value){
      //waste time
    }
}

