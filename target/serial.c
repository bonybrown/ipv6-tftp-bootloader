#include <xc.h>


/*
 * Setup UART1 for serial output at 115200, 8 bits, No parity
 * The xc16 compiler will route puts/putchar/printf
 * via write() to output via this UART
 */
void dbg_setup_uart(void)
{

    //assign UART1 RX input register to the RP14 input (pin 14)
    //RX RP14 (input)
    RPINR18bits.U1RXR = 14;

    //assign the RP15 output (pin 15) to UART1 TX function
    //TX RP15 (output)
    RPOR7bits.RP15R = _RPOUT_U1TX; 

    //peripheral setup
    U1BRG = 85;              //86@80mhz, 85@79.xxx=115200
    U1MODE = 0;              //clear mode register
    U1MODEbits.BRGH = 1;     //use high precision baud generator
    U1STA = 0;               //clear status register
    U1MODEbits.UARTEN = 1;   //enable the UART RX
    IFS0bits.U1RXIF = 0;     //clear the receive flag
    U1STAbits.UTXEN = 1;     //enable UART TX

}
