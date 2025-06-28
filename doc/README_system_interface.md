# UTFS System Interface

This document proposes a system interface for UTFS files in an embedded system.

In this document, the 'system interface for UTFS' shall be referred to as UTFS.

Experience with C is expected

### UTFS File System Paradigm

Common file systems, including EXT4, FAT, NTFS, etc, support a traditional file system paradigm including open, read, write, close, where a file is open or closed, and open files can be read or written to.  Additionally, when open files are written, the user expects the file data to be stored on non-volatile storage implicitly.

This paradigm works for many cases on modern computer systems, however data storage in an embedded system is more nuanced than larger computer systems. For example, the nonvolatile storage medium may be a fixed part flash memory with a limit to the number of writes per sector. In this case, a change or 'write' to a single byte of data may not require wiring to the non-volative storage, but rather waiting until all data is updated before 'flushing' to the non-volatile medium.

The proposed UTFS system interface deviates from a 'traditional' file system paradigm, all data is loaded at one time and written at one time.

### Register

UTFS provides a method where the system registers a ‘file’, which adds the file to the list of data to be read/written to the storage memory.

The file structure contains the name of the desired file, a pointer to a block of RAM which will hold the loaded data, and a size of the data block.

### Reads
Data is read from memories with a ‘load’ function.  This function informs UTFS to access the storage device, parse the header, match the header with any registered file names, and load the stored data in to the RAM pointer associated with that file name.

### Writes
Data is written to the memory with a ‘save’ function.  The function informs UTFS to take the entire file list, and write the files to the memory.  Each file is parsed, the file header is written and then the data pointed to by the file structure is written.

# Details

### Constants

```
// UTFS header handles a string of 8 characters, which is
// a C-string of 7 characters, including \0 terminator
#define UTFS_MAX_FILENAME   7
```

### File Data Structure

UTFS data structure containing 'file name', data storage block pointer, data storage block size, flags for the driver layer (future), and variable for number of bytes loaded from the medium.

```
typedef struct{
    char filename[UTFS_MAX_FILENAME+1];
    uint32_t flags;
    uint32_t size;
    uint32_t size_loaded;
    void * data;
}utfs_file_t;
```

### Return Types

Like all file systems, load/save issues can happen.  The following are return types for operations in the UTFS driver.

```
typedef enum{
    RES_OK = 0,
    RES_FILE_NOT_FOUND,
    RES_READ_ERROR,
    RES_WRITE_ERROR,
    RES_FILENAME_EXISTS,
    RES_FILESYSTEM_FULL,
    RES_INVALID_FS,
}utfs_result_e;
```

### Function Prototypes

```
bool utfs_init();

bool utfs_register(utfs_file_t * f);
void utfs_unregister(utfs_file_t * f);

utfs_result_e utfs_load();
utfs_result_e utfs_save();

utfs_result_e utfs_load_file(utfs_file_t * f);
utfs_result_e utfs_save_file(utfs_file_t * f);
```

### System Interfaces

The system shall provide a method to read and write arbitrary blocks of data to the non-volatile storage medium.

```
// Write data to flash, return number of written bytes
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);

// Read data from flash, return number of read bytes
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);
```


