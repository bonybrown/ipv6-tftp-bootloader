#include <np.h>     /* NovaProva library */
#include "../target/net.h" /* declares the Code Under Test */

#define MY_IP    0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0xde,0xf1,0x78,0x6a,0xc6
#define OTHER_IP 0xfe,0x80,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e

static uint8_t my_addr[ETH_ADDR_LENGTH]   = {0xe0,0xde,0xf1,0x78,0x6a,0xc6};
static uint8_t multicast[ETH_ADDR_LENGTH] = {0x33,0x33,0x00,0x00,0x00,0xfb};
static uint8_t my_ip_addr[IPV6_ADDR_LENGTH]   =  { MY_IP };
    
static void test_eth_is_multicast(void)
{
    struct eth_header header;
    memcpy(header.dest_addr,  multicast, ETH_ADDR_LENGTH );
    int result = eth_is_multicast( &header );
    NP_ASSERT_EQUAL(1, result);
}

static void test_eth_is_unicast(void)
{
    struct eth_header header;
    memcpy(header.dest_addr,  my_addr, ETH_ADDR_LENGTH );
    eth_config_set_address( my_addr );
    int result = eth_is_unicast( &header );
    NP_ASSERT_EQUAL(1, result);
}

static void test_eth_is_ipv6(void)
{
    struct eth_header header;
    header.type = htons( ETH_TYPE_IPV6 );
    int result = eth_is_ipv6( &header );
    NP_ASSERT_EQUAL(1, result);
} 

static void test_eth_address(void)
{
    struct eth_header header;    
    eth_config_set_address( my_addr );
    eth_address( &header, multicast, ETH_TYPE_IPV6 );
    
    NP_ASSERT_EQUAL(ETH_TYPE_IPV6, ntohs(header.type) );
    NP_ASSERT_EQUAL(0, memcmp( header.src_addr, my_addr, ETH_ADDR_LENGTH ));
    NP_ASSERT_EQUAL(0, memcmp( header.dest_addr, multicast, ETH_ADDR_LENGTH ));
}

static void test_eth_config_get_address(void)
{
    struct eth_header header;    
    eth_config_set_address( my_addr );
    
    uint8_t *config_address = eth_config_get_address();
    
    NP_ASSERT(config_address != NULL );
    NP_ASSERT_EQUAL(0, memcmp( config_address, my_addr, ETH_ADDR_LENGTH ));
}

static void test_ipv6_is_for_this_address()
{
  struct ipv6_header header;
  memcpy( header.dest_addr, my_ip_addr, IPV6_ADDR_LENGTH );
  ipv6_config_set_address( my_ip_addr );
  NP_ASSERT_EQUAL(1, ipv6_is_for_this_address( &header ) );
}

static void test_ipv6_payload_length()
{
  struct ipv6_header header;
  header.payload_length = htons(0xcafe);
  NP_ASSERT_EQUAL(0xcafe, ipv6_payload_length( &header ) );
}

static void test_ipv6_physical_map()
{
  ipv6_physical_add_entry( my_ip_addr, my_addr );
  uint8_t *dest_addr = ipv6_physical_address_of( my_ip_addr );
  NP_ASSERT( dest_addr != NULL );
  NP_ASSERT_EQUAL(0, memcmp( dest_addr, my_addr, ETH_ADDR_LENGTH ) );
  my_ip_addr[0] = 0x00;
  dest_addr = ipv6_physical_address_of( my_ip_addr );
  NP_ASSERT( dest_addr == NULL );
  my_addr[0] = 0x00;
  ipv6_physical_add_entry( my_ip_addr, my_addr );
  dest_addr = ipv6_physical_address_of( my_ip_addr );
  NP_ASSERT( dest_addr != NULL );
  NP_ASSERT_EQUAL(0, memcmp( dest_addr, my_addr, ETH_ADDR_LENGTH ) );
  uint8_t other_ip_addr[IPV6_ADDR_LENGTH]   = { OTHER_IP };
  uint8_t other_phy[ETH_ADDR_LENGTH]   = {0xde,0xad,0xbe,0xef,0xca,0xfe};
  ipv6_physical_add_entry( other_ip_addr, other_phy );
  dest_addr = ipv6_physical_address_of( other_ip_addr );
  NP_ASSERT( dest_addr != NULL );
  NP_ASSERT_EQUAL(0, memcmp( dest_addr, other_phy, ETH_ADDR_LENGTH ) );
  dest_addr = ipv6_physical_address_of( my_ip_addr );
  NP_ASSERT( dest_addr != NULL );
  NP_ASSERT_EQUAL(0, memcmp( dest_addr, my_addr, ETH_ADDR_LENGTH ) );
  ipv6_physical_add_entry( other_ip_addr, other_phy );
  
}

static void test_ipv6_prepare()
{
  uint8_t other_ip_addr[IPV6_ADDR_LENGTH]   = { OTHER_IP };
  ipv6_config_set_address( my_ip_addr );

  struct ipv6_header header;
  memset( &header, 0xff, sizeof( header ) );
  ipv6_prepare( &header, other_ip_addr, IPV6_NEXT_HEADER_UDP, 0x1fe,IPV6_DEFAULT_HOP_LIMIT);
  
  const uint8_t expected[IPV6_HEADER_LENGTH] = {
    0x60, 0x00, 0x00, 0x00, //version/flow-control
    0x01, 0xfe, //payload_length
    IPV6_NEXT_HEADER_UDP, //next header
    IPV6_DEFAULT_HOP_LIMIT, //hop limit 
    MY_IP , //src ip
    OTHER_IP //dest ip
  };
  NP_ASSERT_EQUAL(0, memcmp( expected, &header, sizeof( expected )) );
    
}

