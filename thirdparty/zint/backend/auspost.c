/* auspost.c - Handles Australia Post 4-State Barcode */

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

#define GDSET 	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz #"

static const char *AusNTable[10] = {
    "00", "01", "02", "10", "11", "12", "20", "21", "22", "30"
};

static const char *AusCTable[64] = {
    "222", "300", "301", "302", "310", "311", "312", "320", "321", "322",
    "000", "001", "002", "010", "011", "012", "020", "021", "022", "100", "101", "102", "110",
    "111", "112", "120", "121", "122", "200", "201", "202", "210", "211", "212", "220", "221",
    "023", "030", "031", "032", "033", "103", "113", "123", "130", "131", "132", "133", "203",
    "213", "223", "230", "231", "232", "233", "303", "313", "323", "330", "331", "332", "333",
    "003", "013"
};

static const char *AusBarTable[64] = {
    "000", "001", "002", "003", "010", "011", "012", "013", "020", "021",
    "022", "023", "030", "031", "032", "033", "100", "101", "102", "103", "110", "111", "112",
    "113", "120", "121", "122", "123", "130", "131", "132", "133", "200", "201", "202", "203",
    "210", "211", "212", "213", "220", "221", "222", "223", "230", "231", "232", "233", "300",
    "301", "302", "303", "310", "311", "312", "313", "320", "321", "322", "323", "330", "331",
    "332", "333"
};

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "reedsol.h"
#ifdef _MSC_VER
#define inline _inline
#endif

static inline char convert_pattern(char data, int shift) {
    return (data - '0') << shift;
}

/* Adds Reed-Solomon error correction to auspost */
void rs_error(char data_pattern[]) {
    size_t reader, triple_writer = 0;
    char triple[31];
    unsigned char result[5];

    for (reader = 2; reader < strlen(data_pattern); reader += 3, triple_writer++) {
        triple[triple_writer] = convert_pattern(data_pattern[reader], 4)
                + convert_pattern(data_pattern[reader + 1], 2)
                + convert_pattern(data_pattern[reader + 2], 0);
    }

    rs_init_gf(0x43);
    rs_init_code(4, 1);
    rs_encode(triple_writer, (unsigned char*) triple, result);

    for (reader = 4; reader > 0; reader--) {
        strcat(data_pattern, AusBarTable[(int) result[reader - 1]]);
    }
    rs_free();
}

/* Handles Australia Posts's 4 State Codes */
int australia_post(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* Customer Standard Barcode, Barcode 2 or Barcode 3 system determined automatically
       (i.e. the FCC doesn't need to be specified by the user) dependent
       on the length of the input string */

    /* The contents of data_pattern conform to the following standard:
       0 = Tracker, Ascender and Descender
       1 = Tracker and Ascender
       2 = Tracker and Descender
       3 = Tracker only */
    int error_number;
    int writer;
    unsigned int loopey, reader;
    size_t h;

    char data_pattern[200];
    char fcc[3] = {0, 0, 0}, dpid[10];
    char localstr[30];

    error_number = 0;
    strcpy(localstr, "");

    /* Do all of the length checking first to avoid stack smashing */
    if (symbol->symbology == BARCODE_AUSPOST) {
        /* Format control code (FCC) */
        switch (length) {
            case 8:
                strcpy(fcc, "11");
                break;
            case 13:
                strcpy(fcc, "59");
                break;
            case 16:
                strcpy(fcc, "59");
                error_number = is_sane(NEON, source, length);
                break;
            case 18:
                strcpy(fcc, "62");
                break;
            case 23:
                strcpy(fcc, "62");
                error_number = is_sane(NEON, source, length);
                break;
            default:
                strcpy(symbol->errtxt, "401: Auspost input is wrong length");
                return ZINT_ERROR_TOO_LONG;
        }
        if (error_number == ZINT_ERROR_INVALID_DATA) {
            strcpy(symbol->errtxt, "402: Invalid characters in data");
            return error_number;
        }
    } else {
        int zeroes;
        if (length > 8) {
            strcpy(symbol->errtxt, "403: Auspost input is too long");
            return ZINT_ERROR_TOO_LONG;
        }
        switch (symbol->symbology) {
            case BARCODE_AUSREPLY: strcpy(fcc, "45");
                break;
            case BARCODE_AUSROUTE: strcpy(fcc, "87");
                break;
            case BARCODE_AUSREDIRECT: strcpy(fcc, "92");
                break;
        }

        /* Add leading zeros as required */
        zeroes = 8 - length;
        memset(localstr, '0', zeroes);
        localstr[zeroes] = '\0';
    }

    strcat(localstr, (char*) source);
    h = strlen(localstr);
    error_number = is_sane(GDSET, (unsigned char *) localstr, h);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "404: Invalid characters in data");
        return error_number;
    }

    /* Verifiy that the first 8 characters are numbers */
    memcpy(dpid, localstr, 8);
    dpid[8] = '\0';
    error_number = is_sane(NEON, (unsigned char *) dpid, strlen(dpid));
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "405: Invalid characters in DPID");
        return error_number;
    }

    /* Start character */
    strcpy(data_pattern, "13");

    /* Encode the FCC */
    for (reader = 0; reader < 2; reader++) {
        lookup(NEON, AusNTable, fcc[reader], data_pattern);
    }

    /* printf("AUSPOST FCC: %s  ", fcc); */

    /* Delivery Point Identifier (DPID) */
    for (reader = 0; reader < 8; reader++) {
        lookup(NEON, AusNTable, dpid[reader], data_pattern);
    }

    /* Customer Information */
    if (h > 8) {
        if ((h == 13) || (h == 18)) {
            for (reader = 8; reader < h; reader++) {
                lookup(GDSET, AusCTable, localstr[reader], data_pattern);
            }
        } else if ((h == 16) || (h == 23)) {
            for (reader = 8; reader < h; reader++) {
                lookup(NEON, AusNTable, localstr[reader], data_pattern);
            }
        }
    }

    /* Filler bar */
    h = strlen(data_pattern);
    switch (h) {
        case 22:
        case 37:
        case 52:
            strcat(data_pattern, "3");
            break;
        default:
            break;
    }

    /* Reed Solomon error correction */
    rs_error(data_pattern);

    /* Stop character */
    strcat(data_pattern, "13");

    /* Turn the symbol into a bar pattern ready for plotting */
    writer = 0;
    h = strlen(data_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if ((data_pattern[loopey] == '1') || (data_pattern[loopey] == '0')) {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        if ((data_pattern[loopey] == '2') || (data_pattern[loopey] == '0')) {
            set_module(symbol, 2, writer);
        }
        writer += 2;
    }

    symbol->row_height[0] = 3;
    symbol->row_height[1] = 2;
    symbol->row_height[2] = 3;

    symbol->rows = 3;
    symbol->width = writer - 1;

    return error_number;
}



