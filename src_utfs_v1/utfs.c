/***********************************************************************
* UTFS Library
*
* 2025 CLI Systems LLC <contact@clisystems.com>
* 
***********************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "utfs.h"


// Definitions
// ----------------------------------------------------------------------------
#define UTFS_MAGICNUM       0x1984
#define UTFS_VERSION_V0     0
#define UTFS_VERSION_V1     1

// Types
// ----------------------------------------------------------------------------
typedef struct{
    uint16_t magic;
    uint8_t version;
    uint8_t unused;
    uint32_t size;
    char filename[UTFS_FILENAME_V0_MAX+1];
}utfs_header_v0_t;

typedef struct{
    uint16_t magic;
    uint8_t version;
    uint8_t unused;
    uint32_t size;
    uint32_t signature;
    uint32_t timestamp;
    char filename[UTFS_FILENAME_V1_MAX+1];
}utfs_header_v1_t;

typedef struct{
    uint16_t magic;
    uint8_t version;
    uint8_t unused;
    uint32_t size;
}utfs_header_all_t;

typedef union{
    utfs_header_all_t   all;
    utfs_header_v0_t    v0;
    utfs_header_v1_t    v1;
}utfs_header_u;


// Variables
// ----------------------------------------------------------------------------
static utfs_context_t ctx;

// System Prototypes
// ----------------------------------------------------------------------------
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);

// Local Prototypes (Private)
// ----------------------------------------------------------------------------
static void _print_header(utfs_header_u * header);

// Logging
#if defined(UTFS_ENABLE_LOG_PRINTF)
#define _utfs_log(...)        do{ if(ctx.verbose){ printf(__VA_ARGS__); } }while(0)
#elif defined(UTFS_ENABLE_LOG_VPRINTF)
#include <stdarg.h>
static inline void _utfs_log(const char *fmt, ...)
{
	va_list ap;
    if(!ctx.verbose) return;
	va_start(ap, fmt);
    vprintf(fmt, ap);
	va_end(ap);
}
#else
#define _utfs_log(...)
#endif


// Public functions
// ----------------------------------------------------------------------------
bool utfs_init()
{
    memset(&ctx,0,sizeof(ctx));
    ctx.baseaddr=0;
    ctx.saved=false;
    ctx.verbose=false;
    ctx.version = UTFS_VER1;
    return true;
}

void utfs_baseaddress_set(uint32_t baseaddr)
{
    ctx.baseaddr = baseaddr;
    return;
}
void utfs_version_set(utfs_version_e version)
{
    ctx.version = (uint8_t)version;
    return;
}
void utfs_verbose_set(bool verbose)
{
    ctx.verbose = (uint8_t)(verbose==true);
    return;
}

bool utfs_register(utfs_file_t * f, utfs_flags_e flags, utfs_options_e options)
{
    int x;
    if(!f) return false;
    f->flags = flags;
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(ctx.file_list[x]==NULL)
        {
            _utfs_log("Found empty slot %d\n",x);
            ctx.file_list[x] = f;
            return RES_OK;
        }else if(strncmp(ctx.file_list[x]->filename,f->filename,UTFS_FILENAME_V1_MAX+1)
                    && (options&UTFS_OPT_REPLACE)==0){
            _utfs_log("Found %s=%s, replacing\n",ctx.file_list[x]->filename,f->filename);
            ctx.file_list[x] = f;
            return RES_OK;
        }
    }
    if(x==UTFS_MAX_FILES){
        _utfs_log("Could not find slot");
        return RES_FILESYSTEM_FULL;
    }
    return RES_OK;
}

void utfs_unregister(utfs_file_t * f)
{
    int x;
    if(!f) return;
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(ctx.file_list[x]==f){
            _utfs_log("Removed %s at position %d\n",ctx.file_list[x]->filename,x);
            ctx.file_list[x] = NULL;
            return;
        }
    }
    return;
}

utfs_result_e utfs_load()
{
    uint8_t * bptr;
    uint32_t x,f;
    uint32_t pos;
    utfs_header_u * header_ptr;
    uint8_t headbuffer[sizeof(utfs_header_v1_t)+1];
    
    memset(headbuffer,0,sizeof(headbuffer));
    header_ptr = (utfs_header_u*)headbuffer;

    pos = ctx.baseaddr;

    // Read up to UTFS_MAX_FILES files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        // Read the header
        // Read in a v1 header to see if it is v0 or v1
        f = sys_read(pos, headbuffer, sizeof(headbuffer));
        if(f<=0 || header_ptr->all.magic != UTFS_MAGICNUM)
        {
            break;
        }
        if(ctx.verbose) _print_header(header_ptr);
        
        // If this is not v0 or v1, abort
        if(header_ptr->all.version!=UTFS_VERSION_V0 && header_ptr->all.version!=UTFS_VERSION_V1) break;
        
        // Skip the header
        if(header_ptr->all.version==UTFS_VERSION_V0) pos += sizeof(utfs_header_v0_t);
        if(header_ptr->all.version==UTFS_VERSION_V1) pos += sizeof(utfs_header_v1_t);
        
        // Find the file
        for(f=0;f<UTFS_MAX_FILES;f++)
        {
            if(header_ptr->all.version==UTFS_VERSION_V0)
            {
                if(strncmp(ctx.file_list[f]->filename,header_ptr->v0.filename,UTFS_FILENAME_V0_MAX+1)==0)
                {
                    _utfs_log("Found match, position %d\n",f);
                    break;
                }             
            }else if(header_ptr->all.version==UTFS_VERSION_V0){
                if(strncmp(ctx.file_list[f]->filename,header_ptr->v1.filename,UTFS_FILENAME_V1_MAX+1)==0)
                {
                    _utfs_log("Found match, position %d\n",f);
                    break;
                }
            }
        }
        
        // Handle data
        if(f>=UTFS_MAX_FILES)
        {
            _utfs_log("Did not find file %s\n",header_ptr->all.filename);

        }else if(ctx.file_list[f]->data==NULL){
            _utfs_log("Null data, skipping\n");            
            ctx.file_list[f]->size_loaded=0;
            
        }else{
            uint32_t s;
            
            // Found the file, read in the data
            
            // The file is saved with a size, but if this application
            // has a smaller buffer, only read in that much
            s = header_ptr->all.size;
            if(s>ctx.file_list[f]->size) s=ctx.file_list[f]->size;
            
            // Read in the data
            #ifdef UTFS_ENABLE_FLAGS
            if((ctx.file_list[f]->flags&LOAD_EXPLICIT)==0)
            #else
            if(true)
            #endif
            {
                sys_read(pos, ctx.file_list[f]->data, s);
                ctx.file_list[f]->size_loaded=s;
                ctx.file_list[f]->version = header_ptr->all.version;
                
                // Version 0, blank some data
                if(header_ptr->all.version == UTFS_VERSION_V0){
                    
                // Version 1 has more data
                }else if(header_ptr->all.version == UTFS_VERSION_V1){
                    ctx.file_list[f]->signature = header_ptr->v1.signature;
                    ctx.file_list[f]->timestamp = header_ptr->v1.timestamp;
                }
                
            }else{
                _utfs_log("LOAD_EXPLICIT set, skipping read '%s'\n",header.filename);
                ctx.file_list[f]->size_loaded=0;
            }
            
        }
        pos += header_ptr->all.size;
    }
    
    // If we didn't load anything
    if(x==0)
    {
        _utfs_log("Error loading FS\n");
        return RES_INVALID_FS;
    }
    
    return RES_OK;
}

utfs_result_e utfs_save()
{
    uint8_t * bptr;
    uint32_t x;
    uint32_t pos;
    uint32_t timestamp_now;
    utfs_header_u header;
    
    timestamp_now = 1234;
    pos = ctx.baseaddr;
    
    // Write all the files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(ctx.file_list[x]!=NULL)
        {
            _utfs_log("Writing file %d at pos %d\n",x,pos);
            memset(&header,0,sizeof(header));
            header.all.magic = UTFS_MAGICNUM;
            header.all.unused = 0;
            header.all.size = ctx.file_list[x]->size;
            
            // Determine what version we are going to write
            if(ctx.version==UTFS_VERSION_V0){
                header.all.version = UTFS_VERSION_V0;
                strncpy((char*)(header.v0.filename),ctx.file_list[x]->filename,UTFS_FILENAME_V0_MAX);
                // Write header
                pos += sys_write(pos,&(header.v0),sizeof(header.v0));

            }else if(ctx.file_list[x]->version==UTFS_VERSION_V1){
                header.all.version = UTFS_VERSION_V1;
                strncpy((char*)(header.v1.filename),ctx.file_list[x]->filename,UTFS_FILENAME_V1_MAX);
                header.v1.signature = ctx.file_list[x]->signature;
                header.v1.timestamp = timestamp_now;
                // Write header
                pos += sys_write(pos,&(header.v1),sizeof(header.v1));
            }else{
                continue; //?
            }
            
            bptr = (uint8_t*)(ctx.file_list[x]->data);
            
            // Write data
            #ifdef UTFS_ENABLE_FLAGS
            if((ctx.file_list[x]->flags&SAVE_EXPLICIT)==0)
            #else
            if(true)
            #endif
            {
                sys_write(pos,bptr,ctx.file_list[x]->size);
            }else{
                _utfs_log("SAVE_EXPLICIT set, not writing\n");
            }
            
            // Increment by size
            pos += ctx.file_list[x]->size;
        }
    }
    
    // This means we have saved the structure
    ctx.saved=true;

    return RES_OK;
}

utfs_result_e utfs_save_flush()
{
    uint8_t * bptr;
    uint32_t x;
    uint32_t pos;
    uint32_t timestamp_now;
    utfs_header_u header;
    
    pos = ctx.baseaddr;
    timestamp_now = 1234;
    
    // Write all the files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(ctx.file_list[x]!=NULL)
        {
            _utfs_log("Writing file %d at pos %d\n",x,pos);
            memset(&header,0,sizeof(header));
            header.all.magic = UTFS_MAGICNUM;
            header.all.unused = 0;
            header.all.size = ctx.file_list[x]->size;
            
            // Determine what version we are going to write
            if(ctx.version==UTFS_VERSION_V0){
                header.all.version = UTFS_VERSION_V0;
                strncpy((char*)(header.v0.filename),ctx.file_list[x]->filename,UTFS_FILENAME_V0_MAX);
                // Write header
                pos += sys_write(pos,&(header.v0),sizeof(header.v0));

            }else if(ctx.file_list[x]->version==UTFS_VERSION_V1){   
                header.all.version = UTFS_VERSION_V1;
                strncpy((char*)(header.v1.filename),ctx.file_list[x]->filename,UTFS_FILENAME_V1_MAX);
                header.v1.signature = ctx.file_list[x]->signature;
                header.v1.timestamp = timestamp_now;
                // Write header
                pos += sys_write(pos,&(header.v1),sizeof(header.v1));
            }else{
                continue; //?
            }
            
            // Write data
            bptr = (uint8_t*)(ctx.file_list[x]->data);
            sys_write(pos,bptr,ctx.file_list[x]->size);
            
            // Increment by size
            pos += ctx.file_list[x]->size;
        }
    }
    
    // This means we have saved the structure
    ctx.saved=true;

    return RES_OK;
}

utfs_result_e utfs_load_file(utfs_file_t * f)
{
    uint8_t * bptr;
    uint32_t x,i,s;
    uint32_t pos;
    utfs_header_u * header_ptr;
    uint8_t headbuffer[sizeof(utfs_header_v1_t)+1];
    
    memset(headbuffer,0,sizeof(headbuffer));
    header_ptr = (utfs_header_u*)headbuffer;
    
    if(!f) return RES_FILE_NOT_FOUND;

    pos = ctx.baseaddr;

    // Support up to UTFS_MAX_FILES files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        // Read the header
        // Read in a v1 header to see if it is v0 or v1
        i = sys_read(pos, headbuffer, sizeof(headbuffer));
        if(i<=0 || header_ptr->all.magic != UTFS_MAGICNUM)
        {
            return RES_FILE_NOT_FOUND;
        }
        if(ctx.verbose) _print_header(header_ptr);
        
        // If this is not v0 or v1, abort
        if(header_ptr->all.version!=UTFS_VERSION_V0 && header_ptr->all.version!=UTFS_VERSION_V1) return RES_FILE_NOT_FOUND;
        
        
        if(header_ptr->all.version==UTFS_VERSION_V0){
            // Skip the header
            pos += sizeof(utfs_header_v0_t);
            // is it a match?
            if(strncmp(header_ptr->v0.filename,ctx.file_list[x]->filename,UTFS_FILENAME_V0_MAX)!=0)
            {
                // No, just skip the data
                pos += header_ptr->all.size;
                continue;
            }
        }else if(header_ptr->all.version==UTFS_VERSION_V1){
            // Skip the header
            pos += sizeof(utfs_header_v1_t);
            // is it a match?
            if(strncmp(header_ptr->v1.filename,ctx.file_list[x]->filename,UTFS_FILENAME_V0_MAX)!=0)
            {
                // No, just skip the data
                pos += header_ptr->all.size;
                continue;
            }
            ctx.file_list[x]->signature=header_ptr->v1.signature;
            ctx.file_list[x]->timestamp=header_ptr->v1.timestamp;
        }else{
            return RES_FILE_NOT_FOUND;
        }
        

        // It is a match
        _utfs_log("Found file to load, pos %d\n",pos);
        if(ctx.verbose) _print_header(header_ptr);
        
        // Handle null
        if(ctx.file_list[x]->data==NULL){
            _utfs_log("Null data, skipping\n");
            return RES_OK;
        }
        
        // Copy it over
        
        // The file is saved with a size, but if this application
        // has a smaller buffer, only read in that much
        s = header_ptr->all.size;
        if(s>ctx.file_list[x]->size) s=ctx.file_list[x]->size;
        
        // Read in the data
        sys_read(pos, ctx.file_list[x]->data, s);
        ctx.file_list[x]->size_loaded=s;
        
        // Good
        return RES_OK;
    }
    
    return RES_FILE_NOT_FOUND;
}

utfs_result_e utfs_save_file(utfs_file_t * f)
{
    uint8_t * bptr;
    uint32_t x;
    uint32_t pos;
    uint32_t timestamp_now;
    utfs_header_u header;
    
    pos = ctx.baseaddr;
    timestamp_now = 1234;
    
    if(!f) return RES_FILE_NOT_FOUND;
    
    // We can't save an individual file until the full set of files
    // have been written, so write them first
    if(ctx.saved==false){
        utfs_save();
    }
    

    // Loop over the files, find ours and save it
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(ctx.file_list[x]!=NULL)
        {
            if(strncmp(f->filename,ctx.file_list[x]->filename, UTFS_FILENAME_V1_MAX)==0)
            {
                _utfs_log("Writing file %s, id %d at pos %d\n",f->filename,x,pos);
                memset(&header,0,sizeof(header));
                header.all.magic = UTFS_MAGICNUM;
                header.all.unused = 0;
                header.all.size = ctx.file_list[x]->size;
                
                // Determine what version we are going to write
                if(ctx.version==UTFS_VERSION_V0){
                    header.all.version = UTFS_VERSION_V0;
                    strncpy((char*)(header.v0.filename),ctx.file_list[x]->filename,UTFS_FILENAME_V0_MAX);
                    // Write header
                    pos += sys_write(pos,&(header.v0),sizeof(header.v0));

                }else if(ctx.file_list[x]->version==UTFS_VERSION_V1){   
                    header.all.version = UTFS_VERSION_V1;
                    strncpy((char*)(header.v1.filename),ctx.file_list[x]->filename,UTFS_FILENAME_V1_MAX);
                    header.v1.signature = ctx.file_list[x]->signature;
                    header.v1.timestamp = timestamp_now;
                    // Write header
                    pos += sys_write(pos,&(header.v1),sizeof(header.v1));
                }else{
                    continue; //?
                }

                // Write data
                bptr = (uint8_t*)(ctx.file_list[x]->data);
                sys_write(pos,bptr,ctx.file_list[x]->size);
            
                return RES_OK;
            }
            pos += sizeof(header)+ctx.file_list[x]->size;
        }
    }

    return RES_FILE_NOT_FOUND;
}

/// Utility functions
bool utfs_set(utfs_file_t * f,char * name, void * data,uint32_t size)
{
    if(!f) return false;
    strncpy(f->filename,name,UTFS_FILENAME_V1_MAX);
    f->data = data;
    f->size = size;
    return true;
}
bool utfs_set_filename(utfs_file_t * f,char * name)
{
    if(!f) return false;
    strncpy(f->filename,name,UTFS_FILENAME_V1_MAX);
    return true;
}
bool utfs_set_data(utfs_file_t *f,void * data,uint32_t size)
{
    if(!f) return false;
    f->data = data;
    f->size = size;
    return true;
}
const char * utfs_result_str(utfs_result_e res)
{
    switch(res){
    case RES_OK: return "RES_OK";
    case RES_FILE_NOT_FOUND: return "RES_FILE_NOT_FOUND";
    case RES_READ_ERROR: return "RES_READ_ERROR";
    case RES_WRITE_ERROR: return "RES_WRITE_ERROR";
    case RES_FILENAME_EXISTS: return "RES_FILENAME_EXISTS";
    case RES_FILESYSTEM_FULL: return "RES_FILESYSTEM_FULL";
    case RES_INVALID_FS: return "RES_INVALID_FS";
    }
    return "RES_UNKOWN";
}


/// Debug functions
void utfs_status()
{
    int x;
    int count=0;
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(ctx.file_list[x])
        {
            count++;
            printf("Entry %d: '%s' - %d bytes\n",x,ctx.file_list[x]->filename,ctx.file_list[x]->size);
        }
    }
    if(count<=0) printf("No UTFS entries found\n");
    return;
}

// Private functions
// ----------------------------------------------------------------------------
static void _print_header(utfs_header_u * header)
{
    printf("Header:\n");
    printf(" magic: 0x%04X\n",header->all.magic);
    printf(" version: %d\n",header->all.version);
    printf(" unused: 0x%02X\n",header->all.unused);
    printf(" size: %d\n",header->all.size);
    if(header->all.version==UTFS_VERSION_V0){
        printf(" filename: '%s'\n",header->v0.filename);
    }else if(header->all.version==UTFS_VERSION_V1){        
        printf(" signature: %d\n",header->v1.signature);
        printf(" timestamp: %d\n",header->v1.timestamp);
        printf(" filename: '%s'\n",header->v1.filename);
    }
    return;
}

// EOF
