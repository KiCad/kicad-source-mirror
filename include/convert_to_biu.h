#ifndef CONVERT_TO_BIU_H
#define CONVERT_TO_BIU_H

#include <config.h>     // USE_PCBNEW_NANOMETRES is defined here

/**
 * @file convert_to_biu.h
 */


/**
 * @brief inline convert functions to convert a value in decimils (or mils)
 * to the internal unit used in pcbnew or cvpcb(nanometer or decimil)
 * depending on compil option
 */

/// Convert mils to PCBNEW internal units (iu).
inline int Mils2iu( int mils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    return int( mils * 25.4e3 + 0.5 );
#else
    return mils * 10;
#endif
}

/// Convert deci-mils to PCBNEW internal units (iu).
inline int DMils2iu( int dmils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    return int( dmils * 25.4e2 + 0.5 );
#else
    return dmils;
#endif
}

#endif  // #define CONVERT_TO_BIU_H

