
#ifndef __DP_I2C_H__
#define __DP_I2C_H__

#include <stdint.h>

/* Initialise I2C hardware */
void i2c_init( void );

/* Send size bytes of data to address
   Return number of bytes sent  */
uint8_t i2c_send( uint8_t  address , uint8_t *data, uint8_t size );

/* Send size bytes of data to sub_address on the device at address.
   Useful for devices that follow the "first byte after address is sub-address" convention.
   Returns the number of bytes sent */
uint8_t i2c_subaddress_write( uint8_t  address, uint8_t sub_address , uint8_t *data, uint8_t size );

/* Read size bytes from the device at address into data.
   Returns the number of bytes read. */
uint8_t i2c_read( uint8_t  address , uint8_t *data, uint8_t size );


/* Read size bytes starting from sub_address from device at address into data.
   Device must conform to the "write subaddress, repeated start, read" convention.
   Returns the number of bytes actually read */
uint8_t i2c_subaddress_read( uint8_t  address, uint8_t sub_address , uint8_t *data, uint8_t size );


#endif

