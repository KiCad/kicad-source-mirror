/* medical.c - Handles Pharmacode One-Track, Pharmacode Two-Track, Italian Pharmacode and PZN */
/*
    libzint - the open source barcode library
    Copyright (C) 2008-2025 Robin Stuart <rstuart114@gmail.com>

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
/* SPDX-License-Identifier: BSD-3-Clause */

#include <assert.h>
#include <stdio.h>
#include "common.h"

INTERNAL int code39(struct zint_symbol *symbol, unsigned char source[], int length);

/* Pharmacode One-Track */
INTERNAL int pharma(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* "Pharmacode can represent only a single integer from 3 to 131070. Unlike other
       commonly used one-dimensional barcode schemes, pharmacode does not store the data in a
       form corresponding to the human-readable digits; the number is encoded in binary, rather
       than decimal. Pharmacode is read from right to left: with n as the bar position starting
       at 0 on the right, each narrow bar adds 2^n to the value and each wide bar adds 2(2^n).
       The minimum barcode is 2 bars and the maximum 16, so the smallest number that could
       be encoded is 3 (2 narrow bars) and the biggest is 131070 (16 wide bars)."
       - http://en.wikipedia.org/wiki/Pharmacode */

    /* This code uses the One Track Pharamacode calculating algorithm as recommended by
       the specification at http://www.laetus.com/laetus.php?request=file&id=69
       (http://www.gomaro.ch/ftproot/Laetus_PHARMA-CODE.pdf) */

    int i;
    int tester;
    int counter, error_number = 0, h;
    char inter[18] = {0}; /* 131070 -> 17 bits */
    char *in = inter;
    char dest[64]; /* 17 * 2 + 1 */
    char *d = dest;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > 6) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 350, "Input length %d too long (maximum 6)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 351,
                        "Invalid character at position %d in input (digits only)", i);
    }

    tester = to_int(source, length);
    if ((tester < 3) || (tester > 131070)) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 352, "Input value '%d' out of range (3 to 131070)", tester);
    }

    do {
        if (!(tester & 1)) {
            *in++ = 'W';
            tester = (tester - 2) / 2;
        } else {
            *in++ = 'N';
            tester = (tester - 1) / 2;
        }
    } while (tester != 0);

    h = in - inter;
    for (counter = h - 1; counter >= 0; counter--) {
        *d++ = inter[counter] == 'W' ? '3' : '1';
        *d++ = '2';
    }
    *--d = '\0'; /* Chop off final bar */

    expand(symbol, dest, d - dest);

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Laetus Pharmacode Guide 1.2 Standard one-track height 8mm / 0.5mm (X) */
        error_number = set_height(symbol, 16.0f, 0.0f, 0.0f, 0 /*no_errtxt*/);
    } else {
        (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    if (raw_text && rt_cpy(symbol, source, length)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy()` only fails with OOM */
    }

    return error_number;
}

static int pharma_two_calc(int tester, char *d) {
    /* This code uses the Two Track Pharamacode defined in the document at
       http://www.laetus.com/laetus.php?request=file&id=69 and using a modified
       algorithm from the One Track system. This standard accepts integet values
       from 4 to 64570080. */

    int counter, h;
    char inter[17];
    char *in = inter;

    do {
        switch (tester % 3) {
            case 0:
                *in++ = '3';
                tester = (tester - 3) / 3;
                break;
            case 1:
                *in++ = '1';
                tester = (tester - 1) / 3;
                break;
            case 2:
                *in++ = '2';
                tester = (tester - 2) / 3;
                break;
        }
    } while (tester != 0);

    h = in - inter;
    for (counter = h - 1; counter >= 0; counter--) {
        *d++ = inter[counter];
    }
    *d = '\0';

    return h;
}

/* Pharmacode Two-Track */
INTERNAL int pharma_two(struct zint_symbol *symbol, unsigned char source[], int length) {
    /* Draws the patterns for two track pharmacode */
    int i;
    int tester;
    char height_pattern[200];
    unsigned int loopey, h;
    int writer;
    int error_number = 0;
    const int raw_text = symbol->output_options & BARCODE_RAW_TEXT;

    if (length > 8) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 354, "Input length %d too long (maximum 8)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 355,
                        "Invalid character at position %d in input (digits only)", i);
    }

    tester = to_int(source, length);
    if ((tester < 4) || (tester > 64570080)) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 353, "Input value '%d' out of range (4 to 64570080)", tester);
    }
    h = pharma_two_calc(tester, height_pattern);

    writer = 0;
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

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Laetus Pharmacode Guide 1.4
           Two-track height min 8mm / 2mm (X max) = 4X (2X per row), standard 8mm / 1mm = 8X,
           max 12mm / 0.8mm (X min) = 15X */
        error_number = set_height(symbol, 2.0f, 8.0f, 15.0f, 0 /*no_errtxt*/);
    } else {
        (void) set_height(symbol, 0.0f, 10.0f, 0.0f, 1 /*no_errtxt*/);
    }

    if (raw_text && rt_cpy(symbol, source, length)) {
        return ZINT_ERROR_MEMORY; /* `rt_cpy()` only fails with OOM */
    }

    return error_number;
}

/* Italian Pharmacode */
INTERNAL int code32(struct zint_symbol *symbol, unsigned char source[], int length) {
    static const unsigned char TABELLA[] = "0123456789BCDFGHJKLMNPQRSTUVWXYZ";
    int i, zeroes, checksum, checkpart, checkdigit;
    unsigned char local_source[10], risultante[7];
    unsigned int pharmacode, devisor;
    int codeword[6];
    int error_number;
    const int saved_option_2 = symbol->option_2;

    /* Validate the input */
    if (length > 8) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 360, "Input length %d too long (maximum 8)", length);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 361,
                        "Invalid character at position %d in input (digits only)", i);
    }

    /* Add leading zeros as required */
    zeroes = 8 - length;
    memset(local_source, '0', zeroes);
    memcpy(local_source + zeroes, source, length);

    /* Calculate the check digit */
    checksum = 0;
    for (i = 0; i < 4; i++) {
        checkpart = ctoi(local_source[i * 2]);
        checksum += checkpart;
        checkpart = 2 * (ctoi(local_source[(i * 2) + 1]));
        if (checkpart >= 10) {
            checksum += (checkpart - 10) + 1;
        } else {
            checksum += checkpart;
        }
    }

    /* Add check digit to data string */
    checkdigit = checksum % 10;
    local_source[8] = itoc(checkdigit);

    /* Convert string into an integer value */
    pharmacode = to_int(local_source, 9);

    /* Convert from decimal to base-32 */
    devisor = 33554432;
    for (i = 5; i >= 0; i--) {
        unsigned int remainder;
        codeword[i] = pharmacode / devisor;
        remainder = pharmacode % devisor;
        pharmacode = remainder;
        devisor /= 32;
    }

    /* Look up values in 'Tabella di conversione' */
    for (i = 5; i >= 0; i--) {
        risultante[5 - i] = TABELLA[codeword[i]];
    }

    if (symbol->option_2 == 1 || symbol->option_2 == 2) {
        symbol->option_2 = 0; /* Need to overwrite this so `code39()` doesn't add a check digit itself */
    }

    /* Plot the barcode using Code 39 */
    if ((error_number = code39(symbol, risultante, 6))) {
        assert(error_number == ZINT_ERROR_MEMORY); /* Only error that can occur */
        return error_number;
    }

    if (symbol->option_2 == 1 || symbol->option_2 == 2) {
        symbol->option_2 = saved_option_2; /* Restore */
    }

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Allegato A Caratteristiche tecniche del bollino farmaceutico
           (https://www.gazzettaufficiale.it/do/atto/serie_generale/caricaPdf?cdimg=14A0566800100010110001
            &dgu=2014-07-18&art.dataPubblicazioneGazzetta=2014-07-18&art.codiceRedazionale=14A05668&art.num=1
            &art.tiposerie=SG)
           X given as 0.250mm; height (and quiet zones) left to ISO/IEC 16388:2007 (Code 39)
           So min height 5mm = 5mm / 0.25mm = 20 > 15% of width, i.e. (10 * 8 + 19) * 0.15 = 14.85 */
        error_number = set_height(symbol, 20.0f, 20.0f, 0.0f, 0 /*no_errtxt*/); /* Use as default also */
    } else {
        (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    /* Override the normal text output with the Pharmacode number */
    hrt_cpy_chr(symbol, 'A');
    hrt_cat_nochk(symbol, local_source, 9);

    /* Use `raw_text` set by `code39()` */

    return error_number;
}

/* Pharmazentralnummer (PZN) */
/* PZN https://www.ifaffm.de/mandanten/1/documents/04_ifa_coding_system/IFA_Info_Code_39_EN.pdf */
/* PZN https://www.ifaffm.de/mandanten/1/documents/04_ifa_coding_system/
       IFA-Info_Check_Digit_Calculations_PZN_PPN_UDI_EN.pdf */
INTERNAL int pzn(struct zint_symbol *symbol, unsigned char source[], int length) {

    int i, error_number, zeroes;
    int count, check_digit;
    unsigned char have_check_digit = '\0';
    unsigned char local_source[1 + 8]; /* '-' prefix + 8 digits */
    const int pzn7 = symbol->option_2 == 1;
    const int saved_option_2 = symbol->option_2;

    if (length > 8 - pzn7) {
        return ZEXT errtxtf(ZINT_ERROR_TOO_LONG, symbol, 325, "Input length %1$d too long (maximum %2$d)", length,
                            8 - pzn7);
    }
    if (length == 8 - pzn7) {
        have_check_digit = source[7 - pzn7];
        length--;
    }
    if ((i = not_sane(NEON_F, source, length))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 326,
                        "Invalid character at position %d in input (digits only)", i);
    }

    local_source[0] = '-';
    zeroes = 7 - pzn7 - length + 1;
    for (i = 1; i < zeroes; i++)
        local_source[i] = '0';
    memcpy(local_source + zeroes, source, length);

    count = 0;
    for (i = 1; i < 8 - pzn7; i++) {
        count += (i + pzn7) * ctoi(local_source[i]);
    }

    check_digit = count % 11;

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("PZN: %.*s, check digit %d\n", 8 - pzn7, local_source, (int) check_digit);
    }

    if (check_digit == 10) {
        return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 327, "Invalid PZN, check digit is '10'");
    }
    if (have_check_digit && ctoi(have_check_digit) != check_digit) {
        return ZEXT errtxtf(ZINT_ERROR_INVALID_CHECK, symbol, 890, "Invalid check digit '%1$c', expecting '%2$c'",
                            have_check_digit, itoc(check_digit));
    }

    local_source[8 - pzn7] = itoc(check_digit);

    if (symbol->option_2 == 1 || symbol->option_2 == 2) {
        symbol->option_2 = 0; /* Need to overwrite this so `code39()` doesn't add a check digit itself */
    }

    error_number = code39(symbol, local_source, 9 - pzn7);

    if (symbol->option_2 == 1 || symbol->option_2 == 2) {
        symbol->option_2 = saved_option_2; /* Restore */
    }

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Technical Information regarding PZN Coding V 2.1 (25 Feb 2019) Code size
           https://www.ifaffm.de/mandanten/1/documents/04_ifa_coding_system/IFA_Info_Code_39_EN.pdf
           "normal" X 0.25mm (0.187mm - 0.45mm), height 8mm - 20mm for 0.25mm X, 10mm mentioned so use that
           as default, 10mm / 0.25mm = 40 */
        if (error_number < ZINT_ERROR) {
            const float min_height = 17.7777786f; /* 8.0 / 0.45 */
            const float max_height = 106.951874f; /* 20.0 / 0.187 */
            error_number = set_height(symbol, min_height, 40.0f, max_height, 0 /*no_errtxt*/);
        }
    } else {
        if (error_number < ZINT_ERROR) {
            (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
        }
    }

    hrt_cpy_nochk(symbol, (const unsigned char *) "PZN - ", 6); /* Note changed to put space after hyphen */
    hrt_cat_nochk(symbol, local_source + 1, 9 - pzn7 - 1);

    /* Use `raw_text` set by `code39()` */

    return error_number;
}

/* vim: set ts=4 sw=4 et : */
