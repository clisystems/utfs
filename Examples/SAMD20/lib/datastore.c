#include "defs.h"
#include "datastore.h"

#include "nvm.h"


// Definitions
// ---------------------------------------------------------------------------
// a 'block' = 1 row
//#define DATASTORE_BLOCKS    4

// FLASH_PAGE_SIZE = 64
#define PAGES_PER_ROW   4
#define ROW_SIZE        (FLASH_PAGE_SIZE*PAGES_PER_ROW)
#define NUM_ROWS        (FLASH_NB_OF_PAGES/PAGES_PER_ROW)


#define LAST_ROW_ADDR   (FLASH_ADDR+((NUM_ROWS-1)*ROW_SIZE))
#define FIRST_ROW_ADDR  (FLASH_ADDR+((NUM_ROWS-DATASTORE_BLOCKS)*ROW_SIZE))

// Local variables
// ---------------------------------------------------------------------------
uint8_t nvm_row_buffer[ROW_SIZE];

struct nvm_config m_nvm;


// Local prototypes
// ---------------------------------------------------------------------------
static bool _read_row(uint32_t addr, uint8_t * data, int size);
static bool _write_row(uint32_t addr, uint8_t * data, int size);


// Public functions
// ---------------------------------------------------------------------------
void datastore_init()
{
    memset(nvm_row_buffer,0,sizeof(nvm_row_buffer));
    nvm_get_config_defaults(&m_nvm);
    m_nvm.manual_page_write=true;
    if(STATUS_OK!=nvm_set_config(&m_nvm)){
        printf("NVM Error\n");
    }

    return;
}

void datastore_command(char * cmd)
{
    if(cmp_const(cmd,"status"))
    {
        #if 1
        printf("FLASH_PAGE_SIZE: %d\n",FLASH_PAGE_SIZE);
        printf("ROW_SIZE: %d\n",ROW_SIZE);
        printf("PAGES_PER_ROW: %d\n",PAGES_PER_ROW);
        printf("FLASH_NB_OF_PAGES: %d\n",FLASH_NB_OF_PAGES);
        printf("LAST_ROW_ADDR: 0x%X\n",LAST_ROW_ADDR);
        printf("FIRST_ROW_ADDR: 0x%X\n",FIRST_ROW_ADDR);
        #endif
#if 0
    }else if(cmp_const(cmd,"flashtest1")){
        uint8_t data[255];
        int x;
        bool ret;

        printf("Test!\n");

        memset(data,0,sizeof(data));

        // Read data
        ret = datastore_read_block(0,data,sizeof(data));
        printf("read: %d\n",ret);
        pbuf(data,sizeof(data));

        // Fill data
        for(x=0;x<sizeof(data);x++) data[x]=x;
        printf("Filled:\n");
        pbuf(data,sizeof(data));

        // Write data
        ret = datastore_write_block(0,data,sizeof(data));
        printf("write: %d\n",ret);

        // Fill data
        for(x=0;x<sizeof(data);x++) data[x]=0;
        printf("Cleared:\n");
        pbuf(data,sizeof(data));

        // Read data
        ret = datastore_read_block(0,data,sizeof(data));
        printf("read: %d\n",ret);
        pbuf(data,sizeof(data));

#endif
    }
    return;
}

// Disable this to trace reads and writes
#if 0
#undef printf
#define printf(...)
#endif


