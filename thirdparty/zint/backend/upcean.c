/*  upcean.c - Handles UPC, EAN and ISBN

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

#define SODIUM	"0123456789+"
#define EAN2	102
#define EAN5	105

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

/* UPC and EAN tables checked against EN 797:1996 */

static const char *UPCParity0[10] = {
    /* Number set for UPC-E symbol (EN Table 4) */
    "BBBAAA", "BBABAA", "BBAABA", "BBAAAB", "BABBAA", "BAABBA", "BAAABB",
    "BABABA", "BABAAB", "BAABAB"
};

static const char *UPCParity1[10] = {
    /* Not covered by BS EN 797:1995 */
    "AAABBB", "AABABB", "AABBAB", "AABBBA", "ABAABB", "ABBAAB", "ABBBAA",
    "ABABAB", "ABABBA", "ABBABA"
};

static const char *EAN2Parity[4] = {
    /* Number sets for 2-digit add-on (EN Table 6) */
    "AA", "AB", "BA", "BB"
};

static const char *EAN5Parity[10] = {
    /* Number set for 5-digit add-on (EN Table 7) */
    "BBAAA", "BABAA", "BAABA", "BAAAB", "ABBAA", "AABBA", "AAABB", "ABABA",
    "ABAAB", "AABAB"
};

static const char *EAN13Parity[10] = {
    /* Left hand of the EAN-13 symbol (EN Table 3) */
    "AAAAA", "ABABB", "ABBAB", "ABBBA", "BAABB", "BBAAB", "BBBAA", "BABAB",
    "BABBA", "BBABA"
};

static const char *EANsetA[10] = {
    /* Representation set A and C (EN Table 1) */
    "3211", "2221", "2122", "1411", "1132", "1231", "1114", "1312", "1213", "3112"
};

static const char *EANsetB[10] = {
    /* Representation set B (EN Table 1) */
    "1123", "1222", "2212", "1141", "2311", "1321", "4111", "2131", "3121", "2113"
};

/* Calculate the correct check digit for a UPC barcode */
char upc_check(char source[]) {
    unsigned int i, count, check_digit;

    count = 0;

    for (i = 0; i < strlen(source); i++) {
        count += ctoi(source[i]);

        if (!(i & 1)) {
            count += 2 * (ctoi(source[i]));
        }
    }

    check_digit = 10 - (count % 10);
    if (check_digit == 10) {
        check_digit = 0;
    }
    return itoc(check_digit);
}

/* UPC A is usually used for 12 digit numbers, but this function takes a source of any length */
void upca_draw(char source[], char dest[]) {
    unsigned int i, half_way;

    half_way = strlen(source) / 2;

    /* start character */
    strcat(dest, "111");

    for (i = 0; i <= strlen(source); i++) {
        if (i == half_way) {
            /* middle character - separates manufacturer no. from product no. */
            /* also inverts right hand characters */
            strcat(dest, "11111");
        }

        lookup(NEON, EANsetA, source[i], dest);
    }

    /* stop character */
    strcat(dest, "111");
}

/* Make a UPC A barcode when we haven't been given the check digit */
int upca(struct zint_symbol *symbol, unsigned char source[], char dest[]) {
    int length;
    char gtin[15];

    strcpy(gtin, (char*) source);
    length = strlen(gtin);

    if (length == 11) {
        gtin[length] = upc_check(gtin);
        gtin[length + 1] = '\0';
    } else {
        gtin[length - 1] = '\0';
        if (source[length - 1] != upc_check(gtin)) {
            strcpy(symbol->errtxt, "270: Invalid check digit");
            return ZINT_ERROR_INVALID_DATA;
        }
        gtin[length - 1] = upc_check(gtin);
    }
    upca_draw(gtin, dest);
    ustrcpy(symbol->text, (unsigned char*) gtin);
    return 0;
}

