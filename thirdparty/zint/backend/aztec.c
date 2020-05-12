/* aztec.c - Handles Aztec 2D Symbols */

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "aztec.h"
#include "reedsol.h"

static int AztecMap[22801];

static int count_doubles(const unsigned char source[], const int posn, const size_t src_len) {
    int c = 0;
    int i = posn;
    int cond = 1;

    do {
        if (((source[i] == '.') || (source[i] == ',')) && (source[i + 1] == ' ')) {
            c++;
        } else {
            cond = 0;
        }
        i += 2;
    } while ((i < src_len) && cond);

    return c;
}

static int count_cr(unsigned char source[], int posn, int length) {
    int c = 0;
    int i = posn;
    int cond = 1;

    do {
        if (source[i] == 13) {
            c++;
        } else {
            cond = 0;
        }
        i++;
    } while ((i < length) && cond);

    return c;
}

static int count_dotcomma(unsigned char source[], int posn, int length) {
    int c = 0;
    int i = posn;
    int cond = 1;

    do {
        if ((source[i] == '.') || (source[i] == ',')) {
            c++;
        } else {
            cond = 0;
        }
        i++;
    } while ((i < length) && cond);

    return c;
}

static int count_spaces(unsigned char source[], int posn, int length) {
    int c = 0;
    int i = posn;
    int cond = 1;

    do {
        if (source[i] == ' ') {
            c++;
        } else {
            cond = 0;
        }
        i++;
    } while ((i < length) && cond);

    return c;
}

static char get_next_mode(char encode_mode[], const size_t src_len, const int posn) {
    int i = posn;

    do {
        i++;
    } while ((i < src_len) && (encode_mode[i] == encode_mode[posn]));
    if (i >= src_len) {
        return 'E';
    } else {
        return encode_mode[i];
    }
}

