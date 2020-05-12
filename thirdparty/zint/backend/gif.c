/* gif.c - Handles output to gif file */

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
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <math.h>
#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#endif

#define SSET	"0123456789ABCDEF"

/* Index of transparent color, -1 for no transparent color
 * This might be set into a variable if transparency is activated as an option
 */
#define TRANSPARENT_INDEX (-1)

/* Used bit depth, may be changed for bigger pallet in future */
#define DESTINATION_IMAGE_BITS 1
#include <stdlib.h>

typedef struct s_statestruct {
    unsigned char * pOut;
    unsigned char *pIn;
    unsigned int InLen;
    unsigned int OutLength;
    unsigned int OutPosCur;
    unsigned int OutByteCountPos;
    unsigned short ClearCode;
    unsigned short FreeCode;
    char fByteCountByteSet;
    unsigned char OutBitsFree;
    unsigned short NodeAxon[4096];
    unsigned short NodeNext[4096];
    unsigned char NodePix[4096];
} statestruct;

static char BufferNextByte(statestruct *pState) {
    (pState->OutPosCur)++;
    /* Check if this position is a byte count position
     * fg_f_bytecountbyte_set indicates, if byte count position bytes should be
     * inserted in general.
     * If this is true, and the distance to the last byte count position is 256
     * (e.g. 255 bytes in between), a byte count byte is inserted, and the value
     * of the last one is set to 255.
     * */
    if (pState->fByteCountByteSet && (pState->OutByteCountPos + 256 == pState->OutPosCur)) {
        (pState->pOut)[pState->OutByteCountPos] = 255;
        pState->OutByteCountPos = pState->OutPosCur;
        (pState->OutPosCur)++;
    }
    if (pState->OutPosCur >= pState->OutLength)
        return 1;
    (pState->pOut)[pState->OutPosCur] = 0x00;
    return 0;
}

static char AddCodeToBuffer(statestruct *pState, unsigned short CodeIn, unsigned char CodeBits) {
    /* Check, if we may fill up the current byte completely */
    if (CodeBits >= pState->OutBitsFree) {
        (pState->pOut)[pState->OutPosCur] |= (unsigned char)
                (CodeIn << (8 - pState->OutBitsFree));
        if (BufferNextByte(pState))
            return -1;
        CodeIn = (unsigned short) (CodeIn >> pState->OutBitsFree);
        CodeBits -= pState->OutBitsFree;
        pState->OutBitsFree = 8;
        /* Write a full byte if there are at least 8 code bits left */
        if (CodeBits >= pState->OutBitsFree) {
            (pState->pOut)[pState->OutPosCur] = (unsigned char) CodeIn;
            if (BufferNextByte(pState))
                return -1;
            CodeIn = (unsigned short) (CodeIn >> 8);
            CodeBits -= 8;
        }
    }
    /* The remaining bits of CodeIn fit in the current byte. */
    if (CodeBits > 0) {
        (pState->pOut)[pState->OutPosCur] |= (unsigned char)
                (CodeIn << (8 - pState->OutBitsFree));
        pState->OutBitsFree -= CodeBits;
    }
    return 0;
}

static void FlushStringTable(statestruct *pState) {
    unsigned short Pos;
    for (Pos = 0; Pos < pState->ClearCode; Pos++) {
        (pState->NodeAxon)[Pos] = 0;
    }
}

unsigned short FindPixelOutlet(statestruct *pState, unsigned short HeadNode, unsigned char Byte) {
    unsigned short Outlet;

    Outlet = (pState->NodeAxon)[HeadNode];
    while (Outlet) {
        if ((pState->NodePix)[Outlet] == Byte)
            return Outlet;
        Outlet = (pState->NodeNext)[Outlet];
    }
    return 0;
}

static char NextCode(statestruct *pState, unsigned char * pPixelValueCur, unsigned char CodeBits) {
    unsigned short UpNode;
    unsigned short DownNode;
    /* start with the root node for last pixel chain */
    UpNode = *pPixelValueCur;
    if ((pState->InLen) == 0)
        return AddCodeToBuffer(pState, UpNode, CodeBits);

    *pPixelValueCur = (*(pState->pIn)) - '0';
    (pState->pIn)++;
    (pState->InLen)--;
    /* Follow the string table and the data stream to the end of the longest string that has a code */
    while (0 != (DownNode = FindPixelOutlet(pState, UpNode, *pPixelValueCur))) {
        UpNode = DownNode;
        if ((pState->InLen) == 0)
            return AddCodeToBuffer(pState, UpNode, CodeBits);

        *pPixelValueCur = (*(pState->pIn)) - '0';
        (pState->pIn)++;
        (pState->InLen)--;
    }
    /* Submit 'UpNode' which is the code of the longest string */
    if (AddCodeToBuffer(pState, UpNode, CodeBits))
        return -1;
    /* ... and extend the string by appending 'PixelValueCur' */
    /* Create a successor node for 'PixelValueCur' whose code is 'freecode' */
    (pState->NodePix)[pState->FreeCode] = *pPixelValueCur;
    (pState->NodeAxon)[pState->FreeCode] = (pState->NodeNext)[pState->FreeCode] = 0;
    /* ...and link it to the end of the chain emanating from fg_axon[UpNode]. */
    DownNode = (pState->NodeAxon)[UpNode];
    if (!DownNode) {
        (pState->NodeAxon)[UpNode] = pState->FreeCode;
    } else {
        while ((pState->NodeNext)[DownNode]) {
            DownNode = (pState->NodeNext)[DownNode];
        }
        (pState->NodeNext)[DownNode] = pState->FreeCode;
    }
    return 1;
}

