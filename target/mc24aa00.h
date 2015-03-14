#ifndef __MC24AA00_H__
#define __MC24AA00_H__

#include <stdint.h>
#include <stdbool.h>

#define MAC_ADDRESS_SIZE  6
#define MC24AA00_ADDRESS 0xA0

/* initialise the 24AA00 chip at address*/
void mc24aa00_init(uint8_t address);

/* write a block to the device at address from data
   Returns the number of bytes written */
uint8_t mc24aa00_write( uint8_t address, uint8_t sub_address, uint8_t *data, uint8_t size);

/* write all zeros to the user-settable block */
uint8_t mc24aa00_erase( uint8_t address);

/* read a block from the device at address into data
   Returns the number of bytes read */
uint8_t mc24aa00_read( uint8_t address, uint8_t sub_address, uint8_t *data, uint8_t size);


/* Read the MAC address stored in the device at address into data.
   size must be at least MAC_ADDRESS_SIZE.
   returns true on success */
bool mc24aa00_read_mac_address( uint8_t address, uint8_t *data, uint8_t size);


#endif
