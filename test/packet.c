#include <np.h>     /* NovaProva library */
#include "../target/packet.h" /* declares the Code Under Test */
#include "../target/network.h"


struct t{
  uint16_t size16;
  uint32_t size32;
  uint8_t size8;
  uint8_t address[6];
} __attribute__((__packed__));

static struct t test;


static void setup()
{
  test.size16 = htons(0x6667);
  test.size8 = 0x42;
  test.size32 = htonl(0xdeadbeef);
  test.address[0] = 0x01;
  test.address[1] = 0x10;
  test.address[2] = 0x20;
  test.address[3] = 0x30;
  test.address[4] = 0x40;
  test.address[5] = 0x50;
  packet_set( &test, sizeof(test) );
}

static void test_internal_match_byte(void)
{
  setup();
  uint8_t expected = 0x42;
  int result = PACKET_MATCH(struct t, size8 , &expected );
  
  NP_ASSERT_EQUAL(1, result);
}

static void test_internal_match_byte_fail(void)
{
  setup();
  uint8_t expected = 0x43;
  int result = PACKET_MATCH(struct t, size8 , &expected );
  
  NP_ASSERT_EQUAL(0, result);
}


static void test_internal_match_16(void)
{
  setup();
  uint16_t expected = 0x6667;
  int result = PACKET_MATCH(struct t, size16 , &expected );
  
  NP_ASSERT_EQUAL(1, result);
}

static void test_internal_match_32(void)
{
  setup();
  uint32_t expected = 0xdeadbeef;
  int result = PACKET_MATCH(struct t, size32 , &expected );
  
  NP_ASSERT_EQUAL(1, result);
}

static void test_internal_match_string(void)
{
  setup();
  uint8_t expected[6] ={0x01,0x10,0x20,0x30,0x40,0x50};
  int result = PACKET_MATCH(struct t, address , expected );
  
  NP_ASSERT_EQUAL(1, result);
}