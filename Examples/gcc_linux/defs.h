#ifndef __DEFS_H__
#define __DEFS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define USEC_PER_SEC	1000000

#define strset(B,S) do{strncpy(&(B),S,sizeof(S)-1);}while(0)
#define pbuf(B,N)	do{int x=0;for(x=0;x<(N);x++){ printf("0x%02X,",(B)[x]); if((x&0xF)==0xF) printf("\n"); }; printf("\n");}while(0)
#define cmp_const(B,S)	(strncmp((B),(S),sizeof(S)-1)==0)

#endif
