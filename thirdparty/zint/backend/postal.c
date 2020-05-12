/* postal.c - Handles PostNet, PLANET, FIM. RM4SCC and Flattermarken */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2017 Robin Stuart <rstuart114@gmail.com>
    Including bug fixes by Bryan Hatton

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
#include <stdlib.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"

#define DAFTSET	"DAFT"
#define KRSET "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define KASUTSET "1234567890-abcdefgh"
#define CHKASUTSET "0123456789-abcdefgh"
#define SHKASUTSET "1234567890-ABCDEFGHIJKLMNOPQRSTUVWXYZ"

/* PostNet number encoding table - In this table L is long as S is short */
static const char *PNTable[10] = {
    "LLSSS", "SSSLL", "SSLSL", "SSLLS", "SLSSL", "SLSLS", "SLLSS", "LSSSL",
    "LSSLS", "LSLSS"
};

static const char *PLTable[10] = {
    "SSLLL", "LLLSS", "LLSLS", "LLSSL", "LSLLS", "LSLSL", "LSSLL", "SLLLS",
    "SLLSL", "SLSLL"
};

static const char *RoyalValues[36] = {
    "11", "12", "13", "14", "15", "10", "21", "22", "23", "24", "25",
    "20", "31", "32", "33", "34", "35", "30", "41", "42", "43", "44", "45", "40", "51", "52",
    "53", "54", "55", "50", "01", "02", "03", "04", "05", "00"
};

/* 0 = Full, 1 = Ascender, 2 = Descender, 3 = Tracker */
static const char *RoyalTable[36] = {
    "3300", "3210", "3201", "2310", "2301", "2211", "3120", "3030", "3021",
    "2130", "2121", "2031", "3102", "3012", "3003", "2112", "2103", "2013", "1320", "1230",
    "1221", "0330", "0321", "0231", "1302", "1212", "1203", "0312", "0303", "0213", "1122",
    "1032", "1023", "0132", "0123", "0033"
};

static const char *FlatTable[10] = {
    "0504", "18", "0117", "0216", "0315", "0414", "0513", "0612", "0711", "0810"
};

static const char *KoreaTable[10] = {
    "1313150613", "0713131313", "0417131313", "1506131313",
    "0413171313", "17171313", "1315061313", "0413131713", "17131713", "13171713"
};

static const char *JapanTable[19] = {
    "114", "132", "312", "123", "141", "321", "213", "231", "411", "144",
    "414", "324", "342", "234", "432", "243", "423", "441", "111"
};

/* Handles the PostNet system used for Zip codes in the US */
int postnet(struct zint_symbol *symbol, unsigned char source[], char dest[], int length) {
    unsigned int i, sum, check_digit;
    int error_number;

    error_number = 0;

    if (length != 5 && length != 9 && length != 11) {
        strcpy(symbol->errtxt, "480: Input wrong length");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "481: Invalid characters in data");
        return error_number;
    }
    sum = 0;

    /* start character */
    strcpy(dest, "L");

    for (i = 0; i < length; i++) {
        lookup(NEON, PNTable, source[i], dest);
        sum += ctoi(source[i]);
    }

    check_digit = (10 - (sum % 10)) % 10;
    strcat(dest, PNTable[check_digit]);

    /* stop character */
    strcat(dest, "L");

    return error_number;
}

/* Puts PostNet barcodes into the pattern matrix */
int post_plot(struct zint_symbol *symbol, unsigned char source[], int length) {
    char height_pattern[256]; /* 5 + 38 * 5 + 5 + 5 +  1 ~ 256 */
    unsigned int loopey, h;
    int writer;
    int error_number;

    error_number = 0;

    error_number = postnet(symbol, source, height_pattern, length);
    if (error_number != 0) {
        return error_number;
    }

    writer = 0;
    h = strlen(height_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if (height_pattern[loopey] == 'L') {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        writer += 3;
    }
    symbol->row_height[0] = 6;
    symbol->row_height[1] = 6;
    symbol->rows = 2;
    symbol->width = writer - 1;

    return error_number;
}

/* Handles the PLANET  system used for item tracking in the US */
int planet(struct zint_symbol *symbol, unsigned char source[], char dest[], int length) {
    unsigned int i, sum, check_digit;
    int error_number;

    error_number = 0;

    if (length != 11 && length != 13) {
        strcpy(symbol->errtxt, "482: Input wrong length");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "483: Invalid characters in data");
        return error_number;
    }
    sum = 0;

    /* start character */
    strcpy(dest, "L");

    for (i = 0; i < length; i++) {
        lookup(NEON, PLTable, source[i], dest);
        sum += ctoi(source[i]);
    }

    check_digit = (10 - (sum % 10)) % 10;
    strcat(dest, PLTable[check_digit]);

    /* stop character */
    strcat(dest, "L");

    return error_number;
}

