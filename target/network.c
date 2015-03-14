#include "network.h"
#include <string.h>
#include <stdio.h>

const uint8_t ethernet_mac_broadcast[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

inline uint32_t htonl(  uint32_t host_value){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32_t result = 0;
  uint8_t *r = (uint8_t * )&result;
  r[0] = (host_value >> 24);
  r[1] = (host_value >> 16) & 0xff;
  r[2] = (host_value >> 8) & 0xff;
  r[3] = host_value & 0xff;
  return result; 
#else
  return host_value;
#endif
}

inline uint16_t htons(  uint16_t host_value){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint16_t result = ((host_value >> 8) & 0xff) | ( host_value << 8);
  return result; 
#else
  return host_value;
#endif
}

inline uint16_t ntohs(  uint16_t network_value){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint16_t result = ((network_value >> 8) & 0xff) | ( network_value << 8);
  return result; 
#else
  return network_value;
#endif
}

void checksum_summate ( uint16_t *checksum, void * addr, int count) {
  /* Compute Internet Checksum for "count" bytes
  *         beginning at location "addr".
  */
  uint32_t sum = *checksum;
	uint16_t *addr16 = (uint16_t*)addr;
  while( count > 1 )  {
      /*  This is the inner loop */
	  sum += *addr16;
	  addr16++;
	  count -= 2;
  }

      /*  Add left-over byte, if any */
  if( count > 0 )
	  sum += * (uint8_t *) addr16;

      /*  Fold 32-bit sum to 16 bits */
  while (sum>>16){
      sum = (sum & 0xffff) + (sum >> 16);
	}
  *checksum = sum;
}


enum eth_dest_type ethernet_identify_destination( int packet_size, uint8_t *packet, uint8_t *my_mac_address, uint8_t *my_eui_64 ){
  if( packet_size < ETH_HLEN ) return eth_dest_type_error;
  if( memcmp( packet, my_mac_address , ETH_ALEN ) == 0 ) return eth_dest_type_mine;
  if( memcmp( packet, ethernet_mac_broadcast, ETH_ALEN ) == 0 ) return eth_dest_type_broadcast;
  if( packet[0] == 0x33 && 
    packet[1] == 0x33 && 
    packet[2] == (my_eui_64[4] | 0x01) && /* OR in group address bit */
    memcmp( packet+3, my_mac_address+3, ETH_ALEN - 3) == 0 &&
    packet[6 + ETH_ALEN] == 0x86 && packet[7+ETH_ALEN] == 0xdd /* ETHER TYPE IP V6 */
    ) return eth_dest_type_ipv6_multicast;
  return eth_dest_type_other;
}

enum packet_type get_packet_type(  uint8_t *packet ){
  struct ipv6_packet *ip = AS_IP(packet);
  if( IP_VERSION(ip) != 6 ) return packet_type_other;
  if( ip->next_header == IPPROTO_ICMPV6 ){
    struct icmpv6_packet *icmp = AS_ICMP(packet);
    if( icmp->type == ICMPV6_TYPE_NS ){
      return packet_type_neighbour_solicitation;
    }
    if( icmp->type == ICMPV6_TYPE_PING_REQUEST ){
      return packet_type_ping_request;
    }
  }
  /* TODO:handle tftp case 
  if( ip->next_header == IPPROTO_UDP )
    */
  return packet_type_other;
}

const uint8_t link_local[]={0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00};

int is_solicitation_for_me(  uint8_t *packet, uint8_t *my_eui_64 ){
  struct icmpv6_ns_packet *ns = AS_ICMP_NS(packet);
  
  return( memcmp( &(ns->target_addr), link_local, sizeof(link_local)) == 0  &&
    memcmp( &(ns->target_addr[8]), my_eui_64, 8) == 0 );
}

int build_solicitation_response( int packet_size, uint8_t *packet ,  uint8_t *my_mac_address, uint8_t *my_eui_64 ){
  uint16_t checksum = 0;

  struct icmpv6_ns_packet *ns = AS_ICMP_NS(packet);
  if( ns->option.type != 1 && /* source link layer address */
    ns->option.length != 1) /* size == 8 - 2 =>  6 bytes */
    { 
    return 0; //can't do it
  }
  uint8_t *src_mac= &( ns->option.data );
  /* copy src fields into the destination */
  memcpy( packet, src_mac, ETH_ALEN );
  memcpy( packet + ETH_ALEN, my_mac_address, ETH_ALEN );
  struct ipv6_packet *ip = AS_IP(packet) ;
  memcpy( ip->dest_addr, ip->src_addr, IPV6_ADDR_LENGTH);
  memcpy( ip->src_addr, link_local, 8 );
  memcpy( ip->src_addr+8, my_eui_64, 8);
  ns->type = ICMPV6_TYPE_NA;
  ns->flags[0] = 0x60; /* Not a router, solicited reply, override 0110 0000 */
  ns->flags[1] = 0x00;
  ns->flags[2] = 0x00;
  ns->flags[3] = 0x00;
  memcpy( src_mac, my_mac_address, ETH_ALEN );
  ns->option.type = ICMP_OPTION_TYPE_TARGET_LINK_LAYER_ADDRESS;
  
  checksum = icmpv6_pseduo_header_checksum( &ip->src_addr, &ip->dest_addr, ntohs(ip->payload_length), 58 );

  checksum_set( ns-> checksum, 0 );
  checksum_summate(&checksum, ns, ntohs(ip->payload_length));
  checksum_set( ns-> checksum, htons(~checksum) );
  
  return (src_mac + ETH_ALEN) - packet; //length of packet to send
}

int build_ping_response( int packet_size, uint8_t *packet ,  uint8_t *my_mac_address, uint8_t *my_eui_64 ){
  uint16_t checksum = 0;
  memcpy( packet, packet+ETH_ALEN, ETH_ALEN );
  memcpy( packet + ETH_ALEN, my_mac_address, ETH_ALEN );
  struct ipv6_packet *ip = AS_IP(packet) ;
  memcpy( ip->dest_addr, ip->src_addr, IPV6_ADDR_LENGTH);
  memcpy( ip->src_addr, link_local, 8 );
  memcpy( ip->src_addr+8, my_eui_64, 8);
  struct icmpv6_ping_packet *ping = AS_ICMP_PING(packet);
  ping->type = ICMPV6_TYPE_PING_RESPONSE;
  
  checksum = icmpv6_pseduo_header_checksum( &ip->src_addr, &ip->dest_addr, ntohs(ip->payload_length), 58 );
  checksum_set( ping-> checksum, 0 );
  checksum_summate(&checksum, ping, ntohs(ip->payload_length));
  checksum_set( ping-> checksum, htons(~checksum) );
  return ntohs(ip->payload_length) + ICMP_PACKET_OFFSET;
}

uint16_t icmpv6_pseduo_header_checksum( void *src_addr, void *dest_addr, uint32_t upper_layer_packet_length, uint8_t next_header_value ){
  uint16_t checksum = 0;
  uint32_t network ;
  checksum_summate(&checksum, src_addr, IPV6_ADDR_LENGTH);
  checksum_summate(&checksum, dest_addr, IPV6_ADDR_LENGTH);
  network = htonl( upper_layer_packet_length );
  checksum_summate(&checksum, &network, 4);
  network = htonl( next_header_value );
  checksum_summate(&checksum, &network, 4);
  return checksum;
}
  


void checksum_set( uint8_t checksum_buffer[2], uint16_t checksum )
{
  checksum_buffer[0] = checksum >> 8;
  checksum_buffer[1] = checksum & 0xff;
  
}  
