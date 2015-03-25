#ifndef __ICMPV6_H__
#define __ICMPV6_H__

#include "net.h"


struct icmpv6_header{
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
}  __attribute__((__packed__));

#define ICMPV6_TYPE_NS			135
#define ICMPV6_TYPE_NA 			136
#define ICMPV6_TYPE_PING_REQUEST	128
#define ICMPV6_TYPE_PING_RESPONSE	129

struct icmpv6_option{
  uint8_t type;
  uint8_t length;
  uint8_t data[];
}  __attribute__((__packed__));

#define IPV6_OPTION_HEADER_LENGTH 	(2)


#define ICMP_OPTION_TYPE_SOURCE_LINK_LAYER_ADDRESS 	0x01
#define ICMP_OPTION_TYPE_TARGET_LINK_LAYER_ADDRESS 	0x02


struct icmpv6_ns_packet{
  struct icmpv6_header header;
  uint8_t flags[4];
  uint8_t target_addr[IPV6_ADDR_LENGTH];
  struct icmpv6_option option;
}  __attribute__((__packed__));

#define IPV6_NS_HEADER_LENGTH 	(sizeof( struct icmpv6_ns_packet) - sizeof( struct icmpv6_option))


struct icmpv6_ping_packet{
  struct icmpv6_header header;
  uint16_t identifier;
  uint16_t sequence;
  uint8_t data[];
}  __attribute__((__packed__));


/*
 * Process the ip ip_packet
 * Updates the ip_packet with data to be sent
 * Returns the size of the ip payload to be sent
 */
int icmpv6_dispatch( struct ip_packet *ip_packet );

#endif