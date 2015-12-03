/* Copyright (C) 2001-2015 Peter Selinger.
 *  This file is part of Potrace. It is free software and it is covered
 *  by the GNU General Public License. See the file COPYING for details. */


/* Routines for manipulating bitmaps, including reading pbm files. */

#include <stdio.h>

#include "bitmap.h"
#include "bitops.h"

#define INTBITS ( 8 * sizeof(int) )

static int  bm_readbody_bmp( FILE* f, double threshold, potrace_bitmap_t** bmp );
static int  bm_readbody_pnm( FILE* f, double threshold, potrace_bitmap_t** bmp, int magic );

/* ---------------------------------------------------------------------- */
/* routines for reading pnm streams */

/* read next character after whitespace and comments. Return EOF on
 *  end of file or error. */
static int fgetc_ws( FILE* f )
{
    int c;

    while( 1 )
    {
        c = fgetc( f );

        if( c=='#' )
        {
            while( 1 )
            {
                c = fgetc( f );

                if( c=='\n' || c==EOF )
                {
                    break;
                }
            }
        }

        /* space, tab, line feed, carriage return, form-feed */
        if( c!=' ' && c!='\t' && c!='\r' && c!='\n' && c!=12 )
        {
            return c;
        }
    }
}


/* skip whitespace and comments, then read a non-negative decimal
 *  number from a stream. Return -1 on EOF. Tolerate other errors (skip
 *  bad characters). Do not the read any characters following the
 *  number (put next character back into the stream) */

static int readnum( FILE* f )
{
    int c;
    int acc;

    /* skip whitespace and comments */
    while( 1 )
    {
        c = fgetc_ws( f );

        if( c==EOF )
        {
            return -1;
        }

        if( c>='0' && c<='9' )
        {
            break;
        }
    }

    /* first digit is already in c */
    acc = c - '0';

    while( 1 )
    {
        c = fgetc( f );

        if( c==EOF )
        {
            break;
        }

        if( c<'0' || c>'9' )
        {
            ungetc( c, f );
            break;
        }

        acc *= 10;
        acc += c - '0';
    }

    return acc;
}


/* similar to readnum, but read only a single 0 or 1, and do not read
 *  any characters after it. */

static int readbit( FILE* f )
{
    int c;

    /* skip whitespace and comments */
    while( 1 )
    {
        c = fgetc_ws( f );

        if( c==EOF )
        {
            return -1;
        }

        if( c>='0' && c<='1' )
        {
            break;
        }
    }

    return c - '0';
}


/* ---------------------------------------------------------------------- */

/* read a PNM stream: P1-P6 format (see pnm(5)), or a BMP stream, and
 *  convert the output to a bitmap. Return bitmap in *bmp. Return 0 on
 *  success, -1 on error with errno set, -2 on bad file format (with
 *  error message in bm_read_error), and 1 on premature end of file, -3
 *  on empty file (including files which contain only whitespace and
 *  comments), -4 if wrong magic number. If the return value is >=0,
 * bmp is valid. */

const char* bm_read_error = NULL;

int bm_read( FILE* f, double threshold, potrace_bitmap_t** bmp )
{
    int magic[2];

    /* read magic number. We ignore whitespace and comments before the
     *  magic, for the benefit of concatenated files in P1-P3 format.
     *  Multiple P1-P3 images in a single file are not formally allowed
     *  by the PNM standard, but there is no harm in being lenient. */

    magic[0] = fgetc_ws( f );

    if( magic[0] == EOF )
    {
        return -3;
    }

    magic[1] = fgetc( f );

    if( magic[0] == 'P' && magic[1] >= '1' && magic[1] <= '6' )
    {
        return bm_readbody_pnm( f, threshold, bmp, magic[1] );
    }

    if( magic[0] == 'B' && magic[1] == 'M' )
    {
        return bm_readbody_bmp( f, threshold, bmp );
    }

    return -4;
}


