#ifndef __DEFS_H__
#define __DEFS_H__

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "sys.h"

// Enabled driver libraries
#include <compiler.h>
#include <status_codes.h>
#include <interrupt.h>
#include <port.h>
#include <parts.h>
#include <sercom.h>
#include <usart.h>
#include <tc.h>
#include <clock.h>
#include <gclk.h>
#include <system.h>
#include <pinmux.h>
#include <system_interrupt.h>
#include <power.h>
#include <reset.h>

#ifndef nop
#define nop()                   asm("nop")
#endif

#define SYSTEM_TICK_MS          (5)

#define cmp_const(B,S)			(strncmp((B),(S),sizeof(S)-1)==0)
#define pbuf(B,S)       		do{int x;for(x=0;x<(S);x++){ printf("0x%X,",(B)[x]); if((x&0xF)==0xF) printf("\n");} printf("\n");}while(0)

// LED definitions
#define LED1_PIN              	PIN_PB31
#define LED2_PIN				PIN_PA28
#define LED_on                  false
#define LED_off                	!LED_on

#endif
