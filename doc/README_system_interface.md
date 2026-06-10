# UTFS System Interface

This document describes the system interface for UTFS in an embedded system: the API the
application calls, and the two functions (`sys_read` / `sys_write`) the application must provide
to port UTFS to a storage medium.

Experience with C is expected. For a complete, compiling reference, see `Examples/gcc_linux`.

## UTFS File System Paradigm

Common file systems, including EXT4, FAT, NTFS, etc, support a traditional file system paradigm including open, read, write, close, where a file is open or closed, and open files can be read or written to.  Additionally, when open files are written, the user expects the file data to be stored on non-volatile storage implicitly.

This paradigm works for many cases on modern computer systems, however data storage in an embedded system is more nuanced than larger computer systems. For example, the nonvolatile storage medium may be a fixed part flash memory with a limit to the number of writes per sector. In this case, a change or 'write' to a single byte of data may not require writing to the non-volatile storage, but rather waiting until all data is updated before 'flushing' to the non-volatile medium.

The UTFS system interface deviates from a 'traditional' file system paradigm, all data is loaded at one time and written at one time.

## Register

UTFS provides a method where the system registers a ‘file’, which adds the file to the list of data to be read/written to the storage memory.

The file structure contains the name of the desired file, a pointer to a block of RAM which will hold the loaded data, and a size of the data block.

## Reads
Data is read from memories with a ‘load’ function.  This function informs UTFS to access the storage device, parse the header, match the header with any registered file names, and load the stored data in to the RAM pointer associated with that file name.

## Writes
Data is written to the memory with a ‘save’ function.  The function informs UTFS to take the entire file list, and write the files to the memory.  Each file is parsed, the file header is written and then the data pointed to by the file structure is written.

# Details

## Constants

```
// UTFS header handles a string of 12 characters, which is
// a C-string of 11 characters, including \0 terminator
#define UTFS_MAX_FILENAME   11

// Maximum number of files that can be registered. This sizes the
// static pointer table; UTFS allocates nothing dynamically.
// Registering more than this returns RES_FILESYSTEM_FULL.
#define UTFS_MAX_FILES      5
```

## File Data Structure

UTFS data structure containing 'file name', file signature, data storage block pointer, data storage block size, flags for the driver layer, and a variable for the number of bytes loaded from the medium.

```
typedef struct{
    char filename[UTFS_MAX_FILENAME+1];
    uint16_t signature;
    uint16_t flags;
    uint32_t size;
    uint32_t size_loaded;
    void * data;
}utfs_file_t;
```

## Return Types

Like all file systems, load/save issues can happen.  The following are return types for operations in the UTFS driver.

```
typedef enum{
    RES_OK = 0,
    RES_FILE_NOT_FOUND,
    RES_READ_ERROR,
    RES_WRITE_ERROR,
    RES_PARAM_ERROR,
    RES_FILENAME_EXISTS,
    RES_FILESYSTEM_FULL,
    RES_INVALID_FS,
}utfs_result_e;
```

## Function Prototypes

```
utfs_result_e utfs_init(bool verbose);

// Set the byte offset on the medium where the file system begins.
// Defaults to 0. Call before utfs_load() / utfs_save().
utfs_result_e utfs_baseaddress_set(uint32_t baseaddr);

utfs_result_e utfs_register(utfs_file_t * f, utfs_flags_e flags, utfs_options_e options);
utfs_result_e utfs_unregister(utfs_file_t * f);

// Load / save the entire registered file list at once (the typical case)
utfs_result_e utfs_load();
utfs_result_e utfs_save();

// Single-file operations
utfs_result_e utfs_load_file(utfs_file_t * f);
utfs_result_e utfs_save_file(utfs_file_t * f);
```

## System Interfaces

UTFS performs no I/O itself. The application must provide a method to read and write arbitrary
blocks of bytes to the non-volatile storage medium. These two functions are the entire porting
seam.

```
// Write data to the medium, return number of bytes actually written
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);

// Read data from the medium, return number of bytes actually read
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);
```

The contract these functions must honor:

- **`address` is a flat byte offset**, not a sector or page index. It starts at 0 (or at the
  value passed to `utfs_baseaddress_set`) and increases monotonically as UTFS walks the medium.
- **Transfer the full `length`** when the medium contains that many bytes, and return the number
  of bytes moved. UTFS advances its medium position by the value `sys_write` returns for the
  header, so a short write desynchronizes the layout.
- **A return of 0 from `sys_read` is treated as end-of-filesystem.** During `utfs_load()`, UTFS
  reads a header and stops the scan when the read returns 0 (or the bytes read are not a valid
  UTFS header). Your implementation should return 0 for reads past the end of usable storage,
  and clamp `length` so it never reads or writes outside the medium.
- **Bytes are moved verbatim.** The 24-byte header is serialized by writing the header struct
  raw, so the on-medium format inherits the host byte order. UTFS targets little-endian hosts
  (see the endianness note in the project README). `sys_read` / `sys_write` themselves impose no
  alignment requirement beyond byte addressing.

See each example's `sys.c` (or the `.ino` for the Arduino ports) for concrete implementations
against EEPROM, flash, and RAM-backed buffers.