/* UPC E is a zero-compressed version of UPC A */
int upce(struct zint_symbol *symbol, unsigned char source[], char dest[]) {
    unsigned int i, num_system;
    char emode, equivalent[12], check_digit, parity[8], temp[8];
    char hrt[9];

    /* Two number systems can be used - system 0 and system 1 */
    if (symbol->symbology != BARCODE_UPCE_CHK) {
        /* No check digit in input data */
        if (ustrlen(source) == 7) {
            switch (source[0]) {
                case '0': num_system = 0;
                    break;
                case '1': num_system = 1;
                    break;
                default: num_system = 0;
                    source[0] = '0';
                    break;
            }
            strcpy(temp, (char*) source);
            strcpy(hrt, (char*) source);
            for (i = 1; i <= 7; i++) {
                source[i - 1] = temp[i];
            }
        } else {
            num_system = 0;
            hrt[0] = '0';
            hrt[1] = '\0';
            strcat(hrt, (char*) source);
        }
    } else {
        /* Check digit is included in input data */
        if (ustrlen(source) == 8) {
            switch (source[0]) {
                case '0': num_system = 0;
                    break;
                case '1': num_system = 1;
                    break;
                default: num_system = 0;
                    source[0] = '0';
                    break;
            }
            strcpy(temp, (char*) source);
            strcpy(hrt, (char*) source);
            for (i = 1; i <= 7; i++) {
                source[i - 1] = temp[i];
            }
        } else {
            num_system = 0;
            hrt[0] = '0';
            hrt[1] = '\0';
            strcat(hrt, (char*) source);
        }
    }

    /* Expand the zero-compressed UPCE code to make a UPCA equivalent (EN Table 5) */
    emode = source[5];
    for (i = 0; i < 11; i++) {
        equivalent[i] = '0';
    }
    if (num_system == 1) {
        equivalent[0] = temp[0];
    }
    equivalent[1] = source[0];
    equivalent[2] = source[1];
    equivalent[11] = '\0';

    switch (emode) {
        case '0':
        case '1':
        case '2':
            equivalent[3] = emode;
            equivalent[8] = source[2];
            equivalent[9] = source[3];
            equivalent[10] = source[4];
            break;
        case '3':
            equivalent[3] = source[2];
            equivalent[9] = source[3];
            equivalent[10] = source[4];
            if (((source[2] == '0') || (source[2] == '1')) || (source[2] == '2')) {
                /* Note 1 - "X3 shall not be equal to 0, 1 or 2" */
                strcpy(symbol->errtxt, "271: Invalid UPC-E data");
                return ZINT_ERROR_INVALID_DATA;
            }
            break;
        case '4':
            equivalent[3] = source[2];
            equivalent[4] = source[3];
            equivalent[10] = source[4];
            if (source[3] == '0') {
                /* Note 2 - "X4 shall not be equal to 0" */
                strcpy(symbol->errtxt, "272: Invalid UPC-E data");
                return ZINT_ERROR_INVALID_DATA;
            }
            break;
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            equivalent[3] = source[2];
            equivalent[4] = source[3];
            equivalent[5] = source[4];
            equivalent[10] = emode;
            if (source[4] == '0') {
                /* Note 3 - "X5 shall not be equal to 0" */
                strcpy(symbol->errtxt, "273: Invalid UPC-E data");
                return ZINT_ERROR_INVALID_DATA;
            }
            break;
    }

    /* Get the check digit from the expanded UPCA code */

    check_digit = upc_check(equivalent);

    /* Use the number system and check digit information to choose a parity scheme */
    if (num_system == 1) {
        strcpy(parity, UPCParity1[ctoi(check_digit)]);
    } else {
        strcpy(parity, UPCParity0[ctoi(check_digit)]);
    }

    /* Take all this information and make the barcode pattern */

    /* start character */
    strcat(dest, "111");

    for (i = 0; i <= ustrlen(source); i++) {
        switch (parity[i]) {
            case 'A': lookup(NEON, EANsetA, source[i], dest);
                break;
            case 'B': lookup(NEON, EANsetB, source[i], dest);
                break;
        }
    }

    /* stop character */
    strcat(dest, "111111");

    if (symbol->symbology != BARCODE_UPCE_CHK) {
        hrt[7] = check_digit;
        hrt[8] = '\0';
    } else {
        if (hrt[7] != check_digit) {
            strcpy(symbol->errtxt, "274: Invalid check digit");
            return ZINT_ERROR_INVALID_DATA;
        }
    }
    ustrcpy(symbol->text, (unsigned char*) hrt);
    return 0;
}