/* ---------------------------------------------------------------------- */
/* read PNM format */

/* read PNM stream after magic number. Return values as for bm_read */
static int bm_readbody_pnm( FILE* f, double threshold, potrace_bitmap_t** bmp, int magic )
{
    potrace_bitmap_t* bm;
    int x, y, i, b, b1, sum;
    int bpr; /* bytes per row (as opposed to 4*bm->c) */
    int w, h, max;

    bm = NULL;

    w = readnum( f );

    if( w<0 )
    {
        goto format_error;
    }

    h = readnum( f );

    if( h<0 )
    {
        goto format_error;
    }

    /* allocate bitmap */
    bm = bm_new( w, h );

    if( !bm )
    {
        return -1;
    }

    /* zero it out */
    bm_clear( bm, 0 );

    switch( magic )
    {
    default:
        /* not reached */
        goto format_error;

    case '1':
        /* read P1 format: PBM ascii */

        for( y = h - 1; y>=0; y-- )
        {
            for( x = 0; x<w; x++ )
            {
                b = readbit( f );

                if( b<0 )
                {
                    goto eof;
                }

                BM_UPUT( bm, x, y, b );
            }
        }

        break;

    case '2':
        /* read P2 format: PGM ascii */

        max = readnum( f );

        if( max<1 )
        {
            goto format_error;
        }

        for( y = h - 1; y>=0; y-- )
        {
            for( x = 0; x<w; x++ )
            {
                b = readnum( f );

                if( b<0 )
                {
                    goto eof;
                }

                BM_UPUT( bm, x, y, b > threshold * max ? 0 : 1 );
            }
        }

        break;

    case '3':
        /* read P3 format: PPM ascii */

        max = readnum( f );

        if( max<1 )
        {
            goto format_error;
        }

        for( y = h - 1; y>=0; y-- )
        {
            for( x = 0; x<w; x++ )
            {
                sum = 0;

                for( i = 0; i<3; i++ )
                {
                    b = readnum( f );

                    if( b<0 )
                    {
                        goto eof;
                    }

                    sum += b;
                }

                BM_UPUT( bm, x, y, sum > 3 * threshold * max ? 0 : 1 );
            }
        }

        break;

    case '4':
        /* read P4 format: PBM raw */

        b = fgetc( f ); /* read single white-space character after height */

        if( b==EOF )
        {
            goto format_error;
        }

        bpr = (w + 7) / 8;

        for( y = h - 1; y>=0; y-- )
        {
            for( i = 0; i<bpr; i++ )
            {
                b = fgetc( f );

                if( b==EOF )
                {
                    goto eof;
                }

                *bm_index( bm, i * 8,
                        y ) |= ( (potrace_word) b ) <<
                               ( 8 * ( BM_WORDSIZE - 1 - (i % BM_WORDSIZE) ) );
            }
        }

        break;

    case '5':
        /* read P5 format: PGM raw */

        max = readnum( f );

        if( max<1 )
        {
            goto format_error;
        }

        b = fgetc( f ); /* read single white-space character after max */

        if( b==EOF )
        {
            goto format_error;
        }

        for( y = h - 1; y>=0; y-- )
        {
            for( x = 0; x<w; x++ )
            {
                b = fgetc( f );

                if( b==EOF )
                    goto eof;

                if( max>=256 )
                {
                    b   <<= 8;
                    b1  = fgetc( f );

                    if( b1==EOF )
                        goto eof;

                    b |= b1;
                }

                BM_UPUT( bm, x, y, b > threshold * max ? 0 : 1 );
            }
        }

        break;

    case '6':
        /* read P6 format: PPM raw */

        max = readnum( f );

        if( max<1 )
        {
            goto format_error;
        }

        b = fgetc( f ); /* read single white-space character after max */

        if( b==EOF )
        {
            goto format_error;
        }

        for( y = h - 1; y>=0; y-- )
        {
            for( x = 0; x<w; x++ )
            {
                sum = 0;

                for( i = 0; i<3; i++ )
                {
                    b = fgetc( f );

                    if( b==EOF )
                    {
                        goto eof;
                    }

                    if( max>=256 )
                    {
                        b   <<= 8;
                        b1  = fgetc( f );

                        if( b1==EOF )
                            goto eof;

                        b |= b1;
                    }

                    sum += b;
                }

                BM_UPUT( bm, x, y, sum > 3 * threshold * max ? 0 : 1 );
            }
        }

        break;
    }

    *bmp = bm;
    return 0;

eof:
    *bmp = bm;
    return 1;

format_error:
    bm_free( bm );

    if( magic == '1' || magic == '4' )
    {
        bm_read_error = "invalid pbm file";
    }
    else if( magic == '2' || magic == '5' )
    {
        bm_read_error = "invalid pgm file";
    }
    else
    {
        bm_read_error = "invalid ppm file";
    }

    return -2;
}


