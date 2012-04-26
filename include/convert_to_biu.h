#ifndef CONVERT_TO_BIU_H
#define CONVERT_TO_BIU_H

#include <config.h>     // USE_PCBNEW_NANOMETRES is defined here

/**
 * @file convert_to_biu.h
 */


/**
 * @brief some define and functions to convert a value in mils, decimils or mm
 * to the internal unit used in pcbnew, cvpcb or gerbview (nanometer or decimil)
 * depending on compil option
 */



/// Scaling factor to convert mils to internal units.
#if defined(PCBNEW) || defined(CVPCB) || defined(GERBVIEW)
    #if defined( USE_PCBNEW_NANOMETRES )
    #if defined(GERBVIEW)
        #define MM_PER_IU   1e5         // Gerbview uses 10 micrometer.
    #else
        #define MM_PER_IU   1e6         // Pcbnew uses nanometers.
    #endif
    #define MILS_PER_IU ( MM_PER_IU * 0.0254 )
    #define DECIMILS_PER_IU (MM_PER_IU * 0.00254 )
    #else              // Pcbnew in deci-mils.
        #define DECIMILS_PER_IU  1
        #define MILS_PER_IU   10.0
        #define MM_PER_IU   (1e4 / 25.4)
    #endif

/// Convert mils to PCBNEW internal units (iu).
inline int Mils2iu( int mils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    double x = mils * MILS_PER_IU;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
#else
    return mils * MILS_PER_IU;
#endif
}

/// Convert deci-mils to PCBNEW internal units (iu).
inline int DMils2iu( int dmils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    double x = dmils * DECIMILS_PER_IU;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
#else
    return dmils;
#endif
}

#else            // Eeschema and anything else.
#define MILS_PER_IU   1.0
#define MM_PER_IU   (MILS_PER_IU / 0.0254)

inline int Mils2iu( int mils )
{
    return mils;
}
#endif

#endif  // #define CONVERT_TO_BIU_H
