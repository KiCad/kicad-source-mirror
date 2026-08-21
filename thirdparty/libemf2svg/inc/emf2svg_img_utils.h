#include "uemf.h"
#include <stdbool.h>
#include <stddef.h> /* for offsetof() macro */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RLE_MARK 0x00
#define RLE_EOL 0x00
#define RLE_EOB 0x01
#define RLE_DELTA 0x02

#define MAX_BMP_WIDTH 4096
#define MAX_BMP_HEIGHT 4096

/* Returns pixel of bitmap at given point. */
#define RGBPixelAtPoint(image, x, y)                                           \
    *(((image)->pixels) +                                                      \
      (((image)->bytewidth * (y)) + ((x) * (image)->bytes_per_pixel)))

/* Pixels in this bitmap structure are stored as RGB. */
typedef struct _RGBPixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBPixel;

/* Structure for containing decompressed bitmaps. */
typedef struct _RGBBitmap {
    RGBPixel *pixels;
    size_t width;
    size_t height;
    size_t bytewidth;
    uint8_t bytes_per_pixel;
    size_t size;
} RGBBitmap;

/* Pixels in this bitmap structure are stored as RGB. */
typedef struct _RGBAPixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} RGBAPixel;

/* Structure for containing decompressed bitmaps. */
typedef struct _RGBABitmap {
    RGBAPixel *pixels;
    size_t width;
    size_t height;
    size_t bytewidth;
    uint8_t bytes_per_pixel;
    size_t size;
} RGBABitmap;

int rgb2png(RGBABitmap *bitmap, char **out, size_t *size);
float get_pixel_size(uint32_t colortype);
RGBBitmap rle4ToRGB(RGBBitmap img);
RGBBitmap RGB4ToRGB8(RGBBitmap img);
RGBBitmap rle8ToRGB8(RGBBitmap img);
int e2s_get_DIB_params(PU_BITMAPINFO Bmi, const U_RGBQUAD **ct, uint32_t *numCt,
                       uint32_t *width, uint32_t *height, uint32_t *colortype,
                       uint32_t *invert);