static int aztec_text_process(const unsigned char source[], const size_t src_len, char binary_string[], const int gs1, const int eci, const int debug) {

    char *encode_mode;
    int i, j;
    char current_mode;
    int count;
    char next_mode;
    unsigned char *reduced_source;
    char *reduced_encode_mode;
    int reduced_length;
    int byte_mode = 0;

    encode_mode=(char*)malloc(src_len + 1);
    reduced_source=(unsigned char*)malloc(src_len + 1);
    reduced_encode_mode=(char*)malloc(src_len + 1);

    if ((!encode_mode) ||
        (!reduced_source) ||
        (!reduced_encode_mode)) {
        free(encode_mode);
        free(reduced_source);
        free(reduced_encode_mode);
        return -1;
    }

    for (i = 0; i < src_len; i++) {
        if (source[i] >= 128) {
            encode_mode[i] = 'B';
        } else {
            encode_mode[i] = AztecModes[(int) source[i]];
        }
    }

    // Deal first with letter combinations which can be combined to one codeword
    // Combinations are (CR LF) (. SP) (, SP) (: SP) in Punct mode
    current_mode = 'U';
    for (i = 0; i < src_len - 1; i++) {
        // Combination (CR LF) should always be in Punct mode
        if ((source[i] == 13) && (source[i + 1] == 10)) {
            encode_mode[i] = 'P';
            encode_mode[i + 1] = 'P';
        }

        // Combination (: SP) should always be in Punct mode
        if ((source[i] == ':') && (source[i + 1] == ' ')) {
            encode_mode[i + 1] = 'P';
        }

        // Combinations (. SP) and (, SP) sometimes use fewer bits in Digit mode
        if (((source[i] == '.') || (source[i] == ',')) && (source[i + 1] == ' ') && (encode_mode[i] == 'X')) {
            count = count_doubles(source, i, src_len);
            next_mode = get_next_mode(encode_mode, src_len, i);

            if (current_mode == 'U') {
                if ((next_mode == 'D') && (count <= 5)) {
                    for (j = 0; j < (2 * count); j++) {
                        encode_mode[i + j] = 'D';
                    }
                }
            }

            if (current_mode == 'L') {
                if ((next_mode == 'U') && (count == 1)) {
                    encode_mode[i] = 'D';
                    encode_mode[i + 1] = 'D';
                }
                if ((next_mode == 'D') && (count <= 4)) {
                    for (j = 0; j < (2 * count); j++) {
                        encode_mode[i + j] = 'D';
                    }
                }
            }

            if (current_mode == 'M') {
                if ((next_mode == 'D') && (count == 1)) {
                    encode_mode[i] = 'D';
                    encode_mode[i + 1] = 'D';
                }
            }

            if (current_mode == 'D') {
                if ((next_mode != 'D') && (count <= 4)) {
                    for (j = 0; j < (2 * count); j++) {
                        encode_mode[i + j] = 'D';
                    }
                }
                if ((next_mode == 'D') && (count <= 7)) {
                    for (j = 0; j < (2 * count); j++) {
                        encode_mode[i + j] = 'D';
                    }
                }
            }

            // Default is Punct mode
            if (encode_mode[i] == 'X') {
                encode_mode[i] = 'P';
                encode_mode[i + 1] = 'P';
            }
        }

        if ((encode_mode[i] != 'X') && (encode_mode[i] != 'B')) {
            current_mode = encode_mode[i];
        }
    }

    if (debug) {
        printf("First Pass:\n");
        for (i = 0; i < src_len; i++) {
            printf("%c", encode_mode[i]);
        }
        printf("\n");
    }

    // Reduce two letter combinations to one codeword marked as [abcd] in Punct mode
    i = 0;
    j = 0;
    do {
        if ((source[i] == 13) && (source[i + 1] == 10)) { // CR LF
            reduced_source[j] = 'a';
            reduced_encode_mode[j] = encode_mode[i];
            i += 2;
        } else if (((source[i] == '.') && (source[i + 1] == ' ')) && (encode_mode[i] == 'P')) {
            reduced_source[j] = 'b';
            reduced_encode_mode[j] = encode_mode[i];
            i += 2;
        } else if (((source[i] == ',') && (source[i + 1] == ' ')) && (encode_mode[i] == 'P')) {
            reduced_source[j] = 'c';
            reduced_encode_mode[j] = encode_mode[i];
            i += 2;
        } else if ((source[i] == ':') && (source[i + 1] == ' ')) {
            reduced_source[j] = 'd';
            reduced_encode_mode[j] = encode_mode[i];
            i += 2;
        } else {
            reduced_source[j] = source[i];
            reduced_encode_mode[j] = encode_mode[i];
            i++;
        }
        j++;
    } while (i < src_len);

    reduced_length = j;

    current_mode = 'U';
    for(i = 0; i < reduced_length; i++) {
        // Resolve Carriage Return (CR) which can be Punct or Mixed mode
        if (reduced_source[i] == 13) {
            count = count_cr(reduced_source, i, reduced_length);
            next_mode = get_next_mode(reduced_encode_mode, reduced_length, i);

            if ((current_mode == 'U') && ((next_mode == 'U') || (next_mode == 'B')) && (count == 1)) {
                reduced_encode_mode[i] = 'P';
            }

            if ((current_mode == 'L') && ((next_mode == 'L') || (next_mode == 'B')) && (count == 1)) {
                reduced_encode_mode[i] = 'P';
            }

            if ((current_mode == 'P') || (next_mode == 'P')) {
                reduced_encode_mode[i] = 'P';
            }

            if (current_mode == 'D') {
                if (((next_mode == 'E') || (next_mode == 'U') || (next_mode == 'D') || (next_mode == 'B')) && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'P';
                    }
                }
                if ((next_mode == 'L') && (count == 1)) {
                    reduced_encode_mode[i] = 'P';
                }
            }

            // Default is Mixed mode
            if (reduced_encode_mode[i] == 'X') {
                reduced_encode_mode[i] = 'M';
            }
        }

        // Resolve full stop and comma which can be in Punct or Digit mode
        if ((reduced_source[i] == '.') || (reduced_source[i] == ',')) {
            count = count_dotcomma(reduced_source, i, reduced_length);
            next_mode = get_next_mode(reduced_encode_mode, reduced_length, i);

            if (current_mode == 'U') {
                if (((next_mode == 'U') || (next_mode == 'L') || (next_mode == 'M') || (next_mode == 'B')) && (count == 1)) {
                    reduced_encode_mode[i] = 'P';
                }
            }

            if (current_mode == 'L') {
                if ((next_mode == 'L') && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'P';
                    }
                }
                if (((next_mode == 'M') || (next_mode == 'B')) && (count == 1)) {
                    reduced_encode_mode[i] = 'P';
                }
            }

            if (current_mode == 'M') {
                if (((next_mode == 'E') || (next_mode == 'U') || (next_mode == 'L') || (next_mode == 'M')) && (count <= 4)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'P';
                    }
                }
                if ((next_mode == 'B') && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'P';
                    }
                }
            }

            if ((current_mode == 'P') && (next_mode != 'D') && (count <= 9)) {
                for (j = 0; j < count; j++) {
                    reduced_encode_mode[i + j] = 'P';
                }
            }

            // Default is Digit mode
            if (reduced_encode_mode[i] == 'X') {
                reduced_encode_mode[i] = 'D';
            }
        }

        // Resolve Space (SP) which can be any mode except Punct
        if (reduced_source[i] == ' ') {
            count = count_spaces(reduced_source, i, reduced_length);
            next_mode = get_next_mode(reduced_encode_mode, reduced_length, i);

            if (current_mode == 'U') {
                if ((next_mode == 'E') && (count <= 5)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'U';
                    }
                }
                if (((next_mode == 'U') || (next_mode == 'L') || (next_mode == 'M') || (next_mode == 'P') || (next_mode == 'B')) && (count <= 9)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'U';
                    }
                }
            }

            if (current_mode == 'L') {
                if ((next_mode == 'E') && (count <= 5)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'L';
                    }
                }
                if ((next_mode == 'U') && (count == 1)) {
                    reduced_encode_mode[i] = 'L';
                }
                if ((next_mode == 'L') && (count <= 14)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'L';
                    }
                }
                if (((next_mode == 'M') || (next_mode == 'P') || (next_mode == 'B')) && (count <= 9)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'L';
                    }
                }
            }

            if (current_mode == 'M') {
                if (((next_mode == 'E') || (next_mode == 'U')) && (count <= 9)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'M';
                    }
                }

                if (((next_mode == 'L') || (next_mode == 'B')) && (count <= 14)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'M';
                    }
                }

                if (((next_mode == 'M') || (next_mode == 'P')) && (count <= 19)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'M';
                    }
                }
            }

            if (current_mode == 'P') {
                if ((next_mode == 'E') && (count <= 5)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'U';
                    }
                }

                if (((next_mode == 'U') || (next_mode == 'L') || (next_mode == 'M') || (next_mode == 'P') || (next_mode == 'B')) && (count <= 9)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'U';
                    }
                }
            }

            // Default is Digit mode
            if (reduced_encode_mode[i] == 'X') {
                reduced_encode_mode[i] = 'D';
            }
        }

        if (reduced_encode_mode[i] != 'B') {
            current_mode = reduced_encode_mode[i];
        }
    }

    // Decide when to use P/S instead of P/L and U/S instead of U/L
    current_mode = 'U';
    for(i = 0; i < reduced_length; i++) {

        if (reduced_encode_mode[i] != current_mode) {

            for (count = 0; ((i + count) <= reduced_length) && (reduced_encode_mode[i + count] == reduced_encode_mode[i]); count++);
            next_mode = get_next_mode(reduced_encode_mode, reduced_length, i);

            if (reduced_encode_mode[i] == 'P') {
                if ((current_mode == 'U') && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'p';
                    }
                }

                if ((current_mode == 'L') && (next_mode != 'U') && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'p';
                    }
                }

                if ((current_mode == 'L') && (next_mode == 'U') && (count == 1)) {
                    reduced_encode_mode[i] = 'p';
                }

                if ((current_mode == 'M') && (next_mode != 'M') && (count == 1)) {
                    reduced_encode_mode[i] = 'p';
                }

                if ((current_mode == 'M') && (next_mode == 'M') && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'p';
                    }
                }

                if ((current_mode == 'D') && (next_mode != 'D') && (count <= 3)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'p';
                    }
                }

                if ((current_mode == 'D') && (next_mode == 'D') && (count <= 6)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'p';
                    }
                }
            }

            if (reduced_encode_mode[i] == 'U') {
                if ((current_mode == 'L') && ((next_mode == 'L') || (next_mode == 'M')) && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'u';
                    }
                }

                if ((current_mode == 'L') && ((next_mode == 'E') || (next_mode == 'D') || (next_mode == 'B') || (next_mode == 'P')) && (count == 1)) {
                    reduced_encode_mode[i] = 'u';
                }

                if ((current_mode == 'D') && (next_mode == 'D') && (count == 1)) {
                    reduced_encode_mode[i] = 'u';
                }

                if ((current_mode == 'D') && (next_mode == 'P') && (count <= 2)) {
                    for (j = 0; j < count; j++) {
                        reduced_encode_mode[i + j] = 'u';
                    }
                }
            }
        }

        if ((reduced_encode_mode[i] != 'p') && (reduced_encode_mode[i] != 'u') && (reduced_encode_mode[i] != 'B')) {
            current_mode = reduced_encode_mode[i];
        }
    }

    if (debug) {
        for (i = 0; i < reduced_length; i++) {
            printf("%c", reduced_source[i]);
        }
        printf("\n");
        for (i = 0; i < reduced_length; i++) {
            printf("%c", reduced_encode_mode[i]);
        }
        printf("\n");
    }

    strcpy(binary_string, "");

    if (gs1) {
        bin_append(0, 5, binary_string); // P/S
        bin_append(0, 5, binary_string); // FLG(n)
        bin_append(0, 3, binary_string); // FLG(0)
    }

    if (eci != 3) {
        bin_append(0, 5, binary_string); // P/S
        bin_append(0, 5, binary_string); // FLG(n)
        if (eci < 10) {
            bin_append(1, 3, binary_string); // FLG(1)
            bin_append(2 + eci, 4, binary_string);
        }
        if ((eci >= 10) && (eci <= 99)) {
            bin_append(2, 3, binary_string); // FLG(2)
            bin_append(2 + (eci / 10), 4, binary_string);
            bin_append(2 + (eci % 10), 4, binary_string);
        }
        if ((eci >= 100) && (eci <= 999)) {
            bin_append(3, 3, binary_string); // FLG(3)
            bin_append(2 + (eci / 100), 4, binary_string);
            bin_append(2 + ((eci % 100) / 10), 4, binary_string);
            bin_append(2 + (eci % 10), 4, binary_string);
        }
        if ((eci >= 1000) && (eci <= 9999)) {
            bin_append(4, 3, binary_string); // FLG(4)
            bin_append(2 + (eci / 1000), 4, binary_string);
            bin_append(2 + ((eci % 1000) / 100), 4, binary_string);
            bin_append(2 + ((eci % 100) / 10), 4, binary_string);
            bin_append(2 + (eci % 10), 4, binary_string);
        }
        if ((eci >= 10000) && (eci <= 99999)) {
            bin_append(5, 3, binary_string); // FLG(5)
            bin_append(2 + (eci / 10000), 4, binary_string);
            bin_append(2 + ((eci % 10000) / 1000), 4, binary_string);
            bin_append(2 + ((eci % 1000) / 100), 4, binary_string);
            bin_append(2 + ((eci % 100) / 10), 4, binary_string);
            bin_append(2 + (eci % 10), 4, binary_string);
        }
        if (eci >= 100000) {
            bin_append(6, 3, binary_string); // FLG(6)
            bin_append(2 + (eci / 100000), 4, binary_string);
            bin_append(2 + ((eci % 100000) / 10000), 4, binary_string);
            bin_append(2 + ((eci % 10000) / 1000), 4, binary_string);
            bin_append(2 + ((eci % 1000) / 100), 4, binary_string);
            bin_append(2 + ((eci % 100) / 10), 4, binary_string);
            bin_append(2 + (eci % 10), 4, binary_string);
        }
    }

    current_mode = 'U';
    for(i = 0; i < reduced_length; i++) {

        if (reduced_encode_mode[i] != 'B') {
            byte_mode = 0;
        }

        if ((reduced_encode_mode[i] != current_mode) && (!byte_mode)) {
            // Change mode
            if (current_mode == 'U') {
                switch (reduced_encode_mode[i]) {
                    case 'L':
                        bin_append(28, 5, binary_string); // L/L
                        break;
                    case 'M':
                        bin_append(29, 5, binary_string); // M/L
                        break;
                    case 'P':
                        bin_append(29, 5, binary_string); // M/L
                        bin_append(30, 5, binary_string); // P/L
                        break;
                    case 'p':
                        bin_append(0, 5, binary_string); // P/S
                        break;
                    case 'D':
                        bin_append(30, 5, binary_string); // D/L
                        break;
                    case 'B':
                        bin_append(31, 5, binary_string); // B/S
                        break;
                }
            }

            if (current_mode == 'L') {
                switch (reduced_encode_mode[i]) {
                    case 'U':
                        bin_append(30, 5, binary_string); // D/L
                        bin_append(14, 4, binary_string); // U/L
                        break;
                    case 'u':
                        bin_append(28, 5, binary_string); // U/S
                        break;
                    case 'M':
                        bin_append(29, 5, binary_string); // M/L
                        break;
                    case 'P':
                        bin_append(29, 5, binary_string); // M/L
                        bin_append(30, 5, binary_string); // P/L
                        break;
                    case 'p':
                        bin_append(0, 5, binary_string); // P/S
                        break;
                    case 'D':
                        bin_append(30, 5, binary_string); // D/L
                        break;
                    case 'B':
                        bin_append(31, 5, binary_string); // B/S
                        break;
                }
            }

            if (current_mode == 'M') {
                switch (reduced_encode_mode[i]) {
                    case 'U':
                        bin_append(29, 5, binary_string); // U/L
                        break;
                    case 'L':
                        bin_append(28, 5, binary_string); // L/L
                        break;
                    case 'P':
                        bin_append(30, 5, binary_string); // P/L
                        break;
                    case 'p':
                        bin_append(0, 5, binary_string); // P/S
                        break;
                    case 'D':
                        bin_append(29, 5, binary_string); // U/L
                        bin_append(30, 5, binary_string); // D/L
                        break;
                    case 'B':
                        bin_append(31, 5, binary_string); // B/S
                        break;
                }
            }

            if (current_mode == 'P') {
                switch (reduced_encode_mode[i]) {
                    case 'U':
                        bin_append(31, 5, binary_string); // U/L
                        break;
                    case 'L':
                        bin_append(31, 5, binary_string); // U/L
                        bin_append(28, 5, binary_string); // L/L
                        break;
                    case 'M':
                        bin_append(31, 5, binary_string); // U/L
                        bin_append(29, 5, binary_string); // M/L
                        break;
                    case 'D':
                        bin_append(31, 5, binary_string); // U/L
                        bin_append(30, 5, binary_string); // D/L
                        break;
                    case 'B':
                        bin_append(31, 5, binary_string); // U/L
                        current_mode = 'U';
                        bin_append(31, 5, binary_string); // B/S
                        break;
                }
            }

            if (current_mode == 'D') {
                switch (reduced_encode_mode[i]) {
                    case 'U':
                        bin_append(14, 4, binary_string); // U/L
                        break;
                    case 'u':
                        bin_append(15, 4, binary_string); // U/S
                        break;
                    case 'L':
                        bin_append(14, 4, binary_string); // U/L
                        bin_append(28, 5, binary_string); // L/L
                        break;
                    case 'M':
                        bin_append(14, 4, binary_string); // U/L
                        bin_append(29, 5, binary_string); // M/L
                        break;
                    case 'P':
                        bin_append(14, 4, binary_string); // U/L
                        bin_append(29, 5, binary_string); // M/L
                        bin_append(30, 5, binary_string); // P/L
                        break;
                    case 'p':
                        bin_append(0, 4, binary_string); // P/S
                        break;
                    case 'B':
                        bin_append(14, 4, binary_string); // U/L
                        current_mode = 'U';
                        bin_append(31, 5, binary_string); // B/S
                        break;
                }
            }

            // Byte mode length descriptor
            if ((reduced_encode_mode[i] == 'B') && (!byte_mode)) {
                for (count = 0; ((i + count) < reduced_length) && (reduced_encode_mode[i + count] == 'B'); count++);

                if (count > 2079) {
                    return ZINT_ERROR_TOO_LONG;
                }

                if (count > 31) {
                    /* Put 00000 followed by 11-bit number of bytes less 31 */
                    bin_append(0, 5, binary_string);
                    bin_append(count - 31, 11, binary_string);
                } else {
                    /* Put 5-bit number of bytes */
                    bin_append(count, 5, binary_string);
                }
                byte_mode = 1;
            }

            if ((reduced_encode_mode[i] != 'B') && (reduced_encode_mode[i] != 'u') && (reduced_encode_mode[i] != 'p')) {
                current_mode = reduced_encode_mode[i];
            }
        }

        if ((reduced_encode_mode[i] == 'U') || (reduced_encode_mode[i] == 'u')) {
            if (reduced_source[i] == ' ') {
                bin_append(1, 5, binary_string); // SP
            } else {
                bin_append(AztecSymbolChar[(int) reduced_source[i]], 5, binary_string);
            }
        }

        if (reduced_encode_mode[i] == 'L') {
            if (reduced_source[i] == ' ') {
                bin_append(1, 5, binary_string); // SP
            } else {
                bin_append(AztecSymbolChar[(int) reduced_source[i]], 5, binary_string);
            }
        }

        if (reduced_encode_mode[i] == 'M') {
            if (reduced_source[i] == ' ') {
                bin_append(1, 5, binary_string); // SP
            } else if (reduced_source[i] == 13) {
                bin_append(14, 5, binary_string); // CR
            } else {
                bin_append(AztecSymbolChar[(int) reduced_source[i]], 5, binary_string);
            }
        }

        if ((reduced_encode_mode[i] == 'P') || (reduced_encode_mode[i] == 'p')) {
            if (gs1 && (reduced_source[i] == '[')) {
                bin_append(0, 5, binary_string); // FLG(0) = FNC1
            } else if (reduced_source[i] == 13) {
                bin_append(1, 5, binary_string); // CR
            } else if (reduced_source[i] == 'a') {
                bin_append(2, 5, binary_string); // CR LF
            } else if (reduced_source[i] == 'b') {
                bin_append(3, 5, binary_string); // . SP
            } else if (reduced_source[i] == 'c') {
                bin_append(4, 5, binary_string); // , SP
            } else if (reduced_source[i] == 'd') {
                bin_append(5, 5, binary_string); // : SP
            } else if (reduced_source[i] == ',') {
                bin_append(17, 5, binary_string); // Comma
            } else if (reduced_source[i] == '.') {
                bin_append(19, 5, binary_string); // Full stop
            } else {
                bin_append(AztecSymbolChar[(int) reduced_source[i]], 5, binary_string);
            }
        }

        if (reduced_encode_mode[i] == 'D') {
            if (reduced_source[i] == ' ') {
                bin_append(1, 4, binary_string); // SP
            } else if (reduced_source[i] == ',') {
                bin_append(12, 4, binary_string); // Comma
            } else if (reduced_source[i] == '.') {
                bin_append(13, 4, binary_string); // Full stop
            } else {
                bin_append(AztecSymbolChar[(int) reduced_source[i]], 4, binary_string);
            }
        }

        if (reduced_encode_mode[i] == 'B') {
            bin_append(reduced_source[i], 8, binary_string);
        }
    }

    if (debug) {
        printf("Binary String:\n");
        printf("%s\n", binary_string);
    }

    free(encode_mode);
    free(reduced_source);
    free(reduced_encode_mode);

    return 0;
}

