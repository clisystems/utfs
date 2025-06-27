#include "defs.h"
#include "sys.h"

#include "cbuf.h"

// Types and definitions
// ---------------------------------------------------------------------------

// 8MHz/256 = 31250
// 1/31250 = 0.000032
// 0.010/0.000032 = 312.5
// 8MHz/64 = 125000
// 1/125000 = 0.000008
// 0.005/0.000008 = 625
// 8MHz/16 = 500000
// 1/500000 = 0.000002
// 0.005/0.000002 = 2500
#define TICK_CLOCK			8000000
#define TICK_DIV			16
#define TICK_CLOCKS_PER_SEC	(TICK_CLOCK/TICK_DIV)
#define TICK_PRESCALE		TC_CLOCK_PRESCALER_DIV16
#define TICK_COUNT_VALUE 	(TICK_CLOCKS_PER_SEC/TICKS_PER_SEC)


//#define USART_INTERRUPT_TX

// Local variables
// ---------------------------------------------------------------------------
static struct usart_module cdc_uart_module;
static struct tc_module tick_clock;

static uint32_t m_elapsed_sec=0;
static int tick_sec_count=0;

volatile uint32_t g_timestamp;
volatile uint32_t g_timestamp_ms;
volatile uint32_t g_ticks;

char input_buffer[50];
int input_buffer_index;
CBUF_DEF(uart_rx_buffer,50);

#ifdef USART_INTERRUPT_TX
CBUF_DEF(uart_tx_buffer,120);
bool uart_tx_buffer_transmitting;
#endif

// Local prototypes
// ---------------------------------------------------------------------------
void __attribute__((weak)) sys_stdin_handler(char * input);
static void configure_uart_console(void);
void tiny_putc(char c);


// Public functions
// ---------------------------------------------------------------------------
void sys_init()
{
	struct port_config pin_conf;

	g_ticks = 0;
	g_timestamp_ms=0;
	g_timestamp=0;

	tick_sec_count=0;

	memset(input_buffer,0,sizeof(input_buffer));
	input_buffer_index=0;

	cbuf_init(uart_rx_buffer);

    #ifdef USART_INTERRUPT_TX
	cbuf_init(uart_tx_buffer);
	uart_tx_buffer_transmitting=false;
    #endif

	port_get_config_defaults(&pin_conf);

	// Configure LEDs as outputs, turn them off
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED1_PIN, &pin_conf);
	port_pin_set_config(LED2_PIN, &pin_conf);
	port_pin_set_output_level(LED1_PIN, LED_off);
	port_pin_set_output_level(LED2_PIN, LED_off);

#if 0
	// Set DIP switches as inputs
    pin_conf.direction  = PORT_PIN_DIR_INPUT;
    pin_conf.input_pull = PORT_PIN_PULL_DOWN;
    port_group_set_config(&PORTA,0xFF,&pin_conf);
#endif

	// UART lines
	pin_conf.direction  = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(PIN_PA22, &pin_conf);
    pin_conf.direction  = PORT_PIN_DIR_INPUT;
    port_pin_set_config(PIN_PA23, &pin_conf);


	// configure the tick clock
	struct tc_config tick_config;
	tc_get_config_defaults(&tick_config);
	tick_config.clock_prescaler = TICK_PRESCALE;
	tick_config.count_direction = TC_COUNT_DIRECTION_DOWN;
	tick_config.counter_size    = TC_COUNTER_SIZE_16BIT;
	tick_config.counter_16_bit.value = TICK_COUNT_VALUE;
	tc_init(&tick_clock,TC3,&tick_config);
	tc_enable(&tick_clock);
	// Enable interrupts for the TC module
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_TC3);
	// Enable overflow interrupt
	tick_clock.hw->COUNT16.INTENSET.reg = TC_INTENSET_OVF;

	// Configure UART
	configure_uart_console();

	return;
}


void sys_usart_process()
{
    uint8_t d;
    if(cbuf_pop(uart_rx_buffer,&d)==CBUF_OK)
    {
        tiny_putc(d);
        if(d=='\r' || d=='\n')
        {
            if(input_buffer_index>0)
            {
                input_buffer[input_buffer_index]=0;
                sys_stdin_handler(input_buffer);
                input_buffer[0]=0;
                input_buffer_index=0;
            }
        }else if(input_buffer_index<sizeof(input_buffer)-1)
        {
            input_buffer[input_buffer_index]=d;
            input_buffer_index++;
        }
    }
    return;
}

void sys_status()
{
    printf("** System:\n");
    printf("timestamp: %u\n",g_timestamp);
    printf("tick: %u\n",g_ticks);
    printf("ms per tick: %u\n",SYSTEM_TICK_MS);
    printf("Endianess: %s\n", ((BYTE_ORDER==LITTLE_ENDIAN)?"LITTLE":"BIG"));

    return;
}


