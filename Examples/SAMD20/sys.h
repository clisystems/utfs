#ifndef __SYS_H__
#define __SYS_H__

// at 10ms per tick, and 2^32 ticks in a uint32_t, this means
// 42949672960ms = 4294967.296 sec = 71582.788266667 min = 1193.046471111 hr = 49.71026963 days until overflow

#define MS_PER_TICK		(SYSTEM_TICK_MS)
#define TICKS_PER_SEC	(1000/MS_PER_TICK)
#define TICKS_PER_100MS (100/MS_PER_TICK)

// The only 3 externs, for the system ticks and elapsed timestamps
extern volatile uint32_t g_timestamp;
extern volatile uint32_t g_timestamp_ms;
extern volatile uint32_t g_ticks;



void sys_init(void);

void sys_usart_process();

void sys_status();

#endif
