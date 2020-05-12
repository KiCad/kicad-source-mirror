/* code.c - Handles Code 11, 39, 39+ and 93 */

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

/* In version 0.5 this file was 1,553 lines long! */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#define SODIUM	"0123456789-"
#define SILVER	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%abcd"
#define ARSENIC "0123456789ABCDEFGHJKLMNPRSTUVWXYZ"

static const char *C11Table[11] = {
    "111121", "211121", "121121", "221111", "112121", "212111", "122111",
    "111221", "211211", "211111", "112111"
};

/* Code 39 tables checked against ISO/IEC 16388:2007 */

/* Incorporates Table A1 */

static const char *C39Table[43] = {
    /* Code 39 character assignments (Table 1) */
    "1112212111", "2112111121", "1122111121", "2122111111", "1112211121",
    "2112211111", "1122211111", "1112112121", "2112112111", "1122112111", "2111121121",
    "1121121121", "2121121111", "1111221121", "2111221111", "1121221111", "1111122121",
    "2111122111", "1121122111", "1111222111", "2111111221", "1121111221", "2121111211",
    "1111211221", "2111211211", "1121211211", "1111112221", "2111112211", "1121112211",
    "1111212211", "2211111121", "1221111121", "2221111111", "1211211121", "2211211111",
    "1221211111", "1211112121", "2211112111", "1221112111", "1212121111", "1212111211",
    "1211121211", "1112121211"
};

static const char *EC39Ctrl[128] = {
    /* Encoding the full ASCII character set in Code 39 (Table A2) */
    "%U", "$A", "$B", "$C", "$D", "$E", "$F", "$G", "$H", "$I", "$J", "$K",
    "$L", "$M", "$N", "$O", "$P", "$Q", "$R", "$S", "$T", "$U", "$V", "$W", "$X", "$Y", "$Z",
    "%A", "%B", "%C", "%D", "%E", " ", "/A", "/B", "/C", "/D", "/E", "/F", "/G", "/H", "/I", "/J",
    "/K", "/L", "-", ".", "/O", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "/Z", "%F",
    "%G", "%H", "%I", "%J", "%V", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "%K", "%L", "%M", "%N", "%O",
    "%W", "+A", "+B", "+C", "+D", "+E", "+F", "+G", "+H", "+I", "+J", "+K", "+L", "+M", "+N", "+O",
    "+P", "+Q", "+R", "+S", "+T", "+U", "+V", "+W", "+X", "+Y", "+Z", "%P", "%Q", "%R", "%S", "%T"
};

static const char *C93Ctrl[128] = {
    "bU", "aA", "aB", "aC", "aD", "aE", "aF", "aG", "aH", "aI", "aJ", "aK",
    "aL", "aM", "aN", "aO", "aP", "aQ", "aR", "aS", "aT", "aU", "aV", "aW", "aX", "aY", "aZ",
    "bA", "bB", "bC", "bD", "bE", " ", "cA", "cB", "cC", "$", "%", "cF", "cG", "cH", "cI", "cJ",
    "+", "cL", "-", ".", "/", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "cZ", "bF",
    "bG", "bH", "bI", "bJ", "bV", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "bK", "bL", "bM", "bN", "bO",
    "bW", "dA", "dB", "dC", "dD", "dE", "dF", "dG", "dH", "dI", "dJ", "dK", "dL", "dM", "dN", "dO",
    "dP", "dQ", "dR", "dS", "dT", "dU", "dV", "dW", "dX", "dY", "dZ", "bP", "bQ", "bR", "bS", "bT"
};

static const char *C93Table[47] = {
    "131112", "111213", "111312", "111411", "121113", "121212", "121311",
    "111114", "131211", "141111", "211113", "211212", "211311", "221112", "221211", "231111",
    "112113", "112212", "112311", "122112", "132111", "111123", "111222", "111321", "121122",
    "131121", "212112", "212211", "211122", "211221", "221121", "222111", "112122", "112221",
    "122121", "123111", "121131", "311112", "311211", "321111", "112131", "113121", "211131",
    "121221", "312111", "311121", "122211"
};