/* ---------------------------------------------------------------------- */
/* read BMP format */

struct bmp_info_s
{
    unsigned int    FileSize;
    unsigned int    reserved;
    unsigned int    DataOffset;
    unsigned int    InfoSize;
    unsigned int    w;          /* width */
    unsigned int    h;          /* height */
    unsigned int    Planes;
    unsigned int    bits;       /* bits per sample */
    unsigned int    comp;       /* compression mode */
    unsigned int    ImageSize;
    unsigned int    XpixelsPerM;
    unsigned int    YpixelsPerM;
    unsigned int    ncolors;   /* number of colors in palette */
    unsigned int    ColorsImportant;
    unsigned int    RedMask;
    unsigned int    GreenMask;
    unsigned int    BlueMask;
    unsigned int    AlphaMask;
    unsigned int    ctbits;     /* sample size for color table */
    int topdown;                /* top-down mode? */
};
typedef struct bmp_info_s bmp_info_t;

/* auxiliary */

static int  bmp_count = 0;  /* counter for byte padding */
static int  bmp_pos = 0;    /* counter from start of BMP data */

/* read n-byte little-endian integer. Return 1 on EOF or error, else
 *  0. Assume n<=4. */
static int bmp_readint( FILE* f, int n, unsigned int* p )
{
    int i;
    unsigned int sum = 0;
    int b;

    for( i = 0; i<n; i++ )
    {
        b = fgetc( f );

        if( b==EOF )
        {
            return 1;
        }

        sum += b << (8 * i);
    }

    bmp_count += n;
    bmp_pos += n;
    *p = sum;
    return 0;
}


/* reset padding boundary */
static void bmp_pad_reset( void )
{
    bmp_count = 0;
}


/* read padding bytes to 4-byte boundary. Return 1 on EOF or error,
 *  else 0. */
static int bmp_pad( FILE* f )
{
    int c, i, b;

    c = (-bmp_count) & 3;

    for( i = 0; i<c; i++ )
    {
        b = fgetc( f );

        if( b==EOF )
        {
            return 1;
        }
    }

    bmp_pos += c;
    bmp_count = 0;
    return 0;
}


/* forward to the new file position. Return 1 on EOF or error, else 0 */
static int bmp_forward( FILE* f, int pos )
{
    int b;

    while( bmp_pos < pos )
    {
        b = fgetc( f );

        if( b==EOF )
        {
            return 1;
        }

        bmp_pos++;
        bmp_count++;
    }

    return 0;
}


#define TRY( x )        if( x ) \
        goto try_error
#define TRY_EOF( x )    if( x ) \
        goto eof

/* correct y-coordinate for top-down format */
#define ycorr( y ) (bmpinfo.topdown ? bmpinfo.h - 1 - y : y)