/* EAN-2 and EAN-5 add-on codes */
void add_on(unsigned char source[], char dest[], int mode) {
    char parity[6];
    unsigned int i, code_type;

    /* If an add-on then append with space */
    if (mode != 0) {
        strcat(dest, "9");
    }

    /* Start character */
    strcat(dest, "112");

    /* Determine EAN2 or EAN5 add-on */
    if (ustrlen(source) == 2) {
        code_type = EAN2;
    } else {
        code_type = EAN5;
    }

    /* Calculate parity for EAN2 */
    if (code_type == EAN2) {
        int code_value, parity_bit;

        code_value = (10 * ctoi(source[0])) + ctoi(source[1]);
        parity_bit = code_value % 4;
        strcpy(parity, EAN2Parity[parity_bit]);
    }

    if (code_type == EAN5) {
        int values[6], parity_sum, parity_bit;

        for (i = 0; i < 6; i++) {
            values[i] = ctoi(source[i]);
        }

        parity_sum = (3 * (values[0] + values[2] + values[4]));
        parity_sum += (9 * (values[1] + values[3]));

        parity_bit = parity_sum % 10;
        strcpy(parity, EAN5Parity[parity_bit]);
    }

    for (i = 0; i < ustrlen(source); i++) {
        switch (parity[i]) {
            case 'A': lookup(NEON, EANsetA, source[i], dest);
                break;
            case 'B': lookup(NEON, EANsetB, source[i], dest);
                break;
        }

        /* Glyph separator */
        if (i != (ustrlen(source) - 1)) {
            strcat(dest, "11");
        }
    }
}

/* ************************ EAN-13 ****************** */

/* Calculate the correct check digit for a EAN-13 barcode */
char ean_check(char source[]) {
    int i;
    unsigned int h, count, check_digit;

    count = 0;

    h = strlen(source);
    for (i = h - 1; i >= 0; i--) {
        count += ctoi(source[i]);

        if (i & 1) {
            count += 2 * ctoi(source[i]);
        }
    }
    check_digit = 10 - (count % 10);
    if (check_digit == 10) {
        check_digit = 0;
    }
    return itoc(check_digit);
}

int ean13(struct zint_symbol *symbol, unsigned char source[], char dest[]) {
    unsigned int length, i, half_way;
    char parity[6];
    char gtin[15];

    strcpy(parity, "");
    strcpy(gtin, (char*) source);

    /* Add the appropriate check digit */
    length = strlen(gtin);

    if (length == 12) {
        gtin[length] = ean_check(gtin);
        gtin[length + 1] = '\0';
    } else {
        gtin[length - 1] = '\0';
        if (source[length - 1] != ean_check(gtin)) {
            strcpy(symbol->errtxt, "275: Invalid check digit");
            return ZINT_ERROR_INVALID_DATA;
        }
        gtin[length - 1] = ean_check(gtin);
    }

    /* Get parity for first half of the symbol */
    lookup(SODIUM, EAN13Parity, gtin[0], parity);

    /* Now get on with the cipher */
    half_way = 7;

    /* start character */
    strcat(dest, "111");
    length = strlen(gtin);
    for (i = 1; i <= length; i++) {
        if (i == half_way) {
            /* middle character - separates manufacturer no. from product no. */
            /* also inverses right hand characters */
            strcat(dest, "11111");
        }

        if (((i > 1) && (i < 7)) && (parity[i - 2] == 'B')) {
            lookup(NEON, EANsetB, gtin[i], dest);
        } else {
            lookup(NEON, EANsetA, gtin[i], dest);
        }
    }

    /* stop character */
    strcat(dest, "111");

    ustrcpy(symbol->text, (unsigned char*) gtin);
    return 0;
}

