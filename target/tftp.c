#include "tftp.h"
#include <string.h>

/* operations on "files" to be performed elsewhere by other code */
extern void * file_open( const char *filename, const char mode );
extern void file_seek( void *file, uint32_t position );
extern size_t file_read( void *file, uint8_t *buffer, size_t buffer_size );
extern size_t file_write( void *file, uint8_t *buffer, size_t buffer_size );
extern void file_close( void *file );


extern size_t strnlen(const char *s, size_t maxlen);

enum tftp_session_state{
  Idle = 0,		/* no session */
  WriteRequested,	/* waiting for data */
  AwaitingReadAck,	/* contents being sent, waiting for ack */
  AwaitingLastReadAck, /* last block sent, waiting for ack */
};


struct tftp_session{
  enum tftp_session_state state;
  uint16_t remote_port; /* remote end socket number */
  uint16_t local_port; /* local end socket number */
  int block_id; /* id of last block sent/awaiting ack or received/ack sent for*/
  void *file; /* file open for read/write */
};

/* the one, only session in this implementation */
static struct tftp_session session = {Idle,0,0,0,NULL};
static uint16_t local_port = 50000;

const char *tftp_allowed_transfer_mode = "octet";
#define TFTP_OCTET_STRING_SIZE 6

#define TFTP_BLOCK_SIZE 512

#define TFTP_OP_RRQ 	1
#define TFTP_OP_WRQ	2
#define TFTP_OP_DATA	3
#define TFTP_OP_ACK	4
#define TFTP_OP_ERROR	5

#define TFTP_ERROR_NOT_DEFINED		0
#define TFTP_ERROR_FILE_NOT_FOUND	1
#define TFTP_ERROR_ACCESS_VIOLATION	2
#define TFTP_ERROR_DISK_FULL	3
#define TFTP_ERROR_ILLEGAL_OP	4
#define TFTP_ERROR_UNKNOWN_ID	5
#define TFTP_ERROR_FILE_EXISTS	6
#define TFTP_ERROR_NO_SUCH_USER	7


struct tftp_header{
  uint16_t op;
  union{
    uint16_t block_id;
    uint16_t error_code;
    char filename_char;
  };
  union{
    uint8_t data_byte;
    char error_message_char;
  };
} __attribute__((__packed__));



void tftp_set_local_port( uint16_t port_number ){
  local_port = port_number;
}

int tftp_send_ack( struct ip_packet * ip, struct udp_packet * udp, uint16_t block_number );
int tftp_send_data( struct ip_packet * ip, struct udp_packet * udp );
int tftp_send_error(uint16_t error_code, char * error_message, struct ip_packet * ip, struct udp_packet * udp);
int tftp_response(uint16_t length, struct ip_packet * ip, struct udp_packet * udp);
void tftp_tid_inc();

int tftp_write_data( struct ip_packet * ip, struct udp_packet * udp ){
  struct tftp_header * tftp = (struct tftp_header *)udp->payload;
  uint8_t *data = &tftp->data_byte;
  uint16_t length = ntohs( udp->header.length ) - UDP_HEADER_LENGTH - 4;
  int written = file_write( session.file, data, length );
  if( written == length ){
    written = tftp_send_ack( ip, udp, session.block_id );
    session.block_id++;
    if( length != TFTP_BLOCK_SIZE ){
      /* short block is last block. close file  */
      file_close(session.file);
      tftp_session_reset();
    }
    return written;
  }
  return tftp_send_error(TFTP_ERROR_ACCESS_VIOLATION, "cannot write file", ip,udp);
}


int tftp_send_data( struct ip_packet * ip, struct udp_packet * udp ){
  struct tftp_header * tftp = (struct tftp_header *)udp->payload;
  uint8_t *data = &tftp->data_byte;
  uint32_t seek_pos = session.block_id-1;
  seek_pos <<= 9;/* block_id * 512 */
  file_seek( session.file, seek_pos); 
  size_t to_send = file_read( session.file, data, TFTP_BLOCK_SIZE );
  tftp->op  = htons( TFTP_OP_DATA );
  tftp->block_id = htons(session.block_id);
  session.state = (to_send < TFTP_BLOCK_SIZE) ? AwaitingLastReadAck  : AwaitingReadAck;
  return tftp_response( to_send + 4, ip, udp );
}

int tftp_send_ack( struct ip_packet * ip, struct udp_packet * udp, uint16_t block_number ){
  struct tftp_header * tftp = (struct tftp_header *)udp->payload;
  tftp->op  = htons( TFTP_OP_ACK );
  tftp->block_id = htons(block_number);
  return tftp_response( 4, ip, udp );
}


