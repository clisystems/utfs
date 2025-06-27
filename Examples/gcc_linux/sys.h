#ifndef __SYS_H__
#define __SYS_H__

void sys_init();

uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);

uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);

bool sys_flush();


#endif
