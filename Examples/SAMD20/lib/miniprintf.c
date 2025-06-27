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

#ifdef ENABLE_MINIPRINTF

#include <stdarg.h>
#include <stdint.h>

/* External function prototypes (defined in syscalls.c) */
extern int _fwrite(char *str, int len);

/* Private function prototypes */
static int str_calclength(const char *fmt, va_list va);
static int str_fill(char *buf, const char *fmt, va_list va);

// Yes, has basic float support to 2 decimals
//#define ENABLE_FLOAT

// Allow extra characters ._? to be padding characters
//#define ENABLE_EXTRA_PAD_CHARS

// Public functions
// ----------------------------------------------------------------------------
int sprintf(char *buf, const char *fmt, ...)
{
    int length;
    va_list va;
    va_start(va, fmt);
    length = str_fill(buf, fmt, va);
    va_end(va);
    return length;
}


int printf(const char *fmt, ...)
{
    int length = 0;
    va_list va;
    va_start(va, fmt);
    length = str_calclength(fmt, va);
    va_end(va);
    {
        char buf[length]; // the output string is done on the stack
        va_start(va, fmt);
        length = str_fill(buf, fmt, va);
        _fwrite(buf, length);
        va_end(va);
    }
    return length;
}

// Private functions
// ----------------------------------------------------------------------------
static void printnum(char **buf, uint32_t d, int base)
{
    int div = 1;
    int num;

    // Determine the maxumum number of digits by finding
    // a base multiple larger than the target
    while(d/div >= base)
        div = div * base;

    // Print each digit
    while(div > 0)
    {
        // Not super efficient but logical
        // for any base
        num = d/div;
        d = d%div;
        div = div / base;

        // Print the actual number
        if (num > 9)
            *((*buf)++) = (num-10) + 'A';
        else
            *((*buf)++) = num + '0';
    }
    return;
}
static int calcnumlength(unsigned int d, int base)
{
    int div = 1;
    int digits=1;

    // Determine the maxumum number of digits by finding
    // a base multiple larger than the target
    while(d/div >= base){
        div = div * base;
        digits++;
    }

    return digits;
}

#ifdef ENABLE_FLOAT
static void printfloat(char **buf, double d)
{
    int i,whole,part;
    i = (int)(d*100.0);
    if(i<0){
        i*=-1;
        *((*buf)++) = '-';
    }

    part = i%100;
    whole = i/100;

    printnum(buf,whole,10);
    *((*buf)++) = '.';
    printnum(buf,part,10);
    return;
}
#endif

static int is_fmtchar(char c)
{
    #ifdef ENABLE_EXTRA_PAD_CHARS
    return (c==' ' || c=='0' || c=='.'
            || c=='_' || c=='?');
    #else
    return (c==' ' || c=='0');
    #endif
}
static int str_calclength(const char *fmt, va_list va)
{
    int length = 0;
    int pad=0;
    while (*fmt)
    {
        if (*fmt == '%')
        {
            ++fmt;
            // Handle long and long long characters, just drop them
            if(*fmt=='l'){
                fmt++;
                if(*fmt=='l') fmt++;
            }
            // Handle 0 or [space] for padding
            if(is_fmtchar(*fmt)){
                fmt++;
            }
            // Handle single digit number for padding
            if(*fmt>='0' && *fmt<='9'){
                pad=(*fmt)-'0';
                ++fmt;
                length+=pad;
            }
            // Handle the actual pattern
            switch (*fmt){
            case 'c':
                va_arg(va, int);
                // chars are always 1 space
                length++;
                break;
            #ifdef ENABLE_FLOAT
            case 'f':
                va_arg(va,double);
                length+=10;
                break;
            #endif
            case 'd': // No break
            case 'i': // No break
            case 'u':
                va_arg(va, int);
                // int32_t max is -2^31 = -2147483648 (11 characters)
                length += 11;
                break;
            case 's':
            {
                // Count printable characters
                char * str = va_arg(va, char *);
                while(*str++) length++;
            }
            break;
            case 'x':
            case 'X':
                va_arg(va, unsigned int);
                // 32 bit int as hex is max 8 chars
                length += 8;
                break;
            default:
                length++;
                break;
            }
        }else{
            length++;
        }
        fmt++;
    }
    return length;
}

static int str_fill(char *buf, const char *fmt, va_list va)
{
    char *cptr;
    char cpad=' ';
    int npad=0;

    // Save starting pointer
    cptr=buf;

    // Loop over
    while(*fmt)
    {
        /* Character needs formating? */
        if (*fmt == '%')
        {
            npad=0;
            fmt++;
            // Handle long and long long characters, just drop them
            if(*fmt=='l'){
                fmt++;
                if(*fmt=='l') fmt++;
            }
            // Handle padding character
            if(is_fmtchar(*fmt)){
                cpad=*fmt;
                fmt++;
            }
            // Handle single digit number for padding
            if(*fmt>='0' && *fmt<='9'){
                npad=(*fmt)-'0';
                ++fmt;
            }
            // Handle the actual pattern
            switch (*fmt)
            {
              case 'c':
                  *buf = va_arg(va, int32_t);
                  buf++;
                  break;
              #ifdef ENABLE_FLOAT
              case 'f':
              {
                  float var;
                  var = va_arg(va,double);
                  printfloat(&buf,var);
              }
              break;
              #endif
              case 'd':
              case 'i':
              {
                  int32_t val;
                  val = va_arg(va, int32_t);
                  if (val < 0)
                  {
                      val *= -1;
                      *buf = '-';
                      buf++;
                  }
                  if(npad){
                      int pad;
                      pad = npad-calcnumlength(val, 10);
                      if(pad>0) while(pad--){ *buf=cpad; buf++; };
                  }
                  printnum(&buf, val, 10);
              }
              break;
              case 's':
              {
                  char * arg;
                  arg = va_arg(va, char *);
                  while (*arg) *buf++ = *arg++;
              }
              break;
              case 'u':
              {
                  uint32_t val;
                  val = va_arg(va, uint32_t);
                  if(npad){
                      int pad;
                      pad = npad-calcnumlength(val, 10);
                      if(pad>0) while(pad--){ *buf=cpad; buf++; };
                  }
                  printnum(&buf, val, 10);
              }
              break;
              case 'x':
              case 'X':
              {
                  int32_t val;
                  val = va_arg(va, int32_t);
                  if(npad){
                      int pad;
                      pad = npad-calcnumlength(val, 10);
                      if(pad>0) while(pad--){ *buf=cpad; buf++; };
                  }
                  printnum(&buf, val, 16);
              }
              break;
              case '%':
                  *buf = '%';
                  buf++;
                  break;
            }
            fmt++;
        }else{
            // Non format text just copy
            *buf++ = *fmt++;
        }
    }
    // Null terminate
    *buf = 0;

    // Return the site of the string
    return (int32_t)(buf - cptr);
}

#endif

// EOF
