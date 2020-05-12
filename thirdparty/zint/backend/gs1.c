/* gs1.c - Verifies GS1 data */

/*
    libzint - the open source barcode library
    Copyright (C) 2009-2017 Robin Stuart <rstuart114@gmail.com>

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
#include "gs1.h"

/* This code does some checks on the integrity of GS1 data. It is not intended
   to be bulletproof, nor does it report very accurately what problem was found
   or where, but should prevent some of the more common encoding errors */

void itostr(char ai_string[], int ai_value) {
    int thou, hund, ten, unit;
    char temp[2];

    strcpy(ai_string, "(");
    thou = ai_value / 1000;
    hund = (ai_value - (1000 * thou)) / 100;
    ten = (ai_value - ((1000 * thou) + (100 * hund))) / 10;
    unit = ai_value - ((1000 * thou) + (100 * hund) + (10 * ten));

    temp[1] = '\0';
    if (ai_value >= 1000) {
        temp[0] = itoc(thou);
        strcat(ai_string, temp);
    }
    if (ai_value >= 100) {
        temp[0] = itoc(hund);
        strcat(ai_string, temp);
    }
    temp[0] = itoc(ten);
    strcat(ai_string, temp);
    temp[0] = itoc(unit);
    strcat(ai_string, temp);
    strcat(ai_string, ")");
}

