#ifndef __UDP_H__
#define __UDP_H__

#include "net.h"

#define UDP_MAX_NUMBER_OF_BINDS 	4

struct udp_header{
  uint16_t src_port;
  uint16_t dest_port;
  uint16_t length;
  uint16_t checksum;
}  __attribute__((__packed__));


#define UDP_HEADER_LENGTH 	(sizeof( struct udp_header))

struct udp_packet{
  struct udp_header header;
  uint8_t payload[IPV6_PAYLOAD_MAX_LENGTH - UDP_HEADER_LENGTH];
} __attribute__((__packed__));

#define UDP_PAYLOAD_MAX_LENGTH	(IPV6_PAYLOAD_MAX_LENGTH - UDP_HEADER_LENGTH)

/*
 * typedef of callback function to be bound to a port
 */
typedef int (udp_callback)(struct ip_packet *, struct udp_packet *);


/*
 * Bind a callback function to a udp port
 * Returns 0 on success, other numbers on error
 * The function is called for each udp packet received on the bound port
 * The function returns the number of udp payload bytes in the response
 */
int udp_bind( uint16_t port, udp_callback *callback_func );
#define UDP_ERROR_PORT_ALREADY_BOUND  -1
#define UDP_ERROR_NO_BINDS_AVAILABLE  -2

/*
 * Unbind a bound port.
 * Returns 0 on success, UDP_ERROR_PORT_NOT_BOUND
 * if the specified port is not bound
 */
int udp_unbind( uint16_t port );
#define UDP_ERROR_PORT_NOT_BOUND -3
/*
 * Process the ip ip_packet
 * Updates the ip_packet with data to be sent
 * Returns the size of the ip payload to be sent
 */
int udp_dispatch( struct ip_packet *ip);


#endif