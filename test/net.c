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

static void test_ipv6_send()
{
  uint8_t other_ip_addr[IPV6_ADDR_LENGTH]   = { OTHER_IP };
  uint8_t other_phy[ETH_ADDR_LENGTH]   = {0xde,0xad,0xbe,0xef,0xca,0xfe};
  eth_config_set_address( my_addr );
  ipv6_config_set_address( my_ip_addr );
  ipv6_physical_add_entry( other_ip_addr, other_phy );
  struct eth_packet packet;
  memset( &packet, 0xff, sizeof( packet ) );
  int result = ipv6_send( &packet, other_ip_addr, IPV6_NEXT_HEADER_UDP, 0x1fe);
  NP_ASSERT_EQUAL( 1, result );
  const uint8_t expected[IPV6_HEADER_LENGTH+ETH_HEADER_LENGTH] = {
    0xde,0xad,0xbe,0xef,0xca,0xfe, //dest phy
    0xe0,0xde,0xf1,0x78,0x6a,0xc6,  //src phy
    0x86, 0xdd, //type
    0x60, 0x00, 0x00, 0x00, //version/flow-control
    0x01, 0xfe, //payload_length
    IPV6_NEXT_HEADER_UDP, //next header
    IPV6_DEFAULT_HOP_LIMIT, //hop limit 
    MY_IP , //src ip
    OTHER_IP //dest ip
  };
  NP_ASSERT_EQUAL(0, memcmp( expected, &packet, sizeof( expected )) );
    
}
  