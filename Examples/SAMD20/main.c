#include "defs.h"
#include "sys.h"
#include "datastore.h"

#include "utfs.h"


typedef struct{
    uint16_t signature;
    uint8_t version;
    uint8_t unused;
    uint16_t led_time_period_ms;
}app_data_t;

app_data_t appdata;

utfs_file_t appfile;

uint8_t scratchdata[512];


// Flash read/write functions
// ----------------------------------------------------------------------------
#define DATASTORE_MAX_ADDRESS   (DATASTORE_BLOCKS*DATASTORE_BLOCK_SIZE)-1
#define BLOCKID_INVALID         0xFF
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length)
{
    uint8_t datablock[DATASTORE_BLOCK_SIZE];
    uint32_t blockid=BLOCKID_INVALID;
    uint32_t blockindex=0;
    uint8_t * dataptr;
    uint32_t bytes=0;

    if(!ptr || length==0) return 0;
    if(address>DATASTORE_MAX_ADDRESS) return 0;
    if(address+length>DATASTORE_MAX_ADDRESS) length=(DATASTORE_MAX_ADDRESS+1)-address;

    dataptr = (uint8_t*)ptr;

    do{
        // Make sure the current block is loaded
        if(address/DATASTORE_BLOCK_SIZE != blockid)
        {
            if(blockid!=BLOCKID_INVALID){
                datastore_write_block(blockid,datablock,sizeof(datablock));
            }
            blockid=(address/DATASTORE_BLOCK_SIZE);
            datastore_read_block(blockid,datablock,sizeof(datablock));
            blockindex = address%DATASTORE_BLOCK_SIZE;
            printf("blockid %d, blockindex %d, address %d\n",blockid,blockindex,address);
        }

        if(dataptr) datablock[blockindex] = *dataptr;
        dataptr++;
        address++;
        blockindex++;
        bytes++;

    }while(--length);

    if(blockid!=BLOCKID_INVALID){
        datastore_write_block(blockid,datablock,blockindex);
    }

    return bytes;
}
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length)
{
    uint8_t datablock[DATASTORE_BLOCK_SIZE];
    uint32_t blockid=BLOCKID_INVALID;
    uint32_t blockindex=0;
    uint8_t * dataptr;
    uint32_t bytes=0;

    if(!ptr || length==0) return 0;
    if(address>DATASTORE_MAX_ADDRESS) return 0;
    if(address+length>DATASTORE_MAX_ADDRESS) length=(DATASTORE_MAX_ADDRESS+1)-address;

    dataptr = (uint8_t*)ptr;

    do{
        // Make sure the current block is loaded
        if(address/DATASTORE_BLOCK_SIZE != blockid)
        {
            blockid=(address/DATASTORE_BLOCK_SIZE);
            datastore_read_block(blockid,datablock,sizeof(datablock));
            blockindex = address%DATASTORE_BLOCK_SIZE;
        }

        if(ptr) *dataptr = datablock[blockindex];
        dataptr++;
        address++;
        blockindex++;
        bytes++;

    }while(--length);

    return bytes;
}


// Simple 'terminal' for serial port
// ----------------------------------------------------------------------------
#define GETARG(I)   (strtok((I)," "),strtok(NULL," "))
void sys_stdin_handler(char * input)
{
    // This is called from
    //   [main]    [sys]
    // mainloop>sys_usart_process>[here]

    char * arg1;
    //printf("[TERM] process '%s'\n",input);

    if(cmp_const(input,"status")){
        sys_status();
        printf("** Datastore:\n");
        datastore_command(input);
        printf("** appdata:\n");
        printf("  sig: 0x%X\n",appdata.signature);
        printf("  ver: %d\n",appdata.version);
        printf("  time_ms: %d\n",appdata.led_time_period_ms);
    }else if(cmp_const(input,"reboot")){
        system_reset();

    }else if(cmp_const(input,"load")){
            utfs_load();
    }else if(cmp_const(input,"save")){
            utfs_save();

    }else if(cmp_const(input,"time")){
        int time_ms;

        time_ms = atoi(GETARG(input));
        printf("New LED time period: %d\n",time_ms);
        appdata.led_time_period_ms = time_ms;

    }else if(cmp_const(input,"utfs")){
        utfs_status();

    }else if(cmp_const(input,"flashdump")){
        uint32_t n;
        n = sys_read(0,scratchdata,sizeof(scratchdata));
        pbuf(scratchdata,sizeof(scratchdata));

    }else if(cmp_const(input,"flashclear")){
        uint32_t n;

        for(n=0;n<sizeof(scratchdata);n++) scratchdata[n]=(uint8_t)n;
        pbuf(scratchdata,sizeof(scratchdata));

        //n = sys_write(0,scratchdata,sizeof(scratchdata));
        n = sys_write(0,scratchdata,258);
    }else if(cmp_const(input,"flashzero")){
        memset(scratchdata,0,sizeof(scratchdata));
        sys_write(0,scratchdata,sizeof(scratchdata));

    }else{
        printf("[TERM] Unknown command '%s'\n",input);
    }
    return;
}



// Main loop
// ----------------------------------------------------------------------------
int main(void)
{
    uint32_t last_tick=0;
    uint32_t last_timestamp=0;
    uint32_t time;

    utfs_result_e ures;

    // Default 'driver' init function
	system_init();

	// Local project inits
	sys_init();
	datastore_init();
	utfs_init(true);

	// Write to the uart console
    printf("\nFirmware: SAMD20 BOB1\n");
    printf("Build: %s %s\n",__DATE__,__TIME__);


	// Configure UTFS files
	utfs_set(&appfile,"appdata",&appdata,sizeof(appdata));
    utfs_register(&appfile, UTFS_NOFLAGS, UTFS_NOOPT);

    // Load the UTFS data

    // NOTE: The first load on a new Arduino will return
    // RES_INVALID_FS since the file system is blank on the EEPROM

    ures = utfs_load();
    printf("UTFS result: %s\n",utfs_result_str(ures));


    // Handle invalid data
    if(appdata.signature!=0xABCD){
        printf("Defaulting appdata\n");
        memset(&appdata,0,sizeof(appdata));
        appdata.signature=0xABCD;
        appdata.version=1;
        appdata.led_time_period_ms = 100;
    }

	port_pin_set_output_level(LED1_PIN, LED_off);
    port_pin_set_output_level(LED2_PIN, LED_on);

    // System started
    printf("Started...\n");

	// Interrupts take over from here
    // Enable system interrupts
    system_interrupt_enable_global();

	// main loop
	while(1)
	{
		// Track time based on ticks
		time = g_ticks;
		if(time!=last_tick){
			last_tick=time;
		}

		#if 1
		// Every X ms based on g_timestamp_ms
		time = g_timestamp_ms;
		if(time!=last_timestamp)
		{
			last_timestamp=time;

			if((time%appdata.led_time_period_ms)==0){
			    // Toggle LEDs
                port_pin_toggle_output_level(LED1_PIN);
                port_pin_toggle_output_level(LED2_PIN);
			}

		}
		#endif

		// Process routines
		sys_usart_process();
	}

	// Will never get here
}

// EOF

