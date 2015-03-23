#ifndef __NET_H__

#include <stdint.h>

/*
 * 
 * Ethernet definitions
 * 
 */


/* maximum size of an ethernet frame, including crc and vlan tag */
#define ETH_MAX_PACKET_SIZE	1522

#define ETH_ADDR_LENGTH        6               /* Octets in one ethernet addr   */

struct eth_header{
  uint8_t dest_addr[ETH_ADDR_LENGTH];
  uint8_t src_addr[ETH_ADDR_LENGTH];
  uint16_t type;
} __attribute__((__packed__));

#define ETH_TYPE_IPV6		0x86dd
#define ETH_HEADER_LENGTH 	(sizeof( struct eth_header ))

/*
 * Configure the ethernet layer to use this_addr for this nodes target address
 */
void eth_config_set_address( uint8_t this_addr[ETH_ADDR_LENGTH] );

int eth_is_multicast(struct eth_header *header );
int eth_is_unicast(struct eth_header *header );
int eth_is_ipv6(struct eth_header *header );

/*
 * Address this header to the destination, and set the type.
 * The source address will be set to the address set in eth_config_set_address
 */
void eth_address(struct eth_header *header, uint8_t dest_addr[ETH_ADDR_LENGTH], uint16_t type );



/*
 * 
 * IP V6 definitions
 * 
 */

#define IPV6_ADDR_LENGTH	16  

struct ipv6_header{
  uint8_t version; /* top 4 bits are the IP version */
  uint8_t class_flow[3];
  uint16_t payload_length;
  uint8_t next_header;
  uint8_t hop_limit;
  uint8_t src_addr[IPV6_ADDR_LENGTH];
  uint8_t dest_addr[IPV6_ADDR_LENGTH];
} __attribute__ (( __packed__ )) ;


#define IPV6_HEADER_LENGTH 	(sizeof( struct ipv6_header ) )

#define IPV6_HEADER_VERSION(ip)	(((ip)->version >> 4) )

#define IPV6_NEXT_HEADER_ICMPV6  	58
#define IPV6_NEXT_HEADER_UDP		17

#define IPV6_FROM_ETH(eth_ptr)		((ipv6_header *) (eth_ptr + ETH_HEADER_LENGTH ))

#define IPV6_DEFAULT_HOP_LIMIT		64

struct eth_packet{
  struct eth_header eth;
  struct ipv6_header ip;
  uint8_t ip_payload[ETH_MAX_PACKET_SIZE - ETH_HEADER_LENGTH - IPV6_HEADER_LENGTH];
} __attribute__((__packed__));

#define IPV6_PAYLOAD_MAX_LENGTH 	(sizeof( ((eth_packet*)0)->ip_payload ) )
#define IP_PAYLOAD_FROM_IP(ip_ptr)	((uint8_t *) (ip_ptr + IPV6_HEADER_LENGTH ))


struct ip_packet{
  struct ipv6_header ip;
  uint8_t ip_payload[ETH_MAX_PACKET_SIZE - ETH_HEADER_LENGTH - IPV6_HEADER_LENGTH];
} __attribute__((__packed__));

#define IP_PACKET_FROM_IP(ip_ptr)	((ip_packet *) (ip_ptr ))
#define IP_PACKET_FROM_ETH(eth_ptr)	((ip_packet *) (eth_ptr + ETH_HEADER_LENGTH )
/*
 * Configure the ip layer to use this ip address for this node
 */
void ipv6_config_set_address( uint8_t this_addr[IPV6_ADDR_LENGTH] );


int ipv6_is_for_this_address(struct ipv6_header *header );
uint16_t ipv6_payload_length(struct ipv6_header *header );

/*
 * Send the ip packet pointed to by packet to ip_addr
 * Lookup physical address of destination from the ip_addr
 * Returns 1 on success, 0 on failure
 * Will not attempt neighbour soliciation to resolve ip_addr to eth_addr 
 */
int ipv6_send( struct eth_packet *packet, uint8_t ip_addr[IPV6_ADDR_LENGTH], uint8_t next_header,  uint16_t payload_length );


/*
 * Routing / Neighbour solicitation / mapping ip addresses to ethernet ones
 */
void ipv6_physical_add_entry( uint8_t dest_ip_addr[IPV6_ADDR_LENGTH], uint8_t dest_physical_addr[ETH_ADDR_LENGTH] );
uint8_t * ipv6_physical_address_of( uint8_t dest_ip_addr[IPV6_ADDR_LENGTH] );


/*
 * eth = packet
 * int response_length = 0;
 * ip_packet *pkt = IP_PACKET_FROM_ETH( eth )
 * switch( pkt.ip -> next_header ){
 *  case IPV6_NEXT_HEADER_ICMPV6:
 *    response_length = icmpv6_dispatch( pkt );
 *    break;
 *  case IPV6_NEXT_HEADER_UDP:
 *    response_length = udp_dispatch( pkt );
 *    break;
 * }
 * if( response_length > 0 ){
 *   //send a packet of the same type to the src address
 *   ipv6_send( eth_packet, pkt->ip.src_addr, pkt->ip.next_header, response_length );
 * }
*/


/* add words from addr ( count in bytes ) into checksum.
 * This is done in host order.
 * checksum should be zeroed on first, call then passed back in repeatedly.
 * Use checksum_set to copy in to network buffer and set correct network order.
 */
void checksum_summate ( uint16_t *checksum, void * addr, int count);
uint32_t htonl( uint32_t host_value );
uint16_t htons( uint16_t host_value );
uint16_t ntohs( uint16_t network_value );

#endif