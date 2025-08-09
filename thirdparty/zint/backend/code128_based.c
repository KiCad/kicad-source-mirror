/* code128_based.c - Handles Code 128 derivatives NVE-18, EAN-14, DPD and Universal Postal Union S10 */
/*
    libzint - the open source barcode library
    Copyright (C) 2008-2025 Robin Stuart <rstuart114@gmail.com>
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
/* SPDX-License-Identifier: BSD-3-Clause */

#include <assert.h>
#include <stdio.h>
#include "common.h"
#include "code128.h"
#include "gs1.h"

/* Was in "code128.c" */

INTERNAL int gs1_128(struct zint_symbol *symbol, unsigned char source[], int length);

/* Helper to do NVE18 or EAN14 */
static int nve18_or_ean14(struct zint_symbol *symbol, const unsigned char source[], int length, const int data_len) {
    static const char prefix[2][2][5] = {
        { "(01)", "[01]" }, /* EAN14 */
        { "(00)", "[00]" }, /* NVE18 */
    };
    const int idx = data_len == 17;
    unsigned char ean128_equiv[23];
    int i, zeroes;
    unsigned char have_check_digit = '\0';
    unsigned char check_digit;
    int error_number;

    /* Allow and ignore any AI prefix */
    if ((length == data_len + 4 || length == data_len + 1 + 4)
            && (memcmp(source, prefix[idx][0], 4) == 0 || memcmp(source, prefix[idx][1], 4) == 0)) {
        source += 4;
        length -= 4;
    /* Likewise initial '01' (EAN-14) or '00' (NVE-18) */
    } else if ((length == data_len + 2 || length == data_len + 1 + 2)
            && source[0] == prefix[idx][0][1] && source[1] == prefix[idx][0][2]) {
        source += 2;
        length -= 2;
    }
    if (length > data_len + 1) {
        return ZEXT errtxtf(ZINT_ERROR_TOO_LONG, symbol, 345, "Input length %1$d too long (maximum %2$d)",
                            length, data_len + 1);
    }
    if ((i = not_sane(NEON_F, source, length))) {
        /* Note: for all "at position" error messages, escape sequences not accounted for */
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 346,
                        "Invalid character at position %d in input (digits only)", i);
    }
    if (length == data_len + 1) {
        have_check_digit = source[data_len];
        length--;
    }

    zeroes = data_len - length;
    memcpy(ean128_equiv, prefix[idx][!(symbol->input_mode & GS1PARENS_MODE)], 4);
    memset(ean128_equiv + 4, '0', zeroes);
    memcpy(ean128_equiv + 4 + zeroes, source, length);

    check_digit = (unsigned char) gs1_check_digit(ean128_equiv + 4, data_len);
    if (have_check_digit && have_check_digit != check_digit) {
        return ZEXT errtxtf(ZINT_ERROR_INVALID_CHECK, symbol, 347, "Invalid check digit '%1$c', expecting '%2$c'",
                            have_check_digit, check_digit);
    }
    ean128_equiv[data_len + 4] = check_digit;
    ean128_equiv[data_len + 5] = '\0'; /* Terminating NUL required by `c128_cost()` */

    error_number = gs1_128(symbol, ean128_equiv, data_len + 5);

    /* Use `raw_text` set by `gs1_128()` */

    return error_number;
}


/* Add check digit if encoding an NVE18 symbol */
INTERNAL int nve18(struct zint_symbol *symbol, unsigned char source[], int length) {
    return nve18_or_ean14(symbol, source, length, 17 /*data_len*/);
}

/* EAN-14 - A version of EAN-128 */
INTERNAL int ean14(struct zint_symbol *symbol, unsigned char source[], int length) {
    return nve18_or_ean14(symbol, source, length, 13 /*data_len*/);
}