int gif_lzw(unsigned char *pOut, int OutLength, unsigned char *pIn, int InLen) {
    unsigned char PixelValueCur;
    unsigned char CodeBits;
    unsigned short Pos;
    statestruct State;

    State.pIn = pIn;
    State.InLen = InLen;
    State.pOut = pOut;
    State.OutLength = OutLength;
    // > Get first data byte
    if (State.InLen == 0)
        return 0;

    PixelValueCur = (unsigned char) ((*(State.pIn)) - '0');
    (State.pIn)++;
    (State.InLen)--;
    CodeBits = 3;
    State.ClearCode = 4;
    State.FreeCode = 6;
    State.OutBitsFree = 8;
    State.OutPosCur = -1;
    State.fByteCountByteSet = 0;

    if (BufferNextByte(&State))
        return 0;

    for (Pos = 0; Pos < State.ClearCode; Pos++)
        State.NodePix[Pos] = (unsigned char) Pos;

    FlushStringTable(&State);

    /* Write what the GIF specification calls the "code size". */
    (State.pOut)[State.OutPosCur] = 2;
    /* Reserve first bytecount byte */
    if (BufferNextByte(&State))
        return 0;
    State.OutByteCountPos = State.OutPosCur;
    if (BufferNextByte(&State))
        return 0;
    State.fByteCountByteSet = 1;
    /* Submit one 'ClearCode' as the first code */
    if (AddCodeToBuffer(&State, State.ClearCode, CodeBits))
        return 0;

    for (;;) {
        char Res;
        /* generate and save the next code, which may consist of multiple input pixels. */
        Res = NextCode(&State, &PixelValueCur, CodeBits);
        if (Res < 0)
            return 0;
        //* Check for end of data stream */
        if (!Res) {
            /* submit 'eoi' as the last item of the code stream */
            if (AddCodeToBuffer(&State, (unsigned short) (State.ClearCode + 1), CodeBits))
                return 0;
            State.fByteCountByteSet = 0;
            if (State.OutBitsFree < 8) {
                if (BufferNextByte(&State))
                    return 0;
            }
            // > Update last bytecount byte;
            if (State.OutByteCountPos < State.OutPosCur) {
                (State.pOut)[State.OutByteCountPos] = (unsigned char) (State.OutPosCur - State.OutByteCountPos - 1);
            }
            State.OutPosCur++;
            return State.OutPosCur;
        }
        /* Check for currently last code */
        if (State.FreeCode == (1U << CodeBits))
            CodeBits++;
        State.FreeCode++;
        /* Check for full stringtable */
        if (State.FreeCode == 0xfff) {
            FlushStringTable(&State);
            if (AddCodeToBuffer(&State, State.ClearCode, CodeBits))
                return 0;

            CodeBits = (unsigned char) (1 + 2);
            State.FreeCode = (unsigned short) (State.ClearCode + 2);
        }
    }
}