/* Prevent data from obscuring reference grid */
static int avoidReferenceGrid(int output) {

    if (output > 10) {
        output++;
    }
    if (output > 26) {
        output++;
    }
    if (output > 42) {
        output++;
    }
    if (output > 58) {
        output++;
    }
    if (output > 74) {
        output++;
    }
    if (output > 90) {
        output++;
    }
    if (output > 106) {
        output++;
    }
    if (output > 122) {
        output++;
    }
    if (output > 138) {
        output++;
    }

    return output;
}

/* Calculate the position of the bits in the grid */
static void populate_map() {
    int layer, n, i;
    int x, y;

    for (x = 0; x < 151; x++) {
        for (y = 0; y < 151; y++) {
            AztecMap[(x * 151) + y] = 0;
        }
    }

    for (layer = 1; layer < 33; layer++) {
        const int start = (112 * (layer - 1)) + (16 * (layer - 1) * (layer - 1)) + 2;
        const int length = 28 + ((layer - 1) * 4) + (layer * 4);
        /* Top */
        i = 0;
        x = 64 - ((layer - 1) * 2);
        y = 63 - ((layer - 1) * 2);
        for (n = start; n < (start + length); n += 2) {
            AztecMap[(avoidReferenceGrid(y) * 151) + avoidReferenceGrid(x + i)] = n;
            AztecMap[(avoidReferenceGrid(y - 1) * 151) + avoidReferenceGrid(x + i)] = n + 1;
            i++;
        }
        /* Right */
        i = 0;
        x = 78 + ((layer - 1) * 2);
        y = 64 - ((layer - 1) * 2);
        for (n = start + length; n < (start + (length * 2)); n += 2) {
            AztecMap[(avoidReferenceGrid(y + i) * 151) + avoidReferenceGrid(x)] = n;
            AztecMap[(avoidReferenceGrid(y + i) * 151) + avoidReferenceGrid(x + 1)] = n + 1;
            i++;
        }
        /* Bottom */
        i = 0;
        x = 77 + ((layer - 1) * 2);
        y = 78 + ((layer - 1) * 2);
        for (n = start + (length * 2); n < (start + (length * 3)); n += 2) {
            AztecMap[(avoidReferenceGrid(y) * 151) + avoidReferenceGrid(x - i)] = n;
            AztecMap[(avoidReferenceGrid(y + 1) * 151) + avoidReferenceGrid(x - i)] = n + 1;
            i++;
        }
        /* Left */
        i = 0;
        x = 63 - ((layer - 1) * 2);
        y = 77 + ((layer - 1) * 2);
        for (n = start + (length * 3); n < (start + (length * 4)); n += 2) {
            AztecMap[(avoidReferenceGrid(y - i) * 151) + avoidReferenceGrid(x)] = n;
            AztecMap[(avoidReferenceGrid(y - i) * 151) + avoidReferenceGrid(x - 1)] = n + 1;
            i++;
        }
    }

    /* Central finder pattern */
    for (y = 69; y <= 81; y++) {
        for (x = 69; x <= 81; x++) {
            AztecMap[(x * 151) + y] = 1;
        }
    }
    for (y = 70; y <= 80; y++) {
        for (x = 70; x <= 80; x++) {
            AztecMap[(x * 151) + y] = 0;
        }
    }
    for (y = 71; y <= 79; y++) {
        for (x = 71; x <= 79; x++) {
            AztecMap[(x * 151) + y] = 1;
        }
    }
    for (y = 72; y <= 78; y++) {
        for (x = 72; x <= 78; x++) {
            AztecMap[(x * 151) + y] = 0;
        }
    }
    for (y = 73; y <= 77; y++) {
        for (x = 73; x <= 77; x++) {
            AztecMap[(x * 151) + y] = 1;
        }
    }
    for (y = 74; y <= 76; y++) {
        for (x = 74; x <= 76; x++) {
            AztecMap[(x * 151) + y] = 0;
        }
    }

    /* Guide bars */
    for (y = 11; y < 151; y += 16) {
        for (x = 1; x < 151; x += 2) {
            AztecMap[(x * 151) + y] = 1;
            AztecMap[(y * 151) + x] = 1;
        }
    }

    /* Descriptor */
    for (i = 0; i < 10; i++) {
        /* Top */
        AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(66 + i)] = 20000 + i;
    }
    for (i = 0; i < 10; i++) {
        /* Right */
        AztecMap[(avoidReferenceGrid(66 + i) * 151) + avoidReferenceGrid(77)] = 20010 + i;
    }
    for (i = 0; i < 10; i++) {
        /* Bottom */
        AztecMap[(avoidReferenceGrid(77) * 151) + avoidReferenceGrid(75 - i)] = 20020 + i;
    }
    for (i = 0; i < 10; i++) {
        /* Left */
        AztecMap[(avoidReferenceGrid(75 - i) * 151) + avoidReferenceGrid(64)] = 20030 + i;
    }

    /* Orientation */
    AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(64)] = 1;
    AztecMap[(avoidReferenceGrid(65) * 151) + avoidReferenceGrid(64)] = 1;
    AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(65)] = 1;
    AztecMap[(avoidReferenceGrid(64) * 151) + avoidReferenceGrid(77)] = 1;
    AztecMap[(avoidReferenceGrid(65) * 151) + avoidReferenceGrid(77)] = 1;
    AztecMap[(avoidReferenceGrid(76) * 151) + avoidReferenceGrid(77)] = 1;
}

