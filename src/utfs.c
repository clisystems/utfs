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
#define UTFS_IDENTIFIER     0x1984
#define UTFS_VERSION_V1     1


// Types
// ----------------------------------------------------------------------------
typedef struct{
    uint16_t ident;
    uint8_t version;
    uint8_t flags;
    uint16_t signature;
    uint16_t unused;
    uint32_t size;
    char filename[12];
}utfs_header_t;


// Variables
// ----------------------------------------------------------------------------
static utfs_file_t * file_list[UTFS_MAX_FILES];
static bool _structure_saved = false;
static bool _utfs_verbose;
static uint32_t _baseaddr;

// System Prototypes
// ----------------------------------------------------------------------------
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);

// Local Prototypes (Private)
// ----------------------------------------------------------------------------
static void _print_header(utfs_header_t * header);

// Logging
#if defined(UTFS_ENABLE_LOG_PRINTF)
#define _utfs_log(...)        do{ if(_utfs_verbose){ printf(__VA_ARGS__); } }while(0)
#elif defined(UTFS_ENABLE_LOG_VPRINTF)
#include <stdarg.h>
static inline void _utfs_log(const char *fmt, ...)
{
	va_list ap;
    if(!_utfs_verbose) return;
	va_start(ap, fmt);
    vprintf(fmt, ap);
	va_end(ap);
}
#else
#define _utfs_log(...)
#endif


// Public functions
// ----------------------------------------------------------------------------
bool utfs_init(bool verbose)
{
    memset(file_list,0,sizeof(file_list));
    _structure_saved=false;
    _utfs_verbose=verbose;
    _baseaddr=0;
    _utfs_log("_utfs_verbose: %d\n",_utfs_verbose);
    _utfs_log("utfs_file_t size: %ld bytes\n",sizeof(utfs_file_t));
    return true;
}

void utfs_baseaddress_set(uint32_t baseaddr)
{
    _baseaddr = baseaddr;
    return;
}

