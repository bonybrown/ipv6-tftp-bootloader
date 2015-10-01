#ifndef __TFTP_H__
#define __TFTP_H__

#include "udp.h"



/* typedef of function to call on a read or write request.
 * Return non-zero if transfer should proceed
 */   
typedef int (tftp_open_callback)(uint16_t operation, char *filename );

/*
 * typedef of function to call to read data. 
 * It puts the data in the buffer (up to buffer_size bytes)
 * Return number of bytes to send, or negative on error. 
 */
typedef int (tftp_read_callback)(uint16_t file_position, uint8_t * buffer , uint16_t buffer_size);

/*
 * typedef of function to call to transfer data. 
 * It reads the buffer into the file
 * Return number of bytes to send, or negative on error. 
 */
typedef int (tftp_write_callback)(uint16_t file_position, uint8_t * buffer , uint16_t buffer_size);

/*
 * typedef of function to call when transfer is complete
 */
typedef void (tftp_close_callback)(void);


/*
 * The tftp function that is bound to a udp socket (usually, port 69 )
 */
int tftp_packet_handler(struct ip_packet * ip, struct udp_packet * udp);

void tftp_session_reset();

void tftp_set_read_callback( tftp_read_callback read_func );
void tftp_set_open_callback( tftp_open_callback open_func );

#endif