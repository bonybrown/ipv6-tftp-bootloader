#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>

#define offsetof(type, member)  __builtin_offsetof (type, member)


#define PACKET_MATCH(TYPE,MEMBER,VALUE) internal_match( sizeof( ((TYPE *)0)->MEMBER), offsetof(TYPE,MEMBER), VALUE )


void packet_set( void *packet, uint16_t length );

int internal_match( uint16_t member_size, uint16_t member_offset, void* value );



#endif