/* Global Variables for Channel Code */
int S[11], B[11];
long value;
long target_value;
char pattern[30];

/* Function Prototypes */
void NextS(int Chan, int i, int MaxS, int MaxB);
void NextB(int Chan, int i, int MaxB, int MaxS);

/* *********************** CODE 11 ******************** */
int code_11(struct zint_symbol *symbol, unsigned char source[], int length) { /* Code 11 */

    unsigned int i;
    int h, c_digit, c_weight, c_count, k_digit, k_weight, k_count;
    int weight[128], error_number;
    char dest[1024]; /* 6 +  121 * 6 + 2 * 6 + 5 + 1 ~ 1024*/
    char checkstr[3];

    if (length > 121) {
        strcpy(symbol->errtxt, "320: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(SODIUM, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "321: Invalid characters in data");
        return error_number;
    }
    c_weight = 1;
    c_count = 0;
    k_weight = 1;
    k_count = 0;

    /* start character */
    strcpy(dest, "112211");

    /* Draw main body of barcode */
    for (i = 0; i < (unsigned int) length; i++) {
        lookup(SODIUM, C11Table, source[i], dest);
        if (source[i] == '-')
            weight[i] = 10;
        else
            weight[i] = ctoi(source[i]);
    }

    /* Calculate C checksum */
    for (h = length - 1; h >= 0; h--) {
        c_count += (c_weight * weight[h]);
        c_weight++;

        if (c_weight > 10) {
            c_weight = 1;
        }
    }
    c_digit = c_count % 11;

    weight[length] = c_digit;

    /* Calculate K checksum */
    for (h = length; h >= 0; h--) {
        k_count += (k_weight * weight[h]);
        k_weight++;

        if (k_weight > 9) {
            k_weight = 1;
        }
    }
    k_digit = k_count % 11;

    checkstr[0] = itoc(c_digit);
    checkstr[1] = itoc(k_digit);
    if (checkstr[0] == 'A') {
        checkstr[0] = '-';
    }
    if (checkstr[1] == 'A') {
        checkstr[1] = '-';
    }
    checkstr[2] = '\0';
    lookup(SODIUM, C11Table, checkstr[0], dest);
    lookup(SODIUM, C11Table, checkstr[1], dest);

    /* Stop character */
    strcat(dest, "11221");

    expand(symbol, dest);

    ustrcpy(symbol->text, source);
    strcat((char*) symbol->text, checkstr);
    return error_number;
}

/* Code 39 */
int c39(struct zint_symbol *symbol, unsigned char source[], const size_t length) {
    unsigned int i;
    unsigned int counter;
    int error_number;
    char dest[775];
    char localstr[2] = {0};

    counter = 0;

    if ((symbol->option_2 < 0) || (symbol->option_2 > 1)) {
        symbol->option_2 = 0;
    }

    if ((symbol->symbology == BARCODE_LOGMARS) && (length > 59)) {
        strcpy(symbol->errtxt, "322: Input too long");
        return ZINT_ERROR_TOO_LONG;
    } else if (length > 74) {
        strcpy(symbol->errtxt, "323: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    to_upper(source);
    error_number = is_sane(SILVER, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "324: Invalid characters in data");
        return error_number;
    }

    /* Start character */
    strcpy(dest, "1211212111");

    for (i = 0; i < (unsigned int) length; i++) {
        lookup(SILVER, C39Table, source[i], dest);
        counter += posn(SILVER, source[i]);
    }

    if ((symbol->symbology == BARCODE_LOGMARS) || (symbol->option_2 == 1)) {

        char check_digit;
        counter = counter % 43;
        if (counter < 10) {
             check_digit = itoc(counter);
        } else {
            if (counter < 36) {
                check_digit = (counter - 10) + 'A';
            } else {
                switch (counter) {
                    case 36: check_digit = '-';
                        break;
                    case 37: check_digit = '.';
                        break;
                    case 38: check_digit = ' ';
                        break;
                    case 39: check_digit = '$';
                        break;
                    case 40: check_digit = '/';
                        break;
                    case 41: check_digit = '+';
                        break;
                    case 42: check_digit = 37;
                        break;
                    default: check_digit = ' ';
                        break; /* Keep compiler happy */
                }
            }
        }
        lookup(SILVER, C39Table, check_digit, dest);

        /* Display a space check digit as _, otherwise it looks like an error */
        if (check_digit == ' ') {
            check_digit = '_';
        }

        localstr[0] = check_digit;
        localstr[1] = '\0';
    }

    /* Stop character */
    strcat(dest, "121121211");

    if ((symbol->symbology == BARCODE_LOGMARS) || (symbol->symbology == BARCODE_HIBC_39)) {
        /* LOGMARS uses wider 'wide' bars than normal Code 39 */
        counter = (unsigned int) strlen(dest);
        for (i = 0; i < counter; i++) {
            if (dest[i] == '2') {
                dest[i] = '3';
            }
        }
    }

    expand(symbol, dest);

    if (symbol->symbology == BARCODE_CODE39) {
        strcpy((char*) symbol->text, "*");
        strcat((char*) symbol->text, (char*) source);
        strcat((char*) symbol->text, localstr);
        strcat((char*) symbol->text, "*");
    } else {
        strcpy((char*) symbol->text, (char*) source);
        strcat((char*) symbol->text, localstr);
    }
    return error_number;
}

/* Pharmazentral Nummer (PZN) */
int pharmazentral(struct zint_symbol *symbol, unsigned char source[], int length) {

    int i, error_number, zeroes;
    unsigned int count, check_digit;
    char localstr[11];

    count = 0;
    if (length > 7) {
        strcpy(symbol->errtxt, "325: Input wrong length");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "326: Invalid characters in data");
        return error_number;
    }

    localstr[0] = '-';
    zeroes = 7 - length + 1;
    for (i = 1; i < zeroes; i++)
        localstr[i] = '0';
    strcpy(localstr + zeroes, (char *) source);

    for (i = 1; i < 8; i++) {
        count += i * ctoi(localstr[i]);
    }

    check_digit = count % 11;
    if (check_digit == 11) {
        check_digit = 0;
    }
    localstr[8] = itoc(check_digit);
    localstr[9] = '\0';
    if (localstr[8] == 'A') {
        strcpy(symbol->errtxt, "327: Invalid PZN Data");
        return ZINT_ERROR_INVALID_DATA;
    }
    error_number = c39(symbol, (unsigned char *) localstr, strlen(localstr));
    ustrcpy(symbol->text, (unsigned char *) "PZN");
    strcat((char*) symbol->text, localstr);
    return error_number;
}

/* Extended Code 39 - ISO/IEC 16388:2007 Annex A */
int ec39(struct zint_symbol *symbol, unsigned char source[], int length) {

    unsigned char buffer[150] = {0};
    unsigned int i;
    int error_number;

    if (length > 74) {
        strcpy(symbol->errtxt, "328: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* Creates a buffer string and places control characters into it */
    for (i = 0; i < (unsigned int) length; i++) {
        if (source[i] > 127) {
            /* Cannot encode extended ASCII */
            strcpy(symbol->errtxt, "329: Invalid characters in input data");
            return ZINT_ERROR_INVALID_DATA;
        }
        strcat((char*) buffer, EC39Ctrl[source[i]]);
    }

    /* Then sends the buffer to the C39 function */
    error_number = c39(symbol, buffer, ustrlen(buffer));

    for (i = 0; i < (unsigned int) length; i++)
        symbol->text[i] = source[i] ? source[i] : ' ';
    symbol->text[length] = '\0';

    return error_number;
}

/* Code 93 is an advancement on Code 39 and the definition is a lot tighter */
int c93(struct zint_symbol *symbol, unsigned char source[], int length) {

    /* SILVER includes the extra characters a, b, c and d to represent Code 93 specific
       shift characters 1, 2, 3 and 4 respectively. These characters are never used by
       c39() and ec39() */

    int i;
    int h, weight, c, k, values[128], error_number;
    char buffer[220];
    char dest[670];
    char set_copy[] = SILVER;

    error_number = 0;
    strcpy(buffer, "");

    if (length > 107) {
        strcpy(symbol->errtxt, "330: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    /* Message Content */
    for (i = 0; i < length; i++) {
        if (source[i] > 127) {
            /* Cannot encode extended ASCII */
            strcpy(symbol->errtxt, "331: Invalid characters in input data");
            return ZINT_ERROR_INVALID_DATA;
        }
        strcat(buffer, C93Ctrl[source[i]]);
        symbol->text[i] = source[i] ? source[i] : ' ';
    }

    /* Now we can check the true length of the barcode */
    h = (int) strlen(buffer);
    if (h > 107) {
        strcpy(symbol->errtxt, "332: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }

    for (i = 0; i < h; i++) {
        values[i] = posn(SILVER, buffer[i]);
    }

    /* Putting the data into dest[] is not done until after check digits are calculated */

    /* Check digit C */
    c = 0;
    weight = 1;
    for (i = h - 1; i >= 0; i--) {
        c += values[i] * weight;
        weight++;
        if (weight == 21)
            weight = 1;
    }
    c = c % 47;
    values[h] = c;
    buffer[h] = set_copy[c];

    /* Check digit K */
    k = 0;
    weight = 1;
    for (i = h; i >= 0; i--) {
        k += values[i] * weight;
        weight++;
        if (weight == 16)
            weight = 1;
    }
    k = k % 47;
    buffer[++h] = set_copy[k];
    buffer[++h] = '\0';

    /* Start character */
    strcpy(dest, "111141");

    for (i = 0; i < h; i++) {
        lookup(SILVER, C93Table, buffer[i], dest);
    }

    /* Stop character */
    strcat(dest, "1111411");
    expand(symbol, dest);

    symbol->text[length] = set_copy[c];
    symbol->text[length + 1] = set_copy[k];
    symbol->text[length + 2] = '\0';

    return error_number;
}

/* NextS() and NextB() are from ANSI/AIM BC12-1998 and are Copyright (c) AIM 1997 */

/* Their are used here on the understanding that they form part of the specification
   for Channel Code and therefore their use is permitted under the following terms
   set out in that document:

   "It is the intent and understanding of AIM [t]hat the symbology presented in this
   specification is entirely in the public domain and free of all use restrictions,
   licenses and fees. AIM USA, its member companies, or individual officers
   assume no liability for the use of this document." */

void CheckCharacter() {

    if (value == target_value) {
        int i;
        /* Target reached - save the generated pattern */
        strcpy(pattern, "11110");
        for (i = 0; i < 11; i++) {
            char part[3];
            part[0] = itoc(S[i]);
            part[1] = itoc(B[i]);
            part[2] = '\0';
            strcat(pattern, part);
        }
    }
}

void NextB(int Chan, int i, int MaxB, int MaxS) {
    int b;

    b = (S[i] + B[i - 1] + S[i - 1] + B[i - 2] > 4) ? 1 : 2;
    if (i < Chan + 2) {
        for (; b <= MaxB; b++) {
            B[i] = b;
            NextS(Chan, i + 1, MaxS, MaxB + 1 - b);
        }
    } else if (b <= MaxB) {
        B[i] = MaxB;
        CheckCharacter();
        value++;
    }
}

void NextS(int Chan, int i, int MaxS, int MaxB) {
    int s;

    for (s = (i < Chan + 2) ? 1 : MaxS; s <= MaxS; s++) {
        S[i] = s;
        NextB(Chan, i, MaxB, MaxS + 1 - s);
    }
}

/* Channel Code - According to ANSI/AIM BC12-1998 */
int channel_code(struct zint_symbol *symbol, unsigned char source[], int length) {
    int channels, i;
    int error_number = 0, range = 0, zeroes;
    char hrt[9];

    target_value = 0;

    if (length > 7) {
        strcpy(symbol->errtxt, "333: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "334: Invalid characters in data");
        return error_number;
    }

    if ((symbol->option_2 < 3) || (symbol->option_2 > 8)) {
        channels = 0;
    } else {
        channels = symbol->option_2;
    }
    if (channels == 0) {
        channels = length + 1;
    }
    if (channels == 2) {
        channels = 3;
    }

    for (i = 0; i < length; i++) {
        target_value *= 10;
        target_value += ctoi((char) source[i]);
    }

    switch (channels) {
        case 3: if (target_value > 26) {
                range = 1;
            }
            break;
        case 4: if (target_value > 292) {
                range = 1;
            }
            break;
        case 5: if (target_value > 3493) {
                range = 1;
            }
            break;
        case 6: if (target_value > 44072) {
                range = 1;
            }
            break;
        case 7: if (target_value > 576688) {
                range = 1;
            }
            break;
        case 8: if (target_value > 7742862) {
                range = 1;
            }
            break;
    }
    if (range) {
        strcpy(symbol->errtxt, "335: Value out of range");
        return ZINT_ERROR_INVALID_DATA;
    }

    for (i = 0; i < 11; i++) {
        B[i] = 0;
        S[i] = 0;
    }

    B[0] = S[1] = B[1] = S[2] = B[2] = 1;
    value = 0;
    NextS(channels, 3, channels, channels);

    zeroes = channels - 1 - length;
    memset(hrt, '0', zeroes);
    strcpy(hrt + zeroes, (char *) source);
    ustrcpy(symbol->text, (unsigned char *) hrt);

    expand(symbol, pattern);

    return error_number;
}


/* Vehicle Identification Number (VIN) */
int vin(struct zint_symbol *symbol, const unsigned char source[], const size_t in_length) {
    
    /* This code verifies the check digit present in North American VIN codes */
    
    int zeros;
    char local_source[18];
    char dest[200];
    char input_check;
    char output_check;
    int value[17];
    int weight[17] = {8, 7, 6, 5, 4, 3, 2, 10, 0, 9, 8, 7, 6, 5, 4, 3, 2};
    int sum;
    int i;
    int length = (int) in_length;
    
    // Check length
    if (length > 17) {
        strcpy(symbol->errtxt, "336: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    
    // Pad with zeros
    zeros = 17 - length;
    
    for (i = 0; i < 17; i++) {
        local_source[i] = '0';
    }
    local_source[17] = '\0';
    
    for (i = 0; i < length; i++) {
        local_source[zeros + i] = source[i];
    }
    
    to_upper((unsigned char *) local_source);
    
    // Check input characters, I, O and Q are not allowed
    if (is_sane(ARSENIC, (unsigned char *) local_source, length) == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "337: Invalid characters in input data");
        return ZINT_ERROR_INVALID_DATA;
    }
    
    input_check = local_source[8];
    
    for (i = 0; i < 17; i++) {
        if ((local_source[i] >= '0') && (local_source[i] <= '9')) {
            value[i] = local_source[i] - '0';
        }
        if ((local_source[i] >= 'A') && (local_source[i] <= 'I')) {
            value[i] = (local_source[i] - 'A') + 1;
        }
        if ((local_source[i] >= 'J') && (local_source[i] <= 'R')) {
            value[i] = (local_source[i] - 'J') + 1;
        }
        if ((local_source[i] >= 'S') && (local_source[i] <= 'Z')) {
            value[i] = (local_source[i] - 'S') + 2;
        }
    }
    
    sum = 0;
    for (i = 0; i < 17; i++) {
        sum += value[i] * weight[i];
    }
    
    output_check = '0' + (sum % 11);
    
    if (output_check == ':') {
        // Check digit was 10
        output_check = 'X';
    }
    
    if (symbol->debug) {
        printf("Producing VIN code: %s\n", local_source);
        printf("Input check was %c, calculated check is %c\n", input_check, output_check);
    }
    
    if (input_check != output_check) {
        strcpy(symbol->errtxt, "338: Invalid check digit in input data");
        return ZINT_ERROR_INVALID_DATA;
    }
    
    /* Start character */
    strcpy(dest, "1211212111");
    
    // Copy glyphs to symbol
    for (i = 0; i < 17; i++) {
        lookup(SILVER, C39Table, local_source[i], dest);
    }
    
    /* Stop character */
    strcat(dest, "121121211");
    
    ustrcpy(symbol->text, (unsigned char *) local_source);
    expand(symbol, dest);
    
    return 0;
}