bool utfs_register(utfs_file_t * f, utfs_flags_e flags, utfs_options_e options)
{
    int x;
    if(!f) return false;
    f->flags = flags;
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(file_list[x]==NULL)
        {
            _utfs_log("Found empty slot %d\n",x);
            file_list[x] = f;
            return RES_OK;
        }else if(strncmp(file_list[x]->filename,f->filename,UTFS_MAX_FILENAME+1)
                    && (options&UTFS_OPT_REPLACE)==0){
            _utfs_log("Found %s=%s, replacing\n",file_list[x]->filename,f->filename);
            file_list[x] = f;
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
        if(file_list[x]==f){
            _utfs_log("Removed %s at position %d\n",file_list[x]->filename,x);
            file_list[x] = NULL;
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
    utfs_header_t header;
    
    memset(&header,0,sizeof(header));

    pos = _baseaddr;

    // Read up to UTFS_MAX_FILES files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        // Read the header
        f = sys_read(pos, (uint8_t*)&header, sizeof(header));
        if(f<=0 || header.ident != UTFS_IDENTIFIER)
        {
            break;
        }
        if(_utfs_verbose) _print_header(&header);
        pos += sizeof(header);
        
        // Find the file
        for(f=0;f<UTFS_MAX_FILES;f++)
        {
            if(strncmp(file_list[f]->filename,header.filename,UTFS_MAX_FILENAME+1)==0)
            {
                _utfs_log("Found match, position %d\n",f);
                break;
            }
        }
        
        // Handle data
        if(f>=UTFS_MAX_FILES)
        {
            _utfs_log("Did not find file %s\n",header.filename);

        }else if(file_list[f]->data==NULL){
            _utfs_log("Null data, skipping\n");            
            file_list[f]->size_loaded=0;
            file_list[f]->signature=header.signature;
            file_list[f]->flags&=(0xFF00); // blank the lower byte
            file_list[f]->flags|=header.flags; // Add in the lower byte flags from the header
            
        }else{
            uint32_t s;
            
            // Found the file, read in the data
            
            // The file is saved with a size, but if this application
            // has a smaller buffer, only read in that much
            s = header.size;
            if(s>file_list[f]->size) s=file_list[f]->size;
            
            // Read in the data
            #ifdef UTFS_ENABLE_FLAGS
            if((file_list[f]->flags&LOAD_EXPLICIT)==0)
            #else
            if(true)
            #endif
            {
                sys_read(pos, file_list[f]->data, s);
                file_list[f]->size_loaded=s;
                file_list[f]->signature=header.signature;
                file_list[f]->flags&=(0xFF00); // blank the lower byte
                file_list[f]->flags|=header.flags; // Add in the lower byte flags from the header
            }else{
                _utfs_log("LOAD_EXPLICIT set, skipping read '%s'\n",header.filename);
                file_list[f]->size_loaded=0;
                file_list[f]->signature=0;
                file_list[f]->flags&=(0xFF00); // blank the lower byte
            }
            
        }
        pos += header.size;
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
    utfs_header_t header;
    
    pos = _baseaddr;
    
    // Write all the files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(file_list[x]!=NULL)
        {
            _utfs_log("Writing file %d at pos %d\n",x,pos);
            memset(&header,0,sizeof(header));
            header.ident = UTFS_IDENTIFIER;
            header.version = UTFS_VERSION_V1;
            header.flags = ((file_list[x]->flags)&0x00FF);   // Save the lower byte of the flags
            header.signature = file_list[x]->signature;
            header.unused = 0;
            header.size = file_list[x]->size;
            strncpy((char*)(header.filename),file_list[x]->filename,UTFS_MAX_FILENAME);
            
            bptr = (uint8_t*)(file_list[x]->data);
            
            // Write header
            pos += sys_write(pos,&header,sizeof(header));

            // Write data
            #ifdef UTFS_ENABLE_FLAGS
            if((file_list[x]->flags&SAVE_EXPLICIT)==0)
            #else
            if(true)
            #endif
            {
                sys_write(pos,bptr,file_list[x]->size);
            }else{
                _utfs_log("SAVE_EXPLICIT set, not writing '%s'\n",header.filename);
            }
            
            // Increment by size
            pos += file_list[x]->size;
        }
    }
    
    // This means we have saved the structure
    _structure_saved=true;

    return RES_OK;
}

utfs_result_e utfs_save_flush()
{
    uint8_t * bptr;
    uint32_t x;
    uint32_t pos;
    utfs_header_t header;
    
    pos = _baseaddr;
    
    // Write all the files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(file_list[x]!=NULL)
        {
            _utfs_log("Writing file %d at pos %d\n",x,pos);
            memset(&header,0,sizeof(header));
            header.ident = UTFS_IDENTIFIER;
            header.version = UTFS_VERSION_V1;
            header.flags = ((file_list[x]->flags)&0x00FF);   // Save the lower byte of the flags
            header.signature = file_list[x]->signature;
            header.unused = 0;
            header.size = file_list[x]->size;
            strncpy((char*)(header.filename),file_list[x]->filename,UTFS_MAX_FILENAME);
            
            bptr = (uint8_t*)(file_list[x]->data);
            
            // Write header
            pos += sys_write(pos,&header,sizeof(header));

            // Write data
            sys_write(pos,bptr,file_list[x]->size);
            
            // Increment by size
            pos += file_list[x]->size;
        }
    }
    
    // This means we have saved the structure
    _structure_saved=true;

    return RES_OK;
}

