/* png.c - Handles output to PNG file */

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
#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "common.h"

#ifndef NO_PNG
#include <png.h>
#include <zlib.h>
#include <setjmp.h>

#define SSET	"0123456789ABCDEF"

struct mainprog_info_type {
    long width;
    long height;
    FILE *outfile;
    jmp_buf jmpbuf;
};

static void writepng_error_handler(png_structp png_ptr, png_const_charp msg) {
    struct mainprog_info_type *graphic;

    fprintf(stderr, "writepng libpng error: %s (F30)\n", msg);
    fflush(stderr);

    graphic = (struct mainprog_info_type*) png_get_error_ptr(png_ptr);
    if (graphic == NULL) {
        /* we are completely hosed now */
        fprintf(stderr,
                "writepng severe error:  jmpbuf not recoverable; terminating. (F31)\n");
        fflush(stderr);
        return;
    }
    longjmp(graphic->jmpbuf, 1);
}

int png_pixel_plot(struct zint_symbol *symbol, char *pixelbuf) {
    struct mainprog_info_type wpng_info;
    struct mainprog_info_type *graphic;
    png_structp png_ptr;
    png_infop info_ptr;
    int i, row, column;
    int fgred, fggrn, fgblu, bgred, bggrn, bgblu;

#ifndef _MSC_VER
    unsigned char outdata[symbol->bitmap_width * 3];
#else
    unsigned char* outdata = (unsigned char*) _alloca(symbol->bitmap_width * 3);
#endif

    graphic = &wpng_info;

    graphic->width = symbol->bitmap_width;
    graphic->height = symbol->bitmap_height;

    fgred = (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
    fggrn = (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
    fgblu = (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
    bgred = (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
    bggrn = (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
    bgblu = (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);

    /* Open output file in binary mode */
    if (symbol->output_options & BARCODE_STDOUT) {
#ifdef _MSC_VER
        if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
            strcpy(symbol->errtxt, "631: Can't open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
#endif
        graphic->outfile = stdout;
    } else {
        if (!(graphic->outfile = fopen(symbol->outfile, "wb"))) {
            strcpy(symbol->errtxt, "632: Can't open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
    }

    /* Set up error handling routine as proc() above */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, graphic, writepng_error_handler, NULL);
    if (!png_ptr) {
        strcpy(symbol->errtxt, "633: Out of memory");
        return ZINT_ERROR_MEMORY;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        strcpy(symbol->errtxt, "634: Out of memory");
        return ZINT_ERROR_MEMORY;
    }

    /* catch jumping here */
    if (setjmp(graphic->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        strcpy(symbol->errtxt, "635: libpng error occurred");
        return ZINT_ERROR_MEMORY;
    }

    /* open output file with libpng */
    png_init_io(png_ptr, graphic->outfile);

    /* set compression */
    png_set_compression_level(png_ptr, 9);

    /* set Header block */
    png_set_IHDR(png_ptr, info_ptr, graphic->width, graphic->height,
            8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    /* write all chunks up to (but not including) first IDAT */
    png_write_info(png_ptr, info_ptr);

    /* set up the transformations:  for now, just pack low-bit-depth pixels
    into bytes (one, two or four pixels per byte) */
    png_set_packing(png_ptr);

    /* Pixel Plotting */
    for (row = 0; row < symbol->bitmap_height; row++) {
        unsigned char *image_data;
        for (column = 0; column < symbol->bitmap_width; column++) {
            i = column * 3;
            switch (*(pixelbuf + (symbol->bitmap_width * row) + column)) {
                case '1':
                    outdata[i] = fgred;
                    outdata[i + 1] = fggrn;
                    outdata[i + 2] = fgblu;
                    break;
                default:
                    outdata[i] = bgred;
                    outdata[i + 1] = bggrn;
                    outdata[i + 2] = bgblu;
                    break;

            }
        }
        /* write row contents to file */
        image_data = outdata;
        png_write_row(png_ptr, image_data);
    }

    /* End the file */
    png_write_end(png_ptr, NULL);

    /* make sure we have disengaged */
    if (png_ptr && info_ptr) png_destroy_write_struct(&png_ptr, &info_ptr);
    if (symbol->output_options & BARCODE_STDOUT) {
        fflush(wpng_info.outfile);
    } else {
        fclose(wpng_info.outfile);
    }
    return 0;
}
#endif /* NO_PNG */


