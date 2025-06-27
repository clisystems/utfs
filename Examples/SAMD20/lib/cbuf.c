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
#include <string.h>

#include "cbuf.h"

/* Helper macros */
/* ------------------------------------------------------------------------- */
#define GET_NEXT(I,M)       ( ( ((I)+1) >= (M) ) ? 0 : ((I)+1))
#define GET_UNTIL_WRAP(B)   ( (B)->maxlen-(B)->tail )
#define CLEAR_BUFF(B)       do{ (B)->head = (B)->tail = (B)->used = 0; }while(0)

/* Public functions */
/* ------------------------------------------------------------------------- */
int cbuf_init(cbuf_handle_t  buf)
{
    if(!buf) return CBUF_ERR_NULL;

    CLEAR_BUFF(buf);
    if(buf->buffer) memset(buf->buffer,0,buf->maxlen);

    return CBUF_OK;
}
bool cbuf_is_full(cbuf_handle_t  buf)
{
    int next;
    if(!buf) return false;
    next = GET_NEXT(buf->head,buf->maxlen);
    return (next==buf->tail);
}

int cbuf_push(cbuf_handle_t buf, uint8_t data)
{
    int next;

    if(!buf) return CBUF_ERR_NULL;

    /* Increment head and handle wrap */
    next = GET_NEXT(buf->head,buf->maxlen);

    /* Load data and then move+1 */
    buf->buffer[buf->head] = data;
    buf->head = next;
    buf->used++;

    return CBUF_OK;
}
int cbuf_try_push(cbuf_handle_t buf, uint8_t data)
{
    int next;

    if(!buf) return CBUF_ERR_NULL;

    next = GET_NEXT(buf->head,buf->maxlen);

    /* If we are full, don't add */
    if(next==buf->tail)
        return CBUF_ERR_FULL;

    /* Load data and then move */
    buf->buffer[buf->head] = data;
    buf->head = next;
    buf->used++;

    return CBUF_OK;
}

int cbuf_peak(cbuf_handle_t buf, uint8_t *data)
{
    if(!buf||!data) return CBUF_ERR_NULL;

    if(buf->head == buf->tail)
        return CBUF_ERR_EMPTY;

    /* Read data */
    *data = buf->buffer[buf->tail];

    return CBUF_OK;
}

int cbuf_pop(cbuf_handle_t buf, uint8_t *data)
{
    int next;

    if(!buf||!data) return CBUF_ERR_NULL;

    if(buf->head == buf->tail)
        return CBUF_ERR_EMPTY;

    /* Increment the tail */
    next = GET_NEXT(buf->tail,buf->maxlen);

    /* Read data and then move */
    *data = buf->buffer[buf->tail];
    buf->tail = next;
    buf->used--;

    return CBUF_OK;
}
int cbuf_pop_len(cbuf_handle_t buf, uint8_t *data, int size)
{
    int next;

    if(!buf||!data) return CBUF_ERR_NULL;

    /* Empty buffers read 0 bytes */
    if (buf->head == buf->tail)
        return 0;

    /* Can only read the max bytes in the buffer */
    if(size>buf->used) size=buf->used;

    /* Get the data, handling wrap around */
    if(buf->head < buf->tail)
    {
        int x;
        x = GET_UNTIL_WRAP(buf);
        if(size>x){
            /* Handle reading, wrapping, and then reading the rest */
            memcpy(data,&(buf->buffer[buf->tail]),x);
            memcpy(&(data[x]),buf->buffer,size-x);
        }else{
            /* normal read */
            memcpy(data,&(buf->buffer[buf->tail]),size);
        }
    }else{
        /* normal read */
        memcpy(data,&(buf->buffer[buf->tail]),size);
    }

    /* Increment the tail by size, handling wrap around */
    next = buf->tail+size;
    if(next>=buf->maxlen) next-=buf->maxlen;

    /* If the read empties the buffer, reset it */
    if(next == buf->head)
    {
        CLEAR_BUFF(buf);

    /* Else just incremnet the tail */
    }else{
        buf->tail = next;
        buf->used -= size;
    }

    return size;
}

/* EOF */
