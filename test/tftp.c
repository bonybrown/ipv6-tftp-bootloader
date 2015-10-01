#include <np.h>     /* NovaProva library */
#include <stdio.h>
#include "../target/tftp.h" /* declares the Code Under Test */


#define TEST_DATA_SIZE 512

static uint16_t read_position = 0;
int read_func(uint16_t file_position, uint8_t * buffer , uint16_t buffer_size){
  read_position = file_position;
  memset(buffer,file_position >> 9,TEST_DATA_SIZE);
  if( file_position > 512 ){
    return 8;
  }
  return TEST_DATA_SIZE;
}

static char open_filename[255];
static uint16_t open_operation = 0;
int open_func(uint16_t operation, char *filename){
  open_operation = operation;
  strcpy( open_filename, filename );
  return 1;
}

void dump_block( uint8_t *block, size_t length ){
  for(;length > 0  ; length-- ){
    printf("%02x ", *block );
    block++;
  }
  puts("");
}

static void test_tftp_rrq(void)
{
  
  
  struct ip_packet pkt;
  struct udp_packet *udp = &(pkt.payload);
  int i;
  
  udp->header.src_port = htons(1024);
  udp->header.dest_port = htons(69);
  udp->header.length = 13;
  /* RRQ for file: "boot", mode: "octet" */
  memcpy( udp->payload, "\x00\001boot\x00octet\x00", 13);
  dump_block(udp->payload, 13);
  
  tftp_set_open_callback( open_func);
  tftp_set_read_callback( read_func);
  
  int result = tftp_packet_handler( &pkt, udp );
 
  result -= UDP_HEADER_LENGTH;
  dump_block(udp->payload, result);
  
  NP_ASSERT_EQUAL( 1 /* RRQ*/, open_operation);
  NP_ASSERT_EQUAL( 0, strcmp( "boot",  open_filename ));
  NP_ASSERT_EQUAL( 0 , read_position );
  NP_ASSERT_EQUAL(TEST_DATA_SIZE + 4, result );
  
                         /* ACK */
  memcpy( udp->payload, "\x00\004\x00\001", 4);
  udp->header.length = 4;
  result = tftp_packet_handler( &pkt, udp );
  
  NP_ASSERT_EQUAL( TEST_DATA_SIZE , read_position );
  result -= UDP_HEADER_LENGTH;
  dump_block(udp->payload, result);
  
  NP_ASSERT_EQUAL(TEST_DATA_SIZE + 4, result );

                           /* ACK */
  memcpy( udp->payload, "\x00\004\x00\002", 4);
  udp->header.length = 4;
  result = tftp_packet_handler( &pkt, udp );
  
  NP_ASSERT_EQUAL( TEST_DATA_SIZE * 2  , read_position );
  result -= UDP_HEADER_LENGTH;
  dump_block(udp->payload, result);
  
  NP_ASSERT_EQUAL(8 + 4, result );

}
  