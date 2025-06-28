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
#define UTFS_MAX_FILES          5
#define UTFS_FILENAME_V0_MAX    7
#define UTFS_FILENAME_V1_MAX    15
//#define UTFS_ENABLE_LOG_VPRINTF
//#define UTFS_ENABLE_LOG_PRINTF

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
    char filename[UTFS_FILENAME_V1_MAX+1];
    uint8_t version;
    uint8_t flags;
    uint16_t unused;
    uint32_t signature;
    uint32_t timestamp;
    uint32_t size;
    uint32_t size_loaded;
    void * data;
}utfs_file_t;

typedef struct{
    uint8_t version;
    uint8_t verbose;
    uint8_t saved;
    uint8_t unused;
    uint32_t baseaddr;
    utfs_file_t * file_list[UTFS_MAX_FILES];
}utfs_context_t;

typedef enum{
	UTFS_NOFLAGS			= 0,
#ifdef UTFS_ENABLE_FLAGS
    UTFS_LOAD_EXPLICIT   = 0x01,
    UTFS_SAVE_EXPLICIT   = 0x02,
#endif
}utfs_flags_e;

typedef enum{
    UTFS_NOOPT = 0,
    UTFS_OPT_REPLACE,
}utfs_options_e;

typedef enum{
    UTFS_VER0 = 0,
    UTFS_VER1,
}utfs_version_e;

// Functions
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

bool utfs_init();

void utfs_baseaddress_set(uint32_t baseaddr);
void utfs_version_set(utfs_version_e version);
void utfs_verbose_set(bool verbose);

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