/* Puts PLANET barcodes into the pattern matrix */
int planet_plot(struct zint_symbol *symbol, unsigned char source[], int length) {
    char height_pattern[256]; /* 5 + 38 * 5 + 5 + 5 +  1 ~ 256 */
    unsigned int loopey, h;
    int writer;
    int error_number;

    error_number = 0;

    error_number = planet(symbol, source, height_pattern, length);
    if (error_number != 0) {
        return error_number;
    }

    writer = 0;
    h = strlen(height_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if (height_pattern[loopey] == 'L') {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        writer += 3;
    }
    symbol->row_height[0] = 6;
    symbol->row_height[1] = 6;
    symbol->rows = 2;
    symbol->width = writer - 1;
    return error_number;
}

/* Korean Postal Authority */
int korea_post(struct zint_symbol *symbol, unsigned char source[], int length) {
    int total, loop, check, zeroes, error_number;
    char localstr[8], dest[80];

    error_number = 0;
    if (length > 6) {
        strcpy(symbol->errtxt, "484: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "485: Invalid characters in data");
        return error_number;
    }
    zeroes = 6 - length;
    memset(localstr, '0', zeroes);
    strcpy(localstr + zeroes, (char *) source);

    total = 0;
    for (loop = 0; loop < 6; loop++) {
        total += ctoi(localstr[loop]);
    }
    check = 10 - (total % 10);
    if (check == 10) {
        check = 0;
    }
    localstr[6] = itoc(check);
    localstr[7] = '\0';
    *dest = '\0';
    for (loop = 5; loop >= 0; loop--) {
        lookup(NEON, KoreaTable, localstr[loop], dest);
    }
    lookup(NEON, KoreaTable, localstr[6], dest);
    expand(symbol, dest);
    ustrcpy(symbol->text, (unsigned char*) localstr);
    return error_number;
}

/* The simplest barcode symbology ever! Supported by MS Word, so here it is!
    glyphs from http://en.wikipedia.org/wiki/Facing_Identification_Mark */
int fim(struct zint_symbol *symbol, unsigned char source[], int length) {


    char dest[16] = {0};

    if (length > 1) {
        strcpy(symbol->errtxt, "486: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    switch ((char) source[0]) {
        case 'a':
        case 'A':
            strcpy(dest, "111515111");
            break;
        case 'b':
        case 'B':
            strcpy(dest, "13111311131");
            break;
        case 'c':
        case 'C':
            strcpy(dest, "11131313111");
            break;
        case 'd':
        case 'D':
            strcpy(dest, "1111131311111");
            break;
        default:
            strcpy(symbol->errtxt, "487: Invalid characters in data");
            return ZINT_ERROR_INVALID_DATA;
            break;
    }

    expand(symbol, dest);
    return 0;
}

/* Handles the 4 State barcodes used in the UK by Royal Mail */
char rm4scc(char source[], unsigned char dest[], int length) {
    unsigned int i;
    int top, bottom, row, column, check_digit;
    char values[3], set_copy[] = KRSET;

    top = 0;
    bottom = 0;

    /* start character */
    strcpy((char*) dest, "1");

    for (i = 0; i < length; i++) {
        lookup(KRSET, RoyalTable, source[i], (char*) dest);
        strcpy(values, RoyalValues[posn(KRSET, source[i])]);
        top += ctoi(values[0]);
        bottom += ctoi(values[1]);
    }

    /* Calculate the check digit */
    row = (top % 6) - 1;
    column = (bottom % 6) - 1;
    if (row == -1) {
        row = 5;
    }
    if (column == -1) {
        column = 5;
    }
    check_digit = (6 * row) + column;
    strcat((char*) dest, RoyalTable[check_digit]);

    /* stop character */
    strcat((char*) dest, "0");

    return set_copy[check_digit];
}

/* Puts RM4SCC into the data matrix */
int royal_plot(struct zint_symbol *symbol, unsigned char source[], int length) {
    char height_pattern[210];
    unsigned int loopey, h;
    int writer;
    int error_number;
    strcpy(height_pattern, "");

    error_number = 0;

    if (length > 50) {
        strcpy(symbol->errtxt, "488: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    to_upper(source);
    error_number = is_sane(KRSET, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "489: Invalid characters in data");
        return error_number;
    }
    /*check = */rm4scc((char*) source, (unsigned char*) height_pattern, length);

    writer = 0;
    h = strlen(height_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if ((height_pattern[loopey] == '1') || (height_pattern[loopey] == '0')) {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        if ((height_pattern[loopey] == '2') || (height_pattern[loopey] == '0')) {
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

/* Handles Dutch Post TNT KIX symbols
   The same as RM4SCC but without check digit
   Specification at http://www.tntpost.nl/zakelijk/klantenservice/downloads/kIX_code/download.aspx */
int kix_code(struct zint_symbol *symbol, unsigned char source[], int length) {
    char height_pattern[75], localstr[20];
    unsigned int loopey;
    int writer, i, h;
    int error_number; /* zeroes; */
    strcpy(height_pattern, "");

    error_number = 0;

    if (length > 18) {
        strcpy(symbol->errtxt, "490: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    to_upper(source);
    error_number = is_sane(KRSET, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "491: Invalid characters in data");
        return error_number;
    }

    strcpy(localstr, (char *) source);

    /* Encode data */
    for (i = 0; i < length; i++) {
        lookup(KRSET, RoyalTable, localstr[i], height_pattern);
    }

    writer = 0;
    h = strlen(height_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if ((height_pattern[loopey] == '1') || (height_pattern[loopey] == '0')) {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        if ((height_pattern[loopey] == '2') || (height_pattern[loopey] == '0')) {
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

/* Handles DAFT Code symbols */
int daft_code(struct zint_symbol *symbol, unsigned char source[], int length) {
    char height_pattern[100];
    unsigned int loopey, h;
    int writer, i, error_number;
    strcpy(height_pattern, "");

    error_number = 0;
    if (length > 50) {
        strcpy(symbol->errtxt, "492: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    to_upper((unsigned char*) source);
    error_number = is_sane(DAFTSET, (unsigned char*) source, length);

    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "493: Invalid characters in data");
        return error_number;
    }

    for (i = 0; i < length; i++) {
        if (source[i] == 'D') {
            strcat(height_pattern, "2");
        }
        if (source[i] == 'A') {
            strcat(height_pattern, "1");
        }
        if (source[i] == 'F') {
            strcat(height_pattern, "0");
        }
        if (source[i] == 'T') {
            strcat(height_pattern, "3");
        }
    }

    writer = 0;
    h = strlen(height_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if ((height_pattern[loopey] == '1') || (height_pattern[loopey] == '0')) {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        if ((height_pattern[loopey] == '2') || (height_pattern[loopey] == '0')) {
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

/* Flattermarken - Not really a barcode symbology! */
int flattermarken(struct zint_symbol *symbol, unsigned char source[], int length) {
    int loop, error_number;
    char dest[512]; /* 90 * 4 + 1 ~ */

    error_number = 0;

    if (length > 90) {
        strcpy(symbol->errtxt, "494: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "495: Invalid characters in data");
        return error_number;
    }
    *dest = '\0';
    for (loop = 0; loop < length; loop++) {
        lookup(NEON, FlatTable, source[loop], dest);
    }

    expand(symbol, dest);
    return error_number;
}

/* Japanese Postal Code (Kasutama Barcode) */
int japan_post(struct zint_symbol *symbol, unsigned char source[], int length) {
    int error_number, h;
    char pattern[69];
    int writer, loopey, inter_posn, i, sum, check;
    char check_char;
    char inter[23];

#ifndef _MSC_VER
    char local_source[length + 1];
#else
    char* local_source = (char*) _alloca(length + 1);
#endif

    if (length > 20) {
        strcpy(symbol->errtxt, "496: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    inter_posn = 0;
    error_number = 0;

    strcpy(local_source, (char*) source);
    to_upper((unsigned char*) local_source);

    if (is_sane(SHKASUTSET, (unsigned char*) local_source, length) == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "497: Invalid characters in data");
        return ZINT_ERROR_INVALID_DATA;
    }
    memset(inter, 'd', 20); /* Pad character CC4 */
    inter[20] = '\0';

    i = 0;
    inter_posn = 0;
    do {
        if (((local_source[i] >= '0') && (local_source[i] <= '9')) || (local_source[i] == '-')) {
            inter[inter_posn] = local_source[i];
            inter_posn++;
        } else {
            if ((local_source[i] >= 'A') && (local_source[i] <= 'J')) {
                inter[inter_posn] = 'a';
                inter[inter_posn + 1] = local_source[i] - 'A' + '0';
                inter_posn += 2;
            }
            if ((local_source[i] >= 'K') && (local_source[i] <= 'T')) {
                inter[inter_posn] = 'b';
                inter[inter_posn + 1] = local_source[i] - 'K' + '0';
                inter_posn += 2;
            }
            if ((local_source[i] >= 'U') && (local_source[i] <= 'Z')) {
                inter[inter_posn] = 'c';
                inter[inter_posn + 1] = local_source[i] - 'U' + '0';
                inter_posn += 2;
            }
        }
        i++;
    } while ((i < length) && (inter_posn < 20));
    inter[20] = '\0';

    strcpy(pattern, "13"); /* Start */

    sum = 0;
    for (i = 0; i < 20; i++) {
        strcat(pattern, JapanTable[posn(KASUTSET, inter[i])]);
        sum += posn(CHKASUTSET, inter[i]);
    }

    /* Calculate check digit */
    check = 19 - (sum % 19);
    if (check == 19) {
        check = 0;
    }
    if (check <= 9) {
        check_char = check + '0';
    }
    if (check == 10) {
        check_char = '-';
    }
    if (check >= 11) {
        check_char = (check - 11) + 'a';
    }
    strcat(pattern, JapanTable[posn(KASUTSET, check_char)]);

    strcat(pattern, "31"); /* Stop */

    /* Resolve pattern to 4-state symbols */
    writer = 0;
    h = strlen(pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if ((pattern[loopey] == '2') || (pattern[loopey] == '1')) {
            set_module(symbol, 0, writer);
        }
        set_module(symbol, 1, writer);
        if ((pattern[loopey] == '3') || (pattern[loopey] == '1')) {
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