bool datastore_read_block(int block, uint8_t * data, int size)
{
    uint32_t addr;
    int x;
    if(!data) return false;
    if(block>=DATASTORE_BLOCKS){ printf("Invalid block %d\n",block); return false; }
    if(size>DATASTORE_BLOCK_SIZE){
        printf("Error, read block %d bytes > %d!\n",size,DATASTORE_BLOCK_SIZE);
        return false;
    }
    addr = FIRST_ROW_ADDR + (block*ROW_SIZE);

    //printf("Read block %d\n",block);
    if(!_read_row(addr,nvm_row_buffer,ROW_SIZE)){
        printf("Error in write!! %s:%d\n",__func__,__LINE__);
    }

    // Copy over the data
    for(x=0;x<size;x++)
        data[x] = nvm_row_buffer[x];

    return true;
}
bool datastore_write_block(int block, uint8_t * data, int size)
{
    uint32_t addr;
    int x;
    if(!data) return false;
    if(block>=DATASTORE_BLOCKS){ printf("Invalid block %d\n",block); return false; }
    if(size>DATASTORE_BLOCK_SIZE){
        printf("Error, write block %d bytes > %d!\n",size,DATASTORE_BLOCK_SIZE);
        return false;
    }

    addr = FIRST_ROW_ADDR + (block*ROW_SIZE);

    // If we are writing less than a full block, load the block first
    // so it can be updated. If this is a full block, don't read because
    // it doesn't matter
    if(size<DATASTORE_BLOCK_SIZE)
    {
        // Load the row
        if(!_read_row(addr,nvm_row_buffer,ROW_SIZE)){
            printf("Error in read!! %s:%d\n",__func__,__LINE__);
            return false;
        }
    }
    // Update the data
    for(x=0;x<size;x++)
        nvm_row_buffer[x] = data[x];

    // Enable to see buffer as it goes out
    #if 0
    printf("-- Dumping buffer to write\n");
    pbuf(nvm_row_buffer,sizeof(nvm_row_buffer));
    #endif

    // Write the row
    //printf("Write block %d\n",block);
    if(!_write_row(addr,nvm_row_buffer,ROW_SIZE)){
        printf("Error in write!! %s:%d\n",__func__,__LINE__);
    }
    return true;
}


// Private functions
// ---------------------------------------------------------------------------
static bool _read_row(uint32_t addr, uint8_t * data, int size)
{
    int page=0;
    enum status_code res;
    if(size!=ROW_SIZE) return false;

    printf("Reading row: 0x%X - %d bytes\n",addr,size);

    // Load all the pages of the row
    for(page=0;page<PAGES_PER_ROW;page++)
    {
        uint32_t a;
        a = addr+(FLASH_PAGE_SIZE*page);
        res = nvm_read_buffer(a,&(data[FLASH_PAGE_SIZE*page]),FLASH_PAGE_SIZE);
        if(res!=STATUS_OK)
        {
            printf("Error reading row 0x%X page %d\n",addr,page);
            return false;
        }
    }

    return true;
}
static bool _write_row(uint32_t addr, uint8_t * data, int size)
{
    int page=0;
    enum status_code res;
    if(size!=ROW_SIZE) return false;

    printf("Writing row: 0x%X - %d bytes\n",addr,size);

    // Erase the row
    res=nvm_erase_row(addr);
    if(res!=STATUS_OK){
        printf("Error with erase!\n");
        return false;
    }

    // Write all the pages of the row
    for(page=0;page<PAGES_PER_ROW;page++)
    {
        uint32_t a;
        a = addr+(FLASH_PAGE_SIZE*page);
        res = nvm_write_buffer(a,&(data[FLASH_PAGE_SIZE*page]),FLASH_PAGE_SIZE);
        printf("Write: 0x%X = %d\n",a,res);
        if(res!=STATUS_OK)
        {
            printf("Error writing row 0x%X page %d\n",addr,page);
            return false;
        }

        // Execute the write
        res = nvm_execute_command(NVM_COMMAND_WRITE_PAGE,a, 0);
        if(res!=STATUS_OK)
        {
            printf("Error executing WRITE_PAGE\n");
            return false;
        }

        /*
        * NOTE: For SAM D21 RWW devices, see \c SAMD21_64K, command \c NVM_COMMAND_RWWEE_WRITE_PAGE
        * must be executed before any other commands after writing a page,
        * refer to errata 13588.
        */
        #ifdef FEATURE_NVM_RWWEE
        res = nvm_execute_command(NVM_COMMAND_RWEE_WRITE_PAGE,a, 0);
        if(res!=STATUS_OK)
        {
            printf("Error running RWWEE_WRITE_PAGE\n");
            return false;
        }

        #endif

    }

    return true;
}

// EOF
