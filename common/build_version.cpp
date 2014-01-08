/* Date for KiCad build version */
#include <fctsys.h>

#ifdef HAVE_SVN_VERSION
#include <version.h>    // define the KICAD_BUILD_VERSION
#endif

#ifndef KICAD_BUILD_VERSION
#   define KICAD_BUILD_VERSION "(2013-jul-14)"
#endif

/**
 * Function GetBuildVersion
 * Return the build date and version
 */
wxString GetBuildVersion()
{
    wxString msg = wxString::Format(
        wxT( "%s-%s" ),
        wxT( KICAD_BUILD_VERSION ),
        wxT( KICAD_REPO_NAME )
        );

    return msg;
}
