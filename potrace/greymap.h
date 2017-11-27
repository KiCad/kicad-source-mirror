/* Copyright (C) 2001-2017 Peter Selinger.
 *  This file is part of Potrace. It is free software and it is covered
 *  by the GNU General Public License. See the file COPYING for details. */

#ifndef GREYMAP_H
#define GREYMAP_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* type for greymap samples */
typedef signed short int gm_sample_t;

/* internal format for greymaps. Note: in this format, rows are
 *  ordered from bottom to top. The pixels in each row are given from
 *  left to right. */
struct greymap_s
{
    int w;              /* width, in pixels */
    int h;              /* height, in pixels */
    int dy;             /* offset between scanlines (in samples);
                         *           can be negative */
    gm_sample_t* base;  /* root of allocated data */
    gm_sample_t* map;   /* points to the lower left pixel */
};
typedef struct greymap_s greymap_t;

/* macros for accessing pixel at index (x,y). Note that the origin is
 *  in the *lower* left corner. U* macros omit the bounds check. */

#define gm_scanline( gm, y )    ( ( gm )->map + (ptrdiff_t) ( y ) * (ptrdiff_t) ( gm )->dy )
#define gm_index( gm, x, y )    ( gm_scanline( gm, y ) + ( x ) )
#define gm_safe( gm, x, y ) \
    ( (int) ( x ) >= 0 && (int) ( x ) < ( gm )->w && (int) ( y ) >= 0 && (int) ( y ) < ( gm )->h )
#define gm_bound( x, m )        ( ( x ) < 0 ? 0 : ( x ) >= ( m ) ? (m) - 1 : ( x ) )
#define GM_UGET( gm, x, y )     ( *gm_index( gm, x, y ) )
#define GM_UINC( gm, x, y, b )  ( *gm_index( gm, x, y ) += (gm_sample_t) ( b ) )
#define GM_UINV( gm, x, y )     ( *gm_index( gm, x, y ) = 255 - *gm_index( gm, x, y ) )
#define GM_UPUT( gm, x, y, b )  ( *gm_index( gm, x, y ) = (gm_sample_t) ( b ) )
#define GM_GET( gm, x, y )      ( gm_safe( gm, x, y ) ? GM_UGET( gm, x, y ) : 0 )
#define GM_INC( gm, x, y, b )   ( gm_safe( gm, x, y ) ? GM_UINC( gm, x, y, b ) : 0 )
#define GM_INV( gm, x, y )      ( gm_safe( gm, x, y ) ? GM_UINV( gm, x, y ) : 0 )
#define GM_PUT( gm, x, y, b )   ( gm_safe( gm, x, y ) ? GM_UPUT( gm, x, y, b ) : 0 )
#define GM_BGET( gm, x, y )                                                         \
    ( ( gm )->w == 0 || ( gm )->h == 0 ? 0 : GM_UGET( gm, gm_bound( x, ( gm )->w ), \
              gm_bound( y, ( gm )->h ) ) )

/* modes for cutting off out-of-range values. The following names
 *  refer to winding numbers. I.e., make a pixel black if winding
 *  number is nonzero, odd, or positive, respectively. We assume that 0
 *  winding number corresponds to white (255). */
#define GM_MODE_NONZERO     1
#define GM_MODE_ODD         2
#define GM_MODE_POSITIVE    3
#define GM_MODE_NEGATIVE    4

extern const char* gm_read_error;

greymap_t*  gm_new( int w, int h );
greymap_t*  gm_dup( greymap_t* gm );
void        gm_free( greymap_t* gm );
void        gm_clear( greymap_t* gm, int b );
int         gm_read( FILE* f, greymap_t** gmp );
int         gm_writepgm( FILE* f,
        greymap_t* gm,
        const char* comment,
        int raw,
        int mode,
        double gamma );
int gm_print( FILE* f, greymap_t* gm );

#endif /* GREYMAP_H */
