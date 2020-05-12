/*  library.c - external functions of libzint

    libzint - the open source barcode library
    Copyright (C) 2009-2018 Robin Stuart <rstuart114@gmail.com>

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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "common.h"
#include "gs1.h"

#define TECHNETIUM	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%"

struct zint_symbol *ZBarcode_Create() {
    struct zint_symbol *symbol;

    symbol = (struct zint_symbol*) malloc(sizeof (*symbol));
    if (!symbol) return NULL;

    memset(symbol, 0, sizeof (*symbol));
    symbol->symbology = BARCODE_CODE128;
    symbol->height = 50;
    symbol->whitespace_width = 0;
    symbol->border_width = 0;
    symbol->output_options = 0;
    symbol->rows = 0;
    symbol->width = 0;
    strcpy(symbol->fgcolour, "000000");
    strcpy(symbol->bgcolour, "ffffff");
    strcpy(symbol->outfile, "out.png");
    symbol->scale = 1.0;
    symbol->option_1 = -1;
    symbol->option_2 = 0;
    symbol->option_3 = 928; // PDF_MAX
    symbol->show_hrt = 1; // Show human readable text
    symbol->fontsize = 8;
    symbol->input_mode = DATA_MODE;
    strcpy(symbol->primary, "");
    memset(&(symbol->encoded_data[0][0]), 0, sizeof (symbol->encoded_data));
    memset(&(symbol->row_height[0]), 0, sizeof (symbol->row_height));
    symbol->bitmap = NULL;
    symbol->bitmap_width = 0;
    symbol->bitmap_height = 0;
    symbol->eci = 3;
    symbol->dot_size = 4.0 / 5.0;
    symbol->debug = 0;
    return symbol;
}

extern void render_free(struct zint_symbol *symbol); /* Free render structures */
extern void vector_free(struct zint_symbol *symbol); /* Free vector structures */

void ZBarcode_Clear(struct zint_symbol *symbol) {
    int i, j;

    for (i = 0; i < symbol->rows; i++) {
        for (j = 0; j < symbol->width; j++) {
            unset_module(symbol, i, j);
        }
    }
    symbol->rows = 0;
    symbol->width = 0;
    memset(symbol->text, 0, sizeof(symbol->text));
    symbol->errtxt[0] = '\0';
    if (symbol->bitmap != NULL) {
        free(symbol->bitmap);
        symbol->bitmap = NULL;
    }
    symbol->bitmap_width = 0;
    symbol->bitmap_height = 0;

    // If there is a rendered version, ensure its memory is released
    render_free(symbol);
    vector_free(symbol);
}

void ZBarcode_Delete(struct zint_symbol *symbol) {
    if (symbol->bitmap != NULL)
        free(symbol->bitmap);

    // If there is a rendered version, ensure its memory is released
    render_free(symbol);
    vector_free(symbol);

    free(symbol);
}

extern int get_best_eci(unsigned char source[], size_t length); /* Calculate suitable ECI mode */
extern int utf_to_eci(const int eci, const unsigned char source[], unsigned char dest[], size_t *length); /* Convert Unicode to other encodings */