/* Make an EAN-8 barcode when we haven't been given the check digit */
int ean8(struct zint_symbol *symbol, unsigned char source[], char dest[]) {
    /* EAN-8 is basically the same as UPC-A but with fewer digits */
    int length;
    char gtin[10];

    strcpy(gtin, (char*) source);
    length = strlen(gtin);

    if (length == 7) {
        gtin[length] = upc_check(gtin);
        gtin[length + 1] = '\0';
    } else {
        gtin[length - 1] = '\0';
        if (source[length - 1] != upc_check(gtin)) {
            strcpy(symbol->errtxt, "276: Invalid check digit");
            return ZINT_ERROR_INVALID_DATA;
        }
        gtin[length - 1] = upc_check(gtin);
    }
    upca_draw(gtin, dest);
    ustrcpy(symbol->text, (unsigned char*) gtin);

    return 0;
}

/* For ISBN(13) only */
char isbn13_check(unsigned char source[]) {
    unsigned int i, weight, sum, check, h;

    sum = 0;
    weight = 1;
    h = ustrlen(source) - 1;

    for (i = 0; i < h; i++) {
        sum += ctoi(source[i]) * weight;
        if (weight == 1) weight = 3;
        else weight = 1;
    }

    check = sum % 10;
    check = 10 - check;
    if (check == 10) check = 0;
    return itoc(check);
}

/* For ISBN(10) and SBN only */
char isbn_check(unsigned char source[]) {
    unsigned int i, weight, sum, check, h;
    char check_char;

    sum = 0;
    weight = 1;
    h = ustrlen(source) - 1;

    for (i = 0; i < h; i++) {
        sum += ctoi(source[i]) * weight;
        weight++;
    }

    check = sum % 11;
    check_char = itoc(check);
    if (check == 10) {
        check_char = 'X';
    }
    return check_char;
}

/* Make an EAN-13 barcode from an SBN or ISBN */
static int isbn(struct zint_symbol *symbol, unsigned char source[], const size_t src_len, char dest[]) {
    int i, error_number;
    char check_digit;

    to_upper(source);
    error_number = is_sane("0123456789X", source, src_len);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "277: Invalid characters in input");
        return error_number;
    }

    /* Input must be 9, 10 or 13 characters */
    if (((src_len < 9) || (src_len > 13)) || ((src_len > 10) && (src_len < 13))) {
        strcpy(symbol->errtxt, "278: Input wrong length");
        return ZINT_ERROR_TOO_LONG;
    }

    if (src_len == 13) /* Using 13 character ISBN */ {
        if (!(((source[0] == '9') && (source[1] == '7')) &&
                ((source[2] == '8') || (source[2] == '9')))) {
            strcpy(symbol->errtxt, "279: Invalid ISBN");
            return ZINT_ERROR_INVALID_DATA;
        }

        check_digit = isbn13_check(source);
        if (source[src_len - 1] != check_digit) {
            strcpy(symbol->errtxt, "280: Incorrect ISBN check");
            return ZINT_ERROR_INVALID_CHECK;
        }
        source[12] = '\0';

        ean13(symbol, source, dest);
    }

    if (src_len == 10) /* Using 10 digit ISBN */ {
        check_digit = isbn_check(source);
        if (check_digit != source[src_len - 1]) {
            strcpy(symbol->errtxt, "281: Incorrect ISBN check");
            return ZINT_ERROR_INVALID_CHECK;
        }
        for (i = 13; i > 0; i--) {
            source[i] = source[i - 3];
        }
        source[0] = '9';
        source[1] = '7';
        source[2] = '8';
        source[12] = '\0';

        ean13(symbol, source, dest);
    }

    if (src_len == 9) /* Using 9 digit SBN */ {
        /* Add leading zero */
        for (i = 10; i > 0; i--) {
            source[i] = source[i - 1];
        }
        source[0] = '0';

        /* Verify check digit */
        check_digit = isbn_check(source);
        if (check_digit != source[ustrlen(source) - 1]) {
            strcpy(symbol->errtxt, "282: Incorrect SBN check");
            return ZINT_ERROR_INVALID_CHECK;
        }

        /* Convert to EAN-13 number */
        for (i = 13; i > 0; i--) {
            source[i] = source[i - 3];
        }
        source[0] = '9';
        source[1] = '7';
        source[2] = '8';
        source[12] = '\0';

        ean13(symbol, source, dest);
    }

    return 0;
}