int aztec(struct zint_symbol *symbol, unsigned char source[], const size_t length) {
    int x, y, i, j, p, data_blocks, ecc_blocks, layers, total_bits;
    char binary_string[20000], bit_pattern[20045], descriptor[42];
    char adjusted_string[20000];
    unsigned char desc_data[4], desc_ecc[6];
    int err_code, ecc_level, compact, data_length, data_maxsize, codeword_size, adjusted_length;
    int remainder, padbits, count, gs1, adjustment_size;
    int debug = symbol->debug, reader = 0;
    int comp_loop = 4;

#ifdef _MSC_VER
    unsigned int* data_part;
    unsigned int* ecc_part;
#endif

    memset(binary_string, 0, 20000);
    memset(adjusted_string, 0, 20000);

    if (symbol->input_mode == GS1_MODE) {
        gs1 = 1;
    } else {
        gs1 = 0;
    }
    if (symbol->output_options & READER_INIT) {
        reader = 1;
        comp_loop = 1;
    }
    if (gs1 && reader) {
        strcpy(symbol->errtxt, "501: Cannot encode in GS1 and Reader Initialisation mode at the same time");
        return ZINT_ERROR_INVALID_OPTION;
    }

    populate_map();

    err_code = aztec_text_process(source, length, binary_string, gs1, symbol->eci, symbol->debug);

    if (err_code != 0) {
        strcpy(symbol->errtxt, "502: Input too long or too many extended ASCII characters");
        return err_code;
    }

    if (!((symbol->option_1 >= -1) && (symbol->option_1 <= 4))) {
        strcpy(symbol->errtxt, "503: Invalid error correction level - using default instead");
        err_code = ZINT_WARN_INVALID_OPTION;
        symbol->option_1 = -1;
    }

    ecc_level = symbol->option_1;

    if ((ecc_level == -1) || (ecc_level == 0)) {
        ecc_level = 2;
    }

    data_length = (int) strlen(binary_string);

    layers = 0; /* Keep compiler happy! */
    data_maxsize = 0; /* Keep compiler happy! */
    adjustment_size = 0;
    if (symbol->option_2 == 0) { /* The size of the symbol can be determined by Zint */
        do {
            /* Decide what size symbol to use - the smallest that fits the data */
            compact = 0; /* 1 = Aztec Compact, 0 = Normal Aztec */
            layers = 0;

            switch (ecc_level) {
                    /* For each level of error correction work out the smallest symbol which
                    the data will fit in */
                case 1: for (i = 32; i > 0; i--) {
                        if ((data_length + adjustment_size) < Aztec10DataSizes[i - 1]) {
                            layers = i;
                            compact = 0;
                            data_maxsize = Aztec10DataSizes[i - 1];
                        }
                    }
                    for (i = comp_loop; i > 0; i--) {
                        if ((data_length + adjustment_size) < AztecCompact10DataSizes[i - 1]) {
                            layers = i;
                            compact = 1;
                            data_maxsize = AztecCompact10DataSizes[i - 1];
                        }
                    }
                    break;
                case 2: for (i = 32; i > 0; i--) {
                        if ((data_length + adjustment_size) < Aztec23DataSizes[i - 1]) {
                            layers = i;
                            compact = 0;
                            data_maxsize = Aztec23DataSizes[i - 1];
                        }
                    }
                    for (i = comp_loop; i > 0; i--) {
                        if ((data_length + adjustment_size) < AztecCompact23DataSizes[i - 1]) {
                            layers = i;
                            compact = 1;
                            data_maxsize = AztecCompact23DataSizes[i - 1];
                        }
                    }
                    break;
                case 3: for (i = 32; i > 0; i--) {
                        if ((data_length + adjustment_size) < Aztec36DataSizes[i - 1]) {
                            layers = i;
                            compact = 0;
                            data_maxsize = Aztec36DataSizes[i - 1];
                        }
                    }
                    for (i = comp_loop; i > 0; i--) {
                        if ((data_length + adjustment_size) < AztecCompact36DataSizes[i - 1]) {
                            layers = i;
                            compact = 1;
                            data_maxsize = AztecCompact36DataSizes[i - 1];
                        }
                    }
                    break;
                case 4: for (i = 32; i > 0; i--) {
                        if ((data_length + adjustment_size) < Aztec50DataSizes[i - 1]) {
                            layers = i;
                            compact = 0;
                            data_maxsize = Aztec50DataSizes[i - 1];
                        }
                    }
                    for (i = comp_loop; i > 0; i--) {
                        if ((data_length + adjustment_size) < AztecCompact50DataSizes[i - 1]) {
                            layers = i;
                            compact = 1;
                            data_maxsize = AztecCompact50DataSizes[i - 1];
                        }
                    }
                    break;
            }

            if (layers == 0) { /* Couldn't find a symbol which fits the data */
                strcpy(symbol->errtxt, "504: Input too long (too many bits for selected ECC)");
                return ZINT_ERROR_TOO_LONG;
            }

            /* Determine codeword bitlength - Table 3 */
            codeword_size = 6; /* if (layers <= 2) */
            if ((layers >= 3) && (layers <= 8)) {
                codeword_size = 8;
            }
            if ((layers >= 9) && (layers <= 22)) {
                codeword_size = 10;
            }
            if (layers >= 23) {
                codeword_size = 12;
            }

            j = 0;
            i = 0;
            do {
                if ((j + 1) % codeword_size == 0) {
                    /* Last bit of codeword */
                    int t, done = 0;
                    count = 0;

                    /* Discover how many '1's in current codeword */
                    for (t = 0; t < (codeword_size - 1); t++) {
                        if (binary_string[(i - (codeword_size - 1)) + t] == '1') count++;
                    }

                    if (count == (codeword_size - 1)) {
                        adjusted_string[j] = '0';
                        j++;
                        done = 1;
                    }

                    if (count == 0) {
                        adjusted_string[j] = '1';
                        j++;
                        done = 1;
                    }

                    if (done == 0) {
                        adjusted_string[j] = binary_string[i];
                        j++;
                        i++;
                    }
                }
                adjusted_string[j] = binary_string[i];
                j++;
                i++;
            } while (i <= (data_length + 1));
            adjusted_string[j] = '\0';
            adjusted_length = (int) strlen(adjusted_string);
            adjustment_size = adjusted_length - data_length;

            /* Add padding */
            remainder = adjusted_length % codeword_size;

            padbits = codeword_size - remainder;
            if (padbits == codeword_size) {
                padbits = 0;
            }

            for (i = 0; i < padbits; i++) {
                strcat(adjusted_string, "1");
            }
            adjusted_length = (int) strlen(adjusted_string);

            count = 0;
            for (i = (adjusted_length - codeword_size); i < adjusted_length; i++) {
                if (adjusted_string[i] == '1') {
                    count++;
                }
            }
            if (count == codeword_size) {
                adjusted_string[adjusted_length - 1] = '0';
            }

/*
            if (debug) {
                printf("Codewords:\n");
                for (i = 0; i < (adjusted_length / codeword_size); i++) {
                    for (j = 0; j < codeword_size; j++) {
                        printf("%c", adjusted_string[(i * codeword_size) + j]);
                    }
                    printf("\n");
                }
            }
*/

        } while (adjusted_length > data_maxsize);
        /* This loop will only repeat on the rare occasions when the rule about not having all 1s or all 0s
        means that the binary string has had to be lengthened beyond the maximum number of bits that can
        be encoded in a symbol of the selected size */

    } else { /* The size of the symbol has been specified by the user */
        if ((reader == 1) && ((symbol->option_2 >= 2) && (symbol->option_2 <= 4))) {
            symbol->option_2 = 5;
        }
        if ((symbol->option_2 >= 1) && (symbol->option_2 <= 4)) {
            compact = 1;
            layers = symbol->option_2;
        }
        if ((symbol->option_2 >= 5) && (symbol->option_2 <= 36)) {
            compact = 0;
            layers = symbol->option_2 - 4;
        }
        if ((symbol->option_2 < 0) || (symbol->option_2 > 36)) {
            strcpy(symbol->errtxt, "510: Invalid Aztec Code size");
            return ZINT_ERROR_INVALID_OPTION;
        }

        /* Determine codeword bitlength - Table 3 */
        if ((layers >= 0) && (layers <= 2)) {
            codeword_size = 6;
        }
        if ((layers >= 3) && (layers <= 8)) {
            codeword_size = 8;
        }
        if ((layers >= 9) && (layers <= 22)) {
            codeword_size = 10;
        }
        if (layers >= 23) {
            codeword_size = 12;
        }

        j = 0;
        i = 0;
        do {
            if ((j + 1) % codeword_size == 0) {
                /* Last bit of codeword */
                int t, done = 0;
                count = 0;

                /* Discover how many '1's in current codeword */
                for (t = 0; t < (codeword_size - 1); t++) {
                    if (binary_string[(i - (codeword_size - 1)) + t] == '1') count++;
                }

                if (count == (codeword_size - 1)) {
                    adjusted_string[j] = '0';
                    j++;
                    done = 1;
                }

                if (count == 0) {
                    adjusted_string[j] = '1';
                    j++;
                    done = 1;
                }

                if (done == 0) {
                    adjusted_string[j] = binary_string[i];
                    j++;
                    i++;
                }
            }
            adjusted_string[j] = binary_string[i];
            j++;
            i++;
        } while (i <= (data_length + 1));
        adjusted_string[j] = '\0';
        adjusted_length = (int) strlen(adjusted_string);

        remainder = adjusted_length % codeword_size;

        padbits = codeword_size - remainder;
        if (padbits == codeword_size) {
            padbits = 0;
        }

        for (i = 0; i < padbits; i++) {
            strcat(adjusted_string, "1");
        }
        adjusted_length = (int) strlen(adjusted_string);

        count = 0;
        for (i = (adjusted_length - codeword_size); i < adjusted_length; i++) {
            if (adjusted_string[i] == '1') {
                count++;
            }
        }
        if (count == codeword_size) {
            adjusted_string[adjusted_length - 1] = '0';
        }

        /* Check if the data actually fits into the selected symbol size */
        if (compact) {
            data_maxsize = codeword_size * (AztecCompactSizes[layers - 1] - 3);
        } else {
            data_maxsize = codeword_size * (AztecSizes[layers - 1] - 3);
        }

        if (adjusted_length > data_maxsize) {
            strcpy(symbol->errtxt, "505: Data too long for specified Aztec Code symbol size");
            return ZINT_ERROR_TOO_LONG;
        }

        if (debug) {
            printf("Codewords:\n");
            for (i = 0; i < (adjusted_length / codeword_size); i++) {
                for (j = 0; j < codeword_size; j++) {
                    printf("%c", adjusted_string[(i * codeword_size) + j]);
                }
                printf("\n");
            }
        }

    }

    if (reader && (layers > 22)) {
        strcpy(symbol->errtxt, "506: Data too long for reader initialisation symbol");
        return ZINT_ERROR_TOO_LONG;
    }

    data_blocks = adjusted_length / codeword_size;

    if (compact) {
        ecc_blocks = AztecCompactSizes[layers - 1] - data_blocks;
    } else {
        ecc_blocks = AztecSizes[layers - 1] - data_blocks;
    }

    if (debug) {
        printf("Generating a ");
        if (compact) {
            printf("compact");
        } else {
            printf("full-size");
        }
        printf(" symbol with %d layers\n", layers);
        printf("Requires ");
        if (compact) {
            printf("%d", AztecCompactSizes[layers - 1]);
        } else {
            printf("%d", AztecSizes[layers - 1]);
        }
        printf(" codewords of %d-bits\n", codeword_size);
        printf("    (%d data words, %d ecc words)\n", data_blocks, ecc_blocks);
    }

#ifndef _MSC_VER
    unsigned int data_part[data_blocks + 3], ecc_part[ecc_blocks + 3];
#else
    data_part = (unsigned int*) _alloca((data_blocks + 3) * sizeof (unsigned int));
    ecc_part = (unsigned int*) _alloca((ecc_blocks + 3) * sizeof (unsigned int));
#endif
    /* Copy across data into separate integers */
    memset(data_part, 0, (data_blocks + 2) * sizeof (int));
    memset(ecc_part, 0, (ecc_blocks + 2) * sizeof (int));

    /* Split into codewords and calculate reed-solomon error correction codes */
    for (i = 0; i < data_blocks; i++) {
        for (p = 0; p < codeword_size; p++) {
            if (adjusted_string[i * codeword_size + p] == '1') {
                data_part[i] += 0x01 << (codeword_size - (p + 1));
            }
        }
    }

    switch (codeword_size) {
        case 6:
            rs_init_gf(0x43);
            break;
        case 8:
            rs_init_gf(0x12d);
            break;
        case 10:
            rs_init_gf(0x409);
            break;
        case 12:
            rs_init_gf(0x1069);
            break;
    }

    rs_init_code(ecc_blocks, 1);
    rs_encode_long(data_blocks, data_part, ecc_part);
    for (i = (ecc_blocks - 1); i >= 0; i--) {
        bin_append(ecc_part[i], codeword_size, adjusted_string);
    }
    rs_free();

    /* Invert the data so that actual data is on the outside and reed-solomon on the inside */
    memset(bit_pattern, '0', 20045);

    total_bits = (data_blocks + ecc_blocks) * codeword_size;
    for (i = 0; i < total_bits; i++) {
        bit_pattern[i] = adjusted_string[total_bits - i - 1];
    }

    /* Now add the symbol descriptor */
    memset(desc_data, 0, 4);
    memset(desc_ecc, 0, 6);
    memset(descriptor, 0, 42);

    if (compact) {
        /* The first 2 bits represent the number of layers minus 1 */
        if ((layers - 1) & 0x02) {
            descriptor[0] = '1';
        } else {
            descriptor[0] = '0';
        }
        if ((layers - 1) & 0x01) {
            descriptor[1] = '1';
        } else {
            descriptor[1] = '0';
        }
        /* The next 6 bits represent the number of data blocks minus 1 */
        if (reader) {
            descriptor[2] = '1';
        } else {
            if ((data_blocks - 1) & 0x20) {
                descriptor[2] = '1';
            } else {
                descriptor[2] = '0';
            }
        }

        for (i = 3; i < 8; i++) {
            if ((data_blocks - 1) & (0x10 >> (i - 3))) {
                descriptor[i] = '1';
            } else {
                descriptor[i] = '0';
            }
        }

        descriptor[8] = '\0';
        if (debug) printf("Mode Message = %s\n", descriptor);
    } else {
        /* The first 5 bits represent the number of layers minus 1 */
        for (i = 0; i < 5; i++) {
            if ((layers - 1) & (0x10 >> i)) {
                descriptor[i] = '1';
            } else {
                descriptor[i] = '0';
            }
        }

        /* The next 11 bits represent the number of data blocks minus 1 */
        if (reader) {
            descriptor[5] = '1';
        } else {
            if ((data_blocks - 1) & 0x400) {
                descriptor[5] = '1';
            } else {
                descriptor[5] = '0';
            }
        }
        for (i = 6; i < 16; i++) {
            if ((data_blocks - 1) & (0x200 >> (i - 6))) {
                descriptor[i] = '1';
            } else {
                descriptor[i] = '0';
            }
        }
        descriptor[16] = '\0';
        if (debug) printf("Mode Message = %s\n", descriptor);
    }

    /* Split into 4-bit codewords */
    for (i = 0; i < 4; i++) {
        if (descriptor[i * 4] == '1') {
            desc_data[i] += 8;
        }
        if (descriptor[(i * 4) + 1] == '1') {
            desc_data[i] += 4;
        }
        if (descriptor[(i * 4) + 2] == '1') {
            desc_data[i] += 2;
        }
        if (descriptor[(i * 4) + 3] == '1') {
            desc_data[i] += 1;
        }
    }

    /* Add reed-solomon error correction with Galois field GF(16) and prime modulus
    x^4 + x + 1 (section 7.2.3)*/

    rs_init_gf(0x13);
    if (compact) {
        rs_init_code(5, 1);
        rs_encode(2, desc_data, desc_ecc);
        for (i = 0; i < 5; i++) {
            if (desc_ecc[4 - i] & 0x08) {
                descriptor[(i * 4) + 8] = '1';
            } else {
                descriptor[(i * 4) + 8] = '0';
            }
            if (desc_ecc[4 - i] & 0x04) {
                descriptor[(i * 4) + 9] = '1';
            } else {
                descriptor[(i * 4) + 9] = '0';
            }
            if (desc_ecc[4 - i] & 0x02) {
                descriptor[(i * 4) + 10] = '1';
            } else {
                descriptor[(i * 4) + 10] = '0';
            }
            if (desc_ecc[4 - i] & 0x01) {
                descriptor[(i * 4) + 11] = '1';
            } else {
                descriptor[(i * 4) + 11] = '0';
            }
        }
    } else {
        rs_init_code(6, 1);
        rs_encode(4, desc_data, desc_ecc);
        for (i = 0; i < 6; i++) {
            if (desc_ecc[5 - i] & 0x08) {
                descriptor[(i * 4) + 16] = '1';
            } else {
                descriptor[(i * 4) + 16] = '0';
            }
            if (desc_ecc[5 - i] & 0x04) {
                descriptor[(i * 4) + 17] = '1';
            } else {
                descriptor[(i * 4) + 17] = '0';
            }
            if (desc_ecc[5 - i] & 0x02) {
                descriptor[(i * 4) + 18] = '1';
            } else {
                descriptor[(i * 4) + 18] = '0';
            }
            if (desc_ecc[5 - i] & 0x01) {
                descriptor[(i * 4) + 19] = '1';
            } else {
                descriptor[(i * 4) + 19] = '0';
            }
        }
    }
    rs_free();

    /* Merge descriptor with the rest of the symbol */
    for (i = 0; i < 40; i++) {
        if (compact) {
            bit_pattern[2000 + i - 2] = descriptor[i];
        } else {
            bit_pattern[20000 + i - 2] = descriptor[i];
        }
    }

    /* Plot all of the data into the symbol in pre-defined spiral pattern */
    if (compact) {

        for (y = AztecCompactOffset[layers - 1]; y < (27 - AztecCompactOffset[layers - 1]); y++) {
            for (x = AztecCompactOffset[layers - 1]; x < (27 - AztecCompactOffset[layers - 1]); x++) {
                if (CompactAztecMap[(y * 27) + x] == 1) {
                    set_module(symbol, y - AztecCompactOffset[layers - 1], x - AztecCompactOffset[layers - 1]);
                }
                if (CompactAztecMap[(y * 27) + x] >= 2) {
                    if (bit_pattern[CompactAztecMap[(y * 27) + x] - 2] == '1') {
                        set_module(symbol, y - AztecCompactOffset[layers - 1], x - AztecCompactOffset[layers - 1]);
                    }
                }
            }
            symbol->row_height[y - AztecCompactOffset[layers - 1]] = 1;
        }
        symbol->rows = 27 - (2 * AztecCompactOffset[layers - 1]);
        symbol->width = 27 - (2 * AztecCompactOffset[layers - 1]);
    } else {

        for (y = AztecOffset[layers - 1]; y < (151 - AztecOffset[layers - 1]); y++) {
            for (x = AztecOffset[layers - 1]; x < (151 - AztecOffset[layers - 1]); x++) {
                if (AztecMap[(y * 151) + x] == 1) {
                    set_module(symbol, y - AztecOffset[layers - 1], x - AztecOffset[layers - 1]);
                }
                if (AztecMap[(y * 151) + x] >= 2) {
                    if (bit_pattern[AztecMap[(y * 151) + x] - 2] == '1') {
                        set_module(symbol, y - AztecOffset[layers - 1], x - AztecOffset[layers - 1]);
                    }
                }
            }
            symbol->row_height[y - AztecOffset[layers - 1]] = 1;
        }
        symbol->rows = 151 - (2 * AztecOffset[layers - 1]);
        symbol->width = 151 - (2 * AztecOffset[layers - 1]);
    }

    return err_code;
}

