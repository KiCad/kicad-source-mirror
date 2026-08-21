#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_img_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <internal-fmem.h>
#include <png.h>

RGBAPixel *pixel_at(RGBABitmap *bitmap, int x, int y) {
    return bitmap->pixels + (bitmap->width * y + x);
}

// return the size in octet of a pixel
float get_pixel_size(uint32_t colortype) {
    switch (colortype) {
    case U_BCBM_MONOCHROME:
        return 0.125;
    case U_BCBM_COLOR4:
        return 0.5;
    case U_BCBM_COLOR8:
        return 1;
    case U_BCBM_COLOR16:
        return 2;
    case U_BCBM_COLOR24:
        return 3;
    case U_BCBM_COLOR32:
        return 4;
    }
    return 4;
}

/* Attempts to save PNG to file; returns 0 on success, non-zero on error. */
int rgb2png(RGBABitmap *bitmap, char ** fm_out, size_t * fm_out_length) {
    fmem fm;
    fmem_init(&fm);
    *fm_out = NULL;
    *fm_out_length = 0;
    FILE* fp = fmem_open(&fm, "w");
    if (fp == NULL) {
        return -1;
    }
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    size_t width_by_height;
    // png_uint_32 bytes_per_row;
    png_byte **row_pointers = NULL;
    bool alpha_channel_empty = true;

    /* Initialize the write struct. */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return -1;
    }

    /* Initialize the info struct. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return -1;
    }

    /* Set up error handling. */
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return -1;
    }

    /* Set image attributes. */
    png_set_IHDR(png_ptr, info_ptr, bitmap->width, bitmap->height, 8,
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    /* Initialize rows of PNG. */
    // bytes_per_row = bitmap->width * bytes_per_pixel;
    row_pointers = png_malloc(png_ptr, bitmap->height * sizeof(png_byte *));
    // Check to see if alpha channel is used (nonzero)
    width_by_height = bitmap->width * bitmap->height;
    for (x = 0; x < width_by_height; ++x) {
        if (bitmap->pixels[x].alpha) {
            alpha_channel_empty = false;
            break;
        }
    }

    for (y = 0; y < bitmap->height; ++y) {
        uint8_t *row = png_malloc(png_ptr, sizeof(uint8_t) * bitmap->width * 4);
        // row_pointers[y] = (png_byte *)row;
        row_pointers[bitmap->height - y - 1] = row;
        if (alpha_channel_empty) {
            for (x = 0; x < bitmap->width; ++x) {
                // RGBPixel *color = pixel_at(bitmap, x, y);
                RGBAPixel color = bitmap->pixels[((x + bitmap->width * y))];
                // printf("(%d, %d)\n", bitmap->width, bitmap->height);
                // printf("(%d, %d)\n", x, y);
                // printf("color:0x%0X%0X%0x\n", color.red, color.green,
                // color.blue);
                *row++ = color.red;
                *row++ = color.green;
                *row++ = color.blue;
                //*row++ = color.alpha;
                *row++ = 0xFF;
            }
        } else {
            for (x = 0; x < bitmap->width; ++x) {
                RGBAPixel color = bitmap->pixels[((x + bitmap->width * y))];
                *row++ = color.red;
                *row++ = color.green;
                *row++ = color.blue;
                *row++ = color.alpha;
            }
        }
    }

    /* Actually write the image data. */
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* Cleanup. */
    for (y = 0; y < bitmap->height; y++) {
        png_free(png_ptr, row_pointers[y]);
    }
    png_free(png_ptr, row_pointers);

    /* Finish writing. */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fflush(fp);
    void* out;
    fmem_mem(&fm, &out, fm_out_length);
    if (*fm_out_length) {
        *fm_out = (char*)malloc(*fm_out_length+1);
    }
    if (*fm_out) {
        memcpy((void*)(*fm_out), out, *fm_out_length);
        (*fm_out)[*fm_out_length] = 0;
    }
    fclose(fp);
    fmem_term(&fm);
    return (*fm_out)?0:-1;
}

