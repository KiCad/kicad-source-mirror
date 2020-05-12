/* tif.h - Aldus Tagged Image File Format */

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
#ifndef TIF_H
#define	TIF_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#include <windows.h>
#include "stdint_msvc.h"
#else
#include <stdint.h>
#endif

#pragma pack(1)

    typedef struct tiff_header {
        uint16_t byte_order;
        uint16_t identity;
        uint32_t offset;
    } tiff_header_t;

    typedef struct tiff_tag {
        uint16_t tag;
        uint16_t type;
        uint32_t count;
        uint32_t offset;
    } tiff_tag_t;

    typedef struct tiff_ifd {
        uint16_t entries;
        tiff_tag_t new_subset;
        tiff_tag_t image_width;
        tiff_tag_t image_length;
        tiff_tag_t bits_per_sample;
        tiff_tag_t compression;
        tiff_tag_t photometric;
        tiff_tag_t strip_offsets;
        tiff_tag_t samples_per_pixel;
        tiff_tag_t rows_per_strip;
        tiff_tag_t strip_byte_counts;
        tiff_tag_t x_resolution;
        tiff_tag_t y_resolution;
        tiff_tag_t planar_config;
        tiff_tag_t resolution_unit;
        uint32_t offset;
    } tiff_ifd_t;

#pragma pack()

#ifdef	__cplusplus
}
#endif

#endif	/* TIF_H */


