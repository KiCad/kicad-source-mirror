/*  zint.h - definitions for libzint

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

#ifndef ZINT_H
#define ZINT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    struct zint_render_line {
        float x, y, length, width;
        struct zint_render_line *next; /* Pointer to next line */
    };
    
    struct zint_vector_rect {
        float x, y, height, width;
        int colour;
        struct zint_vector_rect *next;
    };

    struct zint_render_string {
        float x, y, fsize;
        float width; /* Suggested string width, may be 0 if none recommended */
        int length;
        unsigned char *text;
        struct zint_render_string *next; /* Pointer to next character */
    };
    
    struct zint_vector_string {
        float x, y, fsize;
        float width; /* Suggested string width, may be 0 if none recommended */
        int length;
        unsigned char *text;
        struct zint_vector_string *next; /* Pointer to next character */
    };

    struct zint_render_ring {
        float x, y, radius, line_width;
        struct zint_render_ring *next; /* Pointer to next ring */
    };

    struct zint_vector_circle {
        float x, y, diameter;
        int colour;
        struct zint_vector_circle *next; /* Pointer to next circle */
    };
    
    struct zint_render_hexagon {
        float x, y, height;
        struct zint_render_hexagon *next; /* Pointer to next hexagon */
    };

    struct zint_vector_hexagon {
        float x, y, diameter;
        struct zint_vector_hexagon *next; /* Pointer to next hexagon */
    };
    
    struct zint_render {
        float width, height;
        struct zint_render_line *lines; /* Pointer to first line */
        struct zint_render_string *strings; /* Pointer to first string */
        struct zint_render_ring *rings; /* Pointer to first ring */
        struct zint_render_hexagon *hexagons; /* Pointer to first hexagon */
    };
    
    struct zint_vector {
        float width, height;
        struct zint_vector_rect *rectangles; /* Pointer to first rectangle */
        struct zint_vector_hexagon *hexagons; /* Pointer to first hexagon */
        struct zint_vector_string *strings; /* Points to first string */
        struct zint_vector_circle *circles; /* Points to first circle */
    };

    struct zint_symbol {
        int symbology;
        int height;
        int whitespace_width;
        int border_width;
        int output_options;
        char fgcolour[10];
        char bgcolour[10];
        char outfile[256];
        float scale;
        int option_1;
        int option_2;
        int option_3;
        int show_hrt;
        int fontsize;
        int input_mode;
        int eci;
        unsigned char text[128];
        int rows;
        int width;
        char primary[128];
        unsigned char encoded_data[200][143];
        int row_height[200]; /* Largest symbol is 189 x 189 Han Xin */
        char errtxt[100];
        char *bitmap;
        int bitmap_width;
        int bitmap_height;
        unsigned int bitmap_byte_length;
        float dot_size;
        struct zint_vector *vector;
        struct zint_render *rendered;
        int debug;
    };

#define ZINT_VERSION_MAJOR      2
#define ZINT_VERSION_MINOR      6
#define ZINT_VERSION_RELEASE    3

    /* Tbarcode 7 codes */
#define BARCODE_CODE11		1
#define BARCODE_C25MATRIX	2
#define BARCODE_C25INTER	3
#define BARCODE_C25IATA		4
#define BARCODE_C25LOGIC	6
#define BARCODE_C25IND		7
#define BARCODE_CODE39		8
#define BARCODE_EXCODE39	9
#define BARCODE_EANX		13
#define BARCODE_EANX_CHK        14
#define BARCODE_EAN128		16
#define BARCODE_CODABAR		18
#define BARCODE_CODE128		20
#define BARCODE_DPLEIT		21
#define BARCODE_DPIDENT		22
#define BARCODE_CODE16K		23
#define BARCODE_CODE49		24
#define BARCODE_CODE93		25
#define BARCODE_FLAT		28
#define BARCODE_RSS14		29
#define BARCODE_RSS_LTD		30
#define BARCODE_RSS_EXP		31
#define BARCODE_TELEPEN		32
#define BARCODE_UPCA		34
#define BARCODE_UPCA_CHK        35
#define BARCODE_UPCE		37
#define BARCODE_UPCE_CHK        38
#define BARCODE_POSTNET		40
#define BARCODE_MSI_PLESSEY	47
#define BARCODE_FIM		49
#define BARCODE_LOGMARS		50
#define BARCODE_PHARMA		51
#define BARCODE_PZN		52
#define BARCODE_PHARMA_TWO	53
#define BARCODE_PDF417		55
#define BARCODE_PDF417TRUNC	56
#define BARCODE_MAXICODE	57
#define BARCODE_QRCODE		58
#define BARCODE_CODE128B	60
#define BARCODE_AUSPOST		63
#define BARCODE_AUSREPLY	66
#define BARCODE_AUSROUTE	67
#define BARCODE_AUSREDIRECT	68
#define BARCODE_ISBNX		69
#define BARCODE_RM4SCC		70
#define BARCODE_DATAMATRIX	71
#define BARCODE_EAN14		72
#define BARCODE_VIN             73
#define BARCODE_CODABLOCKF	74
#define BARCODE_NVE18		75
#define BARCODE_JAPANPOST	76
#define BARCODE_KOREAPOST	77
#define BARCODE_RSS14STACK	79
#define BARCODE_RSS14STACK_OMNI	80
#define BARCODE_RSS_EXPSTACK	81
#define BARCODE_PLANET		82
#define BARCODE_MICROPDF417	84
#define BARCODE_ONECODE		85
#define BARCODE_PLESSEY		86

    /* Tbarcode 8 codes */
