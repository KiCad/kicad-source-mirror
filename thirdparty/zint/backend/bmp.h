/*  bmp.h - header structure for Windows bitmap files

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

#ifndef BMP_H
#define BMP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#include <windows.h>
#include "stdint_msvc.h"
#else
#include <stdint.h>
#endif

#pragma pack (1)

    typedef struct bitmap_file_header {
        uint16_t header_field;
        uint32_t file_size;
        uint32_t reserved;
        uint32_t data_offset;
    } bitmap_file_header_t;

    typedef struct bitmap_info_header {
        uint32_t header_size;
        int32_t width;
        int32_t height;
        uint16_t colour_planes;
        uint16_t bits_per_pixel;
        uint32_t compression_method;
        uint32_t image_size;
        int32_t horiz_res;
        int32_t vert_res;
        uint32_t colours;
        uint32_t important_colours;
    } bitmap_info_header_t;

#pragma pack ()

#ifdef __cplusplus
}
#endif

#endif /* BMP_H */