/* read BMP stream after magic number. Return values as for bm_read.
 *  We choose to be as permissive as possible, since there are many
 *  programs out there which produce BMP. For instance, ppmtobmp can
 *  produce codings with anywhere from 1-8 or 24 bits per sample,
 *  although most specifications only allow 1,4,8,24,32. We can also
 *  read both the old and new OS/2 BMP formats in addition to the
 *  Windows BMP format. */
static int bm_readbody_bmp( FILE* f, double threshold, potrace_bitmap_t** bmp )
{
    bmp_info_t bmpinfo;
    int* coltable;
    unsigned int    b, c;
    unsigned int    i;
    potrace_bitmap_t* bm;
    int mask;
    unsigned int x, y;
    int col[2];
    unsigned int    bitbuf;
    unsigned int    n;
    unsigned int    redshift, greenshift, blueshift;
    int col1[2];

    bm_read_error = NULL;
    bm = NULL;
    coltable = NULL;

    bmp_pos = 2; /* set file position */

    /* file header (minus magic number) */
    TRY( bmp_readint( f, 4, &bmpinfo.FileSize ) );
    TRY( bmp_readint( f, 4, &bmpinfo.reserved ) );
    TRY( bmp_readint( f, 4, &bmpinfo.DataOffset ) );

    /* info header */
    TRY( bmp_readint( f, 4, &bmpinfo.InfoSize ) );

    if( bmpinfo.InfoSize == 40 || bmpinfo.InfoSize == 64
        || bmpinfo.InfoSize == 108 || bmpinfo.InfoSize == 124 )
    {
        /* Windows or new OS/2 format */
        bmpinfo.ctbits = 32; /* sample size in color table */
        TRY( bmp_readint( f, 4, &bmpinfo.w ) );
        TRY( bmp_readint( f, 4, &bmpinfo.h ) );
        TRY( bmp_readint( f, 2, &bmpinfo.Planes ) );
        TRY( bmp_readint( f, 2, &bmpinfo.bits ) );
        TRY( bmp_readint( f, 4, &bmpinfo.comp ) );
        TRY( bmp_readint( f, 4, &bmpinfo.ImageSize ) );
        TRY( bmp_readint( f, 4, &bmpinfo.XpixelsPerM ) );
        TRY( bmp_readint( f, 4, &bmpinfo.YpixelsPerM ) );
        TRY( bmp_readint( f, 4, &bmpinfo.ncolors ) );
        TRY( bmp_readint( f, 4, &bmpinfo.ColorsImportant ) );

        if( bmpinfo.InfoSize >= 108 ) /* V4 and V5 bitmaps */
        {
            TRY( bmp_readint( f, 4, &bmpinfo.RedMask ) );
            TRY( bmp_readint( f, 4, &bmpinfo.GreenMask ) );
            TRY( bmp_readint( f, 4, &bmpinfo.BlueMask ) );
            TRY( bmp_readint( f, 4, &bmpinfo.AlphaMask ) );
        }

        if( bmpinfo.w > 0x7fffffff )
        {
            goto format_error;
        }

        if( bmpinfo.h > 0x7fffffff )
        {
            bmpinfo.h = (-bmpinfo.h) & 0xffffffff;
            bmpinfo.topdown = 1;
        }
        else
        {
            bmpinfo.topdown = 0;
        }

        if( bmpinfo.h > 0x7fffffff )
        {
            goto format_error;
        }
    }
    else if( bmpinfo.InfoSize == 12 )
    {
        /* old OS/2 format */
        bmpinfo.ctbits = 24; /* sample size in color table */
        TRY( bmp_readint( f, 2, &bmpinfo.w ) );
        TRY( bmp_readint( f, 2, &bmpinfo.h ) );
        TRY( bmp_readint( f, 2, &bmpinfo.Planes ) );
        TRY( bmp_readint( f, 2, &bmpinfo.bits ) );
        bmpinfo.comp = 0;
        bmpinfo.ncolors = 0;
        bmpinfo.topdown = 0;
    }
    else
    {
        goto format_error;
    }

    if( bmpinfo.comp == 3 && bmpinfo.InfoSize < 108 )
    {
        /* bitfield feature is only understood with V4 and V5 format */
        goto format_error;
    }

    /* forward to color table (e.g., if bmpinfo.InfoSize == 64) */
    TRY( bmp_forward( f, 14 + bmpinfo.InfoSize ) );

    if( bmpinfo.Planes != 1 )
    {
        bm_read_error = "cannot handle bmp planes";
        goto format_error; /* can't handle planes */
    }

    if( bmpinfo.ncolors == 0 )
    {
        bmpinfo.ncolors = 1 << bmpinfo.bits;
    }

    /* color table, present only if bmpinfo.bits <= 8. */
    if( bmpinfo.bits <= 8 )
    {
        coltable = (int*) calloc( bmpinfo.ncolors, sizeof(int) );

        if( !coltable )
        {
            goto std_error;
        }

        /* NOTE: since we are reading a bitmap, we can immediately convert
         *  the color table entries to bits. */
        for( i = 0; i<bmpinfo.ncolors; i++ )
        {
            TRY( bmp_readint( f, bmpinfo.ctbits / 8, &c ) );
            c = ( (c >> 16) & 0xff ) + ( (c >> 8) & 0xff ) + (c & 0xff);
            coltable[i] = (c > 3 * threshold * 255 ? 0 : 1);

            if( i<2 )
            {
                col1[i] = c;
            }
        }
    }

    /* forward to data */
    if( bmpinfo.InfoSize != 12 ) /* not old OS/2 format */
    {
        TRY( bmp_forward( f, bmpinfo.DataOffset ) );
    }

    /* allocate bitmap */
    bm = bm_new( bmpinfo.w, bmpinfo.h );

    if( !bm )
    {
        goto std_error;
    }

    /* zero it out */
    bm_clear( bm, 0 );

    switch( bmpinfo.bits + 0x100 * bmpinfo.comp )
    {
    default:
        goto format_error;
        break;

    case 0x001:                 /* monochrome palette */

        if( col1[0] < col1[1] ) /* make the darker color black */
        {
            mask = 0xff;
        }
        else
        {
            mask = 0;
        }

        /* raster data */
        for( y = 0; y<bmpinfo.h; y++ )
        {
            bmp_pad_reset();

            for( i = 0; 8 * i<bmpinfo.w; i++ )
            {
                TRY_EOF( bmp_readint( f, 1, &b ) );
                b ^= mask;
                *bm_index( bm, i * 8,
                        ycorr( y ) ) |= ( (potrace_word) b ) <<
                                        ( 8 * ( BM_WORDSIZE - 1 - (i % BM_WORDSIZE) ) );
            }

            TRY( bmp_pad( f ) );
        }

        break;

    case 0x002: /* 2-bit to 8-bit palettes */
    case 0x003:
    case 0x004:
    case 0x005:
    case 0x006:
    case 0x007:
    case 0x008:

        for( y = 0; y<bmpinfo.h; y++ )
        {
            bmp_pad_reset();
            bitbuf = 0; /* bit buffer: bits in buffer are high-aligned */
            n = 0;      /* number of bits currently in bitbuffer */

            for( x = 0; x<bmpinfo.w; x++ )
            {
                if( n < bmpinfo.bits )
                {
                    TRY_EOF( bmp_readint( f, 1, &b ) );
                    bitbuf |= b << (INTBITS - 8 - n);
                    n += 8;
                }

                b = bitbuf >> (INTBITS - bmpinfo.bits);
                bitbuf <<= bmpinfo.bits;
                n -= bmpinfo.bits;
                BM_UPUT( bm, x, ycorr( y ), coltable[b] );
            }

            TRY( bmp_pad( f ) );
        }

        break;

    case 0x010: /* 16-bit encoding */
        /* can't do this format because it is not well-documented and I
         *  don't have any samples */
        bm_read_error = "cannot handle bmp 16-bit coding";
        goto format_error;
        break;

    case 0x018: /* 24-bit encoding */
    case 0x020: /* 32-bit encoding */

        for( y = 0; y<bmpinfo.h; y++ )
        {
            bmp_pad_reset();

            for( x = 0; x<bmpinfo.w; x++ )
            {
                TRY_EOF( bmp_readint( f, bmpinfo.bits / 8, &c ) );
                c = ( (c >> 16) & 0xff ) + ( (c >> 8) & 0xff ) + (c & 0xff);
                BM_UPUT( bm, x, ycorr( y ), c > 3 * threshold * 255 ? 0 : 1 );
            }

            TRY( bmp_pad( f ) );
        }

        break;

    case 0x320: /* 32-bit encoding with bitfields */
        redshift = lobit( bmpinfo.RedMask );
        greenshift  = lobit( bmpinfo.GreenMask );
        blueshift   = lobit( bmpinfo.BlueMask );

        for( y = 0; y<bmpinfo.h; y++ )
        {
            bmp_pad_reset();

            for( x = 0; x<bmpinfo.w; x++ )
            {
                TRY_EOF( bmp_readint( f, bmpinfo.bits / 8, &c ) );
                c = ( (c & bmpinfo.RedMask) >> redshift ) +
                    ( (c & bmpinfo.GreenMask) >> greenshift ) +
                    ( (c & bmpinfo.BlueMask) >> blueshift );
                BM_UPUT( bm, x, ycorr( y ), c > 3 * threshold * 255 ? 0 : 1 );
            }

            TRY( bmp_pad( f ) );
        }

        break;

    case 0x204: /* 4-bit runlength compressed encoding (RLE4) */
        x   = 0;
        y   = 0;

        while( 1 )
        {
            TRY_EOF( bmp_readint( f, 1, &b ) ); /* opcode */
            TRY_EOF( bmp_readint( f, 1, &c ) ); /* argument */

            if( b>0 )
            {
                /* repeat count */
                col[0]  = coltable[(c >> 4) & 0xf];
                col[1]  = coltable[c & 0xf];

                for( i = 0; i<b && x<bmpinfo.w; i++ )
                {
                    if( x>=bmpinfo.w )
                    {
                        x = 0;
                        y++;
                    }

                    if( y>=bmpinfo.h )
                    {
                        break;
                    }

                    BM_UPUT( bm, x, ycorr( y ), col[i & 1] );
                    x++;
                }
            }
            else if( c == 0 )
            {
                /* end of line */
                y++;
                x = 0;
            }
            else if( c == 1 )
            {
                /* end of bitmap */
                break;
            }
            else if( c == 2 )
            {
                /* "delta": skip pixels in x and y directions */
                TRY_EOF( bmp_readint( f, 1, &b ) ); /* x offset */
                TRY_EOF( bmp_readint( f, 1, &c ) ); /* y offset */
                x   += b;
                y   += c;
            }
            else
            {
                /* verbatim segment */
                for( i = 0; i<c; i++ )
                {
                    if( (i & 1)==0 )
                    {
                        TRY_EOF( bmp_readint( f, 1, &b ) );
                    }

                    if( x>=bmpinfo.w )
                    {
                        x = 0;
                        y++;
                    }

                    if( y>=bmpinfo.h )
                    {
                        break;
                    }

                    BM_PUT( bm, x, ycorr( y ), coltable[( b >> ( 4 - 4 * (i & 1) ) ) & 0xf] );
                    x++;
                }

                if( (c + 1) & 2 )
                {
                    /* pad to 16-bit boundary */
                    TRY_EOF( bmp_readint( f, 1, &b ) );
                }
            }
        }

        break;

    case 0x108: /* 8-bit runlength compressed encoding (RLE8) */
        x   = 0;
        y   = 0;

        while( 1 )
        {
            TRY_EOF( bmp_readint( f, 1, &b ) ); /* opcode */
            TRY_EOF( bmp_readint( f, 1, &c ) ); /* argument */

            if( b>0 )
            {
                /* repeat count */
                for( i = 0; i<b; i++ )
                {
                    if( x>=bmpinfo.w )
                    {
                        x = 0;
                        y++;
                    }

                    if( y>=bmpinfo.h )
                    {
                        break;
                    }

                    BM_UPUT( bm, x, ycorr( y ), coltable[c] );
                    x++;
                }
            }
            else if( c == 0 )
            {
                /* end of line */
                y++;
                x = 0;
            }
            else if( c == 1 )
            {
                /* end of bitmap */
                break;
            }
            else if( c == 2 )
            {
                /* "delta": skip pixels in x and y directions */
                TRY_EOF( bmp_readint( f, 1, &b ) ); /* x offset */
                TRY_EOF( bmp_readint( f, 1, &c ) ); /* y offset */
                x   += b;
                y   += c;
            }
            else
            {
                /* verbatim segment */
                for( i = 0; i<c; i++ )
                {
                    TRY_EOF( bmp_readint( f, 1, &b ) );

                    if( x>=bmpinfo.w )
                    {
                        x = 0;
                        y++;
                    }

                    if( y>=bmpinfo.h )
                    {
                        break;
                    }

                    BM_PUT( bm, x, ycorr( y ), coltable[b] );
                    x++;
                }

                if( c & 1 )
                {
                    /* pad input to 16-bit boundary */
                    TRY_EOF( bmp_readint( f, 1, &b ) );
                }
            }
        }

        break;
    } /* switch */

    /* skip any potential junk after the data section, but don't
     *  complain in case EOF is encountered */
    bmp_forward( f, bmpinfo.FileSize );

    free( coltable );
    *bmp = bm;
    return 0;

eof:
    free( coltable );
    *bmp = bm;
    return 1;

format_error:
try_error:
    free( coltable );
    free( bm );

    if( !bm_read_error )
    {
        bm_read_error = "invalid bmp file";
    }

    return -2;

std_error:
    free( coltable );
    free( bm );
    return -1;
}


