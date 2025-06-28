#include <stdint.h>
#include <stdbool.h>

#include <Arduino.h>

#include <EEPROM.h>

#include "utfs.h"

// Support printf on Arduino serial port (40 chars)
#ifdef printf
#undef printf
#endif
#define printf(...)      do{sprintf(m_scratch,__VA_ARGS__);Serial.print(m_scratch);}while(0)
#define cmp_const(B,S)  (strncmp((B),(S),sizeof(S)-1)==0)

// Run a timer at 10ms interval
#define MS_PER_TICK     10

// System variables
uint32_t m_ticks = 0;
int m_tick_ms=0;
int led=0;
char m_scratch[40];


// Data structures used in the system. This is the data
// that will be written to the UTFS files.
struct system_data{
    char serialnumber[10];
    char modelnumber[10];
};

struct application_data{
    uint8_t led_speed;
};

#define LED_SPEED_OFF       0
#define LED_SPEED_SLOW      1
#define LED_SPEED_MED       2
#define LED_SPEED_FAST      3

struct system_data sysdata;
struct application_data appdata;


// Two UTFS file instances
utfs_file_t sysfile;
utfs_file_t appfile;


// timer 1 functions
// This is the clock timer
void timer1_setup(int ms)
{
    noInterrupts();
    m_tick_ms = ms;
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1C = 0;

    // set Compare Match value:
    // ATmega328 crystal is 16MHz
    // timer resolution = 1/( 16E6 /64) = 4E-6 seconds for 64 prescaling
    // target time = timer resolution * (# timer counts + 1)
    // so timer counts = (target time)/(timer resolution) -1
    // For 1 ms interrupt, timer counts = 1E-3/4E-6 - 1 = 249
    OCR1A = (uint16_t)(ms * 249);
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS11) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    TCNT1 = 0;
    interrupts();
    return;
}
ISR(TIMER1_COMPA_vect)
{
    m_ticks+=m_tick_ms;
}



// Serial 'Terminal'
char m_term_input[20];
uint8_t m_term_input_count;
void terminal_command(char * cmd)
{
    if(strlen(cmd)<=0) return;
    printf("Command: '%s'\n",cmd);

    // Dump the data structures
    if(cmp_const(cmd,"dump")){
        printf("Sys:\n");
        printf("  File Sig: 0x%X\n",sysfile.signature);
        printf("  Serial: %s\n",sysdata.serialnumber);
        printf("  Model: %s\n",sysdata.modelnumber);
        printf("App:\n");
        printf("  File Sig: 0x%X\n",appfile.signature);
        printf("  Speed: %d\n",appdata.led_speed);

    // UTFS load and save
    }else if(cmp_const(cmd,"load")){
        utfs_load();
    }else if(cmp_const(cmd,"save")){
        utfs_save();


    // Set the LED speed
    }else if(cmp_const(cmd,"slow")){
        appdata.led_speed=LED_SPEED_SLOW;
    }else if(cmp_const(cmd,"medium")){
        appdata.led_speed=LED_SPEED_MED;
    }else if(cmp_const(cmd,"fast")){
        appdata.led_speed=LED_SPEED_FAST;
    }else if(cmp_const(cmd,"off")){
        appdata.led_speed=LED_SPEED_OFF;

    // Set the 'serial' and 'model' data
    }else if(cmp_const(cmd,"serial")){
        char * pch;
        pch = strtok(cmd," ");
        pch = strtok(NULL," ");
        if(pch){
            snprintf(sysdata.serialnumber,sizeof(sysdata.serialnumber)-1,"%s",pch);
            printf("Serial: %s\n",sysdata.serialnumber);
        }
    }else if(cmp_const(cmd,"model")){
        char * pch;
        pch = strtok(cmd," ");
        pch = strtok(NULL," ");
        if(pch){
            snprintf(sysdata.modelnumber,sizeof(sysdata.modelnumber)-1,"%s",pch);
            printf("Model: %s\n",sysdata.modelnumber);
        }


    // Dump the first 255 bytes of the EEPROM
    }else if(cmp_const(cmd,"eeprom")){
        uint16_t x;
        printf("EEPROM length: %d\n",EEPROM.length());
        for(x=0;x<0xFF;x++)
        {
            printf("0x%02X,",EEPROM.read(x));
            if((x%16)==0 && x>0) Serial.print('\n');
        }

    // Blank the first 255 bytes of the EEPROM
    }else if(cmp_const(cmd,"cleareeprom")){
        uint16_t x;
        uint8_t val=0;
        printf("EEPROM length: %d\n",EEPROM.length());
        for(x=0;x<0xFF;x++)
        {
            EEPROM.write(x,val);
        }
    }
    return;
}
void terminal_process()
{
    char c;
    if (Serial.available()<=0) return;

    c = Serial.read();
    if(c!='\n')
    {
        if(m_term_input_count<sizeof(m_term_input)-1)
        {
            m_term_input[m_term_input_count]=c;
            m_term_input_count++;
        }
    }else{
        m_term_input[m_term_input_count]=0;
        terminal_command(m_term_input);
        memset(m_term_input,0,sizeof(m_term_input));
        m_term_input_count=0;
    }
    return;
}



