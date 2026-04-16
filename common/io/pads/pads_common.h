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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PADS_COMMON_H
#define PADS_COMMON_H

#include <kiid.h>
#include <string>

#include <font/text_attributes.h>
#include <stroke_params.h>
#include <wx/string.h>

/**
 * Common utilities and types for parsing PADS file formats, shared by the PCB and
 * schematic importers.
 */

namespace PADS_COMMON
{

/**
 * Generate a deterministic KIID from a PADS component identifier. The same input
 * always produces the same UUID, so a schematic symbol and its PCB footprint can
 * cross-probe link when both are imported from the same PADS project.
 *
 * @param aIdentifier String identifying the component (typically refdes or
 *                    combination of part type and refdes).
 */
KIID GenerateDeterministicUuid( const std::string& aIdentifier );


enum class PADS_FILE_TYPE
{
    UNKNOWN,
    PCB_ASCII,
    SCHEMATIC_ASCII
};


struct RELATED_FILES
{
    wxString pcbFile;
    wxString schematicFile;

    bool HasPcb() const { return !pcbFile.IsEmpty(); }
    bool HasSchematic() const { return !schematicFile.IsEmpty(); }
    bool HasBoth() const { return HasPcb() && HasSchematic(); }
};


/**
 * Detect the type of a PADS file by examining its header.
 */
PADS_FILE_TYPE DetectPadsFileType( const wxString& aFilePath );


/**
 * Find the related PADS project file (schematic for a PCB source, or vice versa)
 * in the same directory, preferring a matching base filename.
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
 * Convert a PADS net name to KiCad notation. PADS marks inverted signals with a
 * leading "/" (e.g. "/RESET"); KiCad uses overbar "~{name}". Non-inverted names
 * pass through unchanged.
 */
wxString ConvertInvertedNetName( const std::string& aNetName );

/**
 * Decode text from a PADS file, which uses an 8-bit codepage rather than UTF-8.
 *
 * A direct UTF-8 conversion discards the whole string on the first high byte, so
 * decode UTF-8 when valid and otherwise fall back to Windows-1252 / ISO-8859-1.
 *
 * @param aText Raw text bytes from the PADS file.
 * @return Decoded wxString.
 */
wxString ConvertText( const std::string& aText );

/**
 * Convert a PADS line style integer to a KiCad LINE_STYLE enum value.
 *
 * PADS stores line style as an unsigned int that should be interpreted as
 * a signed int8_t for mapping.
 */
LINE_STYLE PadsLineStyleToKiCad( int aPadsStyle );

/**
 * Decode a PADS text justification code into KiCad horizontal and vertical alignment.
 *
 * PADS encodes the anchor of a text string as a single integer combining a vertical
 * band and a horizontal code:
 *   value = vertical_band + horizontal_code
 *   vertical bands  bottom (0..1), top (2..7), middle (8..)
 *   horizontal codes  left=0, right=1, center=4
 *
 * The same encoding is used for free text, part field labels and net name labels, so
 * both the PCB and schematic importers share this decode to stay consistent.
 *
 * @param aJustification Raw PADS justification code.
 * @param aHJustify Receives the decoded horizontal alignment.
 * @param aVJustify Receives the decoded vertical alignment.
 */
void DecodeJustification( int aJustification, GR_TEXT_H_ALIGN_T& aHJustify,
                          GR_TEXT_V_ALIGN_T& aVJustify );

} // namespace PADS_COMMON

#endif // PADS_COMMON_H
