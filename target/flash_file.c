#include <libpic30.h>
#include <stddef.h>
#include <stdint.h>

#define PRG_MEM_BUFFER_SIZE (512*3)

struct mem_file{
  _prog_addressT address;  
  size_t position;
  size_t buffer_position;
  size_t eof;
  uint8_t buffer[PRG_MEM_BUFFER_SIZE];
};

static struct mem_file the_file;

void fill_buffer(struct mem_file *f){
  size_t length = f->eof - f->position;
  length = PRG_MEM_BUFFER_SIZE < length ? PRG_MEM_BUFFER_SIZE : length;
  f->address = (f->position) / 3 * 2 ;
  f->address = _memcpy_p2d24(f->buffer, f->address, length);
  f->buffer_position = 0;
}

void file_seek( struct mem_file *f, size_t position){
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
  return buffer_size;
}

struct mem_file * file_open( const char *filename, const char mode){
  the_file.address = 0;
  the_file.position = 0;
  the_file.buffer_position = PRG_MEM_BUFFER_SIZE+1;
  the_file.eof = 30*512 + 3*32;
  return &the_file;
}

void file_close(struct mem_file *f){
}
