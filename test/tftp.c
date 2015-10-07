#include <np.h>     /* NovaProva library */
#include <stdio.h>
#include "../target/tftp.h" /* declares the Code Under Test */


#define TEST_DATA_SIZE 512
#define TEST_DATA_SIZE_SHORT 42

#define BLOCK_NUMBER_OFFSET_LOW 11
#define BLOCK_NUMBER_OFFSET_HIGH 10  
#define ERROR_CODE_OFFSET_LOW 11
#define ERROR_CODE_OFFSET_HIGH 10  
#define OPERATION_OFFSET_LOW 9
#define OPERATION_OFFSET_HIGH 8
#define UDP_LEGNTH_OFFSET_LOW 5
#define UDP_LEGNTH_OFFSET_HIGH 4

static const uint16_t client_source_port = 60062; /* ea, 9e */

struct file{
  size_t position;
};

static struct file the_file;




static uint16_t read_position = 0;
static uint8_t read_data[TEST_DATA_SIZE+TEST_DATA_SIZE+TEST_DATA_SIZE_SHORT] = {
   [0 ... (TEST_DATA_SIZE-1)] = 0, 
   [TEST_DATA_SIZE ... TEST_DATA_SIZE+TEST_DATA_SIZE-1] = 1, 
   [TEST_DATA_SIZE+TEST_DATA_SIZE ... TEST_DATA_SIZE+TEST_DATA_SIZE+TEST_DATA_SIZE_SHORT-1] = 2 
};

void file_seek( struct file *f, size_t position){
  f->position = position;
}

size_t file_read(struct file *f, uint8_t * buffer , size_t buffer_size){
  read_position = f->position;
  size_t i = 0;
  while( i < TEST_DATA_SIZE && f->position < sizeof(read_data) && i < buffer_size ){
    buffer[i] = read_data[f->position];
    i++;
    f->position++;
  }
  return i;
}


static void check_read_data( uint8_t value,  uint8_t * buffer , uint16_t buffer_size ){
  size_t i = 0;
  for( i = 0 ; i < buffer_size ; i++ ){
    NP_ASSERT_EQUAL( value, buffer[i] );
  }
}

static uint16_t write_position = 0;
static uint16_t bytes_written = 0;
size_t file_write(struct file *f, uint8_t * buffer , uint16_t buffer_size){
  write_position = f->position;
  size_t i = 0;
  for( i = 0 ; i < buffer_size ; i++ ){
    NP_ASSERT_EQUAL( f->position >> 9, buffer[i] );
    bytes_written++;
  }
  f->position += buffer_size;
  return buffer_size;
}

static char open_filename[255];
static char open_mode = '0';
struct file * file_open( const char *filename, const char mode){
  open_mode = mode;
  strcpy( open_filename, filename );
  if( strcmp("not_found", filename ) == 0 ){
    return NULL;
  }
  the_file.position = 0;
  return &the_file;
}

static int file_close_call_count = 0;
void file_close(struct file *f){
  NP_ASSERT_EQUAL( &the_file, f );
  file_close_call_count++;
}

void dump_block( uint8_t *block, size_t length ){
  for(;length > 0  ; length-- ){
    printf("%02x ", *block );
    block++;
  }
  puts("");
}

static int setup(void){
  bytes_written = 0;
  file_close_call_count = 0;
  the_file.position = 0;
  return 0;
}


