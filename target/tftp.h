#ifndef __TFTP_H__
#define __TFTP_H__

#include "udp.h"

/*
 * The tftp function that is bound to a udp socket (usually, port 69 )
 */
int tftp_packet_handler(struct ip_packet * ip, struct udp_packet * udp);

void tftp_session_reset();

void tftp_set_local_port( uint16_t port_number );

#endif