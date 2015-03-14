#include <stdint.h>
#include <p33Fxxxx.h>
#include <string.h>
#include <stdio.h>
#include "network.h"
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
static uint8_t eui64[8];

int main()
{
  //setup internal clock for 80MHz/40MIPS
  //7.37/2=3.685*43=158.455/2=79.2275
  CLKDIVbits.PLLPRE=0; // PLLPRE (N2) 0=/2 
  PLLFBD=41; //pll multiplier (M) = +2
  CLKDIVbits.PLLPOST=0;// PLLPOST (N1) 0=/2
  while(!OSCCONbits.LOCK);//wait for PLL ready
  
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
  
  eui64[0] =  mac_address[0] | 0x02;
  eui64[1] =  mac_address[1];
  eui64[2] =  mac_address[2];
  eui64[3] =  0xff;
  eui64[4] =  0xfe;
  eui64[5] =  mac_address[3];
  eui64[6] =  mac_address[4];
  eui64[7] =  mac_address[5];
  
  printf("EUI64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",eui64[0],eui64[1],eui64[2],eui64[3],eui64[4],eui64[5],eui64[6],eui64[7]);
  
  puts("enc28j60 init");
  enc28j60Init( mac_address );
  
  while(1) {
    uint16_t packet_size, i;
    uint8_t packet_buffer[MAX_FRAMELEN];
    if( enc28j60_poll() ){
//      puts("P");
      packet_size = enc28j60PacketReceive( packet_buffer,  MAX_FRAMELEN);
      if( packet_size > 0 )
      {
/*	printf("got packet of size %d\n", packet_size );
	for( i = 0; i < packet_size ;i++ )
	{
	  printf("%02x ", packet_buffer[i]);
	}
	puts("");
*/      
	enum eth_dest_type dest = ethernet_identify_destination( packet_size, packet_buffer, mac_address, eui64 );
	enum packet_type ptype = get_packet_type( packet_buffer );
//	printf("pkt type: %d\n", ptype );
	
	if( dest == eth_dest_type_mine &&
	  ptype == packet_type_ping_request){
//	    puts("PING REQ");
	    
	    packet_size = build_ping_response( packet_size, packet_buffer ,  mac_address,  eui64 );
	    enc28j60PacketSend1( packet_buffer, packet_size);
	}
	  
	if( (dest == eth_dest_type_ipv6_multicast || dest == eth_dest_type_mine) && 
	  ptype == packet_type_neighbour_solicitation ){
//	  puts("NS");
	
	  struct icmpv6_ns_packet *ns = AS_ICMP_NS(packet_buffer);
/*	  uint8_t *l = (uint8_t*)&(ns->target_addr);
	printf("LL: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",l[0],l[1],l[2],l[3],l[4],l[5],l[6],l[7]);
	l=(uint8_t*)&(ns->target_addr[8]);
	printf("MA: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",l[0],l[1],l[2],l[3],l[4],l[5],l[6],l[7]);
*/	      
	  if( is_solicitation_for_me(  packet_buffer, eui64 ) ){
//	    puts("for me");
	    packet_size = build_solicitation_response( packet_size, packet_buffer ,  mac_address,  eui64 );
	    enc28j60PacketSend1( packet_buffer, packet_size);
	  }
	}
      }
    }
  }
}