/* Add leading zeroes to EAN and UPC strings */
void ean_leading_zeroes(struct zint_symbol *symbol, unsigned char source[], unsigned char local_source[]) {
    unsigned char first_part[20], second_part[20], zfirst_part[20], zsecond_part[20];
    int with_addon = 0;
    int first_len = 0, second_len = 0, zfirst_len = 0, zsecond_len = 0, i, h;

    h = ustrlen(source);
    for (i = 0; i < h; i++) {
        if (source[i] == '+') {
            with_addon = 1;
        } else {
            if (with_addon == 0) {
                first_len++;
            } else {
                second_len++;
            }
        }
    }

    ustrcpy(first_part, (unsigned char *) "");
    ustrcpy(second_part, (unsigned char *) "");
    ustrcpy(zfirst_part, (unsigned char *) "");
    ustrcpy(zsecond_part, (unsigned char *) "");

    /* Split input into two strings */
    for (i = 0; i < first_len; i++) {
        first_part[i] = source[i];
        first_part[i + 1] = '\0';
    }

    for (i = 0; i < second_len; i++) {
        second_part[i] = source[i + first_len + 1];
        second_part[i + 1] = '\0';
    }

    /* Calculate target lengths */
    if (second_len <= 5) {
        zsecond_len = 5;
    }
    if (second_len <= 2) {
        zsecond_len = 2;
    }
    if (second_len == 0) {
        zsecond_len = 0;
    }
    switch (symbol->symbology) {
        case BARCODE_EANX:
        case BARCODE_EANX_CC:
            if (first_len <= 12) {
                zfirst_len = 12;
            }
            if (first_len <= 7) {
                zfirst_len = 7;
            }
            if (second_len == 0) {
                if (first_len <= 5) {
                    zfirst_len = 5;
                }
                if (first_len <= 2) {
                    zfirst_len = 2;
                }
            }
            break;
        case BARCODE_EANX_CHK:
            if (first_len <= 13) {
                zfirst_len = 13;
            }
            if (first_len <= 8) {
                zfirst_len = 8;
            }
            if (second_len == 0) {
                if (first_len <= 5) {
                    zfirst_len = 5;
                }
                if (first_len <= 2) {
                    zfirst_len = 2;
                }
            }
            break;
        case BARCODE_UPCA:
        case BARCODE_UPCA_CC:
            zfirst_len = 11;
            break;
        case BARCODE_UPCA_CHK:
            zfirst_len = 12;
            break;
        case BARCODE_UPCE:
        case BARCODE_UPCE_CC:
            if (first_len == 7) {
                zfirst_len = 7;
            }
            if (first_len <= 6) {
                zfirst_len = 6;
            }
            break;
        case BARCODE_UPCE_CHK:
            if (first_len == 8) {
                zfirst_len = 8;
            }
            if (first_len <= 7) {
                zfirst_len = 7;
            }
            break;
        case BARCODE_ISBNX:
            if (first_len <= 9) {
                zfirst_len = 9;
            }
            break;
    }


    /* Add leading zeroes */
    for (i = 0; i < (zfirst_len - first_len); i++) {
        strcat((char*) zfirst_part, "0");
    }
    strcat((char*) zfirst_part, (char*) first_part);
    for (i = 0; i < (zsecond_len - second_len); i++) {
        strcat((char*) zsecond_part, "0");
    }
    strcat((char*) zsecond_part, (char*) second_part);

    /* Copy adjusted data back to local_source */
    strcat((char*) local_source, (char*) zfirst_part);
    if (zsecond_len != 0) {
        strcat((char*) local_source, "+");
        strcat((char*) local_source, (char*) zsecond_part);
    }
}

