/* common.c - Contains functions needed for a number of barcodes */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2017 Robin Stuart <rstuart114@gmail.com>

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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

/* Local replacement for strlen() with unsigned char strings */
size_t ustrlen(const unsigned char data[]) {
    return strlen((const char*) data);
}

/* Converts a character 0-9 to its equivalent integer value */
int ctoi(const char source) {
    if ((source >= '0') && (source <= '9'))
        return (source - '0');
    if ((source >= 'A') && (source <= 'F'))
        return (source - 'A' + 10);
    if ((source >= 'a') && (source <= 'f'))
        return (source - 'a' + 10);
    return -1;
}


/* Convert an integer value to a string representing its binary equivalent */
void bin_append(const int arg, const int length, char *binary) {
    int i;
    int start;
    size_t posn = strlen(binary);

    start = 0x01 << (length - 1);

    for (i = 0; i < length; i++) {
        binary[posn + i] = '0';
        if (arg & (start >> i)) {
            binary[posn + i] = '1';
        }
    }
    binary[posn + length] = '\0';

    return;
}

/* Converts an integer value to its hexadecimal character */
char itoc(const int source) {
    if ((source >= 0) && (source <= 9)) {
        return ('0' + source);
    } else {
        return ('A' + (source - 10));
    }
}
/* Converts lower case characters to upper case in a string source[] */
void to_upper(unsigned char source[]) {
    size_t i, src_len = ustrlen(source);

    for (i = 0; i < src_len; i++) {
        if ((source[i] >= 'a') && (source[i] <= 'z')) {
            source [i] = (source[i] - 'a') + 'A';
        }
    }
}

/* Verifies that a string only uses valid characters */
int is_sane(const char test_string[], const unsigned char source[], const size_t length) {
    unsigned int j;
    size_t i, lt = strlen(test_string);

    for (i = 0; i < length; i++) {
        unsigned int latch = FALSE;
        for (j = 0; j < lt; j++) {
            if (source[i] == test_string[j]) {
                latch = TRUE;
                break;
            }
        }
        if (!(latch)) {
            return ZINT_ERROR_INVALID_DATA;
        }
    }

    return 0;
}

/* Replaces huge switch statements for looking up in tables */
void lookup(const char set_string[], const char *table[], const char data, char dest[]) {
    size_t i, n = strlen(set_string);

    for (i = 0; i < n; i++) {
        if (data == set_string[i]) {
            strcat(dest, table[i]);
        }
    }
}

/* Returns the position of data in set_string */
int posn(const char set_string[], const char data) {
    int i, n = (int)strlen(set_string);

    for (i = 0; i < n; i++) {
        if (data == set_string[i]) {
         return i;
        }
    }
   return -1;
}

/* Return true (1) if a module is dark/black, otherwise false (0) */
int module_is_set(const struct zint_symbol *symbol, const int y_coord, const int x_coord) {
    return (symbol->encoded_data[y_coord][x_coord / 7] >> (x_coord % 7)) & 1;
}

/* Set a module to dark/black */
void set_module(struct zint_symbol *symbol, const int y_coord, const int x_coord) {
    symbol->encoded_data[y_coord][x_coord / 7] |= 1 << (x_coord % 7);
}

/* Set (or unset) a module to white */
void unset_module(struct zint_symbol *symbol, const int y_coord, const int x_coord) {
    symbol->encoded_data[y_coord][x_coord / 7] &= ~(1 << (x_coord % 7));
}

/* Expands from a width pattern to a bit pattern */
void expand(struct zint_symbol *symbol, const char data[]) {

    size_t reader, n = strlen(data);
    int writer, i;
    char latch;

    writer = 0;
    latch = '1';

    for (reader = 0; reader < n; reader++) {
        for (i = 0; i < ctoi(data[reader]); i++) {
            if (latch == '1') {
                set_module(symbol, symbol->rows, writer);
            }
            writer++;
        }

        latch = (latch == '1' ? '0' : '1');
    }

    if (symbol->symbology != BARCODE_PHARMA) {
        if (writer > symbol->width) {
            symbol->width = writer;
        }
    } else {
        /* Pharmacode One ends with a space - adjust for this */
        if (writer > symbol->width + 2) {
            symbol->width = writer - 2;
        }
    }
    symbol->rows = symbol->rows + 1;
}

