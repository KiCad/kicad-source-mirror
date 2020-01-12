/* Copyright (C) 2001-2017 Peter Selinger.
 *  This file is part of Potrace. It is free software and it is covered
 *  by the GNU General Public License. See the file COPYING for details. */

#ifndef BITMAP_H
#define BITMAP_H

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* The bitmap type is defined in potracelib.h */
#include "potracelib.h"

/* The present file defines some convenient macros and static inline
 *  functions for accessing bitmaps. Since they only produce inline
 *  code, they can be conveniently shared by the library and frontends,
 *  if desired */

/* ---------------------------------------------------------------------- */
/* some measurements */

#define BM_WORDSIZE ( (int) sizeof( potrace_word ) )
#define BM_WORDBITS ( 8 * BM_WORDSIZE )
#define BM_HIBIT    ( ( (potrace_word) 1 ) << ( BM_WORDBITS - 1 ) )
#define BM_ALLBITS  ( ~(potrace_word) 0 )

/* macros for accessing pixel at index (x,y). U* macros omit the
 *  bounds check. */

#define bm_scanline( bm, y )    ( ( bm )->map + (ptrdiff_t) ( y ) * (ptrdiff_t) ( bm )->dy )
#define bm_index( bm, x, y )    ( &bm_scanline( bm, y )[( x ) / BM_WORDBITS] )
#define bm_mask( x )            ( BM_HIBIT >> ( ( x ) & ( BM_WORDBITS - 1 ) ) )
#define bm_range( x, a )        ( (int) ( x ) >= 0 && (int) ( x ) < ( a ) )
#define bm_safe( bm, x, y )     ( bm_range( x, ( bm )->w ) && bm_range( y, ( bm )->h ) )
#define BM_UGET( bm, x, y )     ( ( *bm_index( bm, x, y ) & bm_mask( x ) ) != 0 )
#define BM_USET( bm, x, y )     ( *bm_index( bm, x, y ) |= bm_mask( x ) )
#define BM_UCLR( bm, x, y )     ( *bm_index( bm, x, y ) &= ~bm_mask( x ) )
#define BM_UINV( bm, x, y )     ( *bm_index( bm, x, y ) ^= bm_mask( x ) )
#define BM_UPUT( bm, x, y, b )  ( ( b ) ? BM_USET( bm, x, y ) : BM_UCLR( bm, x, y ) )
#define BM_GET( bm, x, y )      ( bm_safe( bm, x, y ) ? BM_UGET( bm, x, y ) : 0 )
#define BM_SET( bm, x, y )      ( bm_safe( bm, x, y ) ? BM_USET( bm, x, y ) : 0 )
#define BM_CLR( bm, x, y )      ( bm_safe( bm, x, y ) ? BM_UCLR( bm, x, y ) : 0 )
#define BM_INV( bm, x, y )      ( bm_safe( bm, x, y ) ? BM_UINV( bm, x, y ) : 0 )
#define BM_PUT( bm, x, y, b )   ( bm_safe( bm, x, y ) ? BM_UPUT( bm, x, y, b ) : 0 )

/* calculate the size, in bytes, required for the data area of a
 *  bitmap of the given dy and h. Assume h >= 0. Return -1 if the size
 *  does not fit into the ptrdiff_t type. */
static inline ptrdiff_t getsize( int dy, int h )
{
    ptrdiff_t size;

    if( dy < 0 )
    {
        dy = -dy;
    }

    size = (ptrdiff_t) dy * (ptrdiff_t) h * (ptrdiff_t) BM_WORDSIZE;

    /* check for overflow error */
    if( size < 0 || ( h != 0 && dy != 0 && size / h / dy != BM_WORDSIZE ) )
    {
        return -1;
    }

    return size;
}


/* return the size, in bytes, of the data area of the bitmap. Return
 *  -1 if the size does not fit into the ptrdiff_t type; however, this
 *  cannot happen if the bitmap is well-formed, i.e., if created with
 *  bm_new or bm_dup. */
static inline ptrdiff_t bm_size( const potrace_bitmap_t* bm )
{
    return getsize( bm->dy, bm->h );
}


/* calculate the base address of the bitmap data. Assume that the
 *  bitmap is well-formed, i.e., its size fits into the ptrdiff_t type.
 *  This is the case if created with bm_new or bm_dup. The base address
 *  may differ from bm->map if dy is negative */