static void test_ipv6_pseduo_header_checksum(void)
{
  uint8_t src_addr[16]= { 0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x02,0x1e,0xc0,0xff,0xfe,0x81,0xf9,0x1b};
  uint8_t dest_addr[16]={ 0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
			  0xf2,0xde,0xf1,0xff,0xfe,0x77,0x6a,0xc6};
  uint32_t upper_layer_packet_length = 32;
  uint8_t next_header_value = 58;
  uint16_t result =ipv6_pseduo_header_checksum( src_addr, dest_addr, upper_layer_packet_length, next_header_value ); 									
  NP_ASSERT_EQUAL(result, 0x3506);
}

static void test_udp_checksum(void)
{
  uint8_t src_addr[16]= { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0xde, 0xf1, 0xff,
  0xfe, 0x77, 0x6a, 0xc6 };
  uint8_t dest_addr[16]={ 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x1e, 0xc0, 0xff,
  0xfe, 0x81, 0xf9, 0x1b };
  uint32_t upper_layer_packet_length = 25;
  uint8_t next_header_value = 17;
  
  uint8_t udp_data[] = {  
    0xa1, 0x25, 0x00, 0x45, 0x00, 0x19,  0  , 0   , /*src prt, dest, len, checksum */
    0x00, 0x01, 0x62, 0x6f, 0x6f, 0x74, 0x2e, 0x64, 0x61, 0x74, 0x00, /* rrq, boot.dat */
    0x6f, 0x63, 0x74, 0x65, 0x74, 0x00 /* octet */
  };
  uint16_t result =ipv6_pseduo_header_checksum( src_addr, dest_addr, upper_layer_packet_length, next_header_value );  
  checksum_summate( &result, udp_data, sizeof( udp_data ) );
  result = ~result;
  NP_ASSERT_EQUAL( 0x622d, result);
}

static void test_udp_checksum2(void)
{
  uint8_t src_addr[16]={ 
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x1e, 0xc0, 0xff,  0xfe, 0x81, 0xf9, 0x1b };
  uint8_t dest_addr[16]= { 
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xf2, 0xde, 0xf1, 0xff, 0xfe, 0x77, 0x6a, 0xc6 };
  uint32_t upper_layer_packet_length = 12;
  uint8_t next_header_value = 17;
  
  uint8_t udp_data[] = {  
    0xc3, 0x51, 0xe3, 0x0e, 0x00, 0x0c, 0  , 0 ,/*src prt, dest, len, checksum */
    0x00, 0x04, 0x00, 0x00 /* ack, block 0 */
  };
  uint16_t result =ipv6_pseduo_header_checksum( src_addr, dest_addr, upper_layer_packet_length, next_header_value );  
  checksum_summate( &result, udp_data, sizeof( udp_data ) );
  result = ~result;
  NP_ASSERT_EQUAL( 0x9753, result);
}

static void test_htonl(void)
{
    uint32_t input = 0xcafebabe;
    uint32_t result = htonl( input );
    uint8_t *r = (uint8_t *)&result;
    NP_ASSERT_EQUAL(r[0], 0xca);
    NP_ASSERT_EQUAL(r[1], 0xfe);
    NP_ASSERT_EQUAL(r[2], 0xba);
    NP_ASSERT_EQUAL(r[3], 0xbe);

}

static void test_htons(void)
{
    uint16_t input = 0xcafe;
    uint16_t result = htons( input );
    uint8_t *r = (uint8_t *)&result;
    NP_ASSERT_EQUAL(r[0], 0xca);
    NP_ASSERT_EQUAL(r[1], 0xfe);

}

static void test_checksum_summate_zero(void)
{
    uint16_t checksum = 0;
    uint8_t data[4] = {0,0,0,0};
    int count = 4;
    checksum_summate ( &checksum, data, count);
    NP_ASSERT_EQUAL(checksum, 0);
}
static void test_checksum_summate_1(void)
{
    uint16_t checksum = 0;
    uint8_t data[2] = {0x00,0x01};
    int count = 2;
    checksum_summate ( &checksum, data, count);
    NP_ASSERT_EQUAL(checksum, 0x0100);
}
static void test_checksum_summate_2(void)
{
    uint16_t checksum = 0;
    uint8_t data[4] = {0x00,0x01,0x07,0x02};
    int count = 4;
    checksum_summate ( &checksum, data, count);
    NP_ASSERT_EQUAL(checksum, 0x0307);
}
static void test_checksum_summate_rfc(void)
{
    uint16_t checksum = 0;
    uint8_t data[8] = {0x00,0x01,0xf2,0x03,0xf4,0xf5,0xf6,0xf7};
    int count = 8;
    checksum_summate ( &checksum, data, count);
    NP_ASSERT_EQUAL(checksum, 0xf2dd);
}

  