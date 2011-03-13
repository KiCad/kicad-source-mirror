/* Date for kicad build version */
#include "fctsys.h"

#ifdef HAVE_SVN_VERSION
#include "version.h"    // define the KICAD_BUILD_VERSION
#endif

#ifndef KICAD_BUILD_VERSION
#define KICAD_BUILD_VERSION "(2011-03-12)"
#endif

//#define VERSION_STABILITY      "stable"
#define VERSION_STABILITY       "testing"


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