/* Indicates which symbologies can have row binding */
int is_stackable(const int symbology) {
    if (symbology < BARCODE_PDF417) {
        return 1;
    }

    switch (symbology) {
        case BARCODE_CODE128B:
        case BARCODE_ISBNX:
        case BARCODE_EAN14:
        case BARCODE_NVE18:
        case BARCODE_KOREAPOST:
        case BARCODE_PLESSEY:
        case BARCODE_TELEPEN_NUM:
        case BARCODE_ITF14:
        case BARCODE_CODE32:
        case BARCODE_CODABLOCKF:
            return 1;
    }

    return 0;
}

/* Indicates which symbols can have addon (EAN-2 and EAN-5) */
int is_extendable(const int symbology) {
    if (symbology == BARCODE_EANX) {
        return 1;
    }
    if (symbology == BARCODE_UPCA) {
        return 1;
    }
    if (symbology == BARCODE_UPCE) {
        return 1;
    }
    if (symbology == BARCODE_ISBNX) {
        return 1;
    }
    if (symbology == BARCODE_UPCA_CC) {
        return 1;
    }
    if (symbology == BARCODE_UPCE_CC) {
        return 1;
    }
    if (symbology == BARCODE_EANX_CC) {
        return 1;
    }

    return 0;
}

int istwodigits(const unsigned char source[], const size_t position) {
    if ((source[position] >= '0') && (source[position] <= '9')) {
        if ((source[position + 1] >= '0') && (source[position + 1] <= '9')) {
            return 1;
        }
    }

    return 0;
}

int utf8toutf16(struct zint_symbol *symbol, const unsigned char source[], int vals[], size_t *length) {
    size_t bpos;
    int    jpos, error_number;
    int next;

    bpos = 0;
    jpos = 0;
    error_number = 0;
    next = 0;

    do {
        if (source[bpos] <= 0x7f) {
            /* 1 byte mode (7-bit ASCII) */
            vals[jpos] = source[bpos];
            next = bpos + 1;
            jpos++;
        } else {
            if ((source[bpos] >= 0x80) && (source[bpos] <= 0xbf)) {
                strcpy(symbol->errtxt, "240: Corrupt Unicode data");
                return ZINT_ERROR_INVALID_DATA;
            }
            if ((source[bpos] >= 0xc0) && (source[bpos] <= 0xc1)) {
                strcpy(symbol->errtxt, "241: Overlong encoding not supported");
                return ZINT_ERROR_INVALID_DATA;
            }

            if ((source[bpos] >= 0xc2) && (source[bpos] <= 0xdf)) {
                /* 2 byte mode */
                vals[jpos] = ((source[bpos] & 0x1f) << 6) + (source[bpos + 1] & 0x3f);
                next = bpos + 2;
                jpos++;
            } else
                if ((source[bpos] >= 0xe0) && (source[bpos] <= 0xef)) {
                /* 3 byte mode */
                vals[jpos] = ((source[bpos] & 0x0f) << 12) + ((source[bpos + 1] & 0x3f) << 6) + (source[bpos + 2] & 0x3f);
                next = bpos + 3;
                jpos++;
            } else
                if (source[bpos] >= 0xf0) {
                strcpy(symbol->errtxt, "242: Unicode sequences of more than 3 bytes not supported");
                return ZINT_ERROR_INVALID_DATA;
            }
        }

        bpos = next;

    } while (bpos < *length);
    *length = jpos;

    return error_number;
}


void set_minimum_height(struct zint_symbol *symbol, const int min_height) {
    /* Enforce minimum permissable height of rows */
    int fixed_height = 0;
    int zero_count = 0;
    int i;

    for (i = 0; i < symbol->rows; i++) {
        fixed_height += symbol->row_height[i];

        if (symbol->row_height[i] == 0) {
            zero_count++;
        }
    }

    if (zero_count > 0) {
        if (((symbol->height - fixed_height) / zero_count) < min_height) {
            for (i = 0; i < symbol->rows; i++) {
                if (symbol->row_height[i] == 0) {
                    symbol->row_height[i] = min_height;
                }
            }
        }
    }
}

