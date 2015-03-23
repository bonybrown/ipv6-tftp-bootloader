#include "packet.h"
#include "network.h"
#include <string.h>
#include <stdio.h>

static uint8_t *packet;
static uint16_t length;

void packet_set( void *pkt, uint16_t len ){
  packet = (uint8_t*)pkt;
  length = len;
}



int internal_match( uint16_t member_size, uint16_t member_offset, void* value ){
 switch( member_size ){
  case 1:
    return packet[member_offset] == *((uint8_t*) value);
  case 2:
    return *((uint16_t *)(packet+member_offset)) == htons(*((uint16_t*) value));
  case 4:
    return *((uint32_t *)(packet+member_offset)) == htonl(*((uint32_t*) value));
  default:
    return (memcmp( packet+member_offset, value, member_size)) == 0;
  }
}
/*

packet_match_ethernet( ETHERNET_DEST_ADDR, *eth_addr )
packet_match_ethernet( MULTICAST, NULL)
packet_match_ethernet( TYPE, IPV6 )

packet_match_ipv6( DEST_IP, *ipv6_address )
packet_match_ipv6( TYPE, ICMPV6  )
packet_match_ipv6( TYPE, UDP  )

packet_match_icmpv6( TYPE,  PING_REQ )
packet_match_icmpv6( TYPE,  NS )

packet_match_udp( DEST_PORT, 69 )

*/