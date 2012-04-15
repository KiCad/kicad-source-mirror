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

#if defined(PCBNEW) || defined(CVPCB)
/// Convert mils to PCBNEW internal units (iu).
inline int Mils2iu( int mils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    double x = mils * 25.4e3;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
#else
    return mils * 10;
#endif
}

/// Convert deci-mils to PCBNEW internal units (iu).
inline int DMils2iu( int dmils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    double x = dmils * 25.4e2;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
#else
    return dmils;
#endif
}

#elif defined(EESCHEMA)
inline int Mils2iu( int mils )
{
    return mils;
}
#endif

#endif  // #define CONVERT_TO_BIU_H
