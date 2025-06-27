#include "defs.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "sys.h"

// Buffer and file variables
static char outfilename[] = "UTFS.dat";
static uint8_t * utfs_buffer;
static uint32_t utfs_buffer_size;
static uint32_t utfs_buffer_maxindex;
static uint32_t utfs_buffer_loadedsize;


static void sys_free_buffer()
{
    if(utfs_buffer)
    {
        printf("Freeing %d bytes\n",utfs_buffer_size);
        free(utfs_buffer);
        utfs_buffer=NULL;
    }
    utfs_buffer_size=0;
    utfs_buffer_maxindex=0;
    return;
}


void sys_init()
{
    utfs_buffer=NULL;
    utfs_buffer_size=0;
    utfs_buffer_maxindex=0;
    utfs_buffer_loadedsize=0;

    return;
}

uint32_t sys_write(uint32_t address, void * ptr, uint32_t length)
{
    uint8_t * dataptr;

    // Can't write empty data
    if(!ptr || length==0) return 0;


    // If the buffer is not big enough, resize it
    if(address+length>utfs_buffer_size){
        uint32_t size;
        uint8_t * newbuffer;

        size = address+length;

        printf("Realloc to %d bytes\n",size);
        newbuffer = realloc(utfs_buffer,size);
        if(!newbuffer){
            printf("Error resizing buffer \n");
            return 0;
        }
        utfs_buffer = newbuffer;
        utfs_buffer_size = size;
    }

    // Write the section in the RAM buffer
    dataptr = (uint8_t*)ptr;
    memcpy(&(utfs_buffer[address]),dataptr,length);
    if(address+length>utfs_buffer_maxindex) utfs_buffer_maxindex = address+length;

    return length;
}
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length)
{
    int fsize;
    int rsize;
    uint8_t * dataptr;

    // If the buffer is not created, create it and load
    // the entire file in
    if(utfs_buffer_size==0)
    {
        int fd;
        uint8_t * newbuffer;

        // Try and open the file
        fd = open(outfilename, O_RDONLY, 0);
        if(!fd){
            printf("Error opening file %s\n",outfilename);
            return 0;
        }
        // Get file size, allocate
        fsize = lseek(fd,0,SEEK_END);
        if(fsize<=0){
            printf("Error lseek %d\n",fsize);
            close(fd);
            return 0;
        }
        lseek(fd,0,SEEK_SET);

        // Allocate the memory
        printf("Realloc to %d bytes\n",fsize);
        newbuffer = realloc(utfs_buffer,fsize);
        if(!newbuffer){
            printf("Error allocating file\n");
            close(fd);
            return 0;
        }
        utfs_buffer = newbuffer;
        utfs_buffer_size = fsize;
        utfs_buffer_maxindex=0;
        utfs_buffer_loadedsize=fsize;
        memset(utfs_buffer,0,utfs_buffer_size);
        printf("New buffer size %d bytes\n",utfs_buffer_size);

        // Read file in to memory
        rsize = read(fd, utfs_buffer, utfs_buffer_size);
        if(rsize!=utfs_buffer_size){
            printf("Didn't read entire file, %d != %d\n",rsize,utfs_buffer_size);
        }

        // Close file
        close(fd);
    }

    // Bounds check
    if(address>utfs_buffer_size) return 0;
    if(address+length>=utfs_buffer_size) length = utfs_buffer_size-address;

    // From the file in memory, get the section
    printf("sys_read: addr %d, %d bytes\n",address,length);
    dataptr = (uint8_t*)ptr;
    memcpy(dataptr,&(utfs_buffer[address]),length);

    return length;
}
bool sys_flush()
{
    int fd;
    uint32_t writesize;

    if(utfs_buffer_maxindex==0){
        // We did not write data, so use the size of the file
        // read in
        writesize = utfs_buffer_loadedsize;
    }else{
        writesize = utfs_buffer_maxindex;
    }


    if(writesize==0){
        printf("Write size is 0, no data was loaded or written, skip writing\n");
    }else{

        // Open the file and overwrite if it exists
        fd = open(outfilename, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
        if(!fd){
            printf("Error opening file %s\n",outfilename);
            return false;
        }



        // From the file in memory, write it to the file
        write(fd,utfs_buffer,writesize);

        close(fd);
    }

    // free the buffer
    sys_free_buffer();

    return true;
}

// EOF
