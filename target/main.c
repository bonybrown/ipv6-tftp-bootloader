#include <stdint.h>
#include <p33Fxxxx.h>
#include <string.h>
#include "network.h"

//it's important to keep configuration bits that are compatible with the bootloader
//if you change it from the internall/PLL clock, the bootloader won't run correctly

_FOSCSEL(FNOSC_FRCPLL)            //INT OSC with PLL (always keep this setting)
_FOSC(OSCIOFNC_OFF & POSCMD_NONE) //disable external OSC (always keep this setting)
_FWDT(FWDTEN_OFF)                 //watchdog timer off
_FICD(JTAGEN_OFF & ICS_PGD1);     //JTAG debugging off, debugging on PG1 pins enabled


int main()
{
  int reset_button_hold_count = 0;
   uint16_t *checksum = NULL;
   uint8_t *addr = NULL;
   int count  = 4;
   
  //setup internal clock for 80MHz/40MIPS
  //7.37/2=3.685*43=158.455/2=79.2275
  CLKDIVbits.PLLPRE=0; // PLLPRE (N2) 0=/2 
  PLLFBD=41; //pll multiplier (M) = +2
  CLKDIVbits.PLLPOST=0;// PLLPOST (N1) 0=/2
  while(!OSCCONbits.LOCK);//wait for PLL ready
  
  SRbits.IPL = 0;	// All interupt levels enabled
  
  
  while(1) {
    /* Idle! */
    reset_button_hold_count++;
    checksum_summate ( checksum, addr,  count);
    if( reset_button_hold_count == 43 ) {
      break;
    }
  }
}

