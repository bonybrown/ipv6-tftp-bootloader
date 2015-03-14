#include "i2c.h"

#include <p33Fxxxx.h>
#include <stdio.h>
#include <stdbool.h>

static void start();
static void repeated_start();
static void send_byte(uint8_t);
static uint8_t read_byte(bool nack);
static uint8_t send_block(uint8_t *data, uint8_t size);
static uint8_t read_block(uint8_t *data, uint8_t size);
static void stop();


void i2c_init( void ){
  I2C1CONbits.A10M=0;
  //I2C1CONbits.SCLREL=1;
  I2C1BRG=310;

  I2C1ADD=0;
  I2C1MSK=0;

  I2C1CONbits.I2CEN=1;
  
}

#define I2C_READ_ADDRESS( x )   ( x  | 0x01 )
#define I2C_WRITE_ADDRESS( x )  ( x  & 0xFE )



void start(){
#ifdef I2C_DEBUG 
  putchar('[');
#endif
  I2C1CONbits.SEN=1;
  while( I2C1CONbits.SEN );
}

void repeated_start(){
#ifdef I2C_DEBUG 
  putchar('@');
#endif
  I2C1CONbits.RSEN=1;
  while( I2C1CONbits.RSEN );
}

void send_byte( uint8_t byte ){
#ifdef I2C_DEBUG 
  printf("%02x ", byte);
#endif
  I2C1TRN = byte; 
  while(  I2C1STATbits.TBF ||  I2C1STATbits.TRSTAT );
}

uint8_t read_byte(bool nack){
  uint8_t dummy;
#ifdef I2C_DEBUG 
  putchar('r');
#endif
  dummy = I2C1RCV;
  //while( (I2C1CON & 0b11111) != 0 ); //wait for lower 5 bits of status to be 0
  I2C1CONbits.RCEN=1; //enter recieve state
  while( I2C1CONbits.RCEN ); //wait for a byte
  I2C1CONbits.ACKDT = nack; //send an ACK
  I2C1CONbits.ACKEN = 1; //start ACK seq
  while( I2C1CONbits.ACKEN );
  dummy =  I2C1RCV; //return it*/
#ifdef I2C_DEBUG 
  printf("%02x ", dummy);
#endif
  return dummy;
}

uint8_t send_block(uint8_t *data, uint8_t size){
  uint8_t i;
  uint8_t count;
  count = 0;
  for( i = 0 ; i < size; i++ )
  {
    send_byte( *data ); 
    data++;
    count++;
  }
  return count;
}

uint8_t read_block(uint8_t *data, uint8_t size){
  uint8_t i;
  uint8_t count;
  count = 0;
  for( i = 0 ; i < size; i++ )
  {
    *data = read_byte(count == (size-1)); 
    data++;
    count++;
  }
  return count;
}

void stop(){
#ifdef I2C_DEBUG 
  putchar(']');
  putchar('\n');
#endif
  I2C1CONbits.PEN=1;
  while( I2C1CONbits.PEN );
}

uint8_t i2c_send(uint8_t  address, uint8_t *data, uint8_t size ){
  uint8_t count;
  start();
  send_byte( I2C_WRITE_ADDRESS(address) ); //this is a write
  count = send_block( data, size );
  stop();
  return count;
} 

uint8_t i2c_subaddress_write(uint8_t  address, uint8_t sub_address, uint8_t *data, uint8_t size ){
  uint8_t count;
  start();
  send_byte( I2C_WRITE_ADDRESS(address) ); //this is a write
  send_byte( sub_address ); //write sub-address
  count = send_block( data, size );
  stop();
  return count;
}

uint8_t i2c_read( uint8_t  address , uint8_t *data, uint8_t size ){
  uint8_t count;
  start();
  send_byte( I2C_READ_ADDRESS(address) ); //this is a read
  count = read_block( data, size );
  stop();
  return count;
}

uint8_t i2c_subaddress_read( uint8_t  address, uint8_t sub_address , uint8_t *data, uint8_t size ){
  uint8_t count;
  start();
  send_byte( I2C_WRITE_ADDRESS(address) ); //this is a write
  send_byte( sub_address );
  repeated_start(); // repeated start
  send_byte( I2C_READ_ADDRESS(address) ); //this time, perform a read
  count = read_block( data, size );
  stop();
  return count;
}