extern int eanx(struct zint_symbol *symbol, unsigned char source[], int length); /* EAN system barcodes */
extern int c39(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* Code 3 from 9 (or Code 39) */
extern int pharmazentral(struct zint_symbol *symbol, unsigned char source[], int length); /* Pharmazentral Nummer (PZN) */
extern int ec39(struct zint_symbol *symbol, unsigned char source[], int length); /* Extended Code 3 from 9 (or Code 39+) */
extern int codabar(struct zint_symbol *symbol, unsigned char source[], int length); /* Codabar - a simple substitution cipher */
extern int matrix_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length); /* Code 2 of 5 Standard (& Matrix) */
extern int industrial_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length); /* Code 2 of 5 Industrial */
extern int iata_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length); /* Code 2 of 5 IATA */
extern int interleaved_two_of_five(struct zint_symbol *symbol, const unsigned char source[], size_t length); /* Code 2 of 5 Interleaved */
extern int logic_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length); /* Code 2 of 5 Data Logic */
extern int itf14(struct zint_symbol *symbol, unsigned char source[], int length); /* ITF-14 */
extern int dpleit(struct zint_symbol *symbol, unsigned char source[], int length); /* Deutsche Post Leitcode */
extern int dpident(struct zint_symbol *symbol, unsigned char source[], int length); /* Deutsche Post Identcode */
extern int c93(struct zint_symbol *symbol, unsigned char source[], int length); /* Code 93 - a re-working of Code 39+, generates 2 check digits */
extern int code_128(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* Code 128 and NVE-18 */
extern int ean_128(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* EAN-128 (GS1-128) */
extern int code_11(struct zint_symbol *symbol, unsigned char source[], int length); /* Code 11 */
extern int msi_handle(struct zint_symbol *symbol, unsigned char source[], int length); /* MSI Plessey */
extern int telepen(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* Telepen ASCII */
extern int telepen_num(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* Telepen Numeric */
extern int plessey(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* Plessey Code */
extern int pharma_one(struct zint_symbol *symbol, unsigned char source[], int length); /* Pharmacode One Track */
extern int flattermarken(struct zint_symbol *symbol, unsigned char source[], int length); /* Flattermarken */
extern int fim(struct zint_symbol *symbol, unsigned char source[], int length); /* Facing Identification Mark */
extern int pharma_two(struct zint_symbol *symbol, unsigned char source[], int length); /* Pharmacode Two Track */
extern int post_plot(struct zint_symbol *symbol, unsigned char source[], int length); /* Postnet */
extern int planet_plot(struct zint_symbol *symbol, unsigned char source[], int length); /* PLANET */
extern int imail(struct zint_symbol *symbol, unsigned char source[], int length); /* Intelligent Mail (aka USPS OneCode) */
extern int royal_plot(struct zint_symbol *symbol, unsigned char source[], int length); /* RM4SCC */
extern int australia_post(struct zint_symbol *symbol, unsigned char source[], int length); /* Australia Post 4-state */
extern int code16k(struct zint_symbol *symbol, unsigned char source[],const size_t length); /* Code 16k */
extern int pdf417enc(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* PDF417 */
extern int micro_pdf417(struct zint_symbol *symbol, unsigned char chaine[], const size_t length); /* Micro PDF417 */
extern int maxicode(struct zint_symbol *symbol, unsigned char source[], int length); /* Maxicode */
extern int rss14(struct zint_symbol *symbol, unsigned char source[], int length); /* RSS-14 */
extern int rsslimited(struct zint_symbol *symbol, unsigned char source[], int length); /* RSS Limited */
extern int rssexpanded(struct zint_symbol *symbol, unsigned char source[], int length); /* RSS Expanded */
extern int composite(struct zint_symbol *symbol, unsigned char source[], int length); /* Composite Symbology */
extern int kix_code(struct zint_symbol *symbol, unsigned char source[], int length); /* TNT KIX Code */
extern int aztec(struct zint_symbol *symbol, unsigned char source[], const size_t length); /* Aztec Code */
extern int code32(struct zint_symbol *symbol, unsigned char source[], int length); /* Italian Pharmacode */
extern int daft_code(struct zint_symbol *symbol, unsigned char source[], int length); /* DAFT Code */
extern int ean_14(struct zint_symbol *symbol, unsigned char source[], int length); /* EAN-14 */
extern int nve_18(struct zint_symbol *symbol, unsigned char source[], int length); /* NVE-18 */
extern int microqr(struct zint_symbol *symbol, const unsigned char source[], size_t length); /* Micro QR Code */
extern int aztec_runes(struct zint_symbol *symbol, unsigned char source[], int length); /* Aztec Runes */
extern int korea_post(struct zint_symbol *symbol, unsigned char source[], int length); /* Korea Post */
extern int japan_post(struct zint_symbol *symbol, unsigned char source[], int length); /* Japanese Post */
extern int code_49(struct zint_symbol *symbol, unsigned char source[], const int length); /* Code 49 */
extern int channel_code(struct zint_symbol *symbol, unsigned char source[], int length); /* Channel Code */
extern int code_one(struct zint_symbol *symbol, unsigned char source[], int length); /* Code One */
extern int grid_matrix(struct zint_symbol *symbol, const unsigned char source[], size_t length); /* Grid Matrix */
extern int han_xin(struct zint_symbol * symbol, const unsigned char source[], size_t length); /* Han Xin */
extern int dotcode(struct zint_symbol * symbol, const unsigned char source[], int length); /* DotCode */
extern int codablock(struct zint_symbol * symbol, const unsigned char source[], const size_t length); /* Codablock */
extern int upnqr(struct zint_symbol *symbol, const unsigned char source[], size_t length); /* UPNQR */
extern int qr_code(struct zint_symbol *symbol, const unsigned char source[], size_t length); /* QR Code */
extern int dmatrix(struct zint_symbol *symbol, const unsigned char source[], const size_t in_length); /* Data Matrix (IEC16022) */
extern int vin(struct zint_symbol *symbol, const unsigned char source[], const size_t in_length); /* VIN Code (Vehicle Identification Number) */
extern int mailmark(struct zint_symbol *symbol, const unsigned char source[], const size_t in_length); /* Royal Mail 4-state Mailmark */

extern int plot_raster(struct zint_symbol *symbol, int rotate_angle, int file_type); /* Plot to PNG/BMP/PCX */
extern int plot_vector(struct zint_symbol *symbol, int rotate_angle, int file_type); /* Plot to EPS/EMF/SVG */

extern int render_plot(struct zint_symbol *symbol, float width, float height); /* Plot to gLabels */

//extern int ps_plot(struct zint_symbol *symbol); /* Plot to EPS */
//extern int svg_plot(struct zint_symbol *symbol); /* Plot to SVG */
//extern int emf_plot(struct zint_symbol *symbol); /* Plot to Metafile */

void error_tag(char error_string[], int error_number) {

    if (error_number != 0) {
        char error_buffer[100];
        strcpy(error_buffer, error_string);

        if (error_number > 4) {
            strcpy(error_string, "Error ");
        } else {
            strcpy(error_string, "Warning ");
        }

        strcat(error_string, error_buffer);
    }
}

/* Output a hexadecimal representation of the rendered symbol */
int dump_plot(struct zint_symbol *symbol) {
    FILE *f;
    int i, r;
    char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
        '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    int space = 0;

    if (symbol->output_options & BARCODE_STDOUT) {
        f = stdout;
    } else {
        f = fopen(symbol->outfile, "w");
        if (!f) {
            strcpy(symbol->errtxt, "201: Could not open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
    }

    for (r = 0; r < symbol->rows; r++) {
        int byt = 0;
        for (i = 0; i < symbol->width; i++) {
            byt = byt << 1;
            if (module_is_set(symbol, r, i)) {
                byt += 1;
            }
            if (((i + 1) % 4) == 0) {
                fputc(hex[byt], f);
                space++;
                byt = 0;
            }
            if (space == 2) {
                fputc(' ', f);
                space = 0;
            }
        }

        if ((symbol->width % 4) != 0) {
            byt = byt << (4 - (symbol->width % 4));
            fputc(hex[byt], f);
        }
        fputs("\n", f);
        space = 0;
    }

    if (symbol->output_options & BARCODE_STDOUT) {
        fflush(f);
    } else {
        fclose(f);
    }

    return 0;
}

/* Process health industry bar code data */
static int hibc(struct zint_symbol *symbol, unsigned char source[], size_t length) {
    size_t i;
    int    counter, error_number;
    char to_process[113], temp[2], check_digit;

    /* without "+" and check: max 110 characters in HIBC 2.6 */
    if (length > 110) {
        strcpy(symbol->errtxt, "202: Data too long for HIBC LIC");
        return ZINT_ERROR_TOO_LONG;
    }
    to_upper(source);
    error_number = is_sane(TECHNETIUM, source, length);
    if (error_number == ZINT_ERROR_INVALID_DATA) {
        strcpy(symbol->errtxt, "203: Invalid characters in data");
        return error_number;
    }

    strcpy(to_process, "+");
    counter = 41;
    for (i = 0; i < length; i++) {
        counter += posn(TECHNETIUM, source[i]);
    }
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
                case 42: check_digit = '%';
                    break;
                default: check_digit = ' ';
                    break; /* Keep compiler happy */
            }
        }
    }

    temp[0] = check_digit;
    temp[1] = '\0';

    strcat(to_process, (char *) source);
    strcat(to_process, temp);
    length = strlen(to_process);

    switch (symbol->symbology) {
        case BARCODE_HIBC_128:
            error_number = code_128(symbol, (unsigned char *) to_process, length);
            ustrcpy(symbol->text, (unsigned char*) "*");
            strcat((char*) symbol->text, to_process);
            strcat((char*) symbol->text, "*");
            break;
        case BARCODE_HIBC_39:
            symbol->option_2 = 0;
            error_number = c39(symbol, (unsigned char *) to_process, length);
            ustrcpy(symbol->text, (unsigned char*) "*");
            strcat((char*) symbol->text, to_process);
            strcat((char*) symbol->text, "*");
            break;
        case BARCODE_HIBC_DM:
            error_number = dmatrix(symbol, (unsigned char *) to_process, length);
            break;
        case BARCODE_HIBC_QR:
            error_number = qr_code(symbol, (unsigned char *) to_process, length);
            break;
        case BARCODE_HIBC_PDF:
            error_number = pdf417enc(symbol, (unsigned char *) to_process, length);
            break;
        case BARCODE_HIBC_MICPDF:
            error_number = micro_pdf417(symbol, (unsigned char *) to_process, length);
            break;
        case BARCODE_HIBC_AZTEC:
            error_number = aztec(symbol, (unsigned char *) to_process, length);
            break;
        case BARCODE_HIBC_BLOCKF:
            error_number = codablock(symbol, (unsigned char *) to_process, length);
            break;
    }

    return error_number;
}

static void check_row_heights(struct zint_symbol *symbol) {
    /* Check that rows with undefined heights are never less than 5x  */
    int large_bar_count = 0;
    int i;
    int preset_height = 0;
    int large_bar_height = 0;

    for (i = 0; i < symbol->rows; i++) {
        preset_height += symbol->row_height[i];
        if (symbol->row_height[i] == 0) {
            large_bar_count++;
        }
    }

    if (large_bar_count == 0) {
        symbol->height = preset_height;
    } else {
        large_bar_height = (symbol->height - preset_height) / large_bar_count;
    }

    if (large_bar_height < 5) {
        for (i = 0; i < symbol->rows; i++) {
            if (symbol->row_height[i] == 0) {
                symbol->row_height[i] = 5;
                preset_height += 5;
            }
        }
        symbol->height = preset_height;
    }
}

static int gs1_compliant(const int symbology) {
    /* Returns 1 if symbology supports GS1 data */

    int result = 0;

    switch (symbology) {
        case BARCODE_EAN128:
        case BARCODE_RSS_EXP:
        case BARCODE_RSS_EXPSTACK:
        case BARCODE_EANX_CC:
        case BARCODE_EAN128_CC:
        case BARCODE_RSS14_CC:
        case BARCODE_RSS_LTD_CC:
        case BARCODE_RSS_EXP_CC:
        case BARCODE_UPCA_CC:
        case BARCODE_UPCE_CC:
        case BARCODE_RSS14STACK_CC:
        case BARCODE_RSS14_OMNI_CC:
        case BARCODE_RSS_EXPSTACK_CC:
        case BARCODE_CODE16K:
        case BARCODE_AZTEC:
        case BARCODE_DATAMATRIX:
        case BARCODE_CODEONE:
        case BARCODE_CODE49:
        case BARCODE_QRCODE:
        case BARCODE_DOTCODE:
            result = 1;
            break;
    }

    return result;
}

static int is_matrix(const int symbology) {
    /* Returns 1 if symbology is a matrix design */

    int result = 0;

    switch (symbology) {
        case BARCODE_QRCODE:
        case BARCODE_DATAMATRIX:
        case BARCODE_MICROQR:
        case BARCODE_HIBC_DM:
        case BARCODE_AZTEC:
        case BARCODE_HIBC_QR:
        case BARCODE_HIBC_AZTEC:
        case BARCODE_AZRUNE:
        case BARCODE_CODEONE:
        case BARCODE_GRIDMATRIX:
        case BARCODE_HANXIN:
        case BARCODE_DOTCODE:
        case BARCODE_UPNQR:
            result = 1;
            break;
    }

    return result;
}

static int supports_eci(const int symbology) {
    /* Returns 1 if symbology can encode the ECI character */

    int result = 0;

    switch (symbology) {
        case BARCODE_AZTEC:
        case BARCODE_DATAMATRIX:
        case BARCODE_MAXICODE:
        case BARCODE_MICROPDF417:
        case BARCODE_PDF417:
        case BARCODE_PDF417TRUNC:
        case BARCODE_QRCODE:
        case BARCODE_DOTCODE:
        case BARCODE_GRIDMATRIX:
        case BARCODE_HANXIN:
            result = 1;
            break;
    }

    return result;
}

int ZBarcode_ValidID(int symbol_id) {
    /* Checks whether a symbology is supported */

    int result = 0;

    switch (symbol_id) {
        case BARCODE_CODE11:
        case BARCODE_C25MATRIX:
        case BARCODE_C25INTER:
        case BARCODE_C25IATA:
        case BARCODE_C25LOGIC:
        case BARCODE_C25IND:
        case BARCODE_CODE39:
        case BARCODE_EXCODE39:
        case BARCODE_EANX:
        case BARCODE_EANX_CHK:
        case BARCODE_EAN128:
        case BARCODE_CODABAR:
        case BARCODE_CODE128:
        case BARCODE_DPLEIT:
        case BARCODE_DPIDENT:
        case BARCODE_CODE16K:
        case BARCODE_CODE49:
        case BARCODE_CODE93:
        case BARCODE_FLAT:
        case BARCODE_RSS14:
        case BARCODE_RSS_LTD:
        case BARCODE_RSS_EXP:
        case BARCODE_TELEPEN:
        case BARCODE_UPCA:
        case BARCODE_UPCA_CHK:
        case BARCODE_UPCE:
        case BARCODE_UPCE_CHK:
        case BARCODE_POSTNET:
        case BARCODE_MSI_PLESSEY:
        case BARCODE_FIM:
        case BARCODE_LOGMARS:
        case BARCODE_PHARMA:
        case BARCODE_PZN:
        case BARCODE_PHARMA_TWO:
        case BARCODE_PDF417:
        case BARCODE_PDF417TRUNC:
        case BARCODE_MAXICODE:
        case BARCODE_QRCODE:
        case BARCODE_CODE128B:
        case BARCODE_AUSPOST:
        case BARCODE_AUSREPLY:
        case BARCODE_AUSROUTE:
        case BARCODE_AUSREDIRECT:
        case BARCODE_ISBNX:
        case BARCODE_RM4SCC:
        case BARCODE_DATAMATRIX:
        case BARCODE_EAN14:
        case BARCODE_NVE18:
        case BARCODE_JAPANPOST:
        case BARCODE_KOREAPOST:
        case BARCODE_RSS14STACK:
        case BARCODE_RSS14STACK_OMNI:
        case BARCODE_RSS_EXPSTACK:
        case BARCODE_PLANET:
        case BARCODE_MICROPDF417:
        case BARCODE_ONECODE:
        case BARCODE_PLESSEY:
        case BARCODE_TELEPEN_NUM:
        case BARCODE_ITF14:
        case BARCODE_KIX:
        case BARCODE_AZTEC:
        case BARCODE_DAFT:
        case BARCODE_MICROQR:
        case BARCODE_HIBC_128:
        case BARCODE_HIBC_39:
        case BARCODE_HIBC_DM:
        case BARCODE_HIBC_QR:
        case BARCODE_HIBC_PDF:
        case BARCODE_HIBC_MICPDF:
        case BARCODE_HIBC_AZTEC:
        case BARCODE_HIBC_BLOCKF:
        case BARCODE_AZRUNE:
        case BARCODE_CODE32:
        case BARCODE_EANX_CC:
        case BARCODE_EAN128_CC:
        case BARCODE_RSS14_CC:
        case BARCODE_RSS_LTD_CC:
        case BARCODE_RSS_EXP_CC:
        case BARCODE_UPCA_CC:
        case BARCODE_UPCE_CC:
        case BARCODE_RSS14STACK_CC:
        case BARCODE_RSS14_OMNI_CC:
        case BARCODE_RSS_EXPSTACK_CC:
        case BARCODE_CHANNEL:
        case BARCODE_CODEONE:
        case BARCODE_GRIDMATRIX:
        case BARCODE_HANXIN:
        case BARCODE_DOTCODE:
        case BARCODE_CODABLOCKF:
        case BARCODE_UPNQR:
        case BARCODE_VIN:
        case BARCODE_MAILMARK:
            result = 1;
            break;
    }

    return result;
}

static int extended_charset(struct zint_symbol *symbol, const unsigned char *source, const int length) {
    int error_number = 0;
        
    /* These are the "elite" standards which can support multiple character sets */
    switch (symbol->symbology) {
        case BARCODE_QRCODE: error_number = qr_code(symbol, source, length);
            break;
        case BARCODE_MICROQR: error_number = microqr(symbol, source, length);
            break;
        case BARCODE_GRIDMATRIX: error_number = grid_matrix(symbol, source, length);
            break;
        case BARCODE_HANXIN: error_number = han_xin(symbol, source, length);
            break;
        case BARCODE_UPNQR: error_number = upnqr(symbol, source, length);
            break;
    }

    return error_number;
}

static int reduced_charset(struct zint_symbol *symbol, const unsigned char *source, size_t in_length) {
    /* These are the "norm" standards which only support Latin-1 at most */
    int error_number = 0;

#ifndef _MSC_VER
    unsigned char preprocessed[in_length + 1];
#else
    unsigned char* preprocessed = (unsigned char*) _alloca(in_length + 1);
#endif
    
    if (symbol->symbology == BARCODE_CODE16K) {
        symbol->whitespace_width = 16;
        symbol->border_width = 2;
        if (!(symbol->output_options & BARCODE_BIND)) {
            symbol->output_options += BARCODE_BIND;
        }
    }
    else
        if (symbol->symbology == BARCODE_ITF14) {
        symbol->whitespace_width = 20;
        symbol->border_width = 8;
        if (!(symbol->output_options & BARCODE_BOX)) {
            symbol->output_options += BARCODE_BOX;
        }
    }
    
    switch (symbol->input_mode & 0x07) {
        case DATA_MODE:
        case GS1_MODE:
            memcpy(preprocessed, source, in_length);
            preprocessed[in_length] = '\0';
            break;
        case UNICODE_MODE:
            error_number = utf_to_eci(symbol->eci, source, preprocessed, &in_length);
            if (error_number != 0) {
                strcpy(symbol->errtxt, "204: Invalid characters in input data");
                return error_number;
            }
            break;
    }
    
    switch (symbol->symbology) {
        case BARCODE_C25MATRIX: error_number = matrix_two_of_five(symbol, preprocessed, in_length);
            break;
        case BARCODE_C25IND: error_number = industrial_two_of_five(symbol, preprocessed, in_length);
            break;
        case BARCODE_C25INTER: error_number = interleaved_two_of_five(symbol, preprocessed, in_length);
            break;
        case BARCODE_C25IATA: error_number = iata_two_of_five(symbol, preprocessed, in_length);
            break;
        case BARCODE_C25LOGIC: error_number = logic_two_of_five(symbol, preprocessed, in_length);
            break;
        case BARCODE_DPLEIT: error_number = dpleit(symbol, preprocessed, in_length);
            break;
        case BARCODE_DPIDENT: error_number = dpident(symbol, preprocessed, in_length);
            break;
        case BARCODE_UPCA:
        case BARCODE_UPCA_CHK:
        case BARCODE_UPCE:
        case BARCODE_UPCE_CHK:
        case BARCODE_EANX:
        case BARCODE_EANX_CHK:
            error_number = eanx(symbol, preprocessed, in_length);
            break;
        case BARCODE_EAN128: error_number = ean_128(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE39: error_number = c39(symbol, preprocessed, in_length);
            break;
        case BARCODE_PZN: error_number = pharmazentral(symbol, preprocessed, in_length);
            break;
        case BARCODE_EXCODE39: error_number = ec39(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODABAR: error_number = codabar(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE93: error_number = c93(symbol, preprocessed, in_length);
            break;
        case BARCODE_LOGMARS: error_number = c39(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE128:
        case BARCODE_CODE128B:
            error_number = code_128(symbol, preprocessed, in_length);
            break;
        case BARCODE_NVE18: error_number = nve_18(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE11: error_number = code_11(symbol, preprocessed, in_length);
            break;
        case BARCODE_MSI_PLESSEY: error_number = msi_handle(symbol, preprocessed, in_length);
            break;
        case BARCODE_TELEPEN: error_number = telepen(symbol, preprocessed, in_length);
            break;
        case BARCODE_TELEPEN_NUM: error_number = telepen_num(symbol, preprocessed, in_length);
            break;
        case BARCODE_PHARMA: error_number = pharma_one(symbol, preprocessed, in_length);
            break;
        case BARCODE_PLESSEY: error_number = plessey(symbol, preprocessed, in_length);
            break;
        case BARCODE_ITF14: error_number = itf14(symbol, preprocessed, in_length);
            break;
        case BARCODE_FLAT: error_number = flattermarken(symbol, preprocessed, in_length);
            break;
        case BARCODE_FIM: error_number = fim(symbol, preprocessed, in_length);
            break;
        case BARCODE_POSTNET: error_number = post_plot(symbol, preprocessed, in_length);
            break;
        case BARCODE_PLANET: error_number = planet_plot(symbol, preprocessed, in_length);
            break;
        case BARCODE_RM4SCC: error_number = royal_plot(symbol, preprocessed, in_length);
            break;
        case BARCODE_AUSPOST:
        case BARCODE_AUSREPLY:
        case BARCODE_AUSROUTE:
        case BARCODE_AUSREDIRECT:
            error_number = australia_post(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE16K: error_number = code16k(symbol, preprocessed, in_length);
            break;
        case BARCODE_PHARMA_TWO: error_number = pharma_two(symbol, preprocessed, in_length);
            break;
        case BARCODE_ONECODE: error_number = imail(symbol, preprocessed, in_length);
            break;
        case BARCODE_ISBNX: error_number = eanx(symbol, preprocessed, in_length);
            break;
        case BARCODE_RSS14:
        case BARCODE_RSS14STACK:
        case BARCODE_RSS14STACK_OMNI:
            error_number = rss14(symbol, preprocessed, in_length);
            break;
        case BARCODE_RSS_LTD: error_number = rsslimited(symbol, preprocessed, in_length);
            break;
        case BARCODE_RSS_EXP:
        case BARCODE_RSS_EXPSTACK:
            error_number = rssexpanded(symbol, preprocessed, in_length);
            break;
        case BARCODE_EANX_CC:
        case BARCODE_EAN128_CC:
        case BARCODE_RSS14_CC:
        case BARCODE_RSS_LTD_CC:
        case BARCODE_RSS_EXP_CC:
        case BARCODE_UPCA_CC:
        case BARCODE_UPCE_CC:
        case BARCODE_RSS14STACK_CC:
        case BARCODE_RSS14_OMNI_CC:
        case BARCODE_RSS_EXPSTACK_CC:
            error_number = composite(symbol, preprocessed, in_length);
            break;
        case BARCODE_KIX: error_number = kix_code(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE32: error_number = code32(symbol, preprocessed, in_length);
            break;
        case BARCODE_DAFT: error_number = daft_code(symbol, preprocessed, in_length);
            break;
        case BARCODE_EAN14:
            error_number = ean_14(symbol, preprocessed, in_length);
            break;
        case BARCODE_AZRUNE: error_number = aztec_runes(symbol, preprocessed, in_length);
            break;
        case BARCODE_KOREAPOST: error_number = korea_post(symbol, preprocessed, in_length);
            break;
        case BARCODE_HIBC_128:
        case BARCODE_HIBC_39:
        case BARCODE_HIBC_DM:
        case BARCODE_HIBC_QR:
        case BARCODE_HIBC_PDF:
        case BARCODE_HIBC_MICPDF:
        case BARCODE_HIBC_AZTEC:
        case BARCODE_HIBC_BLOCKF:
            error_number = hibc(symbol, preprocessed, in_length);
            break;
        case BARCODE_JAPANPOST: error_number = japan_post(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODE49: error_number = code_49(symbol, preprocessed, in_length);
            break;
        case BARCODE_CHANNEL: error_number = channel_code(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODEONE: error_number = code_one(symbol, preprocessed, in_length);
            break;
        case BARCODE_DATAMATRIX: error_number = dmatrix(symbol, preprocessed, in_length);
            break;
        case BARCODE_PDF417:
        case BARCODE_PDF417TRUNC:
            error_number = pdf417enc(symbol, preprocessed, in_length);
            break;
        case BARCODE_MICROPDF417: error_number = micro_pdf417(symbol, preprocessed, in_length);
            break;
        case BARCODE_MAXICODE: error_number = maxicode(symbol, preprocessed, in_length);
            break;
        case BARCODE_AZTEC: error_number = aztec(symbol, preprocessed, in_length);
            break;
        case BARCODE_DOTCODE: error_number = dotcode(symbol, preprocessed, in_length);
            break;
        case BARCODE_CODABLOCKF: error_number = codablock(symbol, preprocessed, in_length);
            break;
        case BARCODE_VIN: error_number = vin(symbol, preprocessed, in_length);
            break;
        case BARCODE_MAILMARK: error_number = mailmark(symbol, preprocessed, in_length);
            break;
    }

    return error_number;
}

void strip_bom(unsigned char *source, int *input_length) {
    int i;

    if (*input_length > 3) {
        if((source[0] == 0xef) && (source[1] == 0xbb) && (source[2] == 0xbf)) {
            /* BOM at start of input data, strip in accordance with RFC 3629 */
            for (i = 3; i < *input_length; i++) {
                source[i - 3] = source[i];
            }
            *input_length -= 3;
        }
    }
}

int escape_char_process(struct zint_symbol *symbol, unsigned char *input_string, int *length) {
    int error_number;
    int in_posn, out_posn;
    int hex1, hex2;

#ifndef _MSC_VER
    unsigned char escaped_string[*length + 1];
#else
    unsigned char* escaped_string = (unsigned char*) _alloca(*length + 1);
#endif

    in_posn = 0;
    out_posn = 0;
    
    do {
        if (input_string[in_posn] == '\\') {
            switch (input_string[in_posn + 1]) {
                case '0': escaped_string[out_posn] = 0x00; /* Null */
                    in_posn += 2;
                    break;
                case 'E': escaped_string[out_posn] = 0x04; /* End of Transmission */
                    in_posn += 2;
                    break;
                case 'a': escaped_string[out_posn] = 0x07; /* Bell */
                    in_posn += 2;
                    break;
                case 'b': escaped_string[out_posn] = 0x08; /* Backspace */
                    in_posn += 2;
                    break;
                case 't': escaped_string[out_posn] = 0x09; /* Horizontal tab */
                    in_posn += 2;
                    break;
                case 'n': escaped_string[out_posn] = 0x0a; /* Line feed */
                    in_posn += 2;
                    break;
                case 'v': escaped_string[out_posn] = 0x0b; /* Vertical tab */
                    in_posn += 2;
                    break;
                case 'f': escaped_string[out_posn] = 0x0c; /* Form feed */
                    in_posn += 2;
                    break;
                case 'r': escaped_string[out_posn] = 0x0d; /* Carriage return */
                    in_posn += 2;
                    break;
                case 'e': escaped_string[out_posn] = 0x1b; /* Escape */
                    in_posn += 2;
                    break;
                case 'G': escaped_string[out_posn] = 0x1d; /* Group Separator */
                    in_posn += 2;
                    break;
                case 'R': escaped_string[out_posn] = 0x1e; /* Record Separator */
                    in_posn += 2;
                    break;
                case 'x': if (in_posn + 4 > *length) {
                        strcpy(symbol->errtxt, "232: Incomplete escape character in input data");
                        return ZINT_ERROR_INVALID_DATA;
                    }
                    hex1 = ctoi(input_string[in_posn + 2]);
                    hex2 = ctoi(input_string[in_posn + 3]);
                    if ((hex1 >= 0) && (hex2 >= 0)) {
                        if (hex1 > 7 && (symbol->input_mode & UNICODE_MODE) != 0) {
                            // Convert to UTF-8
                            escaped_string[out_posn] = 0xc0 + (hex1 >> 2);
                            out_posn++;
                            escaped_string[out_posn] = 0x80 + ((hex1 & 0x03) << 4) + hex2;
                        } else {
                            escaped_string[out_posn] = (hex1 << 4) + hex2;
                        }
                        in_posn += 4;
                    } else {
                        strcpy(symbol->errtxt, "233: Corrupt escape character in input data");
                        return ZINT_ERROR_INVALID_DATA;
                    }
                    break;
                case '\\': escaped_string[out_posn] = '\\';
                    in_posn += 2;
                    break;
                default: strcpy(symbol->errtxt, "234: Unrecognised escape character in input data");
                    return ZINT_ERROR_INVALID_DATA;
                    break;
            }
        } else {
            escaped_string[out_posn] = input_string[in_posn];
            in_posn++;
        }
        out_posn++;
    } while (in_posn < *length);
    
    memcpy(input_string, escaped_string, out_posn);
    input_string[out_posn] = '\0';
    *length = out_posn;
    
    error_number = 0;

    return error_number;
}

int ZBarcode_Encode(struct zint_symbol *symbol, const unsigned char *source, int in_length) {
    int error_number, error_buffer, i;
    int input_mode = symbol->input_mode;
#ifdef _MSC_VER
    unsigned char* local_source;
#endif
    error_number = 0;

    if (in_length == 0) {
        in_length = (int)ustrlen(source);
    }
    if (in_length == 0) {
        strcpy(symbol->errtxt, "205: No input data");
        error_tag(symbol->errtxt, ZINT_ERROR_INVALID_DATA);
        return ZINT_ERROR_INVALID_DATA;
    }

    if (strcmp(symbol->outfile, "") == 0) {
#ifdef NO_PNG
        strcpy(symbol->outfile, "out.gif");
#else
        strcpy(symbol->outfile, "out.png");
#endif
    }
#ifndef _MSC_VER
    unsigned char local_source[in_length + 1];
#else
    local_source = (unsigned char*) _alloca(in_length + 1);
#endif

    /* First check the symbology field */
    if (symbol->symbology < 1) {
        strcpy(symbol->errtxt, "206: Symbology out of range, using Code 128");
        symbol->symbology = BARCODE_CODE128;
        error_number = ZINT_WARN_INVALID_OPTION;
    }

    /* symbol->symbologys 1 to 86 are defined by tbarcode */
    if (symbol->symbology == 5) {
        symbol->symbology = BARCODE_C25MATRIX;
    }
    if ((symbol->symbology >= 10) && (symbol->symbology <= 12)) {
        symbol->symbology = BARCODE_EANX;
    }
    if (symbol->symbology == 15) {
        symbol->symbology = BARCODE_EANX;
    }
    if (symbol->symbology == 17) {
        symbol->symbology = BARCODE_UPCA;
    }
    if (symbol->symbology == 19) {
        strcpy(symbol->errtxt, "207: Codabar 18 not supported, using Codabar");
        symbol->symbology = BARCODE_CODABAR;
        error_number = ZINT_WARN_INVALID_OPTION;
    }
    if (symbol->symbology == 26) {
        symbol->symbology = BARCODE_UPCA;
    }
    if (symbol->symbology == 27) {
        strcpy(symbol->errtxt, "208: UPCD1 not supported");
        error_number = ZINT_ERROR_INVALID_OPTION;
    }
    if (symbol->symbology == 33) {
        symbol->symbology = BARCODE_EAN128;
    }
    if (symbol->symbology == 36) {
        symbol->symbology = BARCODE_UPCA;
    }
    if (symbol->symbology == 38) {
        symbol->symbology = BARCODE_UPCE;
    }
    if ((symbol->symbology >= 41) && (symbol->symbology <= 45)) {
        symbol->symbology = BARCODE_POSTNET;
    }
    if (symbol->symbology == 46) {
        symbol->symbology = BARCODE_PLESSEY;
    }
    if (symbol->symbology == 48) {
        symbol->symbology = BARCODE_NVE18;
    }
    if (symbol->symbology == 54) {
        strcpy(symbol->errtxt, "210: General Parcel Code not supported, using Code 128");
        symbol->symbology = BARCODE_CODE128;
        error_number = ZINT_WARN_INVALID_OPTION;
    }
    if ((symbol->symbology == 59) || (symbol->symbology == 61)) {
        symbol->symbology = BARCODE_CODE128;
    }
    if (symbol->symbology == 62) {
        symbol->symbology = BARCODE_CODE93;
    }
    if ((symbol->symbology == 64) || (symbol->symbology == 65)) {
        symbol->symbology = BARCODE_AUSPOST;
    }
    if (symbol->symbology == 78) {
        symbol->symbology = BARCODE_RSS14;
    }
    if (symbol->symbology == 83) {
        symbol->symbology = BARCODE_PLANET;
    }
    if (symbol->symbology == 88) {
        symbol->symbology = BARCODE_EAN128;
    }
    if (symbol->symbology == 91) {
        strcpy(symbol->errtxt, "212: Symbology out of range, using Code 128");
        symbol->symbology = BARCODE_CODE128;
        error_number = ZINT_WARN_INVALID_OPTION;
    }
    if ((symbol->symbology >= 94) && (symbol->symbology <= 96)) {
        strcpy(symbol->errtxt, "213: Symbology out of range, using Code 128");
        symbol->symbology = BARCODE_CODE128;
        error_number = ZINT_WARN_INVALID_OPTION;
    }
    if (symbol->symbology == 100) {
        symbol->symbology = BARCODE_HIBC_128;
    }
    if (symbol->symbology == 101) {
        symbol->symbology = BARCODE_HIBC_39;
    }
    if (symbol->symbology == 103) {
        symbol->symbology = BARCODE_HIBC_DM;
    }
    if (symbol->symbology == 105) {
        symbol->symbology = BARCODE_HIBC_QR;
    }
    if (symbol->symbology == 107) {
        symbol->symbology = BARCODE_HIBC_PDF;
    }
    if (symbol->symbology == 109) {
        symbol->symbology = BARCODE_HIBC_MICPDF;
    }
    if (symbol->symbology == 111) {
        symbol->symbology = BARCODE_HIBC_BLOCKF;
    }
    if ((symbol->symbology == 113) || (symbol->symbology == 114)) {
        strcpy(symbol->errtxt, "214: Symbology out of range, using Code 128");
        symbol->symbology = BARCODE_CODE128;
        error_number = ZINT_WARN_INVALID_OPTION;
    }
    if (symbol->symbology == 115) {
        symbol->symbology = BARCODE_DOTCODE;
    }
    if ((symbol->symbology >= 117) && (symbol->symbology <= 127)) {
        if (symbol->symbology != 121) {
            strcpy(symbol->errtxt, "215: Symbology out of range, using Code 128");
            symbol->symbology = BARCODE_CODE128;
            error_number = ZINT_WARN_INVALID_OPTION;
        }
    }
    /* Everything from 128 up is Zint-specific */
    if (symbol->symbology >= 144) {
        strcpy(symbol->errtxt, "216: Symbology out of range, using Code 128");
        symbol->symbology = BARCODE_CODE128;
        error_number = ZINT_WARN_INVALID_OPTION;
    }

    if (error_number > 4) {
        error_tag(symbol->errtxt, error_number);
        return error_number;
    } else {
        error_buffer = error_number;
    }

    if ((!(supports_eci(symbol->symbology))) && (symbol->eci != 3)) {
        strcpy(symbol->errtxt, "217: Symbology does not support ECI switching");
        error_number = ZINT_ERROR_INVALID_OPTION;
    }

    if ((symbol->eci < 3) || (symbol->eci > 999999)) {
        strcpy(symbol->errtxt, "218: Invalid ECI mode");
        error_number = ZINT_ERROR_INVALID_OPTION;
    }

    /* Start acting on input mode */
    if (input_mode == GS1_MODE) {
        for (i = 0; i < in_length; i++) {
            if (source[i] == '\0') {
                strcpy(symbol->errtxt, "219: NULL characters not permitted in GS1 mode");
                error_tag(symbol->errtxt, ZINT_ERROR_INVALID_DATA);
                return ZINT_ERROR_INVALID_DATA;
            }
        }
        if (gs1_compliant(symbol->symbology) == 1) {
            error_number = ugs1_verify(symbol, source, in_length, local_source);
            if (error_number != 0) {
                return error_number;
            }
            in_length =(int)ustrlen(local_source);
        } else {
            strcpy(symbol->errtxt, "220: Selected symbology does not support GS1 mode");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_OPTION;
        }
    } else {
        memcpy(local_source, source, in_length);
        local_source[in_length] = '\0';
    }

    if (input_mode & ESCAPE_MODE) {
        error_number = escape_char_process(symbol, local_source, &in_length);
        if (error_number != 0) {
            error_tag(symbol->errtxt, error_number);
            return error_number;
        }
        input_mode -= ESCAPE_MODE;
    }

    if ((input_mode < 0) || (input_mode > 2)) {
        input_mode = DATA_MODE;
    }

    if ((symbol->eci != 3) && (symbol->eci != 26)) {
        input_mode = DATA_MODE;
    }

    if (input_mode == UNICODE_MODE) {
        strip_bom(local_source, &in_length);
    }

    if ((symbol->dot_size < 0.01) || (symbol->dot_size > 20.0)) {
        strcpy(symbol->errtxt, "221: Invalid dot size");
        error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
        return ZINT_ERROR_INVALID_OPTION;
    }
    
    switch (symbol->symbology) {
        case BARCODE_QRCODE:
        case BARCODE_MICROQR:
        case BARCODE_GRIDMATRIX:
        case BARCODE_HANXIN:
        case BARCODE_UPNQR:
            error_number = extended_charset(symbol, local_source, in_length);
            break;
        default:
            error_number = reduced_charset(symbol, local_source, in_length);
            break;
    }

    if ((error_number == ZINT_ERROR_INVALID_DATA) && (supports_eci(symbol->symbology)
            && (input_mode == UNICODE_MODE))) {
        /* Try another ECI mode */
        symbol->eci = get_best_eci(local_source, in_length);

        switch (symbol->symbology) {
            case BARCODE_QRCODE:
            case BARCODE_MICROQR:
            case BARCODE_GRIDMATRIX:
            case BARCODE_HANXIN:
                error_number = utf_to_eci(symbol->eci, source, local_source, (size_t*)&in_length);
                error_number = extended_charset(symbol, local_source, in_length);
                break;
            default:
                error_number = reduced_charset(symbol, local_source, in_length);
                break;
        }
        
        if (error_number == 0) {
            error_number = ZINT_WARN_USES_ECI;
            strcpy(symbol->errtxt, "222: Encoded data includes ECI");
            if (symbol->debug) printf("Data ECI %d\n", symbol->eci);
        }
    }

    if (error_number == 0) {
        if ((symbol->symbology == BARCODE_CODE128) || (symbol->symbology == BARCODE_CODE128B)) {
            for (i = 0; i < in_length; i++) {
                if (local_source[i] == '\0') {
                    symbol->text[i] = ' ';
                } else {
                    symbol->text[i] = local_source[i];
                }
            }
        }
        error_number = error_buffer;
    }
    error_tag(symbol->errtxt, error_number);

    if (error_number <= 5) {
        check_row_heights(symbol);
    }

    return error_number;
}

int ZBarcode_Print(struct zint_symbol *symbol, int rotate_angle) {
    int error_number;

    switch (rotate_angle) {
        case 0:
        case 90:
        case 180:
        case 270:
            break;
        default:
            strcpy(symbol->errtxt, "223: Invalid rotation angle");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_OPTION;
    }

    if (symbol->output_options & BARCODE_DOTTY_MODE) {
        if (!(is_matrix(symbol->symbology))) {
            strcpy(symbol->errtxt, "224: Selected symbology cannot be rendered as dots");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_OPTION;
        }
    }

    if (strlen(symbol->outfile) > 3) {
        char output[4];
        output[0] = symbol->outfile[strlen(symbol->outfile) - 3];
        output[1] = symbol->outfile[strlen(symbol->outfile) - 2];
        output[2] = symbol->outfile[strlen(symbol->outfile) - 1];
        output[3] = '\0';
        to_upper((unsigned char*) output);

        if (!(strcmp(output, "PNG"))) {
            if (symbol->scale < 1.0) {
                symbol->text[0] = '\0';
            }
            error_number = plot_raster(symbol, rotate_angle, OUT_PNG_FILE);
        } else
            if (!(strcmp(output, "BMP"))) {
            if (symbol->scale < 1.0) {
                symbol->text[0] = '\0';
            }
            error_number = plot_raster(symbol, rotate_angle, OUT_BMP_FILE);
        } else
            if (!(strcmp(output, "PCX"))) {
            if (symbol->scale < 1.0) {
                symbol->text[0] = '\0';
            }
            error_number = plot_raster(symbol, rotate_angle, OUT_PCX_FILE);
        } else
            if (!(strcmp(output, "GIF"))) {
            if (symbol->scale < 1.0) {
                symbol->text[0] = '\0';
            }
            error_number = plot_raster(symbol, rotate_angle, OUT_GIF_FILE);
        } else
            if (!(strcmp(output, "TIF"))) {
            if (symbol->scale < 1.0) {
                symbol->text[0] = '\0';
            }
            error_number = plot_raster(symbol, rotate_angle, OUT_TIF_FILE);
        } else
            if (!(strcmp(output, "TXT"))) {
            error_number = dump_plot(symbol);
        } else
            if (!(strcmp(output, "EPS"))) {
            error_number = plot_vector(symbol, rotate_angle, OUT_EPS_FILE);  
        } else
            if (!(strcmp(output, "SVG"))) {
            error_number = plot_vector(symbol, rotate_angle, OUT_SVG_FILE);
        } else
            if (!(strcmp(output, "EMF"))) {
            error_number = plot_vector(symbol, rotate_angle, OUT_EMF_FILE);
        } else {
            strcpy(symbol->errtxt, "225: Unknown output format");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_OPTION;
        }
    } else {
        strcpy(symbol->errtxt, "226: Unknown output format");
        error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
        return ZINT_ERROR_INVALID_OPTION;
    }

    if (error_number == ZINT_ERROR_INVALID_OPTION) {
        /* If libpng is not installed */
        strcpy(symbol->errtxt, "227: No PNG support - compile with libpng-devel to resolve");
    }

    error_tag(symbol->errtxt, error_number);
    return error_number;
}

int ZBarcode_Buffer(struct zint_symbol *symbol, int rotate_angle) {
    int error_number;

    switch (rotate_angle) {
        case 0:
        case 90:
        case 180:
        case 270:
            break;
        default:
            strcpy(symbol->errtxt, "228: Invalid rotation angle");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_OPTION;
    }

    error_number = plot_raster(symbol, rotate_angle, OUT_BUFFER);
    error_tag(symbol->errtxt, error_number);
    return error_number;
}

int ZBarcode_Buffer_Vector(struct zint_symbol *symbol, int rotate_angle) {
    int error_number;

    switch (rotate_angle) {
        case 0:
        case 90:
        case 180:
        case 270:
            break;
        default:
            strcpy(symbol->errtxt, "228: Invalid rotation angle");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_OPTION;
    }

    error_number = plot_vector(symbol, rotate_angle, OUT_BUFFER);
    error_tag(symbol->errtxt, error_number);
    return error_number;
}

int ZBarcode_Encode_and_Print(struct zint_symbol *symbol, unsigned char *input, int length, int rotate_angle) {
    int error_number;
    int first_err;

    error_number = ZBarcode_Encode(symbol, input, length);
    if (error_number >= 5) {
        return error_number;
    }

    first_err = error_number;
    error_number = ZBarcode_Print(symbol, rotate_angle);
    if (error_number == 0) {
        error_number = first_err;
    }
    return error_number;
}

int ZBarcode_Encode_and_Buffer(struct zint_symbol *symbol, unsigned char *input, int length, int rotate_angle) {
    int error_number;
    int first_err;

    error_number = ZBarcode_Encode(symbol, input, length);
    if (error_number >= 5) {
        return error_number;
    }

    first_err = error_number;
    error_number = ZBarcode_Buffer(symbol, rotate_angle);
    if (error_number == 0) {
        error_number = first_err;
    }
    
    return error_number;
}

int ZBarcode_Encode_and_Buffer_Vector(struct zint_symbol *symbol, unsigned char *input, int length, int rotate_angle) {
    int error_number;
    int first_err;

    error_number = ZBarcode_Encode(symbol, input, length);
    if (error_number >= 5) {
        return error_number;
    }

    first_err = error_number;
    error_number = ZBarcode_Buffer_Vector(symbol, rotate_angle);
    if (error_number == 0) {
        error_number = first_err;
    }
    
    return error_number;
}

int ZBarcode_Encode_File(struct zint_symbol *symbol, char *filename) {
    FILE *file;
    unsigned char *buffer;
    unsigned long fileLen;
    unsigned int nRead = 0, n = 0;
    int ret;

    if (!strcmp(filename, "-")) {
        file = stdin;
        fileLen = 7100;
    } else {
        file = fopen(filename, "rb");
        if (!file) {
            strcpy(symbol->errtxt, "229: Unable to read input file");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_OPTION);
            return ZINT_ERROR_INVALID_DATA;
        }

        /* Get file length */
        fseek(file, 0, SEEK_END);
        fileLen = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (fileLen > 7100) {
            /* The largest amount of data that can be encoded is 7089 numeric digits in QR Code */
            strcpy(symbol->errtxt, "230: Input file too long");
            error_tag(symbol->errtxt, ZINT_ERROR_INVALID_DATA);
            fclose(file);
            return ZINT_ERROR_INVALID_DATA;
        }
    }

    /* Allocate memory */
    buffer = (unsigned char *) malloc(fileLen * sizeof (unsigned char));
    if (!buffer) {
        strcpy(symbol->errtxt, "231: Internal memory error");
        error_tag(symbol->errtxt, ZINT_ERROR_MEMORY);
        if (strcmp(filename, "-"))
            fclose(file);
        return ZINT_ERROR_MEMORY;
    }

    /* Read file contents into buffer */

    do {
        n = fread(buffer + nRead, 1, fileLen - nRead, file);
        if (ferror(file)) {
            strcpy(symbol->errtxt, strerror(errno));
            return ZINT_ERROR_INVALID_DATA;
        }
        nRead += n;
    } while (!feof(file) && (0 < n) && (nRead < fileLen));

    fclose(file);
    ret = ZBarcode_Encode(symbol, buffer, nRead);
    free(buffer);
    return ret;
}

int ZBarcode_Encode_File_and_Print(struct zint_symbol *symbol, char *filename, int rotate_angle) {
    int error_number;
    int first_err;

    error_number = ZBarcode_Encode_File(symbol, filename);
    if (error_number >= 5) {
        return error_number;
    }
    
    first_err = error_number;
    error_number = ZBarcode_Print(symbol, rotate_angle);
    if (error_number == 0) {
        error_number = first_err;
    }
    
    return error_number;
}

int ZBarcode_Encode_File_and_Buffer(struct zint_symbol *symbol, char *filename, int rotate_angle) {
    int error_number;
    int first_err;

    error_number = ZBarcode_Encode_File(symbol, filename);
    if (error_number >= 5) {
        return error_number;
    }
    
    first_err = error_number;
    error_number = ZBarcode_Buffer(symbol, rotate_angle);
    if (error_number == 0) {
        error_number = first_err;
    }

    return error_number;
}

int ZBarcode_Encode_File_and_Buffer_Vector(struct zint_symbol *symbol, char *filename, int rotate_angle) {
    int error_number;
    int first_err;

    error_number = ZBarcode_Encode_File(symbol, filename);
    if (error_number >= 5) {
        return error_number;
    }
    
    first_err = error_number;
    error_number = ZBarcode_Buffer_Vector(symbol, rotate_angle);
    if (error_number == 0) {
        error_number = first_err;
    }

    return error_number;
}

/*
 * Rendering support, initially added by Sam Lown.
 *
 * Converts encoded data into an intermediate format to be interpreted
 * in other applications using this library.
 *
 * If the width and height are not set to zero, the barcode will be resized to those
 * dimensions. The symbol->scale and symbol->height values are totally ignored in this case.
 *
 */
int ZBarcode_Render(struct zint_symbol *symbol, const float width, const float height) {
    // Send the request to the render_plot method
    return render_plot(symbol, width, height);
}

int ZBarcode_Version() {
    return (ZINT_VERSION_MAJOR * 10000) + (ZINT_VERSION_MINOR * 100) + ZINT_VERSION_RELEASE;
}