static void test_tftp_file_not_found(void){
  
  struct ip_packet pkt;
  struct udp_packet *udp = &(pkt.payload);
  uint8_t *udp_bytes = (uint8_t*)udp;

  const uint8_t not_found_rrq[] = "\xea\x9e\x00\x45\x00\x00\xff\xff\x00\001not_found\x00octet\x00";
  uint16_t length = sizeof(not_found_rrq);
  
  memcpy( udp, not_found_rrq, sizeof(not_found_rrq) );
  
  udp_bytes[UDP_LEGNTH_OFFSET_HIGH] = length >> 8;
  udp_bytes[UDP_LEGNTH_OFFSET_LOW] = length & 0xff;
  
  dump_block(udp, sizeof(not_found_rrq) );
  
  int result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);

  NP_ASSERT_EQUAL( client_source_port ,ntohs(udp->header.dest_port) );
  /* should result in error ( code 5 ) */
  NP_ASSERT_EQUAL( 5, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] ); 
  NP_ASSERT_EQUAL( 1, udp_bytes[ERROR_CODE_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[ERROR_CODE_OFFSET_HIGH] ); 
  NP_ASSERT_STR_EQUAL( "file not found", udp_bytes + ERROR_CODE_OFFSET_LOW + 1 );
  
}

static void test_tftp_only_octet_supported(void){
  
  struct ip_packet pkt;
  struct udp_packet *udp = &(pkt.payload);
  uint8_t *udp_bytes = (uint8_t*)udp;

  const uint8_t netascii_rrq[] = "\xea\x9e\x00\x45\x00\x00\xff\xff\x00\001file\x00netascii\x00";
  uint16_t length = sizeof(netascii_rrq);
  
  memcpy( udp, netascii_rrq, sizeof(netascii_rrq) );
  
  udp_bytes[UDP_LEGNTH_OFFSET_HIGH] = length >> 8;
  udp_bytes[UDP_LEGNTH_OFFSET_LOW] = length & 0xff;
  
  dump_block(udp, sizeof(netascii_rrq) );
  
  int result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);

  NP_ASSERT_EQUAL( client_source_port ,ntohs(udp->header.dest_port) );
  /* should result in error ( code 5 ) */
  NP_ASSERT_EQUAL( 5, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] ); 
  NP_ASSERT_EQUAL( 1, udp_bytes[ERROR_CODE_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[ERROR_CODE_OFFSET_HIGH] ); 
  NP_ASSERT_STR_EQUAL( "transfer mode not supported", udp_bytes + ERROR_CODE_OFFSET_LOW + 1 );
  
}