// uncompress RLE8 to get bitmap (section 3.1.6.2 [MS-WMF].pdf)
RGBBitmap rle8ToRGB8(RGBBitmap img) {
    FILE *stream;
    bool decode = true;
    fmem fm;
    fmem_init(&fm);

    RGBBitmap out_img;
    out_img.size = 0;
    out_img.width = 0;
    out_img.height = 0;
    out_img.bytes_per_pixel = 3;
    out_img.pixels = NULL;

    uint8_t *bm = (uint8_t *)img.pixels;
    uint32_t x = 0;
    uint32_t y = img.height - 1;
    uint8_t *bm_next;

    if (img.width > MAX_BMP_WIDTH || img.height > MAX_BMP_HEIGHT ||
        img.width == 0 || img.height == 0) {
        return out_img;
    }

    stream = fmem_open(&fm, "w");
    if (stream == NULL) {
        fmem_term(&fm);
        return out_img;
    }

    uint8_t *end = bm + img.size;
    while (decode && (bm < end)) {
        // check against potential overflow
        if ((bm + 2) > end || x > MAX_BMP_WIDTH || y > MAX_BMP_HEIGHT) {
            fclose(stream);
            return out_img;
        };
        switch (bm[0]) {
        case RLE_MARK:
            switch (bm[1]) {
            case RLE_EOL:
                // end of line, pad the rest of the line with zeros
                for (int i = 0; i < ((int)img.width - (int)x); i++)
                    fputc(0x00, stream);
                bm += 2;
                x = 0;
                y--;
                break;
            case RLE_EOB:
                // end of bitmap
                decode = false;
                break;
            case RLE_DELTA:
                // offset handling, pad with (off.x + off.y * width) zeros
                if ((bm + 3) > end) {
                    fclose(stream);
                    fmem_term(&fm);
                    return out_img;
                };
                for (int i = 0; i < (bm[2] + img.width * bm[3]); i++)
                    fputc(0x00, stream);
                x += bm[2];
                y -= bm[3];
                bm += 4;
                break;
            default:
                // absolute mode handling (no compression)

                // data is padded to a word
                // calculate next address accordingly
                bm_next = bm + 1 + ((bm[1] + 1) / 2) * 2;
                if (bm_next > end) {
                    fclose(stream);
                    fmem_term(&fm);
                    return out_img;
                };
                for (int i = 2; i < bm[1] + 2; i++)
                    fputc(bm[i], stream);
                x += bm[1];
                bm = bm_next + 1;
                break;
            }
            break;
        default:
            for (int i = 0; i < bm[0]; i++)
                fputc(bm[1], stream);
            x += bm[0];
            bm += 2;
            if (x >= img.width) {
                x = x % img.width;
                y--;
            }
            break;
        }
    }
    // pad the rest of the bitmap
    for (size_t i = 0; i < ((img.width - x) + img.width * y); i++)
        fputc(0x00, stream);

    fflush(stream);
    void* out;
    fmem_mem(&fm, &out, &out_img.size);
    if (out_img.size) {
        out_img.pixels = (RGBPixel*)malloc(out_img.size+1);
    }
    if (out_img.pixels) {
        memcpy((void*)out_img.pixels, out, out_img.size);
        ((char*)(out_img.pixels))[out_img.size] = 0;
        out_img.width = img.width;
        out_img.height = img.height;
    }
    else {
        out_img.size = 0;
    }

    fclose(stream);
    fmem_term(&fm);
    return out_img;
}

int e2s_get_DIB_params(PU_BITMAPINFO Bmi, const U_RGBQUAD **ct, uint32_t *numCt,
                       uint32_t *width, uint32_t *height, uint32_t *colortype,
                       uint32_t *invert) {
    uint32_t bic;
    /* if biCompression is not U_BI_RGB some or all of the following might not
     * hold real values */
    PU_BITMAPINFOHEADER Bmih = &(Bmi->bmiHeader);
    bic = Bmih->biCompression;
    *width = Bmih->biWidth;
    *colortype = Bmih->biBitCount;
    if (Bmih->biHeight < 0) {
        *height = -Bmih->biHeight;
        *invert = 1;
    } else {
        *height = Bmih->biHeight;
        *invert = 0;
    }
    if (bic == U_BI_RGB) {
        *numCt = get_real_color_count((const char *)Bmih);
        if (numCt) {
            *ct = (PU_RGBQUAD)((char *)Bmi + Bmih->biSize);
        } else {
            *ct = NULL;
        }
    } else if (bic ==
               U_BI_BITFIELDS) { /* to date only encountered once, for 32 bit,
                                    from PPT*/
        *numCt = 0;
        *ct = NULL;
        bic = U_BI_RGB; /* there seems to be no difference, at least for the 32
                           bit images */
    } else {
        *numCt = Bmih->biSizeImage;
        *ct = NULL;
    }
    return (bic);
}

