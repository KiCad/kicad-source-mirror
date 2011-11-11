/**
 * @file lengthpcb.h
 * @brief Length definitions for PCBNEW.
 */

#ifndef LENGTHPCB_H_INCLUDED
#define LENGTHPCB_H_INCLUDED 1

#ifdef KICAD_NANOMETRE
#include "length.h"

/* switched type! */
typedef LENGTH< LIMITED_INT< int >, 1 > LENGTH_PCB;
typedef LENGTH< double, 1 > LENGTH_PCB_DBL;

/* Transition macros. they are used for unit conversion between
 * nanometre scale and old (0.1 mil) scale */

#define PCB_LEGACY_INCH_SCALE 10000
#define PCB_LEGACY_UNIT( T ) ( LENGTH_UNITS< T >::inch() / PCB_LEGACY_INCH_SCALE )

#define TO_LEGACY_LU( x )      \
  (    (int) (     LENGTH_PCB( x ) / PCB_LEGACY_UNIT( LENGTH_PCB     ) ) )
#define TO_LEGACY_LU_DBL( x )  \
  ( (double) ( LENGTH_PCB_DBL( x ) / PCB_LEGACY_UNIT( LENGTH_PCB_DBL ) ) )

static LENGTH_PCB from_legacy_lu( int x ) {
	return x * PCB_LEGACY_UNIT( LENGTH_PCB );
}
static LENGTH_PCB from_legacy_lu( long x ) {
	return x * PCB_LEGACY_UNIT( LENGTH_PCB );
}
static LENGTH_PCB from_legacy_lu( double x ) {
	return LENGTH_PCB( x * PCB_LEGACY_UNIT( LENGTH_PCB_DBL ) );
}

static LENGTH_PCB_DBL from_legacy_lu_dbl( double x ) {
	return x * LENGTH_UNITS< LENGTH_PCB_DBL >::inch() / PCB_LEGACY_INCH_SCALE;
}

#define FROM_LEGACY_LU( x )     ( from_legacy_lu( x ) )
#define FROM_LEGACY_LU_DBL( x ) ( from_legacy_lu_dbl( x ) )

#define ZERO_LENGTH ( LENGTH_PCB::zero() )



#else

typedef int LENGTH_PCB;
typedef double LENGTH_PCB_DBL;

/* transition macro stubs */

#define TO_LEGACY_LU( x )               ( x )
#define TO_LEGACY_LU_DBL( x )   ( double( x ) )

#define FROM_LEGACY_LU( x )             ( x )
#define FROM_LEGACY_LU_DBL( x ) ( double( x ) )

#define ZERO_LENGTH 0

#endif

/// @TODO: nice template and refiling for it
struct VECTOR_PCB
{
    LENGTH_PCB x, y;
    VECTOR_PCB()
    {
    }
    VECTOR_PCB( LENGTH_PCB ax, LENGTH_PCB ay ): x( ax ), y( ay )
    {
    }
    bool operator == ( const VECTOR_PCB &a ) const {
        return x == a.x && y == a.y;
    }
    bool operator != ( const VECTOR_PCB &a ) const {
        return x != a.x || y != a.y;
    }
    VECTOR_PCB & operator = (const VECTOR_PCB &a ) {
        x = a.x;
        y = a.y;
        return *this;
    }
    VECTOR_PCB operator + (VECTOR_PCB add) const {
        return VECTOR_PCB(x + add.x, y + add.y);
    }
    VECTOR_PCB operator - (VECTOR_PCB add) const {
        return VECTOR_PCB(x - add.x, y - add.y);
    }
    VECTOR_PCB operator * (int factor) const {
        return VECTOR_PCB(x * factor, y * factor);
    }
    VECTOR_PCB operator / (int factor) const {
        return VECTOR_PCB(x / factor, y / factor);
    }
};

#define TO_LEGACY_LU_WXP( p ) ( wxPoint( TO_LEGACY_LU( ( p ).x ), TO_LEGACY_LU( ( p ).y ) ) )
#define TO_LEGACY_LU_WXS( p ) ( wxSize( TO_LEGACY_LU( ( p ).x ), TO_LEGACY_LU( ( p ).y ) ) )


#endif /* def LENGTHPCB_H_INCLUDED */
