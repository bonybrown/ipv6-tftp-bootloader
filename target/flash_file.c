#include <libpic30.h>
#include <p33Fxxxx.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


/* The size of this buffer is 512 instructions.
 * Each instruction is 3 bytes
 * Each instruction is 2 program memory locations
 * 
 * The flash page (erase) size is 512 instructions
 * The flash row (write) size is 64 instructions ( 8 rows per page)
 * 
 * Therefore, this buffer holds one flash page of data 
 */
#define PRG_MEM_BUFFER_SIZE (512*3)

#define ADDR_POS(x) ( x / 2 * 3 )
#define IN_RANGE(start,end,x ) ( x>= ADDR_POS(start) && x < ADDR_POS(end) )

extern uint32_t _CODE_BASE;

struct mem_file{
  uint8_t buffer[PRG_MEM_BUFFER_SIZE];
  _prog_addressT base_address;  
  _prog_addressT write_address;  
  uint32_t position;
  size_t buffer_position;
  uint32_t eof;
  char mode;
};

static struct mem_file the_file;

int eraseable( _prog_addressT addr ){
  if( addr >= 0x400 && addr < 0x3000 ){
    return 0;
  }
  return 1;
}  

int writeable( uint32_t position ){
   
  /* reset vector */
  if( IN_RANGE( 0x0, 0x04, position ) ){
    return 0;
  }
  /* alt interrupt vector */
  if( IN_RANGE( 0x100, 0x0400, position ) ){
    return 0;
  }
  /* bootloader */
  if( IN_RANGE( 0x400, 0x3000, position ) ){
    return 0;
  }
  return 1;
}

void write_buffer(struct mem_file *f){
  int i,j;
  size_t offset;
  int32_t row_buffer[64];
  if( ! eraseable( f->write_address ) ){
    printf("PROTECTED 0x%lx\n", f->write_address );
    f->write_address += 64 * 2 * 8;
    return;
  }
  /* erases the whole page, which is one buffer full */
  printf("ERASE 0x%lx\n", f->write_address );
  SRbits.IPL = 7; // All interupt levels disabled
  _erase_flash(f->write_address);
  offset = 0;
  for( i = 0 ; i < 8 ; i++ ){
    for(j=0;j<64;j++){
      row_buffer[j] = (f->buffer[offset+2]);
      row_buffer[j] <<= 8;
      row_buffer[j] += (f->buffer[offset+1]);
      row_buffer[j] <<= 8;
      row_buffer[j] += (f->buffer[offset]);
      offset+=3;
    }                
    printf("WRITE 0x%lx\n", f->write_address );
    _write_flash24( f->write_address, row_buffer );
    f->write_address += 64 * 2;
  }
  SRbits.IPL = 0; // All interupt levels enabled
}



void fill_buffer(struct mem_file *f){
  uint32_t length = f->eof - f->position;
  _prog_addressT addr;
  length = PRG_MEM_BUFFER_SIZE < length ? PRG_MEM_BUFFER_SIZE : length;  
  addr = f->base_address + ((f->position) / 3 * 2) ;
  _memcpy_p2d24(f->buffer, addr, length);
  f->buffer_position = 0;
}

void file_seek( struct mem_file *f, uint32_t position){
  if(position != f->position ){
    f->position = position;
    fill_buffer(f);
  }
}

size_t file_read(struct mem_file *f, uint8_t * buffer , size_t buffer_size){
  size_t read = 0;
  while( read < buffer_size && f->position < f->eof ){
    if( f->buffer_position >= PRG_MEM_BUFFER_SIZE ){
      fill_buffer(f);
    }
    buffer[read] = f->buffer[f->buffer_position];
    read++;
    f->buffer_position++;
    f->position++;
  }
  return read;
}

size_t file_write(struct mem_file *f, uint8_t * buffer , uint16_t buffer_size){
  size_t written = 0;
  while( written < buffer_size && f->position < f->eof ){
    if( f->buffer_position >= PRG_MEM_BUFFER_SIZE ){
      /* have filled buffer. Erase page, and write rows */
      write_buffer(f);
      fill_buffer(f);
    }
    if( writeable( f->position ) ){
      f->buffer[f->buffer_position] = buffer[written];
    }
    written++;
    f->buffer_position++;
    f->position++;
  }
  return written;
}

struct mem_file * file_open( const char *filename, const char mode){
  if( strcmp("reset",filename)==0 && mode=='w' ){
    asm("RESET");
  }
  /* filename to upload must be boot.bin */
  if( ! ( strcmp("boot.bin",filename)==0 && mode=='w' ) ){
    return NULL;
  }
  memset( the_file.buffer, 0, PRG_MEM_BUFFER_SIZE);
  the_file.base_address = 0;
  the_file.write_address = the_file.base_address;
  the_file.position = 0;
  the_file.buffer_position = 0;
  the_file.eof = 132096; //(0x15600 + 0x200) / 2 * 3;
  the_file.mode = mode;
  fill_buffer(&the_file);
  return &the_file;
}

void file_close(struct mem_file *f){
  if( f->mode == 'w' ){
    write_buffer(f);
  }
    
}