int gif_pixel_plot(struct zint_symbol *symbol, char *pixelbuf) {
    char outbuf[10];
    FILE *gif_file;
    unsigned short usTemp;
    int byte_out;
#ifdef _MSC_VER
    char * lzwoutbuf;
#endif

#ifndef _MSC_VER
    char lzwoutbuf[symbol->bitmap_height * symbol->bitmap_width];
#else
    lzwoutbuf = (char *) _alloca((symbol->bitmap_height * symbol->bitmap_width) * sizeof (char));
#endif /* _MSC_VER */

    /* Open output file in binary mode */
    if ((symbol->output_options & BARCODE_STDOUT) != 0) {
#ifdef _MSC_VER
        if (-1 == _setmode(_fileno(stdout), _O_BINARY)) {
            strcpy(symbol->errtxt, "610: Can't open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
#endif
        gif_file = stdout;
    } else {
        if (!(gif_file = fopen(symbol->outfile, "wb"))) {
            strcpy(symbol->errtxt, "611: Can't open output file");
            return ZINT_ERROR_FILE_ACCESS;
        }
    }
    /*ImageWidth = 2;
    ImageHeight = 2;
    rotated_bitmap[0] = 1;
    rotated_bitmap[1] = 1;
    rotated_bitmap[2] = 0;
    rotated_bitmap[3] = 0;
     */

    /* GIF signature (6) */
    memcpy(outbuf, "GIF87a", 6);
    if (TRANSPARENT_INDEX != -1)
        outbuf[4] = '9';
    fwrite(outbuf, 6, 1, gif_file);
    /* Screen Descriptor (7) */
    /* Screen Width */
    usTemp = (unsigned short) symbol->bitmap_width;
    outbuf[0] = (unsigned char) (0xff & usTemp);
    outbuf[1] = (unsigned char) ((0xff00 & usTemp) / 0x100);
    /* Screen Height */
    usTemp = (unsigned short) symbol->bitmap_height;
    outbuf[2] = (unsigned char) (0xff & usTemp);
    outbuf[3] = (unsigned char) ((0xff00 & usTemp) / 0x100);
    /* write ImageBits-1 to the three least significant bits of byte 5  of
     * the Screen Descriptor
     */
    outbuf[4] = (unsigned char) (0xf0 | (0x7 & (DESTINATION_IMAGE_BITS - 1)));
    /*  Background color = colortable index 0 */
    outbuf[5] = 0x00;
    /* Byte 7 must be 0x00  */
    outbuf[6] = 0x00;
    fwrite(outbuf, 7, 1, gif_file);
    /* Global Color Table (6) */
    /* RGB 0 color */
    outbuf[0] = (unsigned char) (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
    outbuf[1] = (unsigned char) (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
    outbuf[2] = (unsigned char) (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);
    /* RGB 1 color */
    outbuf[3] = (unsigned char) (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
    outbuf[4] = (unsigned char) (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
    outbuf[5] = (unsigned char) (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
    fwrite(outbuf, 6, 1, gif_file);

    /* Graphic control extension (8) */
    /* A graphic control extension block is used for overlay gifs.
     * This is necessary to define a transparent color.
     */
    if (TRANSPARENT_INDEX != -1) {
        /* Extension Introducer = '!' */
        outbuf[0] = '\x21';
        /* Graphic Control Label */
        outbuf[1] = '\xf9';
        /* Block Size */
        outbuf[2] = 4;
        /* Packet fields:
         * 3 Reserved
         * 3 Disposal Method: 0 No Action, 1 No Dispose, 2: Background, 3: Prev.
         * 1 User Input Flag: 0: no user input, 1: user input
         * 1 Transparent Color Flag: 0: No Transparency, 1: Transparency index
         */
        outbuf[3] = 1;
        /* Delay Time */
        outbuf[4] = 0;
        outbuf[5] = 0;
        /* Transparent Color Index */
        outbuf[6] = (unsigned char) TRANSPARENT_INDEX;
        /* Block Terminator */
        outbuf[7] = 0;
        fwrite(outbuf, 8, 1, gif_file);
    }
    /* Image Descriptor */
    /* Image separator character = ',' */
    outbuf[0] = 0x2c;
    /* "Image Left" */
    outbuf[1] = 0x00;
    outbuf[2] = 0x00;
    /* "Image Top" */
    outbuf[3] = 0x00;
    outbuf[4] = 0x00;
    /* Image Width (low byte first) */
    outbuf[5] = (unsigned char) (0xff & symbol->bitmap_width);
    outbuf[6] = (unsigned char) ((0xff00 & symbol->bitmap_width) / 0x100);
    /* Image Height */
    outbuf[7] = (unsigned char) (0xff & symbol->bitmap_height);
    outbuf[8] = (unsigned char) ((0xff00 & symbol->bitmap_height) / 0x100);

    /* Byte 10 contains the interlaced flag and
     * information on the local color table.
     * There is no local color table if its most significant bit is reset.
     */
    outbuf[9] = (unsigned char) (0 | (0x7 & (DESTINATION_IMAGE_BITS - 1)));
    fwrite(outbuf, 10, 1, gif_file);

    /* call lzw encoding */
    byte_out = gif_lzw(
            (unsigned char *) lzwoutbuf,
            symbol->bitmap_height * symbol->bitmap_width,
            (unsigned char *) pixelbuf,
            symbol->bitmap_height * symbol->bitmap_width);
    if (byte_out <= 0) {
        fclose(gif_file);
        return ZINT_ERROR_MEMORY;
    }
    fwrite(lzwoutbuf, byte_out, 1, gif_file);

    /* GIF terminator */
    fputc('\x3b', gif_file);
    fclose(gif_file);

    return 0;
}
