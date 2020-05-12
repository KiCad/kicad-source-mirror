/* plessey.c - Handles Plessey and MSI Plessey */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"


#define SSET	"0123456789ABCDEF"
static const char *PlessTable[16] = {
    "13131313", "31131313", "13311313", "31311313", "13133113", "31133113",
    "13313113", "31313113", "13131331", "31131331", "13311331", "31311331", "13133131",
    "31133131", "13313131", "31313131"
};

static const char *MSITable[10] = {
    "12121212", "12121221", "12122112", "12122121", "12211212", "12211221",
    "12212112", "12212121", "21121212", "21121221"
};

/* Not MSI/Plessey but the older Plessey standard */
int plessey(struct zint_symbol *symbol, unsigned char source[], const size_t length) {

    unsigned int i;
    unsigned char *checkptr;
    static const char grid[9] = {1, 1, 1, 1, 0, 1, 0, 0, 1};
    char dest[1024]; /* 8 + 65 * 8 + 8 * 2 + 9 + 1 ~ 1024 */
    int error_number;

    if (length > 65) {
        strcpy(symbol->errtxt, "370: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(SSET, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "371: Invalid characters in data");
        return error_number;
    }
    checkptr = (unsigned char *) calloc(1, length * 4 + 8);

    /* Start character */
    strcpy(dest, "31311331");

    /* Data area */
    for (i = 0; i < length; i++) {
        unsigned int check = posn(SSET, source[i]);
        lookup(SSET, PlessTable, source[i], dest);
        checkptr[4 * i] = check & 1;
        checkptr[4 * i + 1] = (check >> 1) & 1;
        checkptr[4 * i + 2] = (check >> 2) & 1;
        checkptr[4 * i + 3] = (check >> 3) & 1;
    }

    /* CRC check digit code adapted from code by Leonid A. Broukhis
       used in GNU Barcode */

    for (i = 0; i < (4 * length); i++) {
        if (checkptr[i]) {
            int j;
            for (j = 0; j < 9; j++)
                checkptr[i + j] ^= grid[j];
        }
    }

    for (i = 0; i < 8; i++) {
        switch (checkptr[length * 4 + i]) {
            case 0: strcat(dest, "13");
                break;
            case 1: strcat(dest, "31");
                break;
        }
    }

    /* Stop character */
    strcat(dest, "331311313");

    expand(symbol, dest);
    ustrcpy(symbol->text, source);
    free(checkptr);
    return error_number;
}

/* Plain MSI Plessey - does not calculate any check character */
int msi_plessey(struct zint_symbol *symbol, unsigned char source[], const size_t length) {

	size_t i;
    char dest[512]; /* 2 + 55 * 8 + 3 + 1 ~ 512 */

    if (length > 55) {
        strcpy(symbol->errtxt, "372: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* start character */
    strcpy(dest, "21");

    for (i = 0; i < length; i++) {
        lookup(NEON, MSITable, source[i], dest);
    }

    /* Stop character */
    strcat(dest, "121");

    expand(symbol, dest);
    ustrcpy(symbol->text, source);
    return 0;
}

/* MSI Plessey with Modulo 10 check digit - algorithm from Barcode Island
 * http://www.barcodeisland.com/ */
int msi_plessey_mod10(struct zint_symbol *symbol, unsigned char source[], int length) {


    unsigned long i, wright, dau, pedwar, pump, n;
    char un[200], tri[32];
    int error_number, h;
    char dest[1000];

    error_number = 0;

    if (length > 18) {
        strcpy(symbol->errtxt, "373: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* start character */
    strcpy(dest, "21");

    /* draw data section */
    for (i = 0; i < length; i++) {
        lookup(NEON, MSITable, source[i], dest);
    }

    /* calculate check digit */
    wright = 0;
    n = !(length & 1);
    for (i = n; i < length; i += 2) {
        un[wright++] = source[i];
    }
    un[wright] = '\0';

    dau = strtoul(un, NULL, 10);
    dau *= 2;

    sprintf(tri, "%lu", dau);

    pedwar = 0;
    h = strlen(tri);
    for (i = 0; i < h; i++) {
        pedwar += ctoi(tri[i]);
    }

    n = length & 1;
    for (i = n; i < length; i += 2) {
        pedwar += ctoi(source[i]);
    }

    pump = (10 - pedwar % 10);
    if (pump == 10) {
        pump = 0;
    }

    /* draw check digit */
    lookup(NEON, MSITable, itoc(pump), dest);

    /* Stop character */
    strcat(dest, "121");
    expand(symbol, dest);

    ustrcpy(symbol->text, source);
    symbol->text[length] = itoc(pump);
    symbol->text[length + 1] = '\0';
    return error_number;
}

/* MSI Plessey with two Modulo 10 check digits - algorithm from
 * Barcode Island http://www.barcodeisland.com/ */
int msi_plessey_mod1010(struct zint_symbol *symbol, unsigned char source[], const unsigned int src_len) {


    unsigned long i, n, wright, dau, pedwar, pump, chwech;
    char un[16], tri[32];
    int error_number, h;
    char dest[1000];

    error_number = 0;

    if (src_len > 18) {
        /* No Entry Stack Smashers! limit because of str->number conversion*/
        strcpy(symbol->errtxt, "374: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* start character */
    strcpy(dest, "21");

    /* draw data section */
    for (i = 0; i < src_len; i++) {
        lookup(NEON, MSITable, source[i], dest);
    }

    /* calculate first check digit */
    wright = 0;

    n = !(src_len & 1);
    for (i = n; i < src_len; i += 2) {
        un[wright++] = source[i];
    }
    un[wright] = '\0';

    dau = strtoul(un, NULL, 10);
    dau *= 2;

    sprintf(tri, "%lu", dau);

    pedwar = 0;
    h = strlen(tri);
    for (i = 0; i < h; i++) {
        pedwar += ctoi(tri[i]);
    }

    n = src_len & 1;
    for (i = n; i < src_len; i += 2) {
        pedwar += ctoi(source[i]);
    }

    pump = 10 - pedwar % 10;
    if (pump == 10) {
        pump = 0;
    }

    /* calculate second check digit */
    wright = 0;
    n = src_len & 1;
    for (i = n; i < src_len; i += 2) {
        un[wright++] = source[i];
    }
    un[wright++] = itoc(pump);
    un[wright] = '\0';

    dau = strtoul(un, NULL, 10);
    dau *= 2;

    sprintf(tri, "%lu", dau);

    pedwar = 0;
    h = strlen(tri);
    for (i = 0; i < h; i++) {
        pedwar += ctoi(tri[i]);
    }


    i = !(src_len & 1);
    for (; i < src_len; i += 2) {
        pedwar += ctoi(source[i]);
    }

    chwech = 10 - pedwar % 10;
    if (chwech == 10) {
        chwech = 0;
    }

    /* Draw check digits */
    lookup(NEON, MSITable, itoc(pump), dest);
    lookup(NEON, MSITable, itoc(chwech), dest);

    /* Stop character */
    strcat(dest, "121");

    expand(symbol, dest);

    ustrcpy(symbol->text, source);
    symbol->text[src_len] = itoc(pump);
    symbol->text[src_len + 1] = itoc(chwech);
    symbol->text[src_len + 2] = '\0';

    return error_number;
}

/* Calculate a Modulo 11 check digit using the system discussed on Wikipedia -
    see http://en.wikipedia.org/wiki/Talk:MSI_Barcode */
int msi_plessey_mod11(struct zint_symbol *symbol, unsigned char source[], const unsigned int src_len) {
    /* uses the IBM weight system */
    int i, weight, x, check;
    int error_number;
    char dest[1000];

    error_number = 0;

    if (src_len > 55) {
        strcpy(symbol->errtxt, "375: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* start character */
    strcpy(dest, "21");

    /* draw data section */
    for (i = 0; i < src_len; i++) {
        lookup(NEON, MSITable, source[i], dest);
    }

    /* calculate check digit */
    x = 0;
    weight = 2;
    for (i = src_len - 1; i >= 0; i--) {
        x += weight * ctoi(source[i]);
        weight++;
        if (weight > 7) {
            weight = 2;
        }
    }

    check = (11 - (x % 11)) % 11;
    if (check == 10) {
        lookup(NEON, MSITable, '1', dest);
        lookup(NEON, MSITable, '0', dest);
    } else {
        lookup(NEON, MSITable, itoc(check), dest);
    }

    /* stop character */
    strcat(dest, "121");

    expand(symbol, dest);

    ustrcpy(symbol->text, source);
    if (check == 10) {
        strcat((char*) symbol->text, "10");
    } else {
        symbol->text[src_len] = itoc(check);
        symbol->text[src_len + 1] = '\0';
    }

    return error_number;
}

/* Combining the Barcode Island and Wikipedia code
 * Verified against http://www.bokai.com/BarcodeJSP/applet/BarcodeSampleApplet.htm */
int msi_plessey_mod1110(struct zint_symbol *symbol, unsigned char source[], const unsigned int src_len) {
    /* Weighted using the IBM system */
    unsigned long i, weight, x, check, wright, dau, pedwar, pump;
    size_t h;
    long si;
    char un[16], tri[16];
    int error_number;
    char dest[1000];
    unsigned char temp[32];
    unsigned int temp_len;

    error_number = 0;

    if (src_len > 18) {
        strcpy(symbol->errtxt, "376: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* start character */
    strcpy(dest, "21");

    /* draw data section */
    for (i = 0; i < src_len; i++) {
        lookup(NEON, MSITable, source[i], dest);
    }

    /* calculate first (mod 11) digit */
    x = 0;
    weight = 2;
    for (si = src_len - 1; si >= 0; si--) {
        x += weight * ctoi(source[si]);
        weight++;
        if (weight > 7) {
            weight = 2;
        }
    }

    check = (11 - (x % 11)) % 11;
    ustrcpy(temp, source);
    temp_len = src_len;
    if (check == 10) {
        lookup(NEON, MSITable, '1', dest);
        lookup(NEON, MSITable, '0', dest);
        strcat((char*) temp, "10");
        temp_len += 2;
    } else {
        lookup(NEON, MSITable, itoc(check), dest);
        temp[temp_len++] = itoc(check);
        temp[temp_len] = '\0';
    }

    /* calculate second (mod 10) check digit */
    wright = 0;
    i = !(temp_len & 1);
    for (; i < temp_len; i += 2) {
        un[wright++] = temp[i];
    }
    un[wright] = '\0';

    dau = strtoul(un, NULL, 10);
    dau *= 2;

    sprintf(tri, "%lu", dau);

    pedwar = 0;
    h = strlen(tri);
    for (i = 0; i < h; i++) {
        pedwar += ctoi(tri[i]);
    }

    i = temp_len & 1;
    for (; i < temp_len; i += 2) {
        pedwar += ctoi(temp[i]);
    }

    pump = 10 - pedwar % 10;
    if (pump == 10) {
        pump = 0;
    }

    /* draw check digit */
    lookup(NEON, MSITable, itoc(pump), dest);

    /* stop character */
    strcat(dest, "121");
    expand(symbol, dest);

    temp[temp_len++] = itoc(pump);
    temp[temp_len] = '\0';


    ustrcpy(symbol->text, temp);
    return error_number;
}

int msi_handle(struct zint_symbol *symbol, unsigned char source[], int length) {
    int error_number;

    error_number = is_sane(NEON, source, length);
    if (error_number != 0) {
        strcpy(symbol->errtxt, "377: Invalid characters in input data");
        return ZINT_ERROR_INVALID_DATA;
    }


    if ((symbol->option_2 < 0) || (symbol->option_2 > 4)) {
        symbol->option_2 = 0;
    }

    switch (symbol->option_2) {
        case 0: error_number = msi_plessey(symbol, source, length);
            break;
        case 1: error_number = msi_plessey_mod10(symbol, source, length);
            break;
        case 2: error_number = msi_plessey_mod1010(symbol, source, length);
            break;
        case 3: error_number = msi_plessey_mod11(symbol, source, length);
            break;
        case 4: error_number = msi_plessey_mod1110(symbol, source, length);
            break;
    }

    return error_number;
}