int tftp_send_error(uint16_t error_code, char * error_message, struct ip_packet * ip, struct udp_packet * udp){
  tftp_session_reset();
  struct tftp_header * tftp = (struct tftp_header *)udp->payload;
  uint16_t length = 4 + strlen(error_message) + 1;
  tftp->op = htons( TFTP_OP_ERROR );
  tftp->error_code = htons( error_code );
  strcpy( &tftp->error_message_char, error_message );
  return tftp_response( length, ip,  udp);
}

int tftp_response(uint16_t length, struct ip_packet * ip, struct udp_packet * udp){
  length += UDP_HEADER_LENGTH;
  udp->header.length = htons(length);
  udp->header.dest_port = udp->header.src_port;
  udp->header.src_port = htons( local_port );
  udp->header.checksum = 0;
  ipv6_prepare( &ip->header, ip->header.src_addr, IPV6_NEXT_HEADER_UDP, length, IPV6_DEFAULT_HOP_LIMIT );

  uint16_t checksum = 0;
  checksum = ipv6_pseduo_header_checksum(
    ip->header.src_addr,
    ip->header.dest_addr,
    length,
    IPV6_NEXT_HEADER_UDP);
  checksum_summate(&checksum, udp, length );
  udp->header.checksum = ~checksum;
  return length;
}

void tftp_tid_inc(){
  local_port++;
  if( local_port < 1024 ){
    local_port = 1024;
  }
}

void tftp_session_reset(){
  session.state = Idle;
  udp_unbind( session.local_port );
  session.remote_port = 0;
  session.local_port = 0;
  session.block_id = 0;
}
  

/*
 * The tftp function that is bound to a udp socket (usually, port 69 )
 */
int tftp_packet_handler(struct ip_packet * ip, struct udp_packet * udp){
  struct tftp_header * tftp = (struct tftp_header *)udp->payload;
  uint16_t op = ntohs(tftp->op);
  uint16_t block;
    
  switch( session.state ){
    case Idle:
      if( op == TFTP_OP_RRQ || op ==  TFTP_OP_WRQ){
        char *filename_ptr = &tftp->filename_char;
        int filename_size = strnlen(filename_ptr, udp->header.length );
        if( filename_size > udp->header.length - TFTP_OCTET_STRING_SIZE ){
          return tftp_send_error(TFTP_ERROR_FILE_NOT_FOUND,"cannot determine filename",ip,udp);
        }
        if( strcmp( tftp_allowed_transfer_mode, filename_ptr+filename_size+1 ) != 0 ){
          return tftp_send_error(TFTP_ERROR_FILE_NOT_FOUND,"transfer mode not supported",ip,udp);
        }
        session.file = file_open( filename_ptr, op ==  TFTP_OP_WRQ ? 'w':'r' );
        if( NULL == session.file ){
          return tftp_send_error(TFTP_ERROR_FILE_NOT_FOUND,"file not found",ip,udp);
        }
        tftp_tid_inc();
        session.remote_port = ntohs(udp->header.src_port);
        session.block_id = 1;
        session.local_port = local_port;
        udp_bind( local_port, tftp_packet_handler );
        if( op == TFTP_OP_RRQ ){
          return tftp_send_data( ip,udp );
        }else{
          session.state = WriteRequested;
          return tftp_send_ack( ip,udp, 0 );
        }
      }
      return tftp_send_error(TFTP_ERROR_ILLEGAL_OP,"bad op",ip,udp);
      break;
     case WriteRequested: /* waiting for data */
      if( op != TFTP_OP_DATA ){
        return tftp_send_error(TFTP_ERROR_ILLEGAL_OP,"bad op",ip,udp);
      }
      block = ntohs( tftp->block_id );
      if( block == session.block_id ){
        return tftp_write_data( ip, udp );
      }
      return tftp_send_ack(ip, udp, session.block_id -1 );
      break;
    case AwaitingReadAck:  /* file being sent */
    case AwaitingLastReadAck:
      if( op != TFTP_OP_ACK){
        return tftp_send_error(TFTP_ERROR_ILLEGAL_OP,"bad op",ip,udp);
      }
      block = ntohs( tftp->block_id );
      if( block == session.block_id ){
        session.block_id++;
        if( session.state == AwaitingLastReadAck ){
          file_close(session.file);
          tftp_session_reset();
          /* done */
          return 0;
        }
      }
      return tftp_send_data( ip,udp );
      
      break;
  }
  return 0;
}