#define BARCODE_TELEPEN_NUM	87
#define BARCODE_ITF14		89
#define BARCODE_KIX		90
#define BARCODE_AZTEC		92
#define BARCODE_DAFT		93
#define BARCODE_MICROQR		97

    /* Tbarcode 9 codes */
#define BARCODE_HIBC_128	98
#define BARCODE_HIBC_39		99
#define BARCODE_HIBC_DM		102
#define BARCODE_HIBC_QR		104
#define BARCODE_HIBC_PDF	106
#define BARCODE_HIBC_MICPDF	108
#define BARCODE_HIBC_BLOCKF	110
#define BARCODE_HIBC_AZTEC	112

    /* Tbarcode 10 codes */
#define BARCODE_DOTCODE         115
#define BARCODE_HANXIN          116
    
    /*Tbarcode 11 codes*/
#define BARCODE_MAILMARK        121

    /* Zint specific */
#define BARCODE_AZRUNE		128
#define BARCODE_CODE32		129
#define BARCODE_EANX_CC		130
#define BARCODE_EAN128_CC	131
#define BARCODE_RSS14_CC	132
#define BARCODE_RSS_LTD_CC	133
#define BARCODE_RSS_EXP_CC	134
#define BARCODE_UPCA_CC		135
#define BARCODE_UPCE_CC		136
#define BARCODE_RSS14STACK_CC	137
#define BARCODE_RSS14_OMNI_CC	138
#define BARCODE_RSS_EXPSTACK_CC	139
#define BARCODE_CHANNEL		140
#define BARCODE_CODEONE		141
#define BARCODE_GRIDMATRIX	142
#define BARCODE_UPNQR           143

// Output options
#define BARCODE_NO_ASCII	1
#define BARCODE_BIND		2
#define BARCODE_BOX		4
#define BARCODE_STDOUT		8
#define READER_INIT		16
#define SMALL_TEXT		32
#define BOLD_TEXT               64
#define CMYK_COLOUR             128
#define BARCODE_DOTTY_MODE      256

// Input data types
#define DATA_MODE	0
#define UNICODE_MODE	1
#define GS1_MODE	2
#define ESCAPE_MODE     8

// Data Matrix specific options
#define DM_SQUARE	100
#define DM_DMRE	        101

// Warning and error conditions
#define ZINT_WARN_INVALID_OPTION	2
#define ZINT_WARN_USES_ECI              3
#define ZINT_ERROR_TOO_LONG		5
#define ZINT_ERROR_INVALID_DATA	        6
#define ZINT_ERROR_INVALID_CHECK	7
#define ZINT_ERROR_INVALID_OPTION	8
#define ZINT_ERROR_ENCODING_PROBLEM	9
#define ZINT_ERROR_FILE_ACCESS	        10
#define ZINT_ERROR_MEMORY		11

// Raster file types
#define OUT_BUFFER          0
#define OUT_SVG_FILE        10
#define OUT_EPS_FILE        20
#define OUT_EMF_FILE        30
#define	OUT_PNG_FILE        100
#define	OUT_BMP_FILE        120
#define OUT_GIF_FILE        140
#define OUT_PCX_FILE        160
#define OUT_JPG_FILE        180
#define OUT_TIF_FILE        200

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(_MSC_VER)
#if defined (DLL_EXPORT) || defined(PIC) || defined(_USRDLL)
#define ZINT_EXTERN __declspec(dllexport)
#elif defined(ZINT_DLL)
#define ZINT_EXTERN __declspec(dllimport)
#else
#define ZINT_EXTERN extern
#endif
#else
#define ZINT_EXTERN extern
#endif

    ZINT_EXTERN struct zint_symbol *ZBarcode_Create(void);
    ZINT_EXTERN void ZBarcode_Clear(struct zint_symbol *symbol);
    ZINT_EXTERN void ZBarcode_Delete(struct zint_symbol *symbol);

    ZINT_EXTERN int ZBarcode_Encode(struct zint_symbol *symbol, const unsigned char *source, int in_length);
    ZINT_EXTERN int ZBarcode_Encode_File(struct zint_symbol *symbol, char *filename);
    ZINT_EXTERN int ZBarcode_Print(struct zint_symbol *symbol, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Encode_and_Print(struct zint_symbol *symbol, unsigned char *input, int length, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Encode_File_and_Print(struct zint_symbol *symbol, char *filename, int rotate_angle);

    ZINT_EXTERN int ZBarcode_Render(struct zint_symbol *symbol, const float width, const float height);

    ZINT_EXTERN int ZBarcode_Buffer(struct zint_symbol *symbol, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Buffer_Vector(struct zint_symbol *symbol, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Encode_and_Buffer(struct zint_symbol *symbol, unsigned char *input, int length, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Encode_and_Buffer_Vector(struct zint_symbol *symbol, unsigned char *input, int length, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Encode_File_and_Buffer(struct zint_symbol *symbol, char *filename, int rotate_angle);
    ZINT_EXTERN int ZBarcode_Encode_File_and_Buffer_Vector(struct zint_symbol *symbol, char *filename, int rotate_angle);

    ZINT_EXTERN int ZBarcode_ValidID(int symbol_id);
    ZINT_EXTERN int ZBarcode_Version();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZINT_H */

