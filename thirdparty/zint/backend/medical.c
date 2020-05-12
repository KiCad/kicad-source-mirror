/* medical.c - Handles 1 track and 2 track pharmacode and Codabar */

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
#include <string.h>
#include <stdlib.h>
#include "common.h"

extern int c39(struct zint_symbol *symbol, unsigned char source[], const size_t length);
/* Codabar table checked against EN 798:1995 */

#define CALCIUM	"0123456789-$:/.+ABCD"

static const char *CodaTable[20] = {
    "11111221", "11112211", "11121121", "22111111", "11211211", "21111211",
    "12111121", "12112111", "12211111", "21121111", "11122111", "11221111", "21112121", "21211121",
    "21212111", "11212121", "11221211", "12121121", "11121221", "11122211"
};

int pharma_one(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* "Pharmacode can represent only a single integer from 3 to 131070. Unlike other
       commonly used one-dimensional barcode schemes, pharmacode does not store the data in a
       form corresponding to the human-readable digits; the number is encoded in binary, rather
       than decimal. Pharmacode is read from right to left: with n as the bar position starting
       at 0 on the right, each narrow bar adds 2n to the value and each wide bar adds 2(2^n).
       The minimum barcode is 2 bars and the maximum 16, so the smallest number that could
       be encoded is 3 (2 narrow bars) and the biggest is 131070 (16 wide bars)."
       - http://en.wikipedia.org/wiki/Pharmacode */

    /* This code uses the One Track Pharamacode calculating algorithm as recommended by
       the specification at http://www.laetus.com/laetus.php?request=file&id=69 */

    unsigned long int tester;
    int counter, error_number, h;
    char inter[18] = {0}; /* 131070 -> 17 bits */
    char dest[64]; /* 17 * 2 + 1 */

    if (length > 6) {
        strcpy(symbol->errtxt, "350: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "351: Invalid characters in data");
        return error_number;
    }

    tester = atoi((char*) source);

    if ((tester < 3) || (tester > 131070)) {
        strcpy(symbol->errtxt, "352: Data out of range");
        return ZINT_ERROR_INVALID_DATA;
    }

    do {
        if (!(tester & 1)) {
            strcat(inter, "W");
            tester = (tester - 2) / 2;
        } else {
            strcat(inter, "N");
            tester = (tester - 1) / 2;
        }
    } while (tester != 0);

    h = strlen(inter) - 1;
    *dest = '\0';
    for (counter = h; counter >= 0; counter--) {
        if (inter[counter] == 'W') {
            strcat(dest, "32");
        } else {
            strcat(dest, "12");
        }
    }

    expand(symbol, dest);

    return error_number;
}

int pharma_two_calc(struct zint_symbol *symbol, unsigned char source[], char dest[]) {
    /* This code uses the Two Track Pharamacode defined in the document at
       http://www.laetus.com/laetus.php?request=file&id=69 and using a modified
       algorithm from the One Track system. This standard accepts integet values
       from 4 to 64570080. */

    unsigned long int tester;
    int counter, h;
    char inter[17];
    int error_number;

    tester = atoi((char*) source);

    if ((tester < 4) || (tester > 64570080)) {
        strcpy(symbol->errtxt, "353: Data out of range");
        return ZINT_ERROR_INVALID_DATA;
    }
    error_number = 0;
    strcpy(inter, "");
    do {
        switch (tester % 3) {
            case 0:
                strcat(inter, "3");
                tester = (tester - 3) / 3;
                break;
            case 1:
                strcat(inter, "1");
                tester = (tester - 1) / 3;
                break;
            case 2:
                strcat(inter, "2");
                tester = (tester - 2) / 3;
                break;
        }
    } while (tester != 0);

    h = strlen(inter) - 1;
    for (counter = h; counter >= 0; counter--) {
        dest[h - counter] = inter[counter];
    }
    dest[h + 1] = '\0';

    return error_number;
}

int pharma_two(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* Draws the patterns for two track pharmacode */
    char height_pattern[200];
    unsigned int loopey, h;
    int writer;
    int error_number = 0;
    strcpy(height_pattern, "");

    if (length > 8) {
        strcpy(symbol->errtxt, "354: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "355: Invalid characters in data");
        return error_number;
    }
    error_number = pharma_two_calc(symbol, source, height_pattern);
    if (error_number != 0) {
        return error_number;
    }

    writer = 0;
    h = strlen(height_pattern);
    for (loopey = 0; loopey < h; loopey++) {
        if ((height_pattern[loopey] == '2') || (height_pattern[loopey] == '3')) {
            set_module(symbol, 0, writer);
        }
        if ((height_pattern[loopey] == '1') || (height_pattern[loopey] == '3')) {
            set_module(symbol, 1, writer);
        }
        writer += 2;
    }
    symbol->rows = 2;
    symbol->width = writer - 1;


    return error_number;
}

/* The Codabar system consisting of simple substitution */
int codabar(struct zint_symbol *symbol, unsigned char source[], int length) {

    int i, error_number;
    char dest[512];

    strcpy(dest, "");

    if (length > 60) { /* No stack smashing please */
        strcpy(symbol->errtxt, "356: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    to_upper(source);
    error_number = is_sane(CALCIUM, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "357: Invalid characters in data");
        return error_number;
    }
    /* Codabar must begin and end with the characters A, B, C or D */
    if ((source[0] != 'A') && (source[0] != 'B') && (source[0] != 'C')
            && (source[0] != 'D')) {
        strcpy(symbol->errtxt, "358: Invalid characters in data");
        return ZINT_ERROR_INVALID_DATA;
    }

    if ((source[length - 1] != 'A') && (source[length - 1] != 'B') &&
            (source[length - 1] != 'C') && (source[length - 1] != 'D')) {
        strcpy(symbol->errtxt, "359: Invalid characters in data");
        return ZINT_ERROR_INVALID_DATA;
    }

    for (i = 0; i < length; i++) {
        lookup(CALCIUM, CodaTable, source[i], dest);
    }

    expand(symbol, dest);
    ustrcpy(symbol->text, source);
    return error_number;
}

/* Italian Pharmacode */
int code32(struct zint_symbol *symbol, unsigned char source[], int length) {
    int i, zeroes, error_number, checksum, checkpart, checkdigit;
    char localstr[10], risultante[7];
    long int pharmacode, devisor;
    int codeword[6];
    char tabella[34];

    /* Validate the input */
    if (length > 8) {
        strcpy(symbol->errtxt, "360: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "361: Invalid characters in data");
        return error_number;
    }

    /* Add leading zeros as required */
    zeroes = 8 - length;
    memset(localstr, '0', zeroes);
    strcpy(localstr + zeroes, (char*) source);

    /* Calculate the check digit */
    checksum = 0;
    checkpart = 0;
    for (i = 0; i < 4; i++) {
        checkpart = ctoi(localstr[i * 2]);
        checksum += checkpart;
        checkpart = 2 * (ctoi(localstr[(i * 2) + 1]));
        if (checkpart >= 10) {
            checksum += (checkpart - 10) + 1;
        } else {
            checksum += checkpart;
        }
    }

    /* Add check digit to data string */
    checkdigit = checksum % 10;
    localstr[8] = itoc(checkdigit);
    localstr[9] = '\0';

    /* Convert string into an integer value */
    pharmacode = atoi(localstr);

    /* Convert from decimal to base-32 */
    devisor = 33554432;
    for (i = 5; i >= 0; i--) {
        long int remainder;
        codeword[i] = pharmacode / devisor;
        remainder = pharmacode % devisor;
        pharmacode = remainder;
        devisor /= 32;
    }

    /* Look up values in 'Tabella di conversione' */
    strcpy(tabella, "0123456789BCDFGHJKLMNPQRSTUVWXYZ");
    for (i = 5; i >= 0; i--) {
        risultante[5 - i] = tabella[codeword[i]];
    }
    risultante[6] = '\0';
    /* Plot the barcode using Code 39 */
    error_number = c39(symbol, (unsigned char*) risultante, strlen(risultante));
    if (error_number != 0) {
        return error_number;
    }

    /* Override the normal text output with the Pharmacode number */
    strcpy((char*) symbol->text, "A");
    strcat((char*) symbol->text, (char*) localstr);

    return error_number;
}


