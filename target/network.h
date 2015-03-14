/* maximum size of an ethernet frame, including crc and vlan tag */

#include <stdint.h>
#define ETH_ALEN        6               /* Octets in one ethernet addr   */
#define ETH_HLEN        14              /* Total octets in header.       */
#define MAX_ETHERNET_PACKET_SIZE	1522

#define IPV6_ADDR_LENGTH	16  

#define IPPROTO_ICMPV6  58
#define IPPROTO_UDP	17
#define IP_PACKET_OFFSET (ETH_ALEN + ETH_ALEN + 2)
#define AS_IP(packet) ((struct ipv6_packet*)( packet + IP_PACKET_OFFSET ))
struct ipv6_packet{
  uint8_t version; /* top 4 bits are the IP version */
  uint8_t class_flow[3];
#define IP_VERSION(ip)	(((ip)->version >> 4) )
  uint16_t payload_length;
  uint8_t next_header;
  uint8_t hop_limit;
  uint8_t src_addr[IPV6_ADDR_LENGTH];
  uint8_t dest_addr[IPV6_ADDR_LENGTH];
} __attribute__((__packed__));

#define ICMPV6_TYPE_NS			135
#define ICMPV6_TYPE_NA 			136
#define ICMPV6_TYPE_PING_REQUEST	128
#define ICMPV6_TYPE_PING_RESPONSE	129
#define ICMP_PACKET_OFFSET (IP_PACKET_OFFSET + sizeof(struct ipv6_packet))
#define AS_ICMP(packet) ((struct icmpv6_packet*)( packet + ICMP_PACKET_OFFSET ))
struct icmpv6_packet{
  uint8_t type;
  uint8_t code;
  uint8_t checksum[2];
  uint8_t flags[4];
}  __attribute__((__packed__));


#define AS_ICMP_NS(packet) ((struct icmpv6_ns_packet*)( packet + ICMP_PACKET_OFFSET ))
struct icmpv6_option{
  uint8_t type;
  uint8_t length;
  uint8_t data;
}  __attribute__((__packed__));

#define ICMP_OPTION_TYPE_SOURCE_LINK_LAYER_ADDRESS 	0x01
#define ICMP_OPTION_TYPE_TARGET_LINK_LAYER_ADDRESS 	0x02


struct icmpv6_ns_packet{
  uint8_t type;
  uint8_t code;
  uint8_t checksum[2];
  uint8_t flags[4];
  uint8_t target_addr[16];
  struct icmpv6_option option;
}  __attribute__((__packed__));


#define AS_ICMP_PING(packet) ((struct icmpv6_ping_packet*)( packet + ICMP_PACKET_OFFSET ))
struct icmpv6_ping_packet{
  uint8_t type;
  uint8_t code;
  uint8_t checksum[2];
  uint16_t identifier;
  uint16_t sequence;
  uint8_t data[];
}  __attribute__((__packed__));


enum eth_dest_type{
  eth_dest_type_error = -1,
  eth_dest_type_broadcast  = 0,
  eth_dest_type_ipv6_multicast,
  eth_dest_type_mine,
  eth_dest_type_other
};


enum eth_dest_type ethernet_identify_destination( int packet_size, uint8_t *packet, uint8_t *my_mac_address , uint8_t *my_eui_64 );


enum packet_type{
  packet_type_error = -1,
  packet_type_neighbour_solicitation  = 0,
  packet_type_tftp,
  packet_type_ping_request,
  packet_type_other
};

enum packet_type get_packet_type(  uint8_t *packet );

int is_solicitation_for_me(  uint8_t *packet, uint8_t *my_eui_64 );
int build_solicitation_response( int packet_size, uint8_t *packet ,  uint8_t *my_mac_address, uint8_t *my_eui_64 );

int build_ping_response( int packet_size, uint8_t *packet ,  uint8_t *my_mac_address, uint8_t *my_eui_64 );

/* add words from addr ( count in bytes ) into checksum.
 * This is done in host order.
 * checksum should be zeroed on first, call then passed back in repeatedly.
 * Use checksum_set to copy in to network buffer and set correct network order.
 */
void checksum_summate ( uint16_t *checksum, void * addr, int count);

/* set a checksum buffer to the checksum value, adjusting byte order if required.
 * checksum needs to be negated before passing in
 */
void checksum_set( uint8_t checksum_buffer[2], uint16_t checksum );

/* Calculate the icmpv6 pseudo header for the src, dest, packet length and next header
 * Returns the host-order checksum for passing into checksum_summate
 */
uint16_t icmpv6_pseduo_header_checksum( void *src_addr, void *dest_addr, uint32_t upper_layer_packet_length, uint8_t next_header_value );

uint32_t htonl(  uint32_t host_value );