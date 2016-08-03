#include <stdint.h>
#include <xc.h>
#include <libpic30.h>
#include <string.h>
#include <stdio.h>
#include "net.h"
#include "icmpv6.h"
#include "udp.h"
#include "tftp.h"
#include "i2c.h"
#include "mc24aa00.h"
#include "serial.h"
#include "timer.h"
#include "enc28j60.h"
#include "beep.h"
#include "button.h"

//it's important to keep configuration bits that are compatible with the bootloader
//if you change it from the internall/PLL clock, target program may not run correctly

_FOSCSEL(FNOSC_FRCPLL)            //INT OSC with PLL (always keep this setting)
_FOSC(OSCIOFNC_OFF & POSCMD_NONE) //disable external OSC (always keep this setting)
_FWDT(FWDTEN_OFF)                 //watchdog timer off
_FICD(JTAGEN_OFF & ICS_PGD1);     //JTAG debugging off, debugging on PG1 pins enabled

static uint8_t mac_address[MAC_ADDRESS_SIZE];
static uint8_t ipv6_address[IPV6_ADDR_LENGTH] = {0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/* For passing the button hold count to the target program */
int reset_button_hold_count __attribute__((section("bootloader_data"),address(0x800))) = 0;


/* Because the standard libraries don't include an implementation of strnlen */
size_t strnlen(const char *s, size_t maxlen){
  size_t result = 0 ;
  while( result < maxlen && *s != '\0' ){
    result++;
    s++;
  }
  return result;
}

/* Called to start the target program.
 * Will not return if successful
 * Will sound beeper and return if not
 */
void start_target_prog(uint8_t function_code){
  uint8_t buf[3] = {0};
  _memcpy_p2d24(buf,0x3000, 3);
  printf("%02x,%02x,%02x\n",buf[0],buf[1],buf[2]);
  if( buf[0] == 0xff && buf[1] == 0xff && buf[2] == 0xff ){
    /* guess it's blank?. Do not jump to this */
    beep(BEEP_PITCH_LOW, 300);
    timer_delay_ms(100);
    beep(BEEP_PITCH_LOW, 300);
    timer_delay_ms(100);
    beep(BEEP_PITCH_LOW, 300);
    return;
  }
  /* Going to launch target */
  reset_button_hold_count = function_code;
  SRbits.IPL = 7; // All interupt levels disabled
  INTCON2bits.ALTIVT = 0; //back to original INT VT
  asm("GOTO 0x3000");
}
  

int main()
{
  uint8_t function_code = 0;
  //setup internal clock for 80MHz/40MIPS
  //7.37/2=3.685*43=158.455/2=79.2275
  CLKDIVbits.PLLPRE=0; // PLLPRE (N2) 0=/2 
  PLLFBD=41; //pll multiplier (M) = +2
  CLKDIVbits.PLLPOST=0;// PLLPOST (N1) 0=/2
  while(!OSCCONbits.LOCK);//wait for PLL ready
  
  
  /* 
   * Switch to alt interrupt vector.
   * The bootloader will only use the alternate vectors
   * The primary ones are reserved for the target
   */
  INTCON2bits.ALTIVT = 1;
  
  dbg_setup_uart();
  puts("IPV6 network bootloader");
  timer_init();
  beep_init();
  
  SRbits.IPL = 0; // All interupt levels enabled
  
  function_code = button_hold_count();
  printf("function: %u\n", function_code );

  if( function_code < BUTTON_MAX_COUNT ){
    start_target_prog( function_code );
  }

  i2c_init();
  mc24aa00_init(MC24AA00_ADDRESS);
  /* wait for power on of mc24aa00 */
  timer_delay_ms( 100 );
  mc24aa00_read_mac_address(MC24AA00_ADDRESS, mac_address, MAC_ADDRESS_SIZE );
  printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]);
  
  ipv6_address[8] =  mac_address[0] | 0x02;
  ipv6_address[9] =  mac_address[1];
  ipv6_address[10] =  mac_address[2];
  ipv6_address[11] =  0xff;
  ipv6_address[12] =  0xfe;
  ipv6_address[13] =  mac_address[3];
  ipv6_address[14] =  mac_address[4];
  ipv6_address[15] =  mac_address[5];
  
  printf("IP ADDR: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",ipv6_address[0],ipv6_address[1],ipv6_address[2],ipv6_address[3],ipv6_address[4],ipv6_address[5],ipv6_address[6],ipv6_address[7],ipv6_address[8],ipv6_address[9],ipv6_address[10],ipv6_address[11],ipv6_address[12],ipv6_address[13],ipv6_address[14],ipv6_address[15]);
  
  enc28j60Init( mac_address );
  
  eth_config_set_address( mac_address );
  ipv6_config_set_address( ipv6_address );
  
  udp_bind( 69, tftp_packet_handler );
  
  /* read a packet, process and respond if required */
  while(1) {
    uint16_t packet_size, response_length = 0;
    struct eth_packet eth;
    struct ip_packet *pkt ;
    if( enc28j60_poll() ){
      packet_size = enc28j60PacketReceive( (uint8_t*)&eth,  MAX_FRAMELEN);
      if( packet_size > 0 && eth_is_ipv6(&eth.header)){

        pkt = IP_PACKET_FROM_ETH( &eth );
        
        /* for any packets directed specifically to this IP address, cache the eth src address of the sender */
        if( ipv6_is_for_this_address(&pkt->header) ){
          ipv6_physical_add_entry( pkt->header.src_addr, eth.header.src_addr );
        }

        response_length = 0;
        switch( pkt->header.next_header ){
          case IPV6_NEXT_HEADER_ICMPV6:
            response_length = icmpv6_dispatch( pkt );
            break;
          case IPV6_NEXT_HEADER_UDP:
            response_length = udp_dispatch( pkt );
            break;
        }
        if( response_length > 0){
          /* something to send */
          packet_size = response_length + ETH_HEADER_LENGTH + IPV6_HEADER_LENGTH;
          /* find the ethernet address of the IP destination */
          uint8_t *dest_ll = ipv6_physical_address_of( pkt->header.dest_addr );
          if( dest_ll != NULL ){
            /* send if we can address the packet */
            eth_address( &eth.header, dest_ll, ETH_TYPE_IPV6 );
            enc28j60PacketSend1( (uint8_t*)&eth, packet_size);
          }
        }
      }
    }
  }
}