/* ---------------------------------------------------------------------- */
/* output pbm format */

void bm_writepbm( FILE* f, potrace_bitmap_t* bm )
{
    int w, h, bpr, y, i, c;

    w   = bm->w;
    h   = bm->h;

    bpr = (w + 7) / 8;

    fprintf( f, "P4\n%d %d\n", w, h );

    for( y = h - 1; y>=0; y-- )
    {
        for( i = 0; i<bpr; i++ )
        {
            c =
                ( *bm_index( bm, i * 8,
                          y ) >> ( 8 * ( BM_WORDSIZE - 1 - (i % BM_WORDSIZE) ) ) ) & 0xff;
            fputc( c, f );
        }
    }
}


/* ---------------------------------------------------------------------- */
/* output - for primitive debugging purposes only! */

/* print bitmap to screen */
int bm_print( FILE* f, potrace_bitmap_t* bm )
{
    int x, y;
    int xx, yy;
    int d;
    int sw, sh;

    sw  = bm->w < 79 ? bm->w : 79;
    sh  = bm->w < 79 ? bm->h : bm->h * sw * 44 / (79 * bm->w);

    for( yy = sh - 1; yy>=0; yy-- )
    {
        for( xx = 0; xx<sw; xx++ )
        {
            d = 0;

            for( x = xx * bm->w / sw; x<(xx + 1) * bm->w / sw; x++ )
            {
                for( y = yy * bm->h / sh; y<(yy + 1) * bm->h / sh; y++ )
                {
                    if( BM_GET( bm, x, y ) )
                    {
                        d++;
                    }
                }
            }

            fputc( d ? '*' : ' ', f );
        }

        fputc( '\n', f );
    }

    return 0;
}