static const char KRSET[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define KRSET_F (IS_NUM_F | IS_UPR_F)

/* DPD (Deutscher Paketdienst) Code */
/* Specification at https://esolutions.dpd.com/dokumente/DPD_Parcel_Label_Specification_2.4.1_EN.pdf
 * and identification tag info (Barcode ID) at https://esolutions.dpd.com/dokumente/DPD_Routing_Database_1.3_EN.pdf */
INTERNAL int dpd(struct zint_symbol *symbol, unsigned char source[], int length) {
    int i, p;
    unsigned char ident_tag;
    unsigned char local_source_buf[29];
    unsigned char *local_source;
    unsigned char hrt[37];
    int cd; /* Check digit */
    int error_number;
    const int mod = 36;
    const int relabel = symbol->option_2 == 1; /* A "relabel" has no identification tag */

    if ((length != 27 && length != 28) || (length == 28 && relabel)) {
        if (relabel) {
            return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 830,
                            "DPD relabel input length %d wrong (27 characters required)", length);
        }
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 349, "DPD input length %d wrong (27 or 28 characters required)",
                        length);
    }

    if (length == 27 && !relabel) {
        local_source_buf[0] = '%';
        memcpy(local_source_buf + 1, source, ++length); /* Include terminating NUL (required by `c128_cost()`) */
        local_source = local_source_buf;
    } else {
        local_source = source;
    }

    ident_tag = local_source[0];

    to_upper(local_source + !relabel, length - !relabel);
    if ((i = not_sane(KRSET_F, local_source + !relabel, length - !relabel))) {
        if (local_source == local_source_buf || relabel) {
            return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 300,
                        "Invalid character at position %d in input (alphanumerics only)", i);
        }
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 299,
                        "Invalid character at position %d in input (alphanumerics only after first)", i);
    }

    if (z_iscntrl(ident_tag) || !z_isascii(ident_tag)) {
        return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 343,
                        "Invalid DPD identification tag (first character), ASCII values 32 to 126 only");
    }

    if ((error_number = code128(symbol, local_source, length))) {
        assert(error_number == ZINT_ERROR_MEMORY); /* Too long can't happen */
        return error_number;
    }

    if (!(symbol->output_options & (BARCODE_BOX | BARCODE_BIND | BARCODE_BIND_TOP))) {
        /* If no option has been selected then uses default bind top option */
        symbol->output_options |= BARCODE_BIND_TOP; /* Note won't extend over quiet zones for DPD */
        if (symbol->border_width == 0) { /* Allow override if non-zero */
            symbol->border_width = 3; /* From examples, not mentioned in spec */
        }
    }

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* DPD Parcel Label Specification Version 2.4.1 (19.01.2021) Section 4.6.1.2
           25mm / 0.4mm (X max) = 62.5 min, 25mm / 0.375 (X) ~ 66.66 default */
        if (relabel) { /* If relabel then half-size */
            const float default_height = 33.3333321f; /* 12.5 / 0.375 */
            error_number = set_height(symbol, 31.25f, default_height, 0.0f, 0 /*no_errtxt*/);
        } else {
            const float default_height = 66.6666641f; /* 25.0 / 0.375 */
            error_number = set_height(symbol, 62.5f, default_height, 0.0f, 0 /*no_errtxt*/);
        }
    } else {
        (void) set_height(symbol, 0.0f, relabel ? 25.0f : 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    cd = mod;

    for (i = !relabel, p = 0; i < length; i++) {
        hrt[p++] = local_source[i];

        cd += posn(KRSET, local_source[i]);
        if (cd > mod) cd -= mod;
        cd *= 2;
        if (cd >= (mod + 1)) cd -= mod + 1;

        switch (i + relabel) {
            case 4:
            case 7:
            case 11:
            case 15:
            case 19:
            case 21:
            case 24:
            case 27:
                hrt[p++] = ' ';
                break;
        }
    }

    cd = mod + 1 - cd;
    if (cd == mod) cd = 0;

    hrt[p] = xtoc(cd);

    hrt_cpy_nochk(symbol, hrt, p + 1);

    /* Use `raw_text` from `code128()` */

    /* Some compliance checks */
    if (not_sane(NEON_F, local_source + length - 16, 16)) {
        if (not_sane(NEON_F, local_source + length - 3, 3)) { /* 3-digit Country Code (ISO 3166-1) */
            errtxt(0, symbol, 831, "Destination Country Code (last 3 characters) should be numeric");
        } else if (not_sane(NEON_F, local_source + length - 6, 3)) { /* 3-digit Service Code */
            errtxt(0, symbol, 832, "Service Code (characters 6-4 from end) should be numeric");
        } else { /* Last 10 characters of Tracking No. */
            errtxt(0, symbol, 833,
                    "Last 10 characters of Tracking Number (characters 16-7 from end) should be numeric");
        }
        error_number = ZINT_WARN_NONCOMPLIANT;
    }

    return error_number;
}

/* Universal Postal Union S10 */
/* https://www.upu.int/UPU/media/upu/files/postalSolutions/programmesAndServices/standards/S10-12.pdf */
INTERNAL int upu_s10(struct zint_symbol *symbol, unsigned char source[], int length) {
    static const char weights[8] = { 8, 6, 4, 2, 3, 5, 9, 7 };
    unsigned char local_source[13 + 1];
    unsigned char have_check_digit = '\0';
    unsigned char hrt[18];
    int i, j;
    int check_digit;
    int error_number;

    if (length != 12 && length != 13) {
        return errtxtf(ZINT_ERROR_TOO_LONG, symbol, 834, "Input length %d wrong (12 or 13 characters required)",
                        length);
    }
    if (length == 13) { /* Includes check digit - remove for now */
        have_check_digit = source[10];
        memcpy(local_source, source, 10);
        memcpy(local_source + 10, source + 11, length - 11);
        length--;
    } else {
        memcpy(local_source, source, length);
    }
    to_upper(local_source, length);

    if (!z_isupper(local_source[0]) || !z_isupper(local_source[1])) {
        return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 835,
                        "Invalid character in Service Indictor (first 2 characters) (alphabetic only)");
    }
    if (not_sane(NEON_F, local_source + 2, 12 - 4) || (have_check_digit && !z_isdigit(have_check_digit))) {
        return errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 836,
                        "Invalid character in Serial Number (middle %d characters) (digits only)",
                        have_check_digit ? 9 : 8);
    }
    if (!z_isupper(local_source[10]) || !z_isupper(local_source[11])) {
        return errtxt(ZINT_ERROR_INVALID_DATA, symbol, 837,
                        "Invalid character in Country Code (last 2 characters) (alphabetic only)");
    }

    check_digit = 0;
    for (i = 2; i < 10; i++) { /* Serial Number only */
        check_digit += ctoi(local_source[i]) * weights[i - 2];
    }
    check_digit %= 11;
    check_digit = 11 - check_digit;
    if (check_digit == 10) {
        check_digit = 0;
    } else if (check_digit == 11) {
        check_digit = 5;
    }
    if (have_check_digit && ctoi(have_check_digit) != check_digit) {
        return ZEXT errtxtf(ZINT_ERROR_INVALID_CHECK, symbol, 838, "Invalid check digit '%1$c', expecting '%2$c'",
                            have_check_digit, itoc(check_digit));
    }
    /* Add in (back) check digit */
    local_source[12] = local_source[11];
    local_source[11] = local_source[10];
    local_source[10] = itoc(check_digit);
    local_source[13] = '\0'; /* Terminating NUL required by `c128_cost()` */

    if ((error_number = code128(symbol, local_source, 13))) {
        assert(error_number == ZINT_ERROR_MEMORY); /* Too long can't happen */
        return error_number;
    }

    /* Do some checks on the Service Indicator (first char only) and Country Code */
    if (strchr("JKSTW", local_source[0]) != NULL) { /* These are reserved & cannot be assigned */
        error_number = errtxtf(ZINT_WARN_NONCOMPLIANT, symbol, 839,
                                "Invalid Service Indicator '%.2s' (first character should not be any of \"JKSTW\")",
                                local_source);
    } else if (strchr("FIOXY", local_source[0]) != NULL) { /* These aren't allocated as of spec Oct 2017 */
        error_number = errtxtf(ZINT_WARN_NONCOMPLIANT, symbol, 840,
                                "Non-standard Service Indicator '%.2s' (first 2 characters)", local_source);
    } else if (!gs1_iso3166_alpha2(local_source + 11)) {
        error_number = errtxtf(ZINT_WARN_NONCOMPLIANT, symbol, 841,
                                "Country Code '%.2s' (last two characters) is not ISO 3166-1", local_source + 11);
    }

    for (i = 0, j = 0; i < 13; i++) {
        if (i == 2 || i == 5 || i == 8 || i == 11) {
            hrt[j++] = ' ';
        }
        hrt[j++] = local_source[i];
    }
    hrt_cpy_nochk(symbol, hrt, j);

    /* Use `raw_text` set by `code128()` */

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Universal Postal Union S10 Section 8, using max X 0.51mm & minimum height 12.5mm or 15% of width */
        const float min_height_min = 24.5098038f; /* 12.5 / 0.51 */
        float min_height = stripf(symbol->width * 0.15f);
        if (min_height < min_height_min) {
            min_height = min_height_min;
        }
        /* Using 50 as default as none recommended */
        if (error_number == 0) {
            error_number = set_height(symbol, min_height, min_height > 50.0f ? min_height : 50.0f, 0.0f,
                                        0 /*no_errtxt*/);
        } else {
            (void) set_height(symbol, min_height, min_height > 50.0f ? min_height : 50.0f, 0.0f, 1 /*no_errtxt*/);
        }
    } else {
        (void) set_height(symbol, 0.0f, 50.0f, 0.0f, 1 /*no_errtxt*/);
    }

    return error_number;
}

/* vim: set ts=4 sw=4 et : */