// uncompress RLE4 to get bitmap (section 3.1.6.2 [MS-WMF].pdf)
// FIXME (probably) (handling 4 bits stuff is kind of messy...)
RGBBitmap rle4ToRGB(RGBBitmap img) {
    FILE *stream;
    bool decode = true;
    fmem fm;
    fmem_init(&fm);

    RGBBitmap out_img;
    out_img.size = 0;
    out_img.width = 0;
    out_img.height = 0;
    out_img.bytes_per_pixel = 2;
    out_img.pixels = NULL;

    uint8_t *bm = (uint8_t *)img.pixels;
    uint32_t x = 0;
    uint32_t y = img.height - 1;
    uint8_t *bm_next;

    if (img.width > MAX_BMP_WIDTH || img.height > MAX_BMP_HEIGHT ||
        img.width == 0 || img.height == 0) {
        return out_img;
    }

    stream = fmem_open(&fm, "w");
    if (stream == NULL) {
        return out_img;
    }

    // upper 4 bits of the stuff to write
    uint8_t upper = 0x00;
    // lower 4 bits of the stuff to write
    uint8_t lower = 0x00;
    // Is the current position on upper 4 bits?
    // If it's the case, swap upper and lower of what we read
    // and eventualy print the upper part of previous record
    bool odd = false;

    uint8_t tmp_u;
    uint8_t tmp_l;

    uint8_t *end = bm + img.size;
    while (decode && (bm < end)) {
        // check against potential overflow
        if ((bm + 2) > end || x > MAX_BMP_WIDTH || y > MAX_BMP_HEIGHT) {
            fclose(stream);
            return out_img;
        };
        switch (bm[0]) {
        case RLE_MARK:
            switch (bm[1]) {
            case RLE_EOL:
                if (odd) {
                    fputc(upper | 0x00, stream);
                    upper = 0x00;
                    lower = 0x00;
                }
                // end of line, pad the rest of the line with zeros
                for (int i = 0; i < ((((int)img.width - (int)x) / 2) - 1); i++)
                    fputc(0x00, stream);
                odd = img.width % 2;
                bm += 2;
                x = 0;
                y--;
                break;
            case RLE_EOB:
                // end of bitmap
                decode = false;
                break;
            case RLE_DELTA:
                // offset handling, pad with (off.x + off.y * width) zeros
                if ((bm + 3) > end) {
                    fclose(stream);
                    return out_img;
                };

                tmp_u = 0x00;
                tmp_l = 0x00;
                if (odd) {
                    fputc(upper | tmp_l, stream);
                }
                upper = tmp_u;
                lower = tmp_l;

                for (int i = 0; i < ((bm[2] + img.width * bm[3]) / 2); i++)
                    fputc(0x00, stream);

                odd = ((bm[2] + img.width * bm[3]) + odd) % 2;

                x += bm[2];
                y -= bm[3];
                bm += 4;
                break;
            default:
                // absolute mode handling (no compression)

                // data is padded to a word
                // calculate next address accordingly
                bm_next = bm + (bm[1] / 2) + 2;
                if (bm_next > end) {
                    fclose(stream);
                    return out_img;
                };
                for (int i = 2; i < (bm[1] / 2) + 2; i++) {
                    if (!odd) {
                        tmp_u = bm[i] & 0xF0;
                        tmp_l = bm[i] & 0x0F;
                        fputc(tmp_u | tmp_l, stream);
                    } else {
                        tmp_u = (bm[i] & 0x0F) << 4;
                        tmp_l = (bm[i] & 0xF0) >> 4;
                        fputc(upper | tmp_l, stream);
                    }
                    upper = tmp_u;
                    lower = tmp_l;
                }
                x += bm[1];
                bm = bm_next + 1;
                odd = (bm[1] + odd) % 2;

                if (odd) {
                    upper = (bm[0] & 0x0F) << 4;
                    lower = (bm[0] & 0xF0) >> 4;
                    bm += 1;
                }
                break;
            }
            break;
        default:
            if (!odd) {
                tmp_u = bm[1] & 0xF0;
                tmp_l = bm[1] & 0x0F;
            } else {
                tmp_u = (bm[1] & 0x0F) << 4;
                tmp_l = (bm[1] & 0xF0) >> 4;
                fputc(upper | tmp_l, stream);
            }
            upper = tmp_u;
            lower = tmp_l;

            for (int i = 0; i < (bm[0] / 2); i++) {
                fputc(upper | lower, stream);
            }
            odd = (bm[0] + odd) % 2;
            x += bm[0];
            bm += 2;
            if (x >= img.width) {
                x = x % img.width;
                y--;
            }
            break;
        }
    }
    // pad the rest of the bitmap
    if (odd) {
        fputc(upper | 0x00, stream);
    }
    // end of line, pad the rest of the line with zeros
    for (size_t i = 0; i < ((img.width - x + img.width * y) / 2); i++)
        fputc(0x00, stream);

    fflush(stream);
    void* out;
    fmem_mem(&fm, &out, &out_img.size);
    if (out_img.size) {
        out_img.pixels = (RGBPixel*)malloc(out_img.size+1);
    }
    if (out_img.pixels) {
        memcpy((void*)out_img.pixels, out, out_img.size);
        ((char*)(out_img.pixels))[out_img.size] = 0;
        out_img.width = img.width;
        out_img.height = img.height;
    }
    else {
        out_img.size = 0;
    }

    fclose(stream);
    fmem_term(&fm);
    return out_img;
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
