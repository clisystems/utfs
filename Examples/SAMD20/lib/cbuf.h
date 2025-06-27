/***********************************************************************
* Copyright (C) 2021 Prosource (CLI Systems LLC)
* https://prosource.dev
*
* License:
* THIS SOURCE CODE IS PART OF THE prosource.dev EMBEDDED SYSTEM
* KNOWLEDGE-BASE, AND IS PROVIDED UNDER THE TERMS OF THE PROSOURCE
* USAGE LICENSE.  Please obtain a copy of the Prosource Usage License
* at https://prosource.dev/license/ and read it before using this file.
*
* This source code is only redistributable in accordance with the
* Prosource Usage License which prohibits any public publication or
* listing reguardless of profit.
*
* This source code and all software distributed under the License are
* distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
* EXPRESS OR IMPLIED
*
***********************************************************************/
#ifndef __CIRC_BUFF_H__
#define __CIRC_BUFF_H__

#include <stdint.h>
#include <stdbool.h>

/* Return values from cbuf function calls */
#define CBUF_OK         0
#define CBUF_ERR_NULL   -1
#define CBUF_ERR_FULL   -2
#define CBUF_ERR_EMPTY  -3

/* Internal structures for the buffer */
typedef struct {
    uint8_t * const buffer;
    int head;
    int tail;
    const int maxlen;
    int used;
}cbuf_t;
typedef cbuf_t * cbuf_handle_t;

/* Definition macro */
/*   CBUF_DEF(<buffer handle name>, <size of buffer>) */
#define CBUF_DEF(ptrname,size)  \
uint8_t ptrname##_data[size];   \
cbuf_t ptrname##_obj = {        \
    .buffer = ptrname##_data,   \
    .head = 0,                  \
    .tail = 0,                  \
    .maxlen = size,             \
    .used = 0,                  \
}; \
cbuf_handle_t ptrname = &(ptrname##_obj)


/* Public functions */
#ifdef __cplusplus
extern "C"
{
#endif

int cbuf_init(cbuf_handle_t buf);

bool cbuf_is_full(cbuf_handle_t buf);

int cbuf_push(cbuf_handle_t buf, uint8_t data);
int cbuf_try_push(cbuf_handle_t buf, uint8_t data);

int cbuf_peak(cbuf_handle_t buf, uint8_t *data);
int cbuf_pop(cbuf_handle_t buf, uint8_t *data);
int cbuf_pop_len(cbuf_handle_t buf, uint8_t *data, int size);

#ifdef __cplusplus
}
#endif

/* Lightweight macro 'functions' */
#define cbuf_ret_is_error(R)    ((bool)((R)<0))
#define cbuf_is_empty(B)        ((bool)((B)->head == (B)->tail))
#define cbuf_get_used(B)        ((B)->used)


#endif
