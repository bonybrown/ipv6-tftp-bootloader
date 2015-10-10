#include <np.h>     /* NovaProva library */
#include "../target/udp.h" /* declares the Code Under Test */

#define MY_IP    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x1e, 0xc0, 0xff, 0xfe, 0x81, 0xf9, 0x1b
#define OTHER_IP 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xde, 0xf1, 0xff, 0xfe, 0x77, 0x6a, 0xc6
#define MY_ETH 0x00, 0x1e, 0xc0, 0x81, 0xf9, 0x1b
#define OTHER_ETH 0xf0, 0xde, 0xf1, 0x77, 0x6a, 0xc6
const static uint8_t ip_tftp_req[] =  {
  0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x11, 0x40, 
  OTHER_IP, 
  MY_IP };
const static uint8_t udp_tftp_req[] = {
  0xa9, 0x02, 0x00, 0x45, 0x00, 0x18, 0xea, 0x0c,
  0x00, 0x01, /* opcode 1 = read req*/ 
  0x66, 0x69, 0x6c, 0x65, 0x00, /* file */
  0x6e, 0x65, 0x74, 0x61, 0x73, 0x63, 0x69, 0x69, 0x00}; /* netascii */


static int tftp_handler( struct ip_packet *ip , struct udp_packet *udp ){
  NP_ASSERT_EQUAL( 0x00, udp->payload[0]  );
  NP_ASSERT_EQUAL( 0x01, udp->payload[1]  );
  return 42;
}
  
static void test_udp_bind(void)
{
  int result = udp_bind( 69, &tftp_handler );
  NP_ASSERT_EQUAL( 0 , result ); /* success */
  result = udp_bind( 69, &tftp_handler );
  NP_ASSERT_EQUAL( UDP_ERROR_PORT_ALREADY_BOUND , result ); /* same port */
  int i = 0;
  for( i = 0; i < UDP_MAX_NUMBER_OF_BINDS -1 ; i++ ){
    result = udp_bind( 70 + i , &tftp_handler );
    NP_ASSERT_EQUAL( 0 , result ); /* success */
  }
  result = udp_bind( 8000, &tftp_handler );
  NP_ASSERT_EQUAL( UDP_ERROR_NO_BINDS_AVAILABLE , result ); /* same port */
}


static void test_udp_unbind(void)
{
  int result = udp_bind( 69, &tftp_handler );
  NP_ASSERT_EQUAL( 0 , result ); /* success */
  result = udp_unbind( 6969 );
  NP_ASSERT_EQUAL( UDP_ERROR_PORT_NOT_BOUND , result ); /* not bound port */
  result = udp_unbind( 69 );
  NP_ASSERT_EQUAL( 0 , result ); /* unbind success */
}



static void test_udp_dispatch(void)
{
  struct ip_packet pkt;
  uint8_t me[IPV6_ADDR_LENGTH] = { MY_IP };

  ipv6_config_set_address( me ) ;
  
  udp_bind( 69, &tftp_handler );
  
  memcpy( &pkt.header, ip_tftp_req, sizeof( ip_tftp_req ) );
  memcpy( &pkt.payload, udp_tftp_req, sizeof(udp_tftp_req) );

  int result = udp_dispatch( &pkt );
  NP_ASSERT_EQUAL(42, result );
}

static void test_udp_dispatch_no_match(void)
{
  struct ip_packet pkt;
  uint8_t me[IPV6_ADDR_LENGTH] = { MY_IP };

  ipv6_config_set_address( me ) ;
  
  udp_bind( 70, &tftp_handler );
  
  memcpy( &pkt.header, ip_tftp_req, sizeof( ip_tftp_req ) );
  memcpy( &pkt.payload, udp_tftp_req, sizeof(udp_tftp_req) );

  int result = udp_dispatch( &pkt );
  NP_ASSERT_EQUAL(0, result );
}
  