utfs_result_e utfs_load_file(utfs_file_t * f)
{
    uint8_t * bptr;
    uint32_t x,i,s;
    uint32_t pos;
    utfs_header_t header;
    
    if(!f) return RES_FILE_NOT_FOUND;

    pos = _baseaddr;

    // Support up to UTFS_MAX_FILES files
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        // Read the header
        i = sys_read(pos, (uint8_t*)&header, sizeof(header));
        if(i<=0 || header.ident != UTFS_IDENTIFIER)
        {
            return RES_FILE_NOT_FOUND;
        }
        pos += sizeof(header);
        
        // is it a match?
        if(strncmp(f->filename,file_list[x]->filename,UTFS_MAX_FILENAME)!=0)
        {
            // No, just skip the data
            pos += header.size;
            continue;
        }

        // It is a match
        _utfs_log("Found file to load, pos %d\n",pos);
        if(_utfs_verbose) _print_header(&header);
        
        // Handle null
        if(file_list[x]->data==NULL){
            _utfs_log("Null data, skipping\n");
            return RES_OK;
        }
        
        // Copy it over
        
        // The file is saved with a size, but if this application
        // has a smaller buffer, only read in that much
        s = header.size;
        if(s>file_list[x]->size) s=file_list[x]->size;
        
        // Read in the data
        sys_read(pos, file_list[x]->data, s);
        file_list[x]->size_loaded=s;
        file_list[x]->signature=header.signature;
        file_list[x]->flags&=(0xFF00); // blank the lower byte
        file_list[x]->flags|=header.flags; // Add in the lower byte flags from the header
                        
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
    utfs_header_t header;
    
    if(!f) return RES_FILE_NOT_FOUND;
    
    // We can't save an individual file until the full set of files
    // have been written, so write them first
    if(_structure_saved==false){
        utfs_save();
    }
    
    pos = _baseaddr;
    
    // Loop over the files, find ours and save it
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(file_list[x]!=NULL)
        {
            if(strncmp(f->filename,file_list[x]->filename, UTFS_MAX_FILENAME)==0)
            {
                _utfs_log("Writing file %s, id %d at pos %d\n",f->filename,x,pos);
                memset(&header,0,sizeof(header));
                header.ident = UTFS_IDENTIFIER;
                header.version = UTFS_VERSION_V1;
                header.flags = ((file_list[x]->flags)&0x00FF);   // Save the lower byte of the flags
                header.signature = file_list[x]->signature;
                header.unused = 0;
                header.size = file_list[x]->size;
                strncpy((char*)(header.filename),file_list[x]->filename,UTFS_MAX_FILENAME);
                bptr = (uint8_t*)(file_list[x]->data);
                
                // Write header
                pos += sys_write(pos,&header,sizeof(header));

                // Write data
                sys_write(pos,bptr,file_list[x]->size);
            
                return RES_OK;
            }
            pos += sizeof(header)+file_list[x]->size;
        }
    }

    return RES_FILE_NOT_FOUND;
}

/// Utility functions
bool utfs_set(utfs_file_t * f,char * name, void * data,uint32_t size)
{
    if(!f) return false;
    strncpy(f->filename,name,UTFS_MAX_FILENAME);
    f->data = data;
    f->size = size;
    return true;
}
bool utfs_set_filename(utfs_file_t * f,char * name)
{
    if(!f) return false;
    strncpy(f->filename,name,UTFS_MAX_FILENAME);
    return true;
}
bool utfs_set_data(utfs_file_t *f,void * data,uint32_t size)
{
    if(!f) return false;
    f->data = data;
    f->size = size;
    return true;
}
uint16_t utfs_file_signature(utfs_file_t * f)
{
    if(!f) return 0;
    return f->signature;
}
bool utfs_file_signature_set(utfs_file_t * f, uint16_t sig)
{
    if(!f) return false;
    f->signature=sig;
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
        if(file_list[x])
        {
            count++;
            printf("Entry %d: '%s' - %d bytes\n",x,file_list[x]->filename,file_list[x]->size);
        }
    }
    if(count<=0) printf("No UTFS entries found\n");
    return;
}

// Private functions
// ----------------------------------------------------------------------------
static void _print_header(utfs_header_t * header)
{
    printf("Header:\n");
    printf(" ident: 0x%04X\n",header->ident);
    printf(" version: %d\n",header->version);
    printf(" flags: 0x%02X\n",header->flags);
    printf(" signature: 0x%04X\n",header->signature);
    printf(" unused: 0x%04X\n",header->unused);
    printf(" size: %d\n",header->size);
    printf(" filename: '%s'\n",header->filename);
    return;
}

// EOF