/* splits string to parts before and after '+' parts */
int eanx(struct zint_symbol *symbol, unsigned char source[], int src_len) {
    unsigned char first_part[20] = {0}, second_part[20] = {0}, dest[1000] = {0};
    unsigned char local_source[20] = {0};
    unsigned int latch, reader, writer, with_addon;
    int error_number, i;


    with_addon = FALSE;
    latch = FALSE;
    writer = 0;

    if (src_len > 19) {
        strcpy(symbol->errtxt, "283: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    if (symbol->symbology != BARCODE_ISBNX) {
        /* ISBN has it's own checking routine */
        error_number = is_sane("0123456789+", source, src_len);
        if (error_number == ZINT_ERROR_INVALID_DATA) {
            strcpy(symbol->errtxt, "284: Invalid characters in data");
            return error_number;
        }
    } else {
        error_number = is_sane("0123456789Xx", source, src_len);
        if (error_number == ZINT_ERROR_INVALID_DATA) {
            strcpy(symbol->errtxt, "285: Invalid characters in input");
            return error_number;
        }
    }

    /* Add leading zeroes */
    ustrcpy(local_source, (unsigned char *) "");
    if (symbol->symbology == BARCODE_ISBNX) {
        to_upper(local_source);
    }

    ean_leading_zeroes(symbol, source, local_source);

    for (reader = 0; reader < ustrlen(local_source); reader++) {
        if (local_source[reader] == '+') {
            with_addon = TRUE;
        }
    }

    reader = 0;
    if (with_addon) {
        do {
            if (local_source[reader] == '+') {
                first_part[writer] = '\0';
                latch = TRUE;
                reader++;
                writer = 0;
            }

            if (latch) {
                second_part[writer] = local_source[reader];
                reader++;
                writer++;
            } else {
                first_part[writer] = local_source[reader];
                reader++;
                writer++;
            }
        } while (reader <= ustrlen(local_source));
    } else {
        strcpy((char*) first_part, (char*) local_source);
    }

    switch (symbol->symbology) {
        case BARCODE_EANX:
        case BARCODE_EANX_CHK:
            switch (ustrlen(first_part)) {
                case 2: add_on(first_part, (char*) dest, 0);
                    ustrcpy(symbol->text, first_part);
                    break;
                case 5: add_on(first_part, (char*) dest, 0);
                    ustrcpy(symbol->text, first_part);
                    break;
                case 7:
                case 8: error_number = ean8(symbol, first_part, (char*) dest);
                    break;
                case 12:
                case 13: error_number = ean13(symbol, first_part, (char*) dest);
                    break;
                default: strcpy(symbol->errtxt, "286: Invalid length input");
                    return ZINT_ERROR_TOO_LONG;
            }
            break;
        case BARCODE_EANX_CC:
            switch (ustrlen(first_part)) { /* Adds vertical separator bars according to ISO/IEC 24723 section 11.4 */
                case 7: set_module(symbol, symbol->rows, 1);
                    set_module(symbol, symbol->rows, 67);
                    set_module(symbol, symbol->rows + 1, 0);
                    set_module(symbol, symbol->rows + 1, 68);
                    set_module(symbol, symbol->rows + 2, 1);
                    set_module(symbol, symbol->rows + 1, 67);
                    symbol->row_height[symbol->rows] = 2;
                    symbol->row_height[symbol->rows + 1] = 2;
                    symbol->row_height[symbol->rows + 2] = 2;
                    symbol->rows += 3;
                    error_number = ean8(symbol, first_part, (char*) dest);
                    break;
                case 12:set_module(symbol, symbol->rows, 1);
                    set_module(symbol, symbol->rows, 95);
                    set_module(symbol, symbol->rows + 1, 0);
                    set_module(symbol, symbol->rows + 1, 96);
                    set_module(symbol, symbol->rows + 2, 1);
                    set_module(symbol, symbol->rows + 2, 95);
                    symbol->row_height[symbol->rows] = 2;
                    symbol->row_height[symbol->rows + 1] = 2;
                    symbol->row_height[symbol->rows + 2] = 2;
                    symbol->rows += 3;
                    error_number = ean13(symbol, first_part, (char*) dest);
                    break;
                default: strcpy(symbol->errtxt, "287: Invalid length EAN input");
                    return ZINT_ERROR_TOO_LONG;
            }
            break;
        case BARCODE_UPCA:
        case BARCODE_UPCA_CHK:
            if ((ustrlen(first_part) == 11) || (ustrlen(first_part) == 12)) {
                error_number = upca(symbol, first_part, (char*) dest);
            } else {
                strcpy(symbol->errtxt, "288: Input wrong length (C6I)");
                return ZINT_ERROR_TOO_LONG;
            }
            break;
        case BARCODE_UPCA_CC:
            if (ustrlen(first_part) == 11) {
                set_module(symbol, symbol->rows, 1);
                set_module(symbol, symbol->rows, 95);
                set_module(symbol, symbol->rows + 1, 0);
                set_module(symbol, symbol->rows + 1, 96);
                set_module(symbol, symbol->rows + 2, 1);
                set_module(symbol, symbol->rows + 2, 95);
                symbol->row_height[symbol->rows] = 2;
                symbol->row_height[symbol->rows + 1] = 2;
                symbol->row_height[symbol->rows + 2] = 2;
                symbol->rows += 3;
                error_number = upca(symbol, first_part, (char*) dest);
            } else {
                strcpy(symbol->errtxt, "289: UPCA input wrong length");
                return ZINT_ERROR_TOO_LONG;
            }
            break;
        case BARCODE_UPCE:
        case BARCODE_UPCE_CHK:
            if ((ustrlen(first_part) >= 6) && (ustrlen(first_part) <= 8)) {
                error_number = upce(symbol, first_part, (char*) dest);
            } else {
                strcpy(symbol->errtxt, "290: Input wrong length");
                return ZINT_ERROR_TOO_LONG;
            }
            break;
        case BARCODE_UPCE_CC:
            if ((ustrlen(first_part) >= 6) && (ustrlen(first_part) <= 7)) {
                set_module(symbol, symbol->rows, 1);
                set_module(symbol, symbol->rows, 51);
                set_module(symbol, symbol->rows + 1, 0);
                set_module(symbol, symbol->rows + 1, 52);
                set_module(symbol, symbol->rows + 2, 1);
                set_module(symbol, symbol->rows + 2, 51);
                symbol->row_height[symbol->rows] = 2;
                symbol->row_height[symbol->rows + 1] = 2;
                symbol->row_height[symbol->rows + 2] = 2;
                symbol->rows += 3;
                error_number = upce(symbol, first_part, (char*) dest);
            } else {
                strcpy(symbol->errtxt, "291: UPCE input wrong length");
                return ZINT_ERROR_TOO_LONG;
            }
            break;
        case BARCODE_ISBNX:
            error_number = isbn(symbol, first_part, ustrlen(first_part), (char*) dest);
            break;
    }

    if (error_number > 4) {
        return error_number;
    }

    switch (ustrlen(second_part)) {
        case 0: break;
        case 2:
            add_on(second_part, (char*) dest, 1);
            strcat((char*) symbol->text, "+");
            strcat((char*) symbol->text, (char*) second_part);
            break;
        case 5:
            add_on(second_part, (char*) dest, 1);
            strcat((char*) symbol->text, "+");
            strcat((char*) symbol->text, (char*) second_part);
            break;
        default:
            strcpy(symbol->errtxt, "292: Invalid length input");
            return ZINT_ERROR_TOO_LONG;
    }

    expand(symbol, (char*) dest);

    switch (symbol->symbology) {
        case BARCODE_EANX_CC:
        case BARCODE_UPCA_CC:
        case BARCODE_UPCE_CC:
            /* shift the symbol to the right one space to allow for separator bars */
            for (i = (symbol->width + 1); i >= 1; i--) {
                if (module_is_set(symbol, symbol->rows - 1, i - 1)) {
                    set_module(symbol, symbol->rows - 1, i);
                } else {
                    unset_module(symbol, symbol->rows - 1, i);
                }
            }
            unset_module(symbol, symbol->rows - 1, 0);
            symbol->width += 2;
            break;
    }

    return 0;
}