int gs1_verify(struct zint_symbol *symbol, const unsigned char source[], const size_t src_len, char reduced[]) {
    int i, j, last_ai, ai_latch;
    char ai_string[6];
    int bracket_level, max_bracket_level, ai_length, max_ai_length, min_ai_length;
    int ai_value[100], ai_location[100], ai_count, data_location[100], data_length[100];
    int error_latch;

    /* Detect extended ASCII characters */
    for (i = 0; i < src_len; i++) {
        if (source[i] >= 128) {
            strcpy(symbol->errtxt, "250: Extended ASCII characters are not supported by GS1");
            return ZINT_ERROR_INVALID_DATA;
        }
        if (source[i] < 32) {
            strcpy(symbol->errtxt, "251: Control characters are not supported by GS1 ");
            return ZINT_ERROR_INVALID_DATA;
        }
    }

    if (source[0] != '[') {
        strcpy(symbol->errtxt, "252: Data does not start with an AI");
        return ZINT_ERROR_INVALID_DATA;
    }

    /* Check the position of the brackets */
    bracket_level = 0;
    max_bracket_level = 0;
    ai_length = 0;
    max_ai_length = 0;
    min_ai_length = 5;
    j = 0;
    ai_latch = 0;
    for (i = 0; i < src_len; i++) {
        ai_length += j;
        if (((j == 1) && (source[i] != ']')) && ((source[i] < '0') || (source[i] > '9'))) {
            ai_latch = 1;
        }
        if (source[i] == '[') {
            bracket_level++;
            j = 1;
        }
        if (source[i] == ']') {
            bracket_level--;
            if (ai_length < min_ai_length) {
                min_ai_length = ai_length;
            }
            j = 0;
            ai_length = 0;
        }
        if (bracket_level > max_bracket_level) {
            max_bracket_level = bracket_level;
        }
        if (ai_length > max_ai_length) {
            max_ai_length = ai_length;
        }
    }
    min_ai_length--;

    if (bracket_level != 0) {
        /* Not all brackets are closed */
        strcpy(symbol->errtxt, "253: Malformed AI in input data (brackets don\'t match)");
        return ZINT_ERROR_INVALID_DATA;
    }

    if (max_bracket_level > 1) {
        /* Nested brackets */
        strcpy(symbol->errtxt, "254: Found nested brackets in input data");
        return ZINT_ERROR_INVALID_DATA;
    }

    if (max_ai_length > 4) {
        /* AI is too long */
        strcpy(symbol->errtxt, "255: Invalid AI in input data (AI too long)");
        return ZINT_ERROR_INVALID_DATA;
    }

    if (min_ai_length <= 1) {
        /* AI is too short */
        strcpy(symbol->errtxt, "256: Invalid AI in input data (AI too short)");
        return ZINT_ERROR_INVALID_DATA;
    }

    if (ai_latch == 1) {
        /* Non-numeric data in AI */
        strcpy(symbol->errtxt, "257: Invalid AI in input data (non-numeric characters in AI)");
        return ZINT_ERROR_INVALID_DATA;
    }

    ai_count = 0;
    for (i = 1; i < src_len; i++) {
        if (source[i - 1] == '[') {
            ai_location[ai_count] = i;
            j = 0;
            do {
                ai_string[j] = source[i + j];
                j++;
            } while (ai_string[j - 1] != ']');
            ai_string[j - 1] = '\0';
            ai_value[ai_count] = atoi(ai_string);
            ai_count++;
        }
    }

    for (i = 0; i < ai_count; i++) {
        data_location[i] = ai_location[i] + 3;
        if (ai_value[i] >= 100) {
            data_location[i]++;
        }
        if (ai_value[i] >= 1000) {
            data_location[i]++;
        }
        data_length[i] = 0;
        do {
            data_length[i]++;
        } while ((source[data_location[i] + data_length[i] - 1] != '[') && (data_location[i] + data_length[i] <= src_len));
        data_length[i]--;
    }

    for (i = 0; i < ai_count; i++) {
        if (data_length[i] == 0) {
            /* No data for given AI */
            strcpy(symbol->errtxt, "258: Empty data field in input data");
            return ZINT_ERROR_INVALID_DATA;
        }
    }
    
    strcpy(ai_string, "");
    
    // Check for valid AI values and data lengths according to GS1 General
    // Specification Release 18, January 2018
    for (i = 0; i < ai_count; i++) {
        
        error_latch = 2;
        switch (ai_value[i]) {
            // Length 2 Fixed
            case 20: // VARIANT
                if (data_length[i] != 2) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 3 Fixed
            case 422: // ORIGIN
            case 424: // COUNTRY PROCESS
            case 426: // COUNTRY FULL PROCESS
                if (data_length[i] != 3) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 4 Fixed
            case 8111: // POINTS
                if (data_length[i] != 4) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 6 Fixed
            case 11: // PROD DATE
            case 12: // DUE DATE
            case 13: // PACK DATE
            case 15: // BEST BY
            case 16: // SELL BY
            case 17: // USE BY
            case 7006: // FIRST FREEZE DATE
            case 8005: // PRICE PER UNIT
                if (data_length[i] != 6) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 10 Fixed
            case 7003: // EXPIRY TIME
                if (data_length[i] != 10) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 13 Fixed
            case 410: // SHIP TO LOC
            case 411: // BILL TO
            case 412: // PURCHASE FROM
            case 413: // SHIP FOR LOC
            case 414: // LOC NO
            case 415: // PAY TO
            case 416: // PROD/SERV LOC
            case 7001: // NSN
                if (data_length[i] != 13) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 14 Fixed
            case 1: // GTIN
            case 2: // CONTENT
            case 8001: // DIMENSIONS
                if (data_length[i] != 14) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 17 Fixed
            case 402: // GSIN
                if (data_length[i] != 17) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 18 Fixed
            case 0: // SSCC
            case 8006: // ITIP
            case 8017: // GSRN PROVIDER
            case 8018: // GSRN RECIPIENT
                if (data_length[i] != 18) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 2 Max
            case 7010: // PROD METHOD
                if (data_length[i] > 2) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 3 Max
            case 427: // ORIGIN SUBDIVISION
            case 7008: // AQUATIC SPECIES
                if (data_length[i] > 3) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 4 Max
            case 7004: // ACTIVE POTENCY
                if (data_length[i] > 4) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 6 Max
            case 242: // MTO VARIANT
                if (data_length[i] > 6) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 8 Max
            case 30: // VAR COUNT
            case 37: // COUNT
                if (data_length[i] > 8) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 10 Max
            case 7009: // FISHING GEAR TYPE
            case 8019: // SRIN
                if (data_length[i] > 10) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 12 Max
            case 7005: // CATCH AREA
            case 8011: // CPID SERIAL
                if (data_length[i] > 12) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 20 Max
            case 10: // BATCH/LOT
            case 21: // SERIAL
            case 22: // CPV
            case 243: // PCN
            case 254: // GLN EXTENSION COMPONENT
            case 420: // SHIP TO POST
            case 7020: // REFURB LOT
            case 7021: // FUNC STAT
            case 7022: // REV STAT
            case 710: // NHRN PZN
            case 711: // NHRN CIP
            case 712: // NHRN CN
            case 713: // NHRN DRN
            case 714: // NHRN AIM
            case 8002: // CMT NO
            case 8012: // VERSION
                if (data_length[i] > 20) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 25 Max
            case 8020: // REF NO
                if (data_length[i] > 25) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 30 Max
            case 240: // ADDITIONAL ID
            case 241: // CUST PART NO
            case 250: // SECONDARY SERIAL
            case 251: // REF TO SOURCE
            case 400: // ORDER NUMBER
            case 401: // GINC
            case 403: // ROUTE
            case 7002: // MEAT CUT
            case 7023: // GIAI ASSEMBLY
            case 8004: // GIAI
            case 8010: // CPID
            case 8013: // BUDI-DI
            case 90: // INTERNAL
                if (data_length[i] > 30) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 34 Max
            case 8007: // IBAN
                if (data_length[i] > 34) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
            // Length 70 Max
            case 8110: // Coupon code
            case 8112: // Paperless coupon code
            case 8200: // PRODUCT URL
                if (data_length[i] > 70) {
                    error_latch = 1;
                } else {
                    error_latch = 0;
                }
                break;
                
        }
        
        if (ai_value[i] == 253) { // GDTI
            if ((data_length[i] < 14) || (data_length[i] > 30)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if (ai_value[i] == 255) { // GCN
            if ((data_length[i] < 14) || (data_length[i] > 25)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3100) && (ai_value[i] <= 3169)) {
            if (data_length[i] != 6) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3200) && (ai_value[i] <= 3379)) {
            if (data_length[i] != 6) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3400) && (ai_value[i] <= 3579)) {
            if (data_length[i] != 6) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3600) && (ai_value[i] <= 3699)) {
            if (data_length[i] != 6) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3900) && (ai_value[i] <= 3909)) { // AMOUNT
            if (data_length[i] > 15) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3910) && (ai_value[i] <= 3919)) { // AMOUNT
            if ((data_length[i] < 4) || (data_length[i] > 18)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3920) && (ai_value[i] <= 3929)) { // PRICE
            if (data_length[i] > 15) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3930) && (ai_value[i] <= 3939)) { // PRICE
            if ((data_length[i] < 4) || (data_length[i] > 18)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 3940) && (ai_value[i] <= 3949)) { // PRCNT OFF
            if (data_length[i] != 4) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if (ai_value[i] == 421) { // SHIP TO POST
            if ((data_length[i] < 4) || (data_length[i] > 12)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] == 423) || (ai_value[i] == 425)) {
            // COUNTRY INITIAL PROCESS || COUNTRY DISASSEMBLY
            if ((data_length[i] < 4) || (data_length[i] > 15)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if (ai_value[i] == 7007) { // HARVEST DATE
            if ((data_length[i] < 6) || (data_length[i] > 12)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 7030) && (ai_value[i] <= 7039)) { // PROCESSOR #
            if ((data_length[i] < 4) || (data_length[i] > 30)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if (ai_value[i] == 8003) { // GRAI
            if ((data_length[i] < 15) || (data_length[i] > 30)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if (ai_value[i] == 8008) { // PROD TIME
            if ((data_length[i] < 9) || (data_length[i] > 12)) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }
        
        if ((ai_value[i] >= 91) && (ai_value[i] <= 99)) { // INTERNAL
            if (data_length[i] > 90) {
                error_latch = 1;
            } else {
                error_latch = 0;
            }
        }

        if (error_latch == 1) {
            itostr(ai_string, ai_value[i]);
            strcpy(symbol->errtxt, "259: Invalid data length for AI ");
            strcat(symbol->errtxt, ai_string);
            return ZINT_ERROR_INVALID_DATA;
        }

        if (error_latch == 2) {
            itostr(ai_string, ai_value[i]);
            strcpy(symbol->errtxt, "260: Invalid AI value ");
            strcat(symbol->errtxt, ai_string);
            return ZINT_ERROR_INVALID_DATA;
        }
    }

    /* Resolve AI data - put resulting string in 'reduced' */
    j = 0;
    last_ai = 0;
    ai_latch = 1;
    for (i = 0; i < src_len; i++) {
        if ((source[i] != '[') && (source[i] != ']')) {
            reduced[j++] = source[i];
        }
        if (source[i] == '[') {
            /* Start of an AI string */
            if (ai_latch == 0) {
                reduced[j++] = '[';
            }
            ai_string[0] = source[i + 1];
            ai_string[1] = source[i + 2];
            ai_string[2] = '\0';
            last_ai = atoi(ai_string);
            ai_latch = 0;
            /* The following values from "GS-1 General Specification version 8.0 issue 2, May 2008"
            figure 5.4.8.2.1 - 1 "Element Strings with Pre-Defined Length Using Application Identifiers" */
            if (
                    ((last_ai >= 0) && (last_ai <= 4))
                    || ((last_ai >= 11) && (last_ai <= 20))
                    || (last_ai == 23) /* legacy support - see 5.3.8.2.2 */
                    || ((last_ai >= 31) && (last_ai <= 36))
                    || (last_ai == 41)
                    ) {
                ai_latch = 1;
            }
        }
        /* The ']' character is simply dropped from the input */
    }
    reduced[j] = '\0';

    /* the character '[' in the reduced string refers to the FNC1 character */
    return 0;
}

int ugs1_verify(struct zint_symbol *symbol, const unsigned char source[], const unsigned int src_len, unsigned char reduced[]) {
    /* Only to keep the compiler happy */
#ifndef _MSC_VER
    char temp[src_len + 5];
#else
    char* temp = (char*) _alloca(src_len + 5);
#endif
    int error_number;

    error_number = gs1_verify(symbol, source, src_len, temp);
    if (error_number != 0) {
        return error_number;
    }

    if (strlen(temp) < src_len + 5) {
        ustrcpy(reduced, (unsigned char*) temp);
        return 0;
    }
    strcpy(symbol->errtxt, "261: ugs1_verify overflow");
    return ZINT_ERROR_INVALID_DATA;
}

