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
  (     LENGTH_PCB( x ) / PCB_LEGACY_UNIT( LENGTH_PCB     ) )
#define TO_LEGACY_LU_DBL( x )  \
  ( LENGTH_PCB_DBL( x ) / PCB_LEGACY_UNIT( LENGTH_PCB_DBL ) )

static LENGTH_PCB from_legacy_lu( int x ) {
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


#else

typedef int LENGTH_PCB;
typedef double LENGTH_PCB_DBL;

/* transition macro stubs */

#define TO_LEGACY_LU( x )               ( x )
#define TO_LEGACY_LU_DBL( x )   ( double( x ) )

#define FROM_LEGACY_LU( x )             ( x )
#define FROM_LEGACY_LU_DBL( x ) ( double( x ) )

#endif


#endif /* def LENGTHPCB_H_INCLUDED */
