/***********************************************************************
* UTFS Library
*
* 2025 CLI Systems LLC <contact@clisystems.com>
* 
***********************************************************************/
#ifndef __UTFS_H__
#define __UTFS_H__

// Constants
// ----------------------------------------------------------------------------
#ifndef UTFS_MAX_FILES
#define UTFS_MAX_FILES      5
#endif
#define UTFS_MAX_FILENAME   7

// Types
// ----------------------------------------------------------------------------
typedef enum{
    RES_OK = 0,
    RES_FILE_NOT_FOUND,
    RES_READ_ERROR,
    RES_WRITE_ERROR,
    RES_FILENAME_EXISTS,
    RES_FILESYSTEM_FULL,
    RES_INVALID_FS,
}utfs_result_e;

typedef struct{
    char filename[UTFS_MAX_FILENAME+1];
    uint32_t flags;
    uint32_t size;
    uint32_t size_loaded;
    void * data;
}utfs_file_t;


// Functions
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

bool utfs_init(bool verbose);

void utfs_baseaddress_set(uint32_t baseaddr);

bool utfs_register(utfs_file_t * f);
void utfs_unregister(utfs_file_t * f);

utfs_result_e utfs_load();
utfs_result_e utfs_save();

#ifdef __cplusplus
}
#endif

#endif
