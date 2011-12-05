/* Date for kicad build version */

#ifndef KICAD_BUILD_VERSION_H
#define KICAD_BUILD_VERSION_H

class wxString;


/**
 * Function GetBuildVersion
 * Return the build date and version
 */
wxString GetBuildVersion();

/// The file format revision of the *.brd file created by this build
#define BOARD_FILE_VERSION          1


#endif	// KICAD_BUILD_VERSION_H
