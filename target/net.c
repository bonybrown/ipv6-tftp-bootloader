#include "net.h"
#include <string.h>


static uint8_t this_config_addr[ETH_ADDR_LENGTH];
static uint8_t this_config_ip_addr[IPV6_ADDR_LENGTH];

struct physical_map_entry{
uint8_t dest_ip_addr[IPV6_ADDR_LENGTH];
uint8_t dest_physical_addr[ETH_ADDR_LENGTH];
};

#define PHYSICAL_MAP_ENTRIES	2
static struct physical_map_entry this_physical_map[PHYSICAL_MAP_ENTRIES];
static int physical_map_current = 0;

void eth_config_set_address( uint8_t this_addr[ETH_ADDR_LENGTH] ){
  memcpy( this_config_addr, this_addr, ETH_ADDR_LENGTH);
}

const uint8_t * eth_config_get_address(){
  return this_config_addr;
}

int eth_is_multicast(struct eth_header *header ){
  //destination is multicast if lsb of first byte is 1
  return header->dest_addr && 0x01;
}
int eth_is_unicast(struct eth_header *header ){
  //unicast if dest addr matches our configured addr
  return memcmp( this_config_addr, header->dest_addr, ETH_ADDR_LENGTH ) == 0;
}
int eth_is_ipv6(struct eth_header *header ){
  return ntohs(header->type) == ETH_TYPE_IPV6;
}

void eth_address(struct eth_header *header, uint8_t dest_addr[ETH_ADDR_LENGTH], uint16_t type) {
  header->type  = htons( type );
  memcpy( header->dest_addr, dest_addr, ETH_ADDR_LENGTH );
  memcpy( header->src_addr, this_config_addr, ETH_ADDR_LENGTH );
}

void ipv6_config_set_address( uint8_t this_addr[IPV6_ADDR_LENGTH] ){
  memcpy( this_config_ip_addr, this_addr, IPV6_ADDR_LENGTH );
}


int ipv6_is_for_this_address(struct ipv6_header *header ){
  return memcmp( header -> dest_addr, this_config_ip_addr, IPV6_ADDR_LENGTH) == 0;
}

uint16_t ipv6_payload_length(struct ipv6_header *header ){
  return htons(header->payload_length);
}

void ipv6_physical_add_entry( uint8_t dest_ip_addr[IPV6_ADDR_LENGTH], uint8_t dest_physical_addr[ETH_ADDR_LENGTH] ){
  struct physical_map_entry *entry = &(this_physical_map[physical_map_current]);
  memcpy( entry->dest_ip_addr, dest_ip_addr, IPV6_ADDR_LENGTH);
  memcpy( entry->dest_physical_addr, dest_physical_addr, ETH_ADDR_LENGTH);
  physical_map_current++;
  if( physical_map_current >= PHYSICAL_MAP_ENTRIES ){
    physical_map_current = 0;
  }
}
    
uint8_t * ipv6_physical_address_of( uint8_t dest_ip_addr[IPV6_ADDR_LENGTH] ){
  int i= 0;
  for(i=0; i < PHYSICAL_MAP_ENTRIES; i++ ){
    if( memcmp( dest_ip_addr, this_physical_map[i].dest_ip_addr , IPV6_ADDR_LENGTH) == 0){
      return this_physical_map[i].dest_physical_addr;
    }
  }
  return (uint8_t*)NULL;
}


void ipv6_prepare( struct ipv6_header *header, uint8_t dest_ip_addr[IPV6_ADDR_LENGTH], uint8_t next_header,  uint16_t payload_length ){
  /* clear everything EXCEPT the addresses */
  memset(header, 0 , IPV6_HEADER_LENGTH - 2 * IPV6_ADDR_LENGTH );
  /* set ip header fields */
  header->version = 6 << 4;
  header->payload_length = htons( payload_length );
  header->next_header = next_header;
  header->hop_limit = IPV6_DEFAULT_HOP_LIMIT;
  /* copy dest first - it's probably being copied from the src field of the same header */
  memcpy( header->dest_addr, dest_ip_addr, IPV6_ADDR_LENGTH );
  memcpy( header->src_addr, this_config_ip_addr, IPV6_ADDR_LENGTH );
}

inline uint32_t htonl(  uint32_t host_value){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32_t result = 0;
  uint8_t *r = (uint8_t * )&result;
  r[0] = (host_value >> 24);
  r[1] = (host_value >> 16) & 0xff;
  r[2] = (host_value >> 8) & 0xff;
  r[3] = host_value & 0xff;
  return result; 
#else
  return host_value;
#endif
}

inline uint16_t htons(  uint16_t host_value){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint16_t result = ((host_value >> 8) & 0xff) | ( host_value << 8);
  return result; 
#else
  return host_value;
#endif
}

inline uint16_t ntohs(  uint16_t network_value){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint16_t result = ((network_value >> 8) & 0xff) | ( network_value << 8);
  return result; 
#else
  return network_value;
#endif
}


void checksum_summate ( uint16_t *checksum, void * addr, int count) {
  /* Compute Internet Checksum for "count" bytes
  *         beginning at location "addr".
  */
  uint32_t sum = *checksum;
  uint16_t *addr16 = (uint16_t*)addr;
  
  while( count > 1 ){
    /*  This is the inner loop */
    sum += *addr16;
    addr16++;
    count -= 2;
  }

      /*  Add left-over byte, if any */
  if( count > 0 ){
    sum += * (uint8_t *) addr16;
  }
  
  /*  Fold 32-bit sum to 16 bits */
  while (sum>>16){
    sum = (sum & 0xffff) + (sum >> 16);
  }
  *checksum = sum;
}

uint16_t ipv6_pseduo_header_checksum( void *src_addr, void *dest_addr, uint32_t upper_layer_packet_length, uint8_t next_header_value ){
  uint16_t checksum = 0;
  uint32_t network_long ;
  checksum_summate(&checksum, src_addr, IPV6_ADDR_LENGTH);
  checksum_summate(&checksum, dest_addr, IPV6_ADDR_LENGTH);
  network_long = htonl( upper_layer_packet_length );
  checksum_summate(&checksum, &network_long, 4);
  network_long = htonl( next_header_value );
  checksum_summate(&checksum, &network_long, 4);
  return checksum;
}
