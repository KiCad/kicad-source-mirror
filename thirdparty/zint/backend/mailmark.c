/* mailmark.c - Royal Mail 4-state Mailmark barcodes */

/*
    libzint - the open source barcode library
    Copyright (C) 2008-2018 Robin Stuart <rstuart114@gmail.com>

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

/* 
 * Developed in accordance with "Royal Mail Mailmark barcode C encoding and deconding instructions"
 * (https://www.royalmail.com/sites/default/files/Mailmark-4-state-barcode-C-encoding-and-decoding-instructions-Sept-2015.pdf)
 * and "Royal Mail Mailmark barcode L encoding and decoding"
 * (https://www.royalmail.com/sites/default/files/Mailmark-4-state-barcode-L-encoding-and-decoding-instructions-Sept-2015.pdf)
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "large.h"
#include "reedsol.h"

#define RUBIDIUM "01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ "

// Allowed character values from Table 3
#define SET_F "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define SET_L "ABDEFGHJLNPQRSTUWXYZ"
#define SET_N "0123456789"
#define SET_S " "

static const char *postcode_format[6] = {
    "FNFNLLNLS", "FFNNLLNLS", "FFNNNLLNL", "FFNFNLLNL", "FNNLLNLSS", "FNNNLLNLS"
};

// Data/Check Symbols from Table 5
static const unsigned short data_symbol_odd[32] = {
    0x01, 0x02, 0x04, 0x07, 0x08, 0x0B, 0x0D, 0x0E, 0x10, 0x13, 0x15, 0x16,
    0x19, 0x1A, 0x1C, 0x1F, 0x20, 0x23, 0x25, 0x26, 0x29, 0x2A, 0x2C, 0x2F,
    0x31, 0x32, 0x34, 0x37, 0x38, 0x3B, 0x3D, 0x3E
};

static const unsigned short data_symbol_even[30] = {
    0x03, 0x05, 0x06, 0x09, 0x0A, 0x0C, 0x0F, 0x11, 0x12, 0x14, 0x17, 0x18,
    0x1B, 0x1D, 0x1E, 0x21, 0x22, 0x24, 0x27, 0x28, 0x2B, 0x2D, 0x2E, 0x30,
    0x33, 0x35, 0x36, 0x39, 0x3A, 0x3C
};

static const unsigned short extender_group_c[22] = {
    3, 5, 7, 11, 13, 14, 16, 17, 19, 0, 1, 2, 4, 6, 8, 9, 10, 12, 15, 18, 20, 21
};

static const unsigned short extender_group_l[26] = {
    2, 5, 7, 8, 13, 14, 15, 16, 21, 22, 23, 0, 1, 3, 4, 6, 9, 10, 11, 12, 17, 18, 19, 20, 24, 25
};

int verify_character(char input, char type) {
    int val = 0;
    
    switch (type) {
        case 'F':
            val = posn(SET_F, input);
            break;
        case 'L':
            val = posn(SET_L, input);
            break;
        case 'N':
            val = posn(SET_N, input);
            break;
        case 'S':
            val = posn(SET_S, input);
            break;
    }
    
    if (val == -1) {
        return 0;
    } else {
        return 1;
    }
}

int verify_postcode(char* postcode, int type) {
    int i;
    char pattern[11];
    
    strcpy(pattern, postcode_format[type - 1]);

    for (i = 0; i < 9; i++) {
        if (!(verify_character(postcode[i], pattern[i]))) {
            return 1;
        }
    }

    return 0;
}

/* Royal Mail Mailmark */
int mailmark(struct zint_symbol *symbol, const unsigned char source[], const size_t in_length) {
    
    char local_source[28];
    int format;
    int version_id;
    int mail_class;
    int supply_chain_id;
    long item_id;
    char postcode[10];
    int postcode_type;
    char pattern[10];
    short int destination_postcode[112];
    short int a[112];
    short int b[112];
    short int temp[112];
    short int cdv[112];
    unsigned char data[26];
    int data_top, data_step;
    unsigned char check[7];
    short int extender[27];
    char bar[80];
    int check_count;
    int i, j;
    int length = (int) in_length;
    
    if (length > 26) {
        strcpy(symbol->errtxt, "580: Input too long");
        return ZINT_ERROR_TOO_LONG;
    }
    
    strcpy(local_source, (char*) source);
    
    if (length < 22) {
        for (i = length; i <= 22; i++) {
            strcat(local_source, " ");
        }
        length = 22;
    }
    
    if ((length > 22) && (length < 26)) {
        for (i = length; i <= 26; i++) {
            strcat(local_source, " ");
        }
        length = 26;
    } 
    
    to_upper((unsigned char*) local_source);
    
    if (symbol->debug) {
        printf("Producing Mailmark %s\n", local_source);
    }
    
    if (is_sane(RUBIDIUM, (unsigned char *) local_source, length) != 0) {
        strcpy(symbol->errtxt, "581: Invalid characters in input data");
        return ZINT_ERROR_INVALID_DATA;
    }

    // Format is in the range 0-4
    format = ctoi(local_source[0]);
    if ((format < 0) || (format > 4)) {
        strcpy(symbol->errtxt, "582: Invalid format");
        return ZINT_ERROR_INVALID_DATA;
    }
    
    // Version ID is in the range 1-4
    version_id = ctoi(local_source[1]) - 1;
    if ((version_id < 0) || (version_id > 3)) {
        strcpy(symbol->errtxt, "583: Invalid Version ID");
        return ZINT_ERROR_INVALID_DATA;
    }
    
    // Class is in the range 0-9,A-E
    mail_class = ctoi(local_source[2]);
    if ((mail_class < 0) || (mail_class > 14)) {
        strcpy(symbol->errtxt, "584: Invalid Class");
        return ZINT_ERROR_INVALID_DATA;
    }
    
    // Supply Chain ID is 2 digits for barcode C and 6 digits for barcode L
    supply_chain_id = 0;
    for (i = 3; i < (length - 17); i++) {
        if ((local_source[i] >= '0') && (local_source[i] <= '9')) {
            supply_chain_id *= 10;
            supply_chain_id += ctoi(local_source[i]);
        } else {
            strcpy(symbol->errtxt, "585: Invalid Supply Chain ID");
            return ZINT_ERROR_INVALID_DATA;
        }
    }
    
    // Item ID is 8 digits
    item_id = 0;
    for (i = length - 17; i < (length - 9); i++) {
        if ((local_source[i] >= '0') && (local_source[i] <= '9')) {
            item_id *= 10;
            item_id += (long) ctoi(local_source[i]);
        } else {
            strcpy(symbol->errtxt, "586: Invalid Item ID");
            return ZINT_ERROR_INVALID_DATA;
        }
    }
    
    // Separate Destination Post Code plus DPS field
    for (i = 0; i < 9; i++) {
        postcode[i] = local_source[(length - 9) + i];
    }
    postcode[9] = '\0';
    
    // Detect postcode type
    /* postcode_type is used to select which format of postcode 
     * 
     * 1 = FNFNLLNLS
     * 2 = FFNNLLNLS
     * 3 = FFNNNLLNL
     * 4 = FFNFNLLNL
     * 5 = FNNLLNLSS
     * 6 = FNNNLLNLS
     * 7 = International designation
     */
    
    postcode_type = 0;
    if (strcmp(postcode, "XY11     ") == 0) {
        postcode_type = 7;
    } else {
        if (postcode[7] == ' ') {
            postcode_type = 5;
        } else {
            if (postcode[8] == ' ') {
                // Types 1, 2 and 6
                if ((postcode[1] >= '0') && (postcode[1] <= '9')) {
                    if ((postcode[2] >= '0') && (postcode[2] <= '9')) {
                        postcode_type = 6;
                    } else {
                        postcode_type = 1;
                    }
                } else {
                    postcode_type = 2;
                }
            } else {
                // Types 3 and 4
                if ((postcode[3] >= '0') && (postcode[3] <= '9')) {
                    postcode_type = 3;
                } else {
                    postcode_type = 4;
                }
            }
        }
    }
    
    // Verify postcode type
    if (postcode_type != 7) {
        if (verify_postcode(postcode, postcode_type) != 0) {
            strcpy(symbol->errtxt, "587: Invalid postcode");
            return ZINT_ERROR_INVALID_DATA;
        }
    }
    
    // Convert postcode to internal user field
    for (i = 0; i < 112; i++) {
        destination_postcode[i] = 0;
        a[i] = 0;
        b[i] = 0;
    }
    
    if (postcode_type != 7) {
        strcpy(pattern, postcode_format[postcode_type - 1]);

        binary_load(b, "0", 1);
        
        for (i = 0; i < 9; i++) {
            switch (pattern[i]) {
                case 'F':
                    binary_multiply(b, "26");
                    
                    binary_load(temp, "0", 1);
                    for (j = 0; j < 5; j++) {
                        if (posn(SET_F, postcode[i]) & (0x01 << j)) temp[j] = 1;
                    }

                    binary_add(b, temp);
                    break;
                case 'L':
                    binary_multiply(b, "20");
                    
                    binary_load(temp, "0", 1);
                    for (j = 0; j < 5; j++) {
                        if (posn(SET_L, postcode[i]) & (0x01 << j)) temp[j] = 1;
                    }

                    binary_add(b, temp);
                    break;
                case 'N':
                    binary_multiply(b, "10");
                    
                    binary_load(temp, "0", 1);
                    for (j = 0; j < 4; j++) {
                        if (posn(SET_N, postcode[i]) & (0x01 << j)) temp[j] = 1;
                    }

                    binary_add(b, temp);
                    break;
                // case 'S' ignorred as value is 0
            }
        }
        
        // destination_postcode = a + b
        binary_load(destination_postcode, "0", 1);
        binary_add(destination_postcode, b);
        
        binary_load(a, "1", 1);
        if (postcode_type == 1) {
            binary_add(destination_postcode, a);
        }
        binary_load(temp, "5408000000", 10);
        binary_add(a, temp);
        if (postcode_type == 2) {
            binary_add(destination_postcode, a);
        }
        binary_load(temp, "5408000000", 10);
        binary_add(a, temp);
        if (postcode_type == 3) {
            binary_add(destination_postcode, a);
        }
        binary_load(temp, "54080000000", 11);
        binary_add(a, temp);
        if (postcode_type == 4) {
            binary_add(destination_postcode, a);
        }
        binary_load(temp, "140608000000", 12);
        binary_add(a, temp);
        if (postcode_type == 5) {
            binary_add(destination_postcode, a);
        }
        binary_load(temp, "208000000", 9);
        binary_add(a, temp);
        if (postcode_type == 6) {
            binary_add(destination_postcode, a);
        }
    }
    
    // Conversion from Internal User Fields to Consolidated Data Value
    // Set CDV to 0
    binary_load(cdv, "0", 1);
    
    // Add Destination Post Code plus DPS
    binary_add(cdv, destination_postcode);
    
    // Multiply by 100,000,000
    binary_multiply(cdv, "100000000");
    
    // Add Item ID
    binary_load(temp, "0", 1);
    for (i = 0; i < 31; i++) {
        if (0x01 & (item_id >> i)) temp[i] = 1;
    }
    binary_add(cdv, temp);
    
    if (length == 22) {  
        // Barcode C - Multiply by 100
        binary_multiply(cdv, "100");
    } else {
        // Barcode L - Multiply by 1,000,000
        binary_multiply(cdv, "1000000");
    }
    
    // Add Supply Chain ID
    binary_load(temp, "0", 1);
    for (i = 0; i < 20; i++) {
        if (0x01 & (supply_chain_id >> i)) temp[i] = 1;
    }
    binary_add(cdv, temp);
    
    // Multiply by 15
    binary_multiply(cdv, "15");
    
    // Add Class
    binary_load(temp, "0", 1);
    for (i = 0; i < 4; i++) {
        if (0x01 & (mail_class >> i)) temp[i] = 1;
    }
    binary_add(cdv, temp);
    
    // Multiply by 5
    binary_multiply(cdv, "5");
    
    // Add Format
    binary_load(temp, "0", 1);
    for (i = 0; i < 4; i++) {
        if (0x01 & (format >> i)) temp[i] = 1;
    }
    binary_add(cdv, temp);
    
    // Multiply by 4
    binary_multiply(cdv, "4");
    
    // Add Version ID
    binary_load(temp, "0", 1);
    for (i = 0; i < 4; i++) {
        if (0x01 & (version_id >> i)) temp[i] = 1;
    }
    binary_add(cdv, temp);
    
    if (symbol->debug) {
        printf("DPC type %d\n", postcode_type);
        printf("CDV: ");
        for (i = 96; i >= 0; i-= 4) {
            j = 0;

            j += cdv[i];
            j += cdv[i + 1] * 2;
            j += cdv[i + 2] * 4;
            j += cdv[i + 3] * 8;

            printf("%c", itoc(j));
        }
        printf("\n");
    }
    
    
    if (length == 22) {
        data_top = 15;
        data_step = 8;
        check_count = 6;
    } else {
        data_top = 18;
        data_step = 10;
        check_count = 7;
    }
    
    // Conversion from Consolidated Data Value to Data Numbers
    for (i = 0; i < 112; i++) {
        b[i] = cdv[i];
    }
    
    for (j = data_top; j >= (data_step + 1); j--) {
        for (i = 0; i < 112; i++) {
            cdv[i] = b[i];
            b[i] = 0;
            a[i] = 0;
        }
        a[96] = 1;
        for (i = 91; i >= 0; i--) {
            b[i] = islarger(cdv, a);
            if (b[i] == 1) {
                binary_subtract(cdv, a);
            }
            shiftdown(a);
        }

        data[j] = (cdv[5] * 32) + (cdv[4] * 16) + (cdv[3] * 8) + (cdv[2] * 4) +
                (cdv[1] * 2) + cdv[0];
    }
    
    for (j = data_step; j >= 0; j--) {
        for (i = 0; i < 112; i++) {
            cdv[i] = b[i];
            b[i] = 0;
            a[i] = 0;
        }
        a[95] = 1;
        a[94] = 1;
        a[93] = 1;
        a[92] = 1;
        for (i = 91; i >= 0; i--) {
            b[i] = islarger(cdv, a);
            if (b[i] == 1) {
                binary_subtract(cdv, a);
            }
            shiftdown(a);
        }

        data[j] = (cdv[5] * 32) + (cdv[4] * 16) + (cdv[3] * 8) + (cdv[2] * 4) +
                (cdv[1] * 2) + cdv[0];
    }
    
    // Generation of Reed-Solomon Check Numbers
    rs_init_gf(0x25);
    rs_init_code(check_count, 1);
    rs_encode((data_top + 1), data, check);
    rs_free();
    
    // Append check digits to data
    for (i = 1; i <= check_count; i++) {
        data[data_top + i] = check[check_count - i];
    }
    
    if (symbol->debug) {
        printf("Codewords:  ");
        for (i = 0; i <= data_top + check_count; i++) {
            printf("%d  ", (int) data[i]);
        }
        printf("\n");
    }
    
    // Conversion from Data Numbers and Check Numbers to Data Symbols and Check Symbols
    for (i = 0; i <= data_step; i++) {
        data[i] = data_symbol_even[data[i]];
    }
    for (i = data_step + 1; i <= (data_top + check_count); i++) {
        data[i] = data_symbol_odd[data[i]];
    }
    
    // Conversion from Data Symbols and Check Symbols to Extender Groups
    for (i = 0; i < length; i++) {
        if (length == 22) {
            extender[extender_group_c[i]] = data[i];
        } else {
            extender[extender_group_l[i]] = data[i];
        }
    }
    
    // Conversion from Extender Groups to Bar Identifiers
    strcpy(bar, "");
    
    for (i = 0; i < length; i++) {
        for (j = 0; j < 3; j++) {
            switch(extender[i] & 0x24) {
                case 0x24:
                    strcat(bar, "F");
                    break;
                case 0x20:
                    if (i % 2) {
                        strcat(bar, "D");
                    } else {
                        strcat(bar, "A");
                    }
                    break;
                case 0x04:
                    if (i % 2) {
                        strcat(bar, "A");
                    } else {
                        strcat(bar, "D");
                    }
                    break;
                default:
                    strcat(bar, "T");
                    break;
            }
            extender[i] = extender[i] << 1;
        }
    }
    
    bar[(length * 3)] = '\0';
    
    if (symbol->debug) {
        printf("Bar pattern: %s\n", bar);
    }
    
    /* Translate 4-state data pattern to symbol */
    j = 0;
    for (i = 0; i < strlen(bar); i++) {
        if ((bar[i] == 'F') || (bar[i] == 'A')) {
            set_module(symbol, 0, j);
        }
        set_module(symbol, 1, j);
        if ((bar[i] == 'F') || (bar[i] == 'D')) {
            set_module(symbol, 2, j);
        }
        j += 2;
    }

    symbol->row_height[0] = 4;
    symbol->row_height[1] = 2;
    symbol->row_height[2] = 4;

    symbol->rows = 3;
    symbol->width = j - 1;
    
    return 0;
}
