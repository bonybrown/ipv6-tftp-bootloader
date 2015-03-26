#include "icmpv6.h"
#include <string.h>

static void icmp_response(struct ip_packet *pkt, struct icmpv6_header *icmp, uint8_t icmp_type, uint16_t payload_length );
static int icmp_ping( struct ip_packet *pkt );
static int icmp_neighbour_solicitation( struct ip_packet *pkt );


void icmp_response(struct ip_packet *pkt, struct icmpv6_header *icmp, uint8_t icmp_type, uint16_t payload_length ){
  uint16_t checksum = 0;
  icmp->type = icmp_type;
  icmp->checksum = 0;
  
  ipv6_prepare( &pkt->header, pkt->header.src_addr, IPV6_NEXT_HEADER_ICMPV6, payload_length );

  checksum = ipv6_pseduo_header_checksum( pkt->header.src_addr, pkt->header.dest_addr, payload_length, IPV6_NEXT_HEADER_ICMPV6 );
  checksum_summate(&checksum, icmp, payload_length);
  icmp->checksum = (~checksum); //htons NOT required here?
}

int icmp_ping( struct ip_packet *pkt ){
  struct icmpv6_ping_packet *ping = (struct icmpv6_ping_packet *)(pkt->payload);
  uint16_t payload_length = ntohs( pkt->header.payload_length ) ;
  icmp_response(pkt,&ping->header,ICMPV6_TYPE_PING_RESPONSE,payload_length);
  return payload_length;
}

int icmp_neighbour_solicitation( struct ip_packet *pkt ){
  struct icmpv6_ns_packet *ns = (struct icmpv6_ns_packet *)(pkt->payload);
  uint16_t payload_length = ntohs( pkt->header.payload_length );

  if( payload_length <= IPV6_NS_HEADER_LENGTH ){
    /* packet too short, doesn't contain an option */
    return 0;
  }
  
  if( ns->option.type != ICMP_OPTION_TYPE_SOURCE_LINK_LAYER_ADDRESS && /* source link layer address */
      ns->option.length != 1){ /* size == 8 - 2 =>  6 bytes */
    return 0; //can't do it
  }

  /* Add src ip and it's link layer address to the cache
   * This will be used when the response is addressed back to the sender
   */
  ipv6_physical_add_entry( pkt->header.src_addr,  ns->option.data  );
  
  payload_length = IPV6_NS_HEADER_LENGTH + IPV6_OPTION_HEADER_LENGTH + ETH_ADDR_LENGTH;
  /* send back our own */
  memcpy( ns->option.data , eth_config_get_address(), ETH_ADDR_LENGTH );
  ns->option.type = ICMP_OPTION_TYPE_TARGET_LINK_LAYER_ADDRESS;

  ns->flags[0] = 0x60; /* Not a router, solicited reply, override 0110 0000 */
  ns->flags[1] = 0x00;
  ns->flags[2] = 0x00;
  ns->flags[3] = 0x00;
  
  icmp_response(pkt,&ns->header,ICMPV6_TYPE_NA,payload_length);
  
  return payload_length;
}

int icmpv6_dispatch( struct ip_packet *ip_packet ){
  struct icmpv6_header *icmp = (struct icmpv6_header *)(ip_packet->payload);
  switch( icmp->type ){
    case ICMPV6_TYPE_NS:
      /*neighbour solicitation request*/
      return icmp_neighbour_solicitation( ip_packet );
      break;
    case ICMPV6_TYPE_PING_REQUEST:
      /*ping*/
      return icmp_ping( ip_packet );
      break;
    default:
      /* can't handle this type. drop it */
      return 0;
  }
}