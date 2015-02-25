#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "network.h"

#define PIPE_CHAR '|'
#define PIPE_STRING "|"

unsigned char my_address[ETH_ALEN] = {0x00,0x1e,0xc0,          0x81,0xf9,0x1b};
unsigned char my_eui64[ETH_ALEN+2] = {0x00 | 0x02,0x1e,0xc0,0xff,0xfe,0x81,0xf9,0x1b};


void dump_packet( int size, unsigned char *packet ){
  puts("+---------+---------------+----------+");
  puts("03:03:32,814,282   ETHER");

  //printf("|%d |\t|",size );
  printf("|0   |");
  int i  =0;
  for(i = 0 ; i< size ; i++ ){
	  printf("%02x|", packet[i]);
  }
  puts("\n");
}

void test_processing( int size, unsigned char *packet ){
  enum eth_dest_type dest = ethernet_identify_destination( size, packet, my_address, my_eui64 );
//  printf("eth type: %d\n", dest );
  enum packet_type ptype = get_packet_type( packet );
//  printf("pkt type: %d\n", ptype );
  
  if( dest == eth_dest_type_ipv6_multicast && 
    ptype == packet_type_neighbour_solicitation ){
//    puts("NS");
    if( is_solicitation_for_me(  packet, my_eui64 ) ){
//      puts("for me");
      size = build_solicitation_response( size, packet ,  my_address,  my_eui64 );
      dump_packet(size, packet);
    }

  }
}


int main( int argc, char *argv[]){
	char line[4096];
	unsigned char pkt_buffer[MAX_ETHERNET_PACKET_SIZE];
	int line_number = 0;

	while( fgets( line, sizeof(line) , stdin ) != NULL ){
		line_number++;
		if( line[0] == PIPE_CHAR ){
			int pkt_index = -1;
			char *tok = strtok( line, PIPE_STRING);
			while( tok ){
				if( pkt_index >=  0 ){
					unsigned long data = strtoul( tok, NULL, 16 );
					if( pkt_index >= MAX_ETHERNET_PACKET_SIZE ){
						fprintf(stderr,"Too much data for ethernet packet on line %d\n", line_number);
						break;
					}
					pkt_buffer[pkt_index] = (unsigned char) data;
				}
				pkt_index++;
				tok = strtok( NULL , PIPE_STRING );
			}
			dump_packet(pkt_index-1, pkt_buffer);
			test_processing(pkt_index-1, pkt_buffer);
		}
	}

	return 0;

}
