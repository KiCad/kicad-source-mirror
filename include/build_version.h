/* Date for kicad build version */

#ifndef KICAD_BUILD_VERSION
#define KICAD_BUILD_VERSION

COMMON_GLOBL wxString g_BuildVersion
#ifdef EDA_BASE
    (wxT("(20080429-r1010)"))
#endif
;

#endif	// KICAD_BUILD_VERSION