static void test_tftp_rrq(void)
{
  
  
  struct ip_packet pkt;
  struct udp_packet *udp = &(pkt.payload);
  uint8_t *tftp_data = udp->payload + 4;
  uint8_t *udp_bytes = (uint8_t*)udp;
  uint16_t tftp_source_port;
  const uint16_t client_source_port = 2049;
  udp->header.src_port = htons(client_source_port);
  udp->header.dest_port = htons(69);
  udp->header.length = 13;
  /* RRQ for file: "boot", mode: "octet" */
  memcpy( udp->payload, "\x00\001boot\x00octet\x00", 13);
  dump_block(udp->payload, 13);
    
  int result = tftp_packet_handler( &pkt, udp );
 
  tftp_source_port = udp->header.src_port;
  dump_block(udp, result);
  
  NP_ASSERT_EQUAL(TEST_DATA_SIZE + UDP_HEADER_LENGTH + 4 /* opcode + block number, uint16_t each */, result );
  NP_ASSERT_EQUAL( client_source_port, ntohs( udp->header.dest_port) );
  NP_ASSERT_EQUAL( 'r' /* RRQ*/, open_mode);
  NP_ASSERT_STR_EQUAL( "boot",  open_filename );
  NP_ASSERT_EQUAL( 0 , read_position );
  NP_ASSERT_EQUAL( 3, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 1, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );

  check_read_data( 0, tftp_data, TEST_DATA_SIZE);
  
                         /* ACK */
  memcpy( udp->payload, "\x00\004\x00\001", 4);
  udp->header.length = 4;
  udp->header.src_port = htons(client_source_port);
  udp->header.dest_port = tftp_source_port;
  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  NP_ASSERT_EQUAL(TEST_DATA_SIZE + UDP_HEADER_LENGTH + 4 /* opcode + block number, uint16_t each */, result );
  NP_ASSERT_EQUAL( TEST_DATA_SIZE , read_position );
  NP_ASSERT_EQUAL( tftp_source_port , udp->header.src_port ); /*same src port*/
  NP_ASSERT_EQUAL( client_source_port, ntohs( udp->header.dest_port) );
  NP_ASSERT_EQUAL( 0, file_close_call_count );
  NP_ASSERT_EQUAL( 3, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 2, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  check_read_data( 1, tftp_data, TEST_DATA_SIZE);

                           /* ACK block 2*/
  memcpy( udp->payload, "\x00\004\x00\002", 4);
  udp->header.length = 4;
  udp->header.src_port = htons(client_source_port);
  udp->header.dest_port = tftp_source_port;

  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  /* should result in data block 3 */
  NP_ASSERT_EQUAL(TEST_DATA_SIZE_SHORT + UDP_HEADER_LENGTH + 4 /* opcode + block number, uint16_t each */, result );
  NP_ASSERT_EQUAL( TEST_DATA_SIZE * 2  , read_position );
  NP_ASSERT_EQUAL( 0, file_close_call_count );  
  NP_ASSERT_EQUAL( tftp_source_port , udp->header.src_port ); /*same src port*/
  NP_ASSERT_EQUAL( client_source_port, ntohs( udp->header.dest_port) );
  NP_ASSERT_EQUAL( 3, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 3, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  check_read_data( 2, tftp_data, TEST_DATA_SIZE_SHORT);

  /* test re-sending  ACK for block 2*/
  memcpy( udp->payload, "\x00\004\x00\002", 4);
  udp->header.length = 4;
  udp->header.src_port = htons(client_source_port);
  udp->header.dest_port = tftp_source_port;

  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  /* should result in data block 2 again */
  NP_ASSERT_EQUAL(TEST_DATA_SIZE_SHORT + UDP_HEADER_LENGTH + 4 /* opcode + block number, uint16_t each */, result );
  NP_ASSERT_EQUAL( TEST_DATA_SIZE * 2  , read_position );
  NP_ASSERT_EQUAL( 0, file_close_call_count );  
  NP_ASSERT_EQUAL( tftp_source_port , udp->header.src_port ); /*same src port*/
  NP_ASSERT_EQUAL( client_source_port, ntohs( udp->header.dest_port) );
  NP_ASSERT_EQUAL( 3, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 3, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  check_read_data( 2, tftp_data, TEST_DATA_SIZE_SHORT);

                             /* ACK block 3 */
  memcpy( udp->payload, "\x00\004\x00\003", 4);
  udp->header.length = 4;
  udp->header.src_port = htons(client_source_port);
  udp->header.dest_port = tftp_source_port;

  result = tftp_packet_handler( &pkt, udp );
  /* should result in no packet and file closed */
  NP_ASSERT_EQUAL( 0, result );
  NP_ASSERT_EQUAL( 1, file_close_call_count );

}

static void test_tftp_wrq(void){
  const uint16_t server_port = 38338; /* 95, c2 */
  /* wrq for test.out, octet */
  const uint8_t client_wrq[] = { 0xea, 0x9e, 0x00, 0x45, 0x00, 0x18, 0xfe, 0x2b, 0x00, 0x02, 0x6f, 0x75,
  0x74, 0x2e, 0x74, 0x78, 0x74, 0x00, 0x6f, 0x63, 0x74, 0x65, 0x74, 0x00 };
  const uint8_t server_ack0[] = { 0x95, 0xc2, 0xea, 0x9e, 0x00, 0x0c, 0xfe, 0x1f, 0x00, 0x04, 0x00, 0x00};
  
  const uint8_t client_write_data_header[] = {  0xea, 0x9e, 0x95, 0xc2, 0x02, 0x0c, 0x00, 0x20, 0x00, 0x03, 0x00, 0x01};

  
  
  tftp_set_local_port( server_port - 1 );
  
  
  struct ip_packet pkt;
  struct udp_packet *udp = &(pkt.payload);
  uint8_t *udp_bytes = (uint8_t*)udp;
  
  memcpy( udp, client_wrq, sizeof(client_wrq) );
  
  int result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  NP_ASSERT_STR_EQUAL( "out.txt",  open_filename );
  NP_ASSERT_EQUAL( 'w' /* WRQ*/, open_mode);
  NP_ASSERT_EQUAL( result, sizeof(server_ack0) );
  NP_ASSERT_EQUAL( 4, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  NP_ASSERT_EQUAL( 0, file_close_call_count );
  
  
  memcpy( udp, client_write_data_header, sizeof(client_write_data_header) );
  memset( udp_bytes + sizeof(client_write_data_header), 0, 512 );
  dump_block(udp, sizeof(client_write_data_header) + 512);
  
  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  NP_ASSERT_EQUAL( 512, bytes_written );
  NP_ASSERT_EQUAL( result, sizeof(server_ack0) );
  NP_ASSERT_EQUAL( 4, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 1, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  NP_ASSERT_EQUAL( 0, file_close_call_count );

  /* Test resend of block 1. Should result in ACK for block 1, but no write */
  memcpy( udp, client_write_data_header, sizeof(client_write_data_header) );
  memset( udp_bytes + sizeof(client_write_data_header), 0, 512 );
  dump_block(udp, sizeof(client_write_data_header) + 512);
  
  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  NP_ASSERT_EQUAL( 512, bytes_written );
  NP_ASSERT_EQUAL( result, sizeof(server_ack0) );
  NP_ASSERT_EQUAL( 4, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 1, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  NP_ASSERT_EQUAL( 0, file_close_call_count );

  /* Test send of block 2. Should result in ACK for block 2, and writes */
  memcpy( udp, client_write_data_header, sizeof(client_write_data_header) );
  memset( udp_bytes + sizeof(client_write_data_header), 1, 512 );
  udp_bytes[BLOCK_NUMBER_OFFSET_LOW] = 2;
  dump_block(udp, sizeof(client_write_data_header) + 512);
  
  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  NP_ASSERT_EQUAL( 512 + 512, bytes_written );
  NP_ASSERT_EQUAL( result, sizeof(server_ack0) );
  NP_ASSERT_EQUAL( 4, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 2, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  NP_ASSERT_EQUAL( 0, file_close_call_count );
  
  /* Test send of short block 3. Should result in ACK for block 3, writes and close file */
  memcpy( udp, client_write_data_header, sizeof(client_write_data_header) );
  memset( udp_bytes + sizeof(client_write_data_header), 2, 96 );
  udp_bytes[BLOCK_NUMBER_OFFSET_LOW] = 3;
  uint16_t length = sizeof(client_write_data_header) + 96;
  udp_bytes[UDP_LEGNTH_OFFSET_HIGH] = length >> 8;
  udp_bytes[UDP_LEGNTH_OFFSET_LOW] = length & 0xff;
  dump_block(udp, sizeof(client_write_data_header) + 96);
  
  result = tftp_packet_handler( &pkt, udp );
  dump_block(udp, result);
  
  NP_ASSERT_EQUAL( 512 + 512 + 96, bytes_written );
  NP_ASSERT_EQUAL( result, sizeof(server_ack0) );
  NP_ASSERT_EQUAL( 4, udp_bytes[OPERATION_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[OPERATION_OFFSET_HIGH] );  
  NP_ASSERT_EQUAL( 3, udp_bytes[BLOCK_NUMBER_OFFSET_LOW] );
  NP_ASSERT_EQUAL( 0, udp_bytes[BLOCK_NUMBER_OFFSET_HIGH] );
  NP_ASSERT_EQUAL( 1, file_close_call_count );
}

  