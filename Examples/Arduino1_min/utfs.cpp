/***********************************************************************
* UTFS Library
*
* 2025 CLI Systems LLC <contact@clisystems.com>
*
* UTFS driver example, designed for Arduino Uno
*
* This driver expects the following functions to be defined
* somewhere in the Arduino project:
*
*   uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);
*   uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);
* 
***********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <Arduino.h>

#include "utfs.h"

// Support printf on Arduino serial port (40 chars)
#ifdef printf
#undef printf
#endif
#define printf(...)      do{sprintf(m_scratch,__VA_ARGS__);Serial.print(m_scratch);}while(0)
extern char m_scratch[40]; // m_scrach defined in .ino file


// Enable this #if to turn on _utfs_log() messages from UTFS. To see messages
// also enable with utfs_init(true).  Disable this #if to save flash space
#if 0
#define _utfs_log(...)        do{ if(_utfs_verbose) printf(__VA_ARGS__); }while(0)
#else
#define _utfs_log(...)
#endif

// Definitions
// ----------------------------------------------------------------------------
#define UTFS_SIGNATURE      0x1984
#define UTFS_VERSION        0


// Types
// ----------------------------------------------------------------------------
typedef struct{
    uint16_t signature;
    uint8_t version;
    uint8_t unused;
    uint32_t size;
    char filename[8];
}utfs_header_t;


// Variables
// ----------------------------------------------------------------------------
static utfs_file_t * file_list[UTFS_MAX_FILES];
static bool _structure_saved;
static bool _utfs_verbose;
static uint32_t _baseaddr;

// System Prototypes
// ----------------------------------------------------------------------------
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length);

// Public functions
// ----------------------------------------------------------------------------
bool utfs_init(bool verbose)
{
    memset(file_list,0,sizeof(file_list));
    _structure_saved=false;
    _utfs_verbose=verbose;
    _baseaddr=0;
    return true;
}

void utfs_baseaddress_set(uint32_t baseaddr)
{
    _baseaddr = baseaddr;
    return;
}

bool utfs_register(utfs_file_t * f)
{
    int x;
    if(!f) return false;
    f->flags = 0;
    for(x=0;x<UTFS_MAX_FILES;x++)
    {
        if(file_list[x]==NULL)
        {
            _utfs_log("Found empty slot %d -> %s, %d bytes\n",x,f->filename,f->size);
            file_list[x] = f;
            return RES_OK;
        }
    }
    if(x==UTFS_MAX_FILES){
        _utfs_log("Could not find slot for '%s'",f->filename);
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
        if(f<=0 || header.signature != UTFS_SIGNATURE)
        {
            break;
        }
        pos += sizeof(header);
        
        // Find the file
        for(f=0;f<UTFS_MAX_FILES;f++)
        {
            if(strncmp(file_list[f]->filename,header.filename,UTFS_MAX_FILENAME+1)==0)
            {
                _utfs_log("Found match, %s at position %d\n",file_list[f]->filename,f);
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
            
        }else{
            uint32_t s;
            
            // Found the file, read in the data
            
            // The file is saved with a size, but if this application
            // has a smaller buffer, only read in that much
            s = header.size;
            if(s>file_list[f]->size) s=file_list[f]->size;
            
            // Read in the data
            sys_read(pos, file_list[f]->data, s);
            file_list[f]->size_loaded=s;
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
            header.signature = UTFS_SIGNATURE;
            header.version = UTFS_VERSION;
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

    return RES_OK;
}

// EOF
