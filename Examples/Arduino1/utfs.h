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
#define UTFS_MAX_FILES      5
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

typedef enum{
	NOFLAGS			= 0,
#ifdef UTFS_ENABLE_FLAGS
    LOAD_EXPLICIT   = 0x01,
    SAVE_EXPLICIT   = 0x02,
#endif
}utfs_flags_e;

typedef enum{
    NOOPT           = 0,
    OPT_REPLACE     = 0x01,
}utfs_options_e;

// Functions
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

bool utfs_init(bool verbose);

void utfs_baseaddress_set(uint32_t baseaddr);

bool utfs_register(utfs_file_t * f, utfs_flags_e flags, utfs_options_e options);
void utfs_unregister(utfs_file_t * f);

utfs_result_e utfs_load();
utfs_result_e utfs_save();

// Write all files, including those with SAVE_EXPLICIT
utfs_result_e utfs_save_flush();

utfs_result_e utfs_load_file(utfs_file_t * f);
utfs_result_e utfs_save_file(utfs_file_t * f);


/// Utility functions
bool utfs_set(utfs_file_t * f,char * name, void * data, uint32_t size);
bool utfs_set_filename(utfs_file_t * f,char * name);
bool utfs_set_data(utfs_file_t *f,void * data, uint32_t size);
const char * utfs_result_str(utfs_result_e res);

/// Debug
void utfs_status();

#ifdef __cplusplus
}
#endif

#endif
