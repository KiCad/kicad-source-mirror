/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PADS_COMMON_H
#define PADS_COMMON_H

#include <kiid.h>
#include <string>

#include <stroke_params.h>
#include <wx/string.h>

/**
 * @file pads_common.h
 * @brief Common utilities and types for parsing PADS file formats.
 *
 * This header provides shared functionality used by both the PCB (pcbnew) and
 * schematic (eeschema) PADS importers. Utilities include deterministic UUID
 * generation, file type detection, and safe parsing helpers.
 */

namespace PADS_COMMON
{

/**
 * Generate a deterministic KIID from a PADS component identifier.
 *
 * This function creates a reproducible UUID based on the input string, enabling
 * cross-probe linking between schematic symbols and PCB footprints when both
 * are imported from the same PADS project.
 *
 * The UUID is generated using a hash of the input string formatted into a valid
 * UUID structure. The same input will always produce the same UUID.
 *
 * @param aIdentifier String identifying the component (typically refdes or
 *                    combination of part type and refdes).
 * @return A deterministic KIID that can be used for cross-probe linking.
 */
KIID GenerateDeterministicUuid( const std::string& aIdentifier );


/**
 * Types of PADS files that can be detected.
 */
enum class PADS_FILE_TYPE
{
    UNKNOWN,
    PCB_ASCII,       ///< PADS PowerPCB ASCII (.asc)
    SCHEMATIC_ASCII  ///< PADS Logic ASCII (.asc or .txt)
};


/**
 * Result of detecting related PADS project files.
 */
struct RELATED_FILES
{
    wxString pcbFile;        ///< Path to PCB file if found
    wxString schematicFile;  ///< Path to schematic file if found

    bool HasPcb() const { return !pcbFile.IsEmpty(); }
    bool HasSchematic() const { return !schematicFile.IsEmpty(); }
    bool HasBoth() const { return HasPcb() && HasSchematic(); }
};


/**
 * Detect the type of a PADS file by examining its header.
 *
 * @param aFilePath Path to the file to examine.
 * @return Detected file type.
 */
PADS_FILE_TYPE DetectPadsFileType( const wxString& aFilePath );


/**
 * Find related PADS project files from a given source file.
 *
 * When importing a PADS PCB file, looks for matching schematic files.
 * When importing a PADS schematic file, looks for matching PCB files.
 * Matches are found by:
 *   1. Same base filename with different file headers
 *   2. Files in same directory with compatible headers
 *
 * @param aFilePath Path to the source file being imported.
 * @return Structure containing paths to any related files found.
 */
RELATED_FILES FindRelatedPadsFiles( const wxString& aFilePath );


/**
 * Parse integer from string with error context.
 * Returns aDefault on failure and logs a trace warning.
 */
int ParseInt( const std::string& aStr, int aDefault = 0, const std::string& aContext = {} );

/**
 * Parse double from string with error context.
 * Returns aDefault on failure and logs a trace warning.
 */
double ParseDouble( const std::string& aStr, double aDefault = 0.0,
                    const std::string& aContext = {} );

/**
 * Convert a PADS net name to KiCad format, handling inverted signal notation.
 *
 * PADS uses a "/" prefix to indicate inverted signals (e.g. "/RESET").
 * KiCad uses overbar notation "~{name}" for the same purpose.
 * Non-inverted names pass through unchanged.
 *
 * This function is shared between the PCB and schematic importers to ensure
 * both sides produce identical net names.
 *
 * @param aNetName Raw PADS net name.
 * @return Net name in KiCad notation.
 */
wxString ConvertInvertedNetName( const std::string& aNetName );

/**
 * Convert a PADS line style integer to a KiCad LINE_STYLE enum value.
 *
 * PADS stores line style as an unsigned int that should be interpreted as
 * a signed int8_t for mapping.
 */
LINE_STYLE PadsLineStyleToKiCad( int aPadsStyle );

} // namespace PADS_COMMON

#endif // PADS_COMMON_H
