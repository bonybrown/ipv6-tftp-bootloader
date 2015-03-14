

#ifndef _EN28J60_HW_H_
#define _EN28J60_HW_H_

#include <p33Fxxxx.h>

//custom pin assignments for our hardware
// ENC28J60 I/O pins

//mapping:
//A2 ETH-INT
//C2 MISO
//C1 MOSI
//C0 CLK
//B3 CS
//B2 RST
//CS and RST pins
#define ENC_RST_TRIS		(TRISBbits.TRISB2)	
#define ENC_RST_IO		(PORTBbits.RB2)
#define ENC_RST_LAT         	(LATBbits.LATB2)
#define ENC_CS_TRIS		(TRISBbits.TRISB3)
#define ENC_CS_IO		(PORTBbits.RB3)
#define ENC_CS_LAT          	(LATBbits.LATB3)
#define ENC_INT_TRIS		(TRISAbits.TRISA2)	
#define ENC_INT_IO		(PORTAbits.RA2)
#define ENC_INT_LAT         	(LATAbits.LATA2)
//assign SPI module to ENC28J60
#define ENC_SPI_IF		(IFS0bits.SPI1IF)
#define ENC_SPIBUF		(SPI1BUF)
#define ENC_SPISTAT		(SPI1STAT)
#define ENC_SPISTATbits		(SPI1STATbits)
#define ENC_SPICON1		(SPI1CON1)
#define ENC_SPICON1bits		(SPI1CON1bits)
#define ENC_SPICON2		(SPI1CON2)


// start TX buffer approx 1500 bytes back from end of buffer 0x1FFF
#define TXSTART_INIT   	0x1A00	

//macro functions for working with the ENC interface
#define ENC_CS_EN()    ENC_CS_LAT=0 
#define ENC_CS_DIS()   ENC_CS_LAT=1
#define ENC_HARDRESET() ENC_RST_LAT = 0; timer_delay_ms(10); ENC_RST_LAT = 1
#define ENC_SPITXRX() 	while ((ENC_SPISTATbits.SPITBF == 1) || (ENC_SPISTATbits.SPIRBF == 0));

#endif