/* Encodes Aztec runes as specified in ISO/IEC 24778:2008 Annex A */
int aztec_runes(struct zint_symbol *symbol, unsigned char source[], int length) {
    int input_value, error_number, i, y, x;
    char binary_string[28];
    unsigned char data_codewords[3], ecc_codewords[6];

    error_number = 0;
    input_value = 0;
    if (length > 3) {
        strcpy(symbol->errtxt, "507: Input too large");
        return ZINT_ERROR_INVALID_DATA;
    }
    error_number = is_sane(NEON, source, length);
    if (error_number != 0) {
        strcpy(symbol->errtxt, "508: Invalid characters in input");
        return ZINT_ERROR_INVALID_DATA;
    }
    switch (length) {
        case 3: input_value = 100 * ctoi(source[0]);
            input_value += 10 * ctoi(source[1]);
            input_value += ctoi(source[2]);
            break;
        case 2: input_value = 10 * ctoi(source[0]);
            input_value += ctoi(source[1]);
            break;
        case 1: input_value = ctoi(source[0]);
            break;
    }

    if (input_value > 255) {
        strcpy(symbol->errtxt, "509: Input too large");
        return ZINT_ERROR_INVALID_DATA;
    }

    strcpy(binary_string, "");
    bin_append(input_value, 8, binary_string);

    data_codewords[0] = 0;
    data_codewords[1] = 0;

    for (i = 0; i < 2; i++) {
        if (binary_string[i * 4] == '1') {
            data_codewords[i] += 8;
        }
        if (binary_string[(i * 4) + 1] == '1') {
            data_codewords[i] += 4;
        }
        if (binary_string[(i * 4) + 2] == '1') {
            data_codewords[i] += 2;
        }
        if (binary_string[(i * 4) + 3] == '1') {
            data_codewords[i] += 1;
        }
    }

    rs_init_gf(0x13);
    rs_init_code(5, 1);
    rs_encode(2, data_codewords, ecc_codewords);
    rs_free();

    strcpy(binary_string, "");

    for (i = 0; i < 5; i++) {
        if (ecc_codewords[4 - i] & 0x08) {
            binary_string[(i * 4) + 8] = '1';
        } else {
            binary_string[(i * 4) + 8] = '0';
        }
        if (ecc_codewords[4 - i] & 0x04) {
            binary_string[(i * 4) + 9] = '1';
        } else {
            binary_string[(i * 4) + 9] = '0';
        }
        if (ecc_codewords[4 - i] & 0x02) {
            binary_string[(i * 4) + 10] = '1';
        } else {
            binary_string[(i * 4) + 10] = '0';
        }
        if (ecc_codewords[4 - i] & 0x01) {
            binary_string[(i * 4) + 11] = '1';
        } else {
            binary_string[(i * 4) + 11] = '0';
        }
    }

    for (i = 0; i < 28; i += 2) {
        if (binary_string[i] == '1') {
            binary_string[i] = '0';
        } else {
            binary_string[i] = '1';
        }
    }

    for (y = 8; y < 19; y++) {
        for (x = 8; x < 19; x++) {
            if (CompactAztecMap[(y * 27) + x] == 1) {
                set_module(symbol, y - 8, x - 8);
            }
            if (CompactAztecMap[(y * 27) + x] >= 2) {
                if (binary_string[CompactAztecMap[(y * 27) + x] - 2000] == '1') {
                    set_module(symbol, y - 8, x - 8);
                }
            }
        }
        symbol->row_height[y - 8] = 1;
    }
    symbol->rows = 11;
    symbol->width = 11;

    return 0;
}