static inline potrace_word* bm_base( const potrace_bitmap_t* bm )
{
    int dy = bm->dy;

    if( dy >= 0 || bm->h == 0 )
    {
        return bm->map;
    }
    else
    {
        return bm_scanline( bm, bm->h - 1 );
    }
}


/* free the given bitmap. Leaves errno untouched. */
static inline void bm_free( potrace_bitmap_t* bm )
{
    if( bm && bm->map )
    {
        free( bm_base( bm ) );
    }

    free( bm );
}


/* return new bitmap initialized to 0. NULL with errno on error.
 *  Assumes w, h >= 0. */
static inline potrace_bitmap_t* bm_new( int w, int h )
{
    potrace_bitmap_t* bm;
    int dy = w == 0 ? 0 : ( w - 1 ) / BM_WORDBITS + 1;
    ptrdiff_t size;

    size = getsize( dy, h );

    if( size < 0 )
    {
        errno = ENOMEM;
        return NULL;
    }

    if( size == 0 )
    {
        size = 1;    /* make sure calloc() doesn't return NULL */
    }

    bm = (potrace_bitmap_t*) malloc( sizeof( potrace_bitmap_t ) );

    if( !bm )
    {
        return NULL;
    }

    bm->w   = w;
    bm->h   = h;
    bm->dy  = dy;
    bm->map = (potrace_word*) calloc( 1, size );

    if( !bm->map )
    {
        free( bm );
        return NULL;
    }

    return bm;
}


/* clear the given bitmap. Set all bits to c. Assumes a well-formed
 *  bitmap. */
static inline void bm_clear( potrace_bitmap_t* bm, int c )
{
    /* Note: if the bitmap was created with bm_new, then it is
     *  guaranteed that size will fit into the ptrdiff_t type. */
    ptrdiff_t size = bm_size( bm );

    if( size > 0 )
        memset( bm_base( bm ), c ? -1 : 0, size );
}


/* duplicate the given bitmap. Return NULL on error with errno
 *  set. Assumes a well-formed bitmap. */
static inline potrace_bitmap_t* bm_dup( const potrace_bitmap_t* bm )
{
    potrace_bitmap_t* bm1 = bm_new( bm->w, bm->h );
    int y;

    if( !bm1 )
    {
        return NULL;
    }

    for( y = 0; y < bm->h; y++ )
    {
        memcpy( bm_scanline( bm1, y ), bm_scanline( bm, y ),
                (size_t) bm1->dy * (size_t) BM_WORDSIZE );
    }

    return bm1;
}


/* invert the given bitmap. */
static inline void bm_invert( potrace_bitmap_t* bm )
{
    int dy = bm->dy;
    int y;
    int i;
    potrace_word* p;

    if( dy < 0 )
    {
        dy = -dy;
    }

    for( y = 0; y < bm->h; y++ )
    {
        p = bm_scanline( bm, y );

        for( i = 0; i < dy; i++ )
        {
            p[i] ^= BM_ALLBITS;
        }
    }
}


/* turn the given bitmap upside down. This does not move the bitmap
 *  data or change the bm_base() address. */
static inline void bm_flip( potrace_bitmap_t* bm )
{
    int dy = bm->dy;

    if( bm->h == 0 || bm->h == 1 )
    {
        return;
    }

    bm->map = bm_scanline( bm, bm->h - 1 );
    bm->dy  = -dy;
}


/* resize the bitmap to the given new height. The bitmap data remains
 *  bottom-aligned (truncated at the top) when dy >= 0 and top-aligned
 *  (truncated at the bottom) when dy < 0. Return 0 on success, or 1 on
 *  error with errno set. If the new height is <= the old one, no error
 *  should occur. If the new height is larger, the additional bitmap
 *  data is *not* initialized. */
static inline int bm_resize( potrace_bitmap_t* bm, int h )
{
    int dy = bm->dy;
    ptrdiff_t newsize;
    potrace_word* newmap;

    if( dy < 0 )
    {
        bm_flip( bm );
    }

    newsize = getsize( dy, h );

    if( newsize < 0 )
    {
        errno = ENOMEM;
        goto error;
    }

    if( newsize == 0 )
    {
        newsize = 1;    /* make sure realloc() doesn't return NULL */
    }

    newmap = (potrace_word*) realloc( bm->map, newsize );

    if( newmap == NULL )
    {
        goto error;
    }

    bm->map = newmap;
    bm->h = h;

    if( dy < 0 )
    {
        bm_flip( bm );
    }

    return 0;

error:

    if( dy < 0 )
    {
        bm_flip( bm );
    }

    return 1;
}


#endif /* BITMAP_H */
