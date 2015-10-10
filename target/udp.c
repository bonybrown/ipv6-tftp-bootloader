#include "udp.h"


struct udp_bind_entry{
  uint16_t port;
  udp_callback *func;
};

static struct udp_bind_entry udp_bind_list[UDP_MAX_NUMBER_OF_BINDS];

int udp_bind( uint16_t port, udp_callback * callback_func ){
  int i;
  /* check port not already bound */
  for(i =0 ; i < UDP_MAX_NUMBER_OF_BINDS; i++ ){
    if( port == udp_bind_list[i].port ){
      return UDP_ERROR_PORT_ALREADY_BOUND;
    }
  }
  /* find a free bind and assign */
  for(i =0 ; i < UDP_MAX_NUMBER_OF_BINDS; i++ ){
    if( 0 == udp_bind_list[i].port ){
      udp_bind_list[i].port = port;
      udp_bind_list[i].func = callback_func;
      return 0;
    }
  }
  return UDP_ERROR_NO_BINDS_AVAILABLE;
}  

int udp_unbind( uint16_t port ){
  int i;
  /* find bind */
  for(i =0 ; i < UDP_MAX_NUMBER_OF_BINDS; i++ ){
    if( port == udp_bind_list[i].port ){
      udp_bind_list[i].port = 0;
      udp_bind_list[i].func = 0;
      return 0;
    }
  }
  return UDP_ERROR_PORT_NOT_BOUND;
}
/*
 * Process the ip ip_packet
 * Updates the ip_packet with data to be sent
 * Returns the size of the ip payload to be sent
 */
int udp_dispatch( struct ip_packet *ip ){
  struct udp_packet *udp = (struct udp_packet*)ip->payload;
  uint16_t target_port = ntohs( udp->header.dest_port );
  int i;
  for( i = 0; i < UDP_MAX_NUMBER_OF_BINDS; i++ ){
    if( target_port == udp_bind_list[i].port ){
      return udp_bind_list[i].func( ip, udp );
    }
  }
  /* nothing bound to this port, drop packet */
  return 0;
}
