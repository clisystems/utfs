#ifndef __DATA_STORE_H__
#define __DATA_STORE_H__

#define DATASTORE_BLOCKS        4   // blocks
#define DATASTORE_BLOCK_SIZE    256 // bytes

void datastore_init();

void datastore_command(char * cmd);

bool datastore_read_block(int block, uint8_t * data, int size);

bool datastore_write_block(int block, uint8_t * data, int size);

#endif
