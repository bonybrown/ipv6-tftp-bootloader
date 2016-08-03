#include "mc24aa00.h"
#include "i2c.h"
#include <xc.h>

#define MAC_ADDRESS_SUB_ADDRESS 0xFA

/* initialise the 24AA00 chip at address*/
void mc24aa00_init(uint8_t address){
  //power it on. powered by RB13 output
  TRISBbits.TRISB13 = 0; //set output
  LATBbits.LATB13 = 1; //set high
}

/* write a block to the device at address from data
   Returns the number of bytes written */
uint8_t mc24aa00_write( uint8_t address, uint8_t sub_address, uint8_t *data, uint8_t size){
  return i2c_subaddress_write( address, sub_address, data, size );
}

/* write all zeros to the user-settable block */
uint8_t mc24aa00_erase( uint8_t address){
  uint8_t page = 0;
  uint8_t data[8] = {0,0,0,0,0,0,0,0};
  uint8_t written = 0;
  for( page =0 ; page < 16 ; page++ ){
     written += i2c_subaddress_write( address, page * 8, data, 8 );
  }
  return written;
}


/* read a block from the device at address into data
   Returns the number of bytes read */
uint8_t mc24aa00_read( uint8_t address,uint8_t sub_address, uint8_t *data, uint8_t size){
  return i2c_subaddress_read( address, sub_address, data, size );
}

/* Read the MAC address stored in the device at address into data.
   size must be at least MAC_ADDRESS_SIZE.
   returns true on success */
bool mc24aa00_read_mac_address( uint8_t address, uint8_t *data, uint8_t size){
  uint8_t length;
  if( size  < MAC_ADDRESS_SIZE){
    return false;
  }
  length = i2c_subaddress_read( address, MAC_ADDRESS_SUB_ADDRESS, data, MAC_ADDRESS_SIZE );
  return length == MAC_ADDRESS_SIZE;
}


