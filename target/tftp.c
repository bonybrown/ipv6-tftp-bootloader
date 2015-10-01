#include "tftp.h"
#include <string.h>

enum tftp_session_state{
  Idle = 0,		/* no session */
  ReadRequested,	/* get request parsed */
  WriteRequested,	/* put request parsed */
  AwaitingReadAck,	/* contents being sent */
  AwaitingLastReadAck, /* last block sent, waiting for ack */
  AwaitingWriteAck	/* contents being received */
};


struct tftp_session{
  enum tftp_session_state state;
  uint16_t fpos; /* file position*/
  uint16_t remote_port; /* remote end socket number */
  uint16_t local_port; /* local end socket number */
  int block_id; /* id of last block sent/awaiting ack or received/ack sent for*/
};

/* thie one, only session in this implementation */
static struct tftp_session session = {Idle,0,0,0,0};
static uint16_t local_port = 1024;

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



static tftp_open_callback* open_callback = NULL;
static tftp_read_callback* read_callback = NULL;
static tftp_write_callback* write_callback = NULL;
static tftp_close_callback* close_callback = NULL;

void tftp_set_open_callback( tftp_open_callback open_func ){
  open_callback = open_func;
}
void tftp_set_read_callback( tftp_read_callback read_func ){
  read_callback = read_func;
}

int tftp_send_data( struct ip_packet * ip, struct udp_packet * udp );
int tftp_send_error(uint16_t error_code, char * error_message, struct ip_packet * ip, struct udp_packet * udp);
int tftp_response(uint16_t length, struct ip_packet * ip, struct udp_packet * udp);
void tftp_tid_inc();


int tftp_send_data( struct ip_packet * ip, struct udp_packet * udp ){
  struct tftp_header * tftp = (struct tftp_header *)udp->payload;
  uint8_t *data = &tftp->data_byte;
  session.fpos = (session.block_id-1) << 9; /* block_id * 512 */
  int to_send = read_callback( session.fpos, data, TFTP_BLOCK_SIZE );
  if( to_send < 0 ){
    return tftp_send_error(TFTP_ERROR_ACCESS_VIOLATION, "cannot read file", ip,udp);
  }
  tftp->op  = htons( TFTP_OP_DATA );
  tftp->block_id = htons(session.block_id);
  session.state = AwaitingReadAck;
  return tftp_response( to_send + 4, ip, udp );
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
  checksum_summate(&(udp->header.checksum), udp, length );
  ipv6_prepare( &ip->header, ip->header.src_addr, IPV6_NEXT_HEADER_UDP, length );
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
  session.fpos = 0;
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
  int return_size;
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
        if(! open_callback(op, filename_ptr ) ){
          return tftp_send_error(TFTP_ERROR_FILE_NOT_FOUND,"file not found",ip,udp);
        }
        session.state = op == TFTP_OP_RRQ ? ReadRequested : WriteRequested;
        session.remote_port = ntohs(udp->header.src_port);
        session.fpos = 0;
        session.block_id = 1;
        session.local_port = local_port;
        tftp_tid_inc();
        return tftp_send_data( ip,udp );
      }
      return tftp_send_error(TFTP_ERROR_ILLEGAL_OP,"bad op",ip,udp);
      break;
    case ReadRequested: /* get request parsed */
      break;
    case WriteRequested: /* put request parsed */
      break;
    case AwaitingReadAck:  /* contents being sent */
    case AwaitingLastReadAck:
      if( op != TFTP_OP_ACK){
        return tftp_send_error(TFTP_ERROR_ILLEGAL_OP,"bad op",ip,udp);
      }
      block = ntohs( tftp->block_id );
      if( block == session.block_id ){
        session.block_id++;
        if( session.state == AwaitingLastReadAck ){
          session.state = Idle;
          /* done */
          return 0;
        }
      }
      return_size = tftp_send_data( ip,udp );
      if( return_size < TFTP_BLOCK_SIZE ){
        session.state = AwaitingLastReadAck;
      }
      return return_size;
      break;
    case AwaitingWriteAck:  /* contents being received */
      break;
  }
  return 0;
}

