/*  eci.c - Extended Channel Interpretations

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
#include <stdio.h>
#include <stdlib.h>
#include "eci.h"
#include "zint.h"
#ifdef _MSC_VER
#include <malloc.h>
#endif

/* Convert Unicode to other character encodings */
int utf_to_eci(const int eci, const unsigned char source[], unsigned char dest[], size_t *length) {
    int in_posn;
    int out_posn;
    int ext;
    int done;
    
    if (eci == 26) {
        int in_length = (int) *length;
        /* Unicode mode, do not process - just copy data across */
        for (in_posn = 0; in_posn < in_length; in_posn++) {
            dest[in_posn] = source[in_posn];
        }
        return 0;
    }

    in_posn = 0;
    out_posn = 0;
    do {
        /* Single byte (ASCII) character */
        int bytelen = 1;
        int glyph = (int) source[in_posn];

        if ((source[in_posn] >= 0x80) && (source[in_posn] < 0xc0)) {
            /* Something has gone wrong, abort */
            return ZINT_ERROR_INVALID_DATA;
        }

        if ((source[in_posn] >= 0xc0) && (source[in_posn] < 0xe0)) {
            /* Two-byte character */
            bytelen = 2;
            glyph = (source[in_posn] & 0x1f) << 6;

            if (*length < (in_posn + 2)) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (source[in_posn + 1] > 0xc0) {
                return ZINT_ERROR_INVALID_DATA;
            }

            glyph += (source[in_posn + 1] & 0x3f);
        }

        if ((source[in_posn] >= 0xe0) && (source[in_posn] < 0xf0)) {
            /* Three-byte character */
            bytelen = 3;
            glyph = (source[in_posn] & 0x0f) << 12;

            if (*length < (in_posn + 2)) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (*length < (in_posn + 3)) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (source[in_posn + 1] > 0xc0) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (source[in_posn + 2] > 0xc0) {
                return ZINT_ERROR_INVALID_DATA;
            }

            glyph += (source[in_posn + 1] & 0x3f) << 6;
            glyph += (source[in_posn + 2] & 0x3f);
        }

        if ((source[in_posn] >= 0xf0) && (source[in_posn] < 0xf7)) {
            /* Four-byte character */
            bytelen = 4;
            glyph = (source[in_posn] & 0x07) << 18;

            if (*length < (in_posn + 2)) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (*length < (in_posn + 3)) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (*length < (in_posn + 4)) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (source[in_posn + 1] > 0xc0) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (source[in_posn + 2] > 0xc0) {
                return ZINT_ERROR_INVALID_DATA;
            }

            if (source[in_posn + 3] > 0xc0) {
                return ZINT_ERROR_INVALID_DATA;
            }

            glyph += (source[in_posn + 1] & 0x3f) << 12;
            glyph += (source[in_posn + 2] & 0x3f) << 6;
            glyph += (source[in_posn + 3] & 0x3f);
        }

        if (source[in_posn] >= 0xf7) {
            /* More than 4 bytes not supported */
            return ZINT_ERROR_INVALID_DATA;
        }

        if (glyph < 128) {
            dest[out_posn] = glyph;
        } else {
            done = 0;
            for (ext = 0; ext < 128; ext++) {
                switch (eci) {
                    case 3: // Latin-1
                        if (glyph == iso_8859_1[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 4: // Latin-2
                        if (glyph == iso_8859_2[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 5: // Latin-3
                        if (glyph == iso_8859_3[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 6: // Latin-4
                        if (glyph == iso_8859_4[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 7: // Latin/Cyrillic
                        if (glyph == iso_8859_5[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 8: // Latin/Arabic
                        if (glyph == iso_8859_6[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 9: // Latin/Greek
                        if (glyph == iso_8859_7[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 10: // Latin/Hebrew
                        if (glyph == iso_8859_8[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 11: // Latin-5
                        if (glyph == iso_8859_9[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 12: // Latin-6
                        if (glyph == iso_8859_10[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 13: // Latin/Thai
                        if (glyph == iso_8859_11[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 15: // Latin-7
                        if (glyph == iso_8859_13[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 16: // Latin-8
                        if (glyph == iso_8859_14[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 17: // Latin-9
                        if (glyph == iso_8859_15[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 18: // Latin-10
                        if (glyph == iso_8859_16[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 21: // Windows-1250
                        if (glyph == windows_1250[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 22: // Windows-1251
                        if (glyph == windows_1251[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 23: // Windows-1252
                        if (glyph == windows_1252[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    case 24: // Windows-1256
                        if (glyph == windows_1256[ext]) {
                            dest[out_posn] = ext + 128;
                            done = 1;
                        }
                        break;
                    default:
                        break;
                }
            }

            if (!(done)) {
                return ZINT_ERROR_INVALID_DATA;
            }
        }

        in_posn += bytelen;
        out_posn++;
    } while (in_posn < *length);
    dest[out_posn] = '\0';
    *length = out_posn;

    return 0;
}

/* Find the lowest ECI mode which will encode a given set of Unicode text */
int get_best_eci(unsigned char source[], size_t length) {
    int eci = 3;

#ifndef _MSC_VER
    unsigned char local_source[length + 1];
#else
    unsigned char *local_source = (unsigned char*) _alloca(length + 1);
#endif

    do {
        if (utf_to_eci(eci, source, local_source, &length) == 0) {
            return eci;
        }
        eci++;
    } while (eci < 25);

    return 26; // If all of these fail, use Unicode!
}


