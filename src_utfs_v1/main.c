#include "defs.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <getopt.h>

#include "utfs.h"
#include "sys.h"

// Definitions
// ----------------------------------------------------------------------------

// Types and enums
// ----------------------------------------------------------------------------

// Variables
// ----------------------------------------------------------------------------
bool g_running = false;
bool g_verbose = true;

// Testing buffer
static uint8_t testbuffer[1024];

// UTFS file info

typedef struct{
    uint16_t signature;
    uint8_t version;
    uint8_t unused;
    uint32_t test_value;
}app_data_t;

app_data_t appdata;

utfs_file_t appfile;


// Local prototypes
// ----------------------------------------------------------------------------

// Functions
// ----------------------------------------------------------------------------
void sigint_handler(int arg)
{
	printf("<---- catch ctrl-c\n");
    g_running = false;
}

void help()
{
    printf("Usage: main.bin [-vh?]\n");
    printf("   -v       Enable verbose output\n");
    printf("   -h?      Program help (This output)\n");
    return;
}
void usage()
{
    help();
    exit(0);
}

// File pointer read/write functions
// ----------------------------------------------------------------------------



void setup()
{
    utfs_result_e ures;

    sys_init();

    // Setup UTFS
    utfs_init();
    utfs_verbose_set(g_verbose);

    // Register UTFS files
    utfs_set(&appfile,"appdata",&appdata,sizeof(appdata));
    utfs_register(&appfile, UTFS_NOFLAGS, UTFS_NOOPT);

    utfs_baseaddress_set(100);

    // Load UTFS
    ures = utfs_load();
    if(ures!=RES_OK) printf("ures: %s\n",utfs_result_str(ures));

    // Handle invalid data
    if(appdata.signature!=0xABCD){
        printf("Defaulting appdata\n");
        memset(&appdata,0,sizeof(appdata));
        appdata.signature=0xABCD;
        appdata.version=1;
        appdata.test_value = 1234;
    }


    return;
}

void command_process(char * input)
{
    // This is called from
    //   [main]    [sys]
    // mainloop>sys_usart_process>[here]

    char * arg1;
    //printf("[TERM] process '%s'\n",input);

    if(cmp_const(input,"exit")){
        g_running = false;
        return;
    }else if(cmp_const(input,"status")){
        printf("** appdata:\n");
        printf("  sig: 0x%X\n",appdata.signature);
        printf("  ver: %d\n",appdata.version);
        printf("  test_value: %d\n",appdata.test_value);

    }else if(cmp_const(input,"help")){
        printf("Command line usage:\n");
        help();
        printf("\n\nCommands:\n");
        printf("exit  - Shutdown the program (Also Ctrl-C)\n");
        printf("load  - UTFS load\n");
        printf("save  - UTFS save\n");
        printf("flush - Flush the written data to the output file\n");
        printf("utfs  - Show info about the UTFS filsystem\n");
        printf("status- Show appdata\n");
        printf("value NUM - Set the test value to NUM\n");
        printf("\n");
    }else if(cmp_const(input,"load")){
        utfs_load();
    }else if(cmp_const(input,"save")){
        utfs_save();

    }else if(cmp_const(input,"flush")){
        sys_flush();

    }else if(cmp_const(input,"utfs")){
        utfs_status();

    }else if(cmp_const(input,"value")){
        char * pch;
        pch = strtok(input," ,");
        pch = strtok(NULL," ,");
        if(pch){
            uint32_t newval;
            newval = atoi(pch);
            appdata.test_value = newval;
        }


    }else if(cmp_const(input,"testread")){
        int size;
        size = 512;
        memset(testbuffer,0xAA,sizeof(testbuffer));
        sys_read(0,testbuffer,size);
        pbuf(testbuffer,size);

    }else if(cmp_const(input,"testwrite")){
        int size;
        int n;

        size = 128;
        for(n=0;n<size;n++) testbuffer[n]=(uint8_t)n;

        pbuf(testbuffer,size);
        sys_write(0,testbuffer,size);

    }else{
        printf("[TERM] Unknown command '%s'\n",input);
    }
    return;
}

// Main function
// ----------------------------------------------------------------------------
int main(int argc,char** argv)
{
    int tty_fd;
    fd_set rfds;
    struct timeval tv;
    int retval;
    int optchar;
    
    
    struct option longopts[] = {
    { "verbose", no_argument,       0, 'v' },
    { 0, 0, 0, 0 }
    };

    // Process the command line options
    while ((optchar = getopt_long(argc, argv, "vh?", \
           longopts, NULL)) != -1)
    {
       switch (optchar)
       {
       case 'v':
           printf("Verbose = true\n");
           g_verbose = true;
           break;
       default:
           usage();
           break;
       }
    };
    
    // Process non-flagged arguments
    // https://stackoverflow.com/questions/18079340/using-getopt-in-c-with-non-option-arguments
    for (int index = optind; index < argc; index++){
        printf("Non-option argument %s\n", argv[index]);
    }

    
    // Setup system
    g_running = true;
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    
    setup();

    // Handle data from stdin
    printf("System started\n");
    
    command_process("help");
    
    printf("> ");fflush(0);
	while(g_running)
	{
		// Setup for select
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = (0.1 * USEC_PER_SEC);
		
		// Read inputs from serial and stdio
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval>0){
			//printf("Data is available now.\n");
			if(FD_ISSET(0, &rfds))
			{
				int b;
				char cmd[50];
				uint8_t outcmd[20];

				memset(outcmd,0,sizeof(outcmd));
				
				FD_CLR(tty_fd,&rfds);
				b = read(0,cmd,sizeof(cmd));
				cmd[b] = 0;
				if(strstr(cmd,"\n")){
				    *strstr(cmd,"\n") =0;
				    if(strlen(cmd)<=0){
                        printf("> ");
                        fflush(0);
				    }
				}
				if(strlen(cmd)<=0) continue;
				
				command_process(cmd);
				if(g_running){
                    printf("> ");
                    fflush(0);
				}
			}
		}else if (retval == -1){
			printf("select() ret -1\n");
		}else{
			//printf("timeout?!\n");
			
		} // end select
		
		//loop
		
	} // end while
	
	
	// Shutdown the system, flush the data
	sys_flush();

	// Shutdown system	
    printf("Terminating\n");
    
    return EXIT_SUCCESS;
}

// EOF
