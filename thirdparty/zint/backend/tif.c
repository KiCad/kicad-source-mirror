/* tif.c - Aldus Tagged Image File Format support */

/*
    libzint - the open source barcode library
    Copyright (C) 2016-2017 Robin Stuart <rstuart114@gmail.com>

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
#include <math.h>
#include "common.h"
#include "tif.h"
#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#endif

int tbump_up(int input) {
    /* Strings length must be a multiple of 4 bytes */
    if ((input % 2) == 1) {
        input++;
    }
    return input;
}

int tif_pixel_plot(struct zint_symbol *symbol, char *pixelbuf) {
    int fgred, fggrn, fgblu, bgred, bggrn, bgblu;
    int i;
    int rows_per_strip, strip_count;
    int free_memory;
    int row, column, strip, bytes_put;
    FILE *tif_file;
#ifdef _MSC_VER
    uint32_t* strip_offset;
    uint32_t* strip_bytes;
#endif

    tiff_header_t header;
    tiff_ifd_t ifd;
    uint16_t temp;
    uint32_t temp32;

    fgred = (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
    fggrn = (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
    fgblu = (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
    bgred = (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
    bggrn = (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
    bgblu = (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);

    rows_per_strip = 8192 / (symbol->bitmap_width * 3);
    if (rows_per_strip == 0) {
        rows_per_strip = 1;
    }

    strip_count = symbol->bitmap_height / rows_per_strip;
    if ((symbol->bitmap_height % rows_per_strip) != 0) {
        strip_count++;
    }
    
    if (rows_per_strip > symbol->bitmap_height) {
        rows_per_strip = symbol->bitmap_height;
    }
    
    if (strip_count == 1) {
        rows_per_strip = (rows_per_strip / 2) + 1;
        strip_count++;
    }
    
#ifndef _MSC_VER
    uint32_t strip_offset[strip_count];
    uint32_t strip_bytes[strip_count];
#else
    strip_offset = (uint32_t*) _alloca(strip_count * sizeof(uint32_t));
    strip_bytes = (uint32_t*) _alloca(strip_count * sizeof(uint32_t));
#endif
    free_memory = 8;

    for(i = 0; i < strip_count; i++) {
        strip_offset[i] = free_memory;
        if (i != (strip_count - 1)) {
            strip_bytes[i] = rows_per_strip * symbol->bitmap_width * 3;
        } else {
            if ((symbol->bitmap_height % rows_per_strip) != 0) {
                strip_bytes[i] = (symbol->bitmap_height % rows_per_strip) * symbol->bitmap_width * 3;
            } else {
                strip_bytes[i] = rows_per_strip * symbol->bitmap_width * 3;
            }
        }
        free_memory += strip_bytes[i];
        if ((free_memory % 2) == 1) {
            free_memory++;
        }
    }

    if (free_memory > 0xffff0000) {
#ifdef _MSC_VER
        free(strip_offset);
        free(strip_bytes);
#endif
        strcpy(symbol->errtxt, "670: Output file size too big");
        return ZINT_ERROR_MEMORY;
    }

    /* Open output file in binary mode */
    if (symbol->output_options & BARCODE_STDOUT) {
#ifdef _MSC_VER
        if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
            strcpy(symbol->errtxt, "671: Can't open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
#endif
        tif_file = stdout;
    } else {
        if (!(tif_file = fopen(symbol->outfile, "wb"))) {
            strcpy(symbol->errtxt, "672: Can't open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
    }

    /* Header */
    header.byte_order = 0x4949;
    header.identity = 42;
    header.offset = free_memory;

    fwrite(&header, sizeof(tiff_header_t), 1, tif_file);
    free_memory += sizeof(tiff_ifd_t);
    
    /* Pixel data */
    strip = 0;
    bytes_put = 0;
    for (row = 0; row < symbol->bitmap_height; row++) {
        for (column = 0; column < symbol->bitmap_width; column++) {
            if (pixelbuf[(row * symbol->bitmap_width) + column] == '1') {
                putc(fgred, tif_file);
                putc(fggrn, tif_file);
                putc(fgblu, tif_file);
            } else {
                putc(bgred, tif_file);
                putc(bggrn, tif_file);
                putc(bgblu, tif_file);
            }
            bytes_put += 3;
        }
        
        if ((bytes_put + 3) >= strip_bytes[strip]) {
            // End of strip, pad if strip length is odd
            if (strip_bytes[strip] % 2 == 1) {
                putc(0, tif_file);
            }
            strip++;
            bytes_put = 0;
        }
    }

    /* Image File Directory */
    ifd.entries = 14;
    ifd.offset = 0;

    ifd.new_subset.tag = 0xfe;
    ifd.new_subset.type = 4;
    ifd.new_subset.count = 1;
    ifd.new_subset.offset = 0;

    ifd.image_width.tag = 0x0100;
    ifd.image_width.type = 3; // SHORT
    ifd.image_width.count = 1;
    ifd.image_width.offset = symbol->bitmap_width;

    ifd.image_length.tag = 0x0101;
    ifd.image_length.type = 3; // SHORT
    ifd.image_length.count = 1;
    ifd.image_length.offset = symbol->bitmap_height;

    ifd.bits_per_sample.tag = 0x0102;
    ifd.bits_per_sample.type = 3; // SHORT
    ifd.bits_per_sample.count = 3;
    ifd.bits_per_sample.offset = free_memory;
    free_memory += 6;

    ifd.compression.tag = 0x0103;
    ifd.compression.type = 3;
    ifd.compression.count = 1;
    ifd.compression.offset = 1; // Uncompressed

    ifd.photometric.tag = 0x0106;
    ifd.photometric.type = 3; // SHORT
    ifd.photometric.count = 1;
    ifd.photometric.offset = 2; // RGB Model

    ifd.strip_offsets.tag = 0x0111;
    ifd.strip_offsets.type = 4; // LONG
    ifd.strip_offsets.count = strip_count;
    ifd.strip_offsets.offset = free_memory;
    free_memory += strip_count * 4;

    ifd.samples_per_pixel.tag = 0x0115;
    ifd.samples_per_pixel.type = 3;
    ifd.samples_per_pixel.count = 1;
    ifd.samples_per_pixel.offset = 3;

    ifd.rows_per_strip.tag = 0x0116;
    ifd.rows_per_strip.type = 4;
    ifd.rows_per_strip.count = 1;
    ifd.rows_per_strip.offset = rows_per_strip;

    ifd.strip_byte_counts.tag = 0x0117;
    ifd.strip_byte_counts.type = 4;
    ifd.strip_byte_counts.count = strip_count;
    ifd.strip_byte_counts.offset = free_memory;
    free_memory += strip_count * 4;

    ifd.x_resolution.tag = 0x011a;
    ifd.x_resolution.type = 5;
    ifd.x_resolution.count = 1;
    ifd.x_resolution.offset = free_memory;
    free_memory += 8;

    ifd.y_resolution.tag = 0x011b;
    ifd.y_resolution.type = 5;
    ifd.y_resolution.count = 1;
    ifd.y_resolution.offset = free_memory;
//    free_memory += 8;

    ifd.planar_config.tag = 0x11c;
    ifd.planar_config.type = 3;
    ifd.planar_config.count = 1;
    ifd.planar_config.offset = 1;

    ifd.resolution_unit.tag = 0x0128;
    ifd.resolution_unit.type = 3;
    ifd.resolution_unit.count = 1;
    ifd.resolution_unit.offset = 2; // Inches

    fwrite(&ifd, sizeof(tiff_ifd_t), 1, tif_file);

    /* Bits per sample */
    temp = 8;
    fwrite(&temp, 2, 1, tif_file); // Red Bytes
    fwrite(&temp, 2, 1, tif_file); // Green Bytes
    fwrite(&temp, 2, 1, tif_file); // Blue Bytes

    /* Strip offsets */
    for(i = 0; i < strip_count; i++) {
        fwrite(&strip_offset[i], 4, 1, tif_file);
    }

    /* Strip byte lengths */
    for(i = 0; i < strip_count; i++) {
        fwrite(&strip_bytes[i], 4, 1, tif_file);
    }

    /* X Resolution */
    temp32 = 72;
    fwrite(&temp32, 4, 1, tif_file);
    temp32 = 1;
    fwrite(&temp32, 4, 1, tif_file);

    /* Y Resolution */
    temp32 = 72;
    fwrite(&temp32, 4, 1, tif_file);
    temp32 = 1;
    fwrite(&temp32, 4, 1, tif_file);

    if (symbol->output_options & BARCODE_STDOUT) {
        fflush(tif_file);
    } else {
        fclose(tif_file);
    }

    return 0;
}
