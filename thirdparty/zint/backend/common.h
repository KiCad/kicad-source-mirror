/* common.h - Header for all common functions in common.c */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2017 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */

/* Used in some logic */
#ifndef __COMMON_H
#define __COMMON_H

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1
#endif

/* The most commonly used set */
#define NEON	"0123456789"

#include "zint.h"
#include <stdlib.h>

#define ustrcpy(target,source) strcpy((char*)target,(const char*)source)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    extern size_t ustrlen(const unsigned char data[]);
    extern int ctoi(const char source);
    extern char itoc(const int source);
    extern void to_upper(unsigned char source[]);
    extern int is_sane(const char test_string[], const unsigned char source[], const size_t length);
    extern void lookup(const char set_string[], const char *table[], const char data, char dest[]);
    extern void bin_append(const int arg, const int length, char *binary);
    extern int posn(const char set_string[], const char data);
    extern int module_is_set(const struct zint_symbol *symbol, const int y_coord, const int x_coord);
    extern void set_module(struct zint_symbol *symbol, const int y_coord, const int x_coord);
    extern int istwodigits(const unsigned char source[], const size_t position);
    extern int parunmodd(const unsigned char llyth);
    extern void expand(struct zint_symbol *symbol, const char data[]);
    extern void unset_module(struct zint_symbol *symbol, const int y_coord, const int x_coord);
    extern int is_stackable(const int symbology);
    extern int is_extendable(const int symbology);
    extern int utf8toutf16(struct zint_symbol *symbol, const unsigned char source[], int vals[], size_t *length);
    extern void set_minimum_height(struct zint_symbol *symbol, const int min_height);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __COMMON_H */
