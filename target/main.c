#include <stdint.h>
#include <p33Fxxxx.h>
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
//it's important to keep configuration bits that are compatible with the bootloader
//if you change it from the internall/PLL clock, the bootloader won't run correctly

_FOSCSEL(FNOSC_FRCPLL)            //INT OSC with PLL (always keep this setting)
_FOSC(OSCIOFNC_OFF & POSCMD_NONE) //disable external OSC (always keep this setting)
_FWDT(FWDTEN_OFF)                 //watchdog timer off
_FICD(JTAGEN_OFF & ICS_PGD1);     //JTAG debugging off, debugging on PG1 pins enabled

static uint8_t mac_address[MAC_ADDRESS_SIZE];
//static uint8_t eui64[8];
static uint8_t ipv6_address[IPV6_ADDR_LENGTH] = {0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

void dump_packet(uint8_t *p, size_t s ){
  size_t i = 0;
  for(i=0; i < s ; i++){
    printf("%02x ", *p++ );
  }
  puts("");
}

size_t strnlen(const char *s, size_t maxlen){
  size_t result = 0 ;
  while( result < maxlen && *s != '\0' ){
    result++;
    s++;
  }
  return result;
}

int main()
{
  //setup internal clock for 80MHz/40MIPS
  //7.37/2=3.685*43=158.455/2=79.2275
  CLKDIVbits.PLLPRE=0; // PLLPRE (N2) 0=/2 
  PLLFBD=41; //pll multiplier (M) = +2
  CLKDIVbits.PLLPOST=0;// PLLPOST (N1) 0=/2
  while(!OSCCONbits.LOCK);//wait for PLL ready
  
  /*switch to alt interrupt vector */
  INTCON2bits.ALTIVT = 1;
  
  dbg_setup_uart();
  puts("IPV6 network bootloader");
  SRbits.IPL = 0;	// All interupt levels enabled

  puts("timer init");
  timer_init();
  puts("i2c init");
  i2c_init();
  puts("mc24aa00 init");
  mc24aa00_init(MC24AA00_ADDRESS);
  puts("get mac address");
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
  
  puts("enc28j60 init");
  enc28j60Init( mac_address );
  
  eth_config_set_address( mac_address );
  ipv6_config_set_address( ipv6_address );
  
  udp_bind( 69, tftp_packet_handler );
  
  while(1) {
    uint16_t packet_size, response_length = 0;
    struct eth_packet eth;
    struct ip_packet *pkt ;
    if( enc28j60_poll() ){
      packet_size = enc28j60PacketReceive( (uint8_t*)&eth,  MAX_FRAMELEN);
      if( packet_size > 0 && eth_is_ipv6(&eth.header)){
        //printf("R: %d\n", packet_size );
        //dump_packet((uint8_t*)&eth, packet_size);
        pkt = IP_PACKET_FROM_ETH( &eth );
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
          //printf("S: %d\n", response_length );
          packet_size = response_length + ETH_HEADER_LENGTH + IPV6_HEADER_LENGTH;
          uint8_t *dest_ll = ipv6_physical_address_of( pkt->header.dest_addr );
          if( dest_ll != NULL ){
            eth_address( &eth.header, dest_ll, ETH_TYPE_IPV6 );
            //dump_packet((uint8_t*)&eth, packet_size);
            enc28j60PacketSend1( (uint8_t*)&eth, packet_size);
          }
        }
      }
    }
  }
}

