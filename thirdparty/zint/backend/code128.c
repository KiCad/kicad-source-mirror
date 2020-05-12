/* code128.c - Handles Code 128 and derivatives */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2017 Robin Stuart <rstuart114@gmail.com>
    Bugfixes thanks to Christian Sakowski and BogDan Vatra

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
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "gs1.h"

#define TRUE 1
#define FALSE 0
#define SHIFTA 90
#define LATCHA 91
#define SHIFTB 92
#define LATCHB 93
#define SHIFTC 94
#define LATCHC 95
#define AORB 96
#define ABORC 97

#define DPDSET	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ*"

static int list[2][170];

/* Code 128 tables checked against ISO/IEC 15417:2007 */

static const char *C128Table[107] = {
    /* Code 128 character encodation - Table 1 */
    "212222", "222122", "222221", "121223", "121322", "131222", "122213",
    "122312", "132212", "221213", "221312", "231212", "112232", "122132", "122231", "113222",
    "123122", "123221", "223211", "221132", "221231", "213212", "223112", "312131", "311222",
    "321122", "321221", "312212", "322112", "322211", "212123", "212321", "232121", "111323",
    "131123", "131321", "112313", "132113", "132311", "211313", "231113", "231311", "112133",
    "112331", "132131", "113123", "113321", "133121", "313121", "211331", "231131", "213113",
    "213311", "213131", "311123", "311321", "331121", "312113", "312311", "332111", "314111",
    "221411", "431111", "111224", "111422", "121124", "121421", "141122", "141221", "112214",
    "112412", "122114", "122411", "142112", "142211", "241211", "221114", "413111", "241112",
    "134111", "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112",
    "421211", "212141", "214121", "412121", "111143", "111341", "131141", "114113", "114311",
    "411113", "411311", "113141", "114131", "311141", "411131", "211412", "211214", "211232",
    "2331112"
};

/* Determine appropriate mode for a given character */
int parunmodd(const unsigned char llyth) {
    int modd;
    modd = 0;

    if (llyth <= 31) {
        modd = SHIFTA;
    } else if ((llyth >= 48) && (llyth <= 57)) {
        modd = ABORC;
    } else if (llyth <= 95) {
        modd = AORB;
    } else if (llyth <= 127) {
        modd = SHIFTB;
    } else if (llyth <= 159) {
        modd = SHIFTA;
    } else if (llyth <= 223) {
        modd = AORB;
    } else {
        modd = SHIFTB;
    }

    return modd;
}

/**
 * bring together same type blocks
 */
void grwp(int *indexliste) {

    /* bring together same type blocks */
    if (*(indexliste) > 1) {
        int i = 1;
        while (i < *(indexliste)) {
            if (list[1][i - 1] == list[1][i]) {
                int j;
                /* bring together */
                list[0][i - 1] = list[0][i - 1] + list[0][i];
                j = i + 1;

                /* decreace the list */
                while (j < *(indexliste)) {
                    list[0][j - 1] = list[0][j];
                    list[1][j - 1] = list[1][j];
                    j++;
                }
                *(indexliste) = *(indexliste) - 1;
                i--;
            }
            i++;
        }
    }
}

/**
 * Implements rules from ISO 15417 Annex E
 */