// UTFS required functions sys_write and sys_read
// Write data to flash, return number of written bytes
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length)
{
    uint16_t x;
    uint8_t * data;
    data = (uint8_t*)ptr;
    if(!ptr) return 0;
    // Arduino UNO has 1024 bytes of EEPROM
    if(address>1023) return 0;
    if(address+length>1023) length=1023-address;
    for(x=0;x<length;x++)
    {
        if(ptr) EEPROM.write(address+x,data[x]);
    }
    return length;
}

// Read data from flash, return number of read bytes
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length)
{
    uint16_t x;
    uint8_t * data;
    data = (uint8_t*)ptr;
    if(!ptr) return 0;
    // Arduino UNO has 1024 bytes of EEPROM
    if(address>1023) return 0;
    if(address+length>1023) length=1023-address;
    for(x=0;x<length;x++)
    {
        if(ptr) data[x] = EEPROM.read(address+x);
    }
    return length;
}




// Arduino
void setup()
{
    utfs_result_e res;

    // Blank variables
    memset(&sysfile,0,sizeof(sysfile));
    memset(&appfile,0,sizeof(appfile));
    memset(m_scratch,0,sizeof(m_scratch));
    pinMode(LED_BUILTIN, OUTPUT);

    // Setup subsystems
    Serial.begin(9600);
    timer1_setup(MS_PER_TICK);
    utfs_init(true); // true=verbose output, false=no output from UTFS driver

    // Register the UTFS files
    sprintf(sysfile.filename,"system");
    sysfile.data = &(sysdata);
    sysfile.size = sizeof(sysdata);
    utfs_register(&sysfile, UTFS_NOFLAGS, UTFS_NOOPT);

    sprintf(appfile.filename,"appdata");
    appfile.data = &(appdata);
    appfile.size = sizeof(appdata);
    utfs_register(&appfile, UTFS_NOFLAGS, UTFS_NOOPT);

    // Load the UTFS data

    // NOTE: The first load on a new Arduino will return
    // RES_INVALID_FS since the file system is blank on the EEPROM

    res = utfs_load();

    // Example 1: How to handle RES_INVALID_FS on first run
    #if 0
    if(res==RES_INVALID_FS){
        // default data
        // save?
    }
    #endif

    // Example 2: Use the 'signature' variable in the file

    // Check the signature after the data is loaded to see if
    // they match an expected value. If they do not match, then the
    // data is not correct and set the default values
    if(sysfile.signature != 0xA1){
        printf("Default sysdata\n");
        memset(&sysdata,0,sizeof(sysdata));
        sysfile.signature=0xA1;
    }
    if(appfile.signature != 0xF2){

        if(appfile.signature==0xA2){
            // upgrade the data from an earlier structure!
        }else{
            printf("Default appdata\n");
            memset(&appdata,0,sizeof(appdata));
            appfile.signature=0xF2;
            appdata.led_speed = LED_SPEED_MED;
        }
    }

    printf("System ready\n");

    return;
}

// Arduino mainloop
uint32_t lasttime=0;
uint32_t last_tick=0;
uint8_t led_count=0;
void loop()
{
    // Every 100ms
    if(m_ticks!=last_tick && (m_ticks%100)==0)
    {
	    last_tick=m_ticks;

	    // Toggle the LED based on the application data structure
	    if(appdata.led_speed==LED_SPEED_FAST)
	    {
            // Toggle LED 5HZ
            led = !led;

	    }else if(appdata.led_speed==LED_SPEED_MED){
	        // Toggle 1Hz
	        led_count++;
	        if(led_count>5){
	            led_count=0;
	            led = !led;
	        }

	    }else if(appdata.led_speed==LED_SPEED_SLOW){
	        // Toggle 2Hz
            led_count++;
            if(led_count>10){
                led_count=0;
                led = !led;
            }
	    }else if(appdata.led_speed==LED_SPEED_OFF){
	        led=0;
	    }
	    digitalWrite(LED_BUILTIN, led);
        
    }

    // Handle serial input
    terminal_process();

    return;
}

// EOF

