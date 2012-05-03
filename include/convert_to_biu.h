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
        #define IU_PER_MM   1e5         // Gerbview uses 10 micrometer.
    #else
        #define IU_PER_MM   1e6         // Pcbnew uses nanometers.
    #endif
    #define IU_PER_MILS ( IU_PER_MM * 0.0254 )
    #define IU_PER_DECIMILS (IU_PER_MM * 0.00254 )
    #else              // Pcbnew in deci-mils.
        #define IU_PER_DECIMILS  1
        #define IU_PER_MILS   10.0
        #define IU_PER_MM   (1e4 / 25.4)
    #endif

/// Convert mils to PCBNEW internal units (iu).
inline int Mils2iu( int mils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    double x = mils * IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
#else
    return mils * IU_PER_MILS;
#endif
}

/// Convert deci-mils to PCBNEW internal units (iu).
inline int DMils2iu( int dmils )
{
#if defined( USE_PCBNEW_NANOMETRES )
    double x = dmils * IU_PER_DECIMILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
#else
    return dmils;
#endif
}

#else            // Eeschema and anything else.
#define IU_PER_DECIMILS  0.1
#define IU_PER_MILS   1.0
#define IU_PER_MM   (IU_PER_MILS / 0.0254)

inline int Mils2iu( int mils )
{
    return mils;
}
#endif

#endif  // #define CONVERT_TO_BIU_H
