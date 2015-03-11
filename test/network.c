#include <np.h>     /* NovaProva library */
#include "../target/network.h" /* declares the Code Under Test */

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

static void test_checksum_set(void)
{
    uint16_t checksum = 0xdead;
    uint8_t data[2] = {0x00,0x00};
    checksum_set( data, checksum);
    NP_ASSERT_EQUAL(data[0], 0xde);
    NP_ASSERT_EQUAL(data[1], 0xad);    
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

static void test_icmpv6_pseduo_header_checksum(void)
{
  uint8_t src_addr[16]= { 0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
			  0x02,0x1e,0xc0,0xff,0xfe,0x81,0xf9,0x1b};
  uint8_t dest_addr[16]={ 0xfe,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
			  0xf2,0xde,0xf1,0xff,0xfe,0x77,0x6a,0xc6};
  uint32_t upper_layer_packet_length = 32;
  uint8_t next_header_value = 58;
  uint16_t result =icmpv6_pseduo_header_checksum( src_addr, dest_addr, upper_layer_packet_length, next_header_value ); 									
  NP_ASSERT_EQUAL(result, 0x3506);
}