// Local functions
// ---------------------------------------------------------------------------
#if 0
void __attribute__((weak)) sys_stdin_handler(char * input)
{
    printf("Got command '%s'\n",input);
    return;
}
#endif
static void configure_uart_console(void)
{
	// Use the USART driver to configure the SERCOM
	// This takes like 3448 (O0) bytes or 1436 (Os) of flash
	struct usart_config usart_conf;

	usart_get_config_defaults(&usart_conf);

	usart_conf.mux_setting = USART_RX_1_TX_0_XCK_1;
	usart_conf.pinmux_pad0 = PINMUX_PA22C_SERCOM3_PAD0;
	usart_conf.pinmux_pad1 = PINMUX_PA23C_SERCOM3_PAD1;
	usart_conf.pinmux_pad2 = PINMUX_UNUSED;
	usart_conf.pinmux_pad3 = PINMUX_UNUSED;
	usart_conf.baudrate    = 38400;

	usart_init(&cdc_uart_module, SERCOM3, &usart_conf);

	usart_enable(&cdc_uart_module);

	// Enable Data empty and data received interrupts
    system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_SERCOM3);

    // Enable RX interrupt
    cdc_uart_module.hw->USART.INTENSET.reg = SERCOM_USART_INTFLAG_RXC;

    #ifdef USART_INTERRUPT_TX
    cdc_uart_module.hw->USART.INTENSET.reg = SERCOM_USART_INTFLAG_TXC;
    #endif
}
void SERCOM3_Handler(void)
{
    struct usart_module *module = &cdc_uart_module;

    SercomUsart *const usart_hw = &(module->hw->USART);

    /* Read and mask interrupt flag register */
    uint16_t interrupt_status = usart_hw->INTFLAG.reg;
    interrupt_status &= usart_hw->INTENSET.reg;

    if(interrupt_status & SERCOM_USART_INTFLAG_RXC)
    {
        uint8_t d;
        d = (uint8_t)usart_hw->DATA.reg;
        cdc_uart_module.hw->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_RXC;
        cbuf_try_push(uart_rx_buffer,d);
    }
    if(interrupt_status & SERCOM_USART_INTFLAG_TXC)
    {
        #ifdef USART_INTERRUPT_TX
        uint8_t d;
        if(cbuf_pop(uart_tx_buffer,&d)==CBUF_OK)
        {
            usart_hw->DATA.reg = d;
        }else{
            cdc_uart_module.hw->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_TXC;
            uart_tx_buffer_transmitting=false;
        }
        #else
        cdc_uart_module.hw->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_TXC;
        #endif
    }
    return;
}

void TC3_Handler(void)
{
	struct tc_module *module = &tick_clock;

	// Check if Overflow interrupt has occurred
	if(module->hw->COUNT16.INTFLAG.bit.OVF)
	{
		// Track the ticks in the system
	    g_ticks++;
	    g_timestamp_ms+=SYSTEM_TICK_MS;

		// Toggle LED every tick?
		//port_pin_toggle_output_level(LED0_PIN);

		// Tack

		// Track the seconds
		tick_sec_count++;
		if(tick_sec_count>=TICKS_PER_SEC)
		{
			g_timestamp++;
			//tick_sec_count-=TICKS_PER_SEC)
			tick_sec_count=0;
		}

		// Restart
		tc_set_count_value(module,TICK_COUNT_VALUE);

		// Clear interrupt flag
		module->hw->COUNT8.INTFLAG.reg = TC_INTFLAG_OVF;
	}

	return;

}

#ifdef USART_INTERRUPT_TX
void tiny_put(uint8_t * buf, int length)
{
    if(!buf) return;
    while(length--){ if(cbuf_try_push(uart_tx_buffer,*buf++)!=CBUF_OK) break; }
    if(uart_tx_buffer_transmitting==false){
        uint8_t d;
        if(cbuf_pop(uart_tx_buffer,&d)==CBUF_OK)
        {
            uart_tx_buffer_transmitting=true;
            cdc_uart_module.hw->USART.DATA.reg = d;
        }
    }
    return;
}
void tiny_putc(char c)
{
    tiny_put((uint8_t*)&c,1);
    return;
}
#else
// TODO: These take forever, so put all this in a circbuff
// and send it out from ISR
int _fwrite(char *str, int len)
{
    if(!str) return 0;
    while(len--){ tiny_putc(*str++); }
    return len;
}
void tiny_putc(char c)
{
    cdc_uart_module.hw->USART.DATA.reg = c;
    while(!cdc_uart_module.hw->USART.INTFLAG.bit.TXC);

    cdc_uart_module.hw->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_TXC;
}
#endif

// EOF