void dxsmooth(int *indexliste) {
    int i, last, next;

    for (i = 0; i < *(indexliste); i++) {
        int current = list[1][i];
        int length = list[0][i];
        if (i != 0) {
            last = list[1][i - 1];
        } else {
            last = FALSE;
        }
        if (i != *(indexliste) - 1) {
            next = list[1][i + 1];
        } else {
            next = FALSE;
        }

        if (i == 0) { /* first block */
            if ((*(indexliste) == 1) && ((length == 2) && (current == ABORC))) {
                /* Rule 1a */
                list[1][i] = LATCHC;
            }
            if (current == ABORC) {
                if (length >= 4) {
                    /* Rule 1b */
                    list[1][i] = LATCHC;
                } else {
                    list[1][i] = AORB;
                    current = AORB;
                }
            }
            if (current == SHIFTA) {
                /* Rule 1c */
                list[1][i] = LATCHA;
            }
            if ((current == AORB) && (next == SHIFTA)) {
                /* Rule 1c */
                list[1][i] = LATCHA;
                current = LATCHA;
            }
            if (current == AORB) {
                /* Rule 1d */
                list[1][i] = LATCHB;
            }
        } else {
            if ((current == ABORC) && (length >= 4)) {
                /* Rule 3 */
                list[1][i] = LATCHC;
                current = LATCHC;
            }
            if (current == ABORC) {
                list[1][i] = AORB;
                current = AORB;
            }
            if ((current == AORB) && (last == LATCHA)) {
                list[1][i] = LATCHA;
                current = LATCHA;
            }
            if ((current == AORB) && (last == LATCHB)) {
                list[1][i] = LATCHB;
                current = LATCHB;
            }
            if ((current == AORB) && (next == SHIFTA)) {
                list[1][i] = LATCHA;
                current = LATCHA;
            }
            if ((current == AORB) && (next == SHIFTB)) {
                list[1][i] = LATCHB;
                current = LATCHB;
            }
            if (current == AORB) {
                list[1][i] = LATCHB;
                current = LATCHB;
            }
            if ((current == SHIFTA) && (length > 1)) {
                /* Rule 4 */
                list[1][i] = LATCHA;
                current = LATCHA;
            }
            if ((current == SHIFTB) && (length > 1)) {
                /* Rule 5 */
                list[1][i] = LATCHB;
                current = LATCHB;
            }
            if ((current == SHIFTA) && (last == LATCHA)) {
                list[1][i] = LATCHA;
                current = LATCHA;
            }
            if ((current == SHIFTB) && (last == LATCHB)) {
                list[1][i] = LATCHB;
                current = LATCHB;
            }
            if ((current == SHIFTA) && (last == LATCHC)) {
                list[1][i] = LATCHA;
                current = LATCHA;
            }
            if ((current == SHIFTB) && (last == LATCHC)) {
                list[1][i] = LATCHB;
                current = LATCHB;
            }
        } /* Rule 2 is implimented elsewhere, Rule 6 is implied */
    }
    grwp(indexliste);

}

/**
 * Translate Code 128 Set A characters into barcodes.
 * This set handles all control characters NULL to US.
 */
void c128_set_a(unsigned char source, char dest[], int values[], int *bar_chars) {

    if (source > 127) {
        if (source < 160) {
            strcat(dest, C128Table[(source - 128) + 64]);
            values[(*bar_chars)] = (source - 128) + 64;
        } else {
            strcat(dest, C128Table[(source - 128) - 32]);
            values[(*bar_chars)] = (source - 128) - 32;
        }
    } else {
        if (source < 32) {
            strcat(dest, C128Table[source + 64]);
            values[(*bar_chars)] = source + 64;
        } else {
            strcat(dest, C128Table[source - 32]);
            values[(*bar_chars)] = source - 32;
        }
    }
    (*bar_chars)++;
}

/**
 * Translate Code 128 Set B characters into barcodes.
 * This set handles all characters which are not part of long numbers and not
 * control characters.
 */
void c128_set_b(unsigned char source, char dest[], int values[], int *bar_chars) {
    if (source > 127) {
        strcat(dest, C128Table[source - 32 - 128]);
        values[(*bar_chars)] = source - 32 - 128;
    } else {
        strcat(dest, C128Table[source - 32]);
        values[(*bar_chars)] = source - 32;
    }
    (*bar_chars)++;
}

/* Translate Code 128 Set C characters into barcodes
 * This set handles numbers in a compressed form
 */
void c128_set_c(unsigned char source_a, unsigned char source_b, char dest[], int values[], int *bar_chars) {
    int weight;

    weight = (10 * ctoi(source_a)) + ctoi(source_b);
    strcat(dest, C128Table[weight]);
    values[(*bar_chars)] = weight;
    (*bar_chars)++;
}

