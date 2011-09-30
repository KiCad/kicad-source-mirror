/* Date for KiCad build version */
#include "fctsys.h"

#ifdef HAVE_SVN_VERSION
#include "version.h"    // define the KICAD_BUILD_VERSION
#endif

#ifndef KICAD_BUILD_VERSION
#if defined KICAD_GOST
#   define KICAD_BUILD_VERSION "(2011-aug-04 GOST)"
#else
#   define KICAD_BUILD_VERSION "(2011-aug-04)"
#endif
#endif


#if defined KICAD_TESTING_VERSION
#   define VERSION_STABILITY  "testing"
#elif defined KICAD_STABLE_VERSION
#   define VERSION_STABILITY  "stable"
#else
#   define VERSION_STABILITY  "unknown"
#   warning "unknown version stability"
#   warning "please: when running CMAKE, add -DKICAD_TESTING_VERSION=ON"
#   warning "or -DKICAD_STABLE_VERSION=ON option"
#endif

/**
 * Function GetBuildVersion
 * Return the build date and version
 */
wxString GetBuildVersion()
{
    static wxString msg;
    msg.Printf( wxT("%s-%s"),
        wxT( KICAD_BUILD_VERSION ), wxT( VERSION_STABILITY ));
    return msg;
}
