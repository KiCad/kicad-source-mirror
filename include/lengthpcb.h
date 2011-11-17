/**
 * @file lengthpcb.h
 * @brief Length definitions for PCBNEW.
 */

#ifndef LENGTHPCB_H_INCLUDED
#define LENGTHPCB_H_INCLUDED 1

#ifdef KICAD_NANOMETRE
#include "length.h"
#include "limited_int.h"

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

#ifdef __GNUC__
#define NOT_USED __attribute__ ((unused))
#else
#define NOT_USED
#endif

static LENGTH_PCB NOT_USED from_legacy_lu( int x )
{
    return x * PCB_LEGACY_UNIT( LENGTH_PCB );
}

static LENGTH_PCB NOT_USED from_legacy_lu( long x )
{
    return x * PCB_LEGACY_UNIT( LENGTH_PCB );
}

static LENGTH_PCB NOT_USED from_legacy_lu( double x )
{
    return LENGTH_PCB( x * PCB_LEGACY_UNIT( LENGTH_PCB_DBL ) );
}

static LENGTH_PCB_DBL NOT_USED from_legacy_lu_dbl( double x )
{
    return x * LENGTH_UNITS< LENGTH_PCB_DBL >::inch() / PCB_LEGACY_INCH_SCALE;
}

#undef NOT_USED

#define FROM_LEGACY_LU( x )     ( from_legacy_lu( x ) )
#define FROM_LEGACY_LU_DBL( x ) ( from_legacy_lu_dbl( x ) )

#define ZERO_LENGTH ( LENGTH_PCB::zero() )

/* SAVE FILE macros */
#define FM_LENSV "%lf" ///< format specifier for saving
#define ARG_LENSV( x ) TO_LEGACY_LU_DBL( x ) ///< argument for saving

#else

typedef int LENGTH_PCB;
typedef double LENGTH_PCB_DBL;

/* transition macro stubs */

#define TO_LEGACY_LU( x )               ( x )
#define TO_LEGACY_LU_DBL( x )   ( double( x ) )

#define FROM_LEGACY_LU( x )             ( x )
#define FROM_LEGACY_LU_DBL( x ) ( double( x ) )

#define ZERO_LENGTH 0


/* SAVE FILE macros */
/** @TODO: after transition process this could be wrapped in some class */
#define FM_LENSV "%d"
#define ARG_LENSV( x ) ( (int)( x ) )

#endif


/* LOAD FILE macros */
#define FM_LENLD "%lf" ///< scanf format macro for loading
#define ARG_LENLD_TYPE double ///< tmp variable for length read
// they're set to flat type to help the transition process
// there would be some period while program gain ability to
// load float data but not acually save it

#define LENGTH_LOAD_TMP( x ) ( FROM_LEGACY_LU( x ) ) ///< reads tmp value from above
#define LENGTH_LOAD_STR( s ) ( FROM_LEGACY_LU( atof( s ) ) ) ///< reads string

/* used fo loading/saving config */
#define LENGTH_MAX_CONFIG ( FROM_LEGACY_LU( 393700 ) )

#endif /* def LENGTHPCB_H_INCLUDED */