/* Handle Code 128 and NVE-18 */
int code_128(struct zint_symbol *symbol, unsigned char source[], const size_t length) {
    int i, j, k, values[170] = {0}, bar_characters, read, total_sum;
    int error_number, indexchaine, indexliste, f_state;
    size_t sourcelen;
    char set[170] = {' '}, fset[170] = {' '}, mode, last_set, current_set = ' ';
    float glyph_count;
    char dest[1000];

    error_number = 0;
    strcpy(dest, "");

    sourcelen = length;

    j = 0;
    bar_characters = 0;
    f_state = 0;

    if (sourcelen > 160) {
        /* This only blocks rediculously long input - the actual length of the
           resulting barcode depends on the type of data, so this is trapped later */
        strcpy(symbol->errtxt, "340: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* Detect extended ASCII characters */
    for (i = 0; i < sourcelen; i++) {
        if (source[i] >= 128)
            fset[i] = 'f';
    }
    fset[i] = '\0';

    /* Decide when to latch to extended mode - Annex E note 3 */
    j = 0;
    for (i = 0; i < sourcelen; i++) {
        if (fset[i] == 'f') {
            j++;
        } else {
            j = 0;
        }

        if (j >= 5) {
            for (k = i; k > (i - 5); k--) {
                fset[k] = 'F';
            }
        }

        if ((j >= 3) && (i == (sourcelen - 1))) {
            for (k = i; k > (i - 3); k--) {
                fset[k] = 'F';
            }
        }
    }

    /* Decide if it is worth reverting to 646 encodation for a few characters as described in 4.3.4.2 (d) */
    for (i = 1; i < sourcelen; i++) {
        if ((fset[i - 1] == 'F') && (fset[i] == ' ')) {
            /* Detected a change from 8859-1 to 646 - count how long for */
            for (j = 0; (fset[i + j] == ' ') && ((i + j) < sourcelen); j++);
            if ((j < 5) || ((j < 3) && ((i + j) == (sourcelen - 1)))) {
                /* Uses the same figures recommended by Annex E note 3 */
                /* Change to shifting back rather than latching back */
                for (k = 0; k < j; k++) {
                    fset[i + k] = 'n';
                }
            }
        }
    }

    /* Decide on mode using same system as PDF417 and rules of ISO 15417 Annex E */
    indexliste = 0;
    indexchaine = 0;

    mode = parunmodd(source[indexchaine]);
    if ((symbol->symbology == BARCODE_CODE128B) && (mode == ABORC)) {
        mode = AORB;
    }

    for (i = 0; i < 170; i++) {
        list[0][i] = 0;
    }

    do {
        list[1][indexliste] = mode;
        while ((list[1][indexliste] == mode) && (indexchaine < sourcelen)) {
            list[0][indexliste]++;
            indexchaine++;
            mode = parunmodd(source[indexchaine]);
            if ((symbol->symbology == BARCODE_CODE128B) && (mode == ABORC)) {
                mode = AORB;
            }
        }
        indexliste++;
    } while (indexchaine < sourcelen);

    dxsmooth(&indexliste);

    /* Resolve odd length LATCHC blocks */
    if ((list[1][0] == LATCHC) && (list[0][0] & 1)) {
        /* Rule 2 */
        list[0][1]++;
        list[0][0]--;
        if (indexliste == 1) {
            list[0][1] = 1;
            list[1][1] = LATCHB;
            indexliste = 2;
        }
    }
    if (indexliste > 1) {
        for (i = 1; i < indexliste; i++) {
            if ((list[1][i] == LATCHC) && (list[0][i] & 1)) {
                /* Rule 3b */
                list[0][i - 1]++;
                list[0][i]--;
            }
        }
    }

    /* Put set data into set[] */

    read = 0;
    for (i = 0; i < indexliste; i++) {
        for (j = 0; j < list[0][i]; j++) {
            switch (list[1][i]) {
                case SHIFTA: set[read] = 'a';
                    break;
                case LATCHA: set[read] = 'A';
                    break;
                case SHIFTB: set[read] = 'b';
                    break;
                case LATCHB: set[read] = 'B';
                    break;
                case LATCHC: set[read] = 'C';
                    break;
            }
            read++;
        }
    }

    /* Adjust for strings which start with shift characters - make them latch instead */
    if (set[0] == 'a') {
        i = 0;
        do {
            set[i] = 'A';
            i++;
        } while (set[i] == 'a');
    }

    if (set[0] == 'b') {
        i = 0;
        do {
            set[i] = 'B';
            i++;
        } while (set[i] == 'b');
    }

    /* Now we can calculate how long the barcode is going to be - and stop it from
       being too long */
    last_set = ' ';
    glyph_count = 0.0;
    for (i = 0; i < sourcelen; i++) {
        if ((set[i] == 'a') || (set[i] == 'b')) {
            glyph_count = glyph_count + 1.0;
        }
        if ((fset[i] == 'f') || (fset[i] == 'n')) {
            glyph_count = glyph_count + 1.0;
        }
        if (((set[i] == 'A') || (set[i] == 'B')) || (set[i] == 'C')) {
            if (set[i] != last_set) {
                last_set = set[i];
                glyph_count = glyph_count + 1.0;
            }
        }
        if (i == 0) {
            if (fset[i] == 'F') {
                glyph_count = glyph_count + 2.0;
            }
        } else {
            if ((fset[i] == 'F') && (fset[i - 1] != 'F')) {
                glyph_count = glyph_count + 2.0;
            }
            if ((fset[i] != 'F') && (fset[i - 1] == 'F')) {
                glyph_count = glyph_count + 2.0;
            }
        }

        if (set[i] == 'C') {
            glyph_count = glyph_count + 0.5;
        } else {
            glyph_count = glyph_count + 1.0;
        }
    }
    if (glyph_count > 60.0) {
        strcpy(symbol->errtxt, "341: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* So now we know what start character to use - we can get on with it! */
    if (symbol->output_options & READER_INIT) {
        /* Reader Initialisation mode */
        switch (set[0]) {
            case 'A': /* Start A */
                strcat(dest, C128Table[103]);
                values[0] = 103;
                current_set = 'A';
                strcat(dest, C128Table[96]); /* FNC3 */
                values[1] = 96;
                bar_characters++;
                break;
            case 'B': /* Start B */
                strcat(dest, C128Table[104]);
                values[0] = 104;
                current_set = 'B';
                strcat(dest, C128Table[96]); /* FNC3 */
                values[1] = 96;
                bar_characters++;
                break;
            case 'C': /* Start C */
                strcat(dest, C128Table[104]); /* Start B */
                values[0] = 104;
                strcat(dest, C128Table[96]); /* FNC3 */
                values[1] = 96;
                strcat(dest, C128Table[99]); /* Code C */
                values[2] = 99;
                bar_characters += 2;
                current_set = 'C';
                break;
        }
    } else {
        /* Normal mode */
        switch (set[0]) {
            case 'A': /* Start A */
                strcat(dest, C128Table[103]);
                values[0] = 103;
                current_set = 'A';
                break;
            case 'B': /* Start B */
                strcat(dest, C128Table[104]);
                values[0] = 104;
                current_set = 'B';
                break;
            case 'C': /* Start C */
                strcat(dest, C128Table[105]);
                values[0] = 105;
                current_set = 'C';
                break;
        }
    }
    bar_characters++;
    last_set = set[0];

    if (fset[0] == 'F') {
        switch (current_set) {
            case 'A':
                strcat(dest, C128Table[101]);
                strcat(dest, C128Table[101]);
                values[bar_characters] = 101;
                values[bar_characters + 1] = 101;
                break;
            case 'B':
                strcat(dest, C128Table[100]);
                strcat(dest, C128Table[100]);
                values[bar_characters] = 100;
                values[bar_characters + 1] = 100;
                break;
        }
        bar_characters += 2;
        f_state = 1;
    }

    /* Encode the data */
    read = 0;
    do {

        if ((read != 0) && (set[read] != current_set)) {
            /* Latch different code set */
            switch (set[read]) {
                case 'A': strcat(dest, C128Table[101]);
                    values[bar_characters] = 101;
                    bar_characters++;
                    current_set = 'A';
                    break;
                case 'B': strcat(dest, C128Table[100]);
                    values[bar_characters] = 100;
                    bar_characters++;
                    current_set = 'B';
                    break;
                case 'C': strcat(dest, C128Table[99]);
                    values[bar_characters] = 99;
                    bar_characters++;
                    current_set = 'C';
                    break;
            }
        }

        if (read != 0) {
            if ((fset[read] == 'F') && (f_state == 0)) {
                /* Latch beginning of extended mode */
                switch (current_set) {
                    case 'A':
                        strcat(dest, C128Table[101]);
                        strcat(dest, C128Table[101]);
                        values[bar_characters] = 101;
                        values[bar_characters + 1] = 101;
                        break;
                    case 'B':
                        strcat(dest, C128Table[100]);
                        strcat(dest, C128Table[100]);
                        values[bar_characters] = 100;
                        values[bar_characters + 1] = 100;
                        break;
                }
                bar_characters += 2;
                f_state = 1;
            }
            if ((fset[read] == ' ') && (f_state == 1)) {
                /* Latch end of extended mode */
                switch (current_set) {
                    case 'A':
                        strcat(dest, C128Table[101]);
                        strcat(dest, C128Table[101]);
                        values[bar_characters] = 101;
                        values[bar_characters + 1] = 101;
                        break;
                    case 'B':
                        strcat(dest, C128Table[100]);
                        strcat(dest, C128Table[100]);
                        values[bar_characters] = 100;
                        values[bar_characters + 1] = 100;
                        break;
                }
                bar_characters += 2;
                f_state = 0;
            }
        }

        if ((fset[read] == 'f') || (fset[read] == 'n')) {
            /* Shift to or from extended mode */
            switch (current_set) {
                case 'A':
                    strcat(dest, C128Table[101]); /* FNC 4 */
                    values[bar_characters] = 101;
                    break;
                case 'B':
                    strcat(dest, C128Table[100]); /* FNC 4 */
                    values[bar_characters] = 100;
                    break;
            }
            bar_characters++;
        }

        if ((set[read] == 'a') || (set[read] == 'b')) {
            /* Insert shift character */
            strcat(dest, C128Table[98]);
            values[bar_characters] = 98;
            bar_characters++;
        }

        switch (set[read]) { /* Encode data characters */
            case 'a':
            case 'A': c128_set_a(source[read], dest, values, &bar_characters);
                read++;
                break;
            case 'b':
            case 'B': c128_set_b(source[read], dest, values, &bar_characters);
                read++;
                break;
            case 'C': c128_set_c(source[read], source[read + 1], dest, values, &bar_characters);
                read += 2;
                break;
        }

    } while (read < sourcelen);

    /* check digit calculation */
    total_sum = 0;

    for (i = 0; i < bar_characters; i++) {
        if (i > 0) {
            values[i] *= i;
        }
        total_sum += values[i];
    }
    strcat(dest, C128Table[total_sum % 103]);

    /* Stop character */
    strcat(dest, C128Table[106]);
    expand(symbol, dest);
    return error_number;
}

/* Handle EAN-128 (Now known as GS1-128) */
int ean_128(struct zint_symbol *symbol, unsigned char source[], const size_t length) {
    int i, j, values[170], bar_characters, read, total_sum;
    int error_number, indexchaine, indexliste;
    char set[170], mode, last_set;
    float glyph_count;
    char dest[1000];
    int separator_row, linkage_flag, c_count;
#ifndef _MSC_VER
    char reduced[length + 1];
#else
    char* reduced = (char*) _alloca(length + 1);
#endif
    error_number = 0;
    strcpy(dest, "");
    linkage_flag = 0;

    j = 0;
    bar_characters = 0;
    separator_row = 0;

    memset(values, 0, sizeof (values));
    memset(set, ' ', sizeof (set));

    if (length > 160) {
        /* This only blocks rediculously long input - the actual length of the
        resulting barcode depends on the type of data, so this is trapped later */
        strcpy(symbol->errtxt, "342: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    for (i = 0; i < length; i++) {
        if (source[i] == '\0') {
            /* Null characters not allowed! */
            strcpy(symbol->errtxt, "343: NULL character in input data");
            return ZINT_ERROR_INVALID_DATA;
        }
    }

    /* if part of a composite symbol make room for the separator pattern */
    if (symbol->symbology == BARCODE_EAN128_CC) {
        separator_row = symbol->rows;
        symbol->row_height[symbol->rows] = 1;
        symbol->rows += 1;
    }

    if (symbol->input_mode != GS1_MODE) {
        /* GS1 data has not been checked yet */
        error_number = gs1_verify(symbol, source, length, reduced);
        if (error_number != 0) {
            return error_number;
        }
    }

    /* Decide on mode using same system as PDF417 and rules of ISO 15417 Annex E */
    indexliste = 0;
    indexchaine = 0;

    mode = parunmodd(reduced[indexchaine]);
    if (reduced[indexchaine] == '[') {
        mode = ABORC;
    }

    for (i = 0; i < 170; i++) {
        list[0][i] = 0;
    }

    do {
        list[1][indexliste] = mode;
        while ((list[1][indexliste] == mode) && (indexchaine < (int) strlen(reduced))) {
            list[0][indexliste]++;
            indexchaine++;
            mode = parunmodd(reduced[indexchaine]);
            if (reduced[indexchaine] == '[') {
                mode = ABORC;
            }
        }
        indexliste++;
    } while (indexchaine < (int) strlen(reduced));

    dxsmooth(&indexliste);

    /* Put set data into set[] */
    read = 0;
    for (i = 0; i < indexliste; i++) {
        for (j = 0; j < list[0][i]; j++) {
            switch (list[1][i]) {
                case SHIFTA: set[read] = 'a';
                    break;
                case LATCHA: set[read] = 'A';
                    break;
                case SHIFTB: set[read] = 'b';
                    break;
                case LATCHB: set[read] = 'B';
                    break;
                case LATCHC: set[read] = 'C';
                    break;
            }
            read++;
        }
    }

    /* Watch out for odd-length Mode C blocks */
    c_count = 0;
    for (i = 0; i < read; i++) {
        if (set[i] == 'C') {
            if (reduced[i] == '[') {
                if (c_count & 1) {
                    if ((i - c_count) != 0) {
                        set[i - c_count] = 'B';
                    } else {
                        set[i - 1] = 'B';
                    }
                }
                c_count = 0;
            } else {
                c_count++;
            }
        } else {
            if (c_count & 1) {
                if ((i - c_count) != 0) {
                    set[i - c_count] = 'B';
                } else {
                    set[i - 1] = 'B';
                }
            }
            c_count = 0;
        }
    }
    if (c_count & 1) {
        if ((i - c_count) != 0) {
            set[i - c_count] = 'B';
        } else {
            set[i - 1] = 'B';
        }
    }
    for (i = 1; i < read - 1; i++) {
        if ((set[i] == 'C') && ((set[i - 1] == 'B') && (set[i + 1] == 'B'))) {
            set[i] = 'B';
        }
    }

    /* Now we can calculate how long the barcode is going to be - and stop it from
    being too long */
    last_set = ' ';
    glyph_count = 0.0;
    for (i = 0; i < (int) strlen(reduced); i++) {
        if ((set[i] == 'a') || (set[i] == 'b')) {
            glyph_count = glyph_count + 1.0;
        }
        if (((set[i] == 'A') || (set[i] == 'B')) || (set[i] == 'C')) {
            if (set[i] != last_set) {
                last_set = set[i];
                glyph_count = glyph_count + 1.0;
            }
        }

        if ((set[i] == 'C') && (reduced[i] != '[')) {
            glyph_count = glyph_count + 0.5;
        } else {
            glyph_count = glyph_count + 1.0;
        }
    }
    if (glyph_count > 60.0) {
        strcpy(symbol->errtxt, "344: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* So now we know what start character to use - we can get on with it! */
    switch (set[0]) {
        case 'A': /* Start A */
            strcat(dest, C128Table[103]);
            values[0] = 103;
            break;
        case 'B': /* Start B */
            strcat(dest, C128Table[104]);
            values[0] = 104;
            break;
        case 'C': /* Start C */
            strcat(dest, C128Table[105]);
            values[0] = 105;
            break;
    }
    bar_characters++;

    strcat(dest, C128Table[102]);
    values[1] = 102;
    bar_characters++;

    /* Encode the data */
    read = 0;
    do {

        if ((read != 0) && (set[read] != set[read - 1])) { /* Latch different code set */
            switch (set[read]) {
                case 'A': strcat(dest, C128Table[101]);
                    values[bar_characters] = 101;
                    bar_characters++;
                    break;
                case 'B': strcat(dest, C128Table[100]);
                    values[bar_characters] = 100;
                    bar_characters++;
                    break;
                case 'C': strcat(dest, C128Table[99]);
                    values[bar_characters] = 99;
                    bar_characters++;
                    break;
            }
        }

        if ((set[read] == 'a') || (set[read] == 'b')) {
            /* Insert shift character */
            strcat(dest, C128Table[98]);
            values[bar_characters] = 98;
            bar_characters++;
        }

        if (reduced[read] != '[') {
            switch (set[read]) { /* Encode data characters */
                case 'A':
                case 'a':
                    c128_set_a(reduced[read], dest, values, &bar_characters);
                    read++;
                    break;
                case 'B':
                case 'b':
                    c128_set_b(reduced[read], dest, values, &bar_characters);
                    read++;
                    break;
                case 'C':
                    c128_set_c(reduced[read], reduced[read + 1], dest, values, &bar_characters);
                    read += 2;
                    break;
            }
        } else {
            strcat(dest, C128Table[102]);
            values[bar_characters] = 102;
            bar_characters++;
            read++;
        }
    } while (read < (int) strlen(reduced));

    /* "...note that the linkage flag is an extra code set character between
    the last data character and the Symbol Check Character" (GS1 Specification) */

    /* Linkage flags in GS1-128 are determined by ISO/IEC 24723 section 7.4 */

    switch (symbol->option_1) {
        case 1:
        case 2:
            /* CC-A or CC-B 2D component */
            switch (set[strlen(reduced) - 1]) {
                case 'A': linkage_flag = 100;
                    break;
                case 'B': linkage_flag = 99;
                    break;
                case 'C': linkage_flag = 101;
                    break;
            }
            break;
        case 3:
            /* CC-C 2D component */
            switch (set[strlen(reduced) - 1]) {
                case 'A': linkage_flag = 99;
                    break;
                case 'B': linkage_flag = 101;
                    break;
                case 'C': linkage_flag = 100;
                    break;
            }
            break;
    }

    if (linkage_flag != 0) {
        strcat(dest, C128Table[linkage_flag]);
        values[bar_characters] = linkage_flag;
        bar_characters++;
    }

    /* check digit calculation */
    total_sum = 0;
    for (i = 0; i < bar_characters; i++) {
        if (i > 0) {
            values[i] *= i;

        }
        total_sum += values[i];
    }
    strcat(dest, C128Table[total_sum % 103]);
    values[bar_characters] = total_sum % 103;
    bar_characters++;

    /* Stop character */
    strcat(dest, C128Table[106]);
    values[bar_characters] = 106;
    bar_characters++;
    expand(symbol, dest);

    /* Add the separator pattern for composite symbols */
    if (symbol->symbology == BARCODE_EAN128_CC) {
        for (i = 0; i < symbol->width; i++) {
            if (!(module_is_set(symbol, separator_row + 1, i))) {
                set_module(symbol, separator_row, i);
            }
        }
    }

    for (i = 0; i < length; i++) {
        if ((source[i] != '[') && (source[i] != ']')) {
            symbol->text[i] = source[i];
        }
        if (source[i] == '[') {
            symbol->text[i] = '(';
        }
        if (source[i] == ']') {
            symbol->text[i] = ')';
        }
    }

    return error_number;
}

/* Add check digit if encoding an NVE18 symbol */
int nve_18(struct zint_symbol *symbol, unsigned char source[], int length) {
    int error_number, zeroes, i, nve_check, total_sum, sourcelen;
    unsigned char ean128_equiv[25];

    memset(ean128_equiv, 0, 25);
    sourcelen = length;

    if (sourcelen > 17) {
        strcpy(symbol->errtxt, "345: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "346: Invalid characters in data");
        return error_number;
    }
    zeroes = 17 - sourcelen;
    strcpy((char *) ean128_equiv, "[00]");
    memset(ean128_equiv + 4, '0', zeroes);
    strcpy((char*) ean128_equiv + 4 + zeroes, (char*) source);

    total_sum = 0;
    for (i = sourcelen - 1; i >= 0; i--) {
        total_sum += ctoi(source[i]);

        if (!(i & 1)) {
            total_sum += 2 * ctoi(source[i]);
        }
    }
    nve_check = 10 - total_sum % 10;
    if (nve_check == 10) {
        nve_check = 0;
    }
    ean128_equiv[21] = itoc(nve_check);
    ean128_equiv[22] = '\0';

    error_number = ean_128(symbol, ean128_equiv, ustrlen(ean128_equiv));

    return error_number;
}

/* EAN-14 - A version of EAN-128 */
int ean_14(struct zint_symbol *symbol, unsigned char source[], int length) {
    int i, count, check_digit;
    int error_number, zeroes;
    unsigned char ean128_equiv[20];

    if (length > 13) {
        strcpy(symbol->errtxt, "347: Input wrong length");
        return ZINT_ERROR_TOO_LONG;
    }

    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "348: Invalid character in data");
        return error_number;
    }

    zeroes = 13 - length;
    strcpy((char*) ean128_equiv, "[01]");
    memset(ean128_equiv + 4, '0', zeroes);
    ustrcpy(ean128_equiv + 4 + zeroes, source);

    count = 0;
    for (i = length - 1; i >= 0; i--) {
        count += ctoi(source[i]);

        if (!(i & 1)) {
            count += 2 * ctoi(source[i]);
        }
    }
    check_digit = 10 - (count % 10);
    if (check_digit == 10) {
        check_digit = 0;
    }
    ean128_equiv[17] = itoc(check_digit);
    ean128_equiv[18] = '\0';

    error_number = ean_128(symbol, ean128_equiv, ustrlen(ean128_equiv));

    return error_number;
}


