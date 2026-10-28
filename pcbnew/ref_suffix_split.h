/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef REF_SUFFIX_SPLIT_H
#define REF_SUFFIX_SPLIT_H

#include <map>
#include <optional>
#include <string>

#include <kiid.h>
#include <pcb_item_containers.h>
#include <wx/string.h>


/// A schematic sheet path's composed reference suffix and its position in page order.
struct SHEET_REF_SUFFIX
{
    size_t   Rank = 0;
    wxString Suffix;
};

/// Sheet suffixes keyed by the sheet path string that starts the paths of its footprints, and
/// package suffixes keyed by the full path of a footprint whose package takes another sheet's.
using SHEET_REF_SUFFIXES = std::map<wxString, SHEET_REF_SUFFIX>;

/**
 * @return the suffixes in a #MAIL_SCH_GET_REF_SUFFIXES reply, or nothing when no schematic answered.
 */
std::optional<SHEET_REF_SUFFIXES> ParseRefSuffixes( const std::string& aPayload );


struct FOOTPRINT_REF_SPLIT
{
    wxString Base;            ///< Reference without its sheet suffix
    wxString Suffix;          ///< Composed suffix of the footprint's sheet path
    KIID     Representative;  ///< Copy whose placement numbers the base this footprint shares
    bool     Unknown = false; ///< Text follows the number and no known suffix accounts for it
    bool     Shared = false;  ///< One of the copies on reused sheets that share a base
};

/**
 * Split each footprint reference into its base and sheet suffix.
 *
 * Copies of one symbol on reused sheets that share a base form a group, which is numbered from the
 * copy on the first page so every copy keeps the same base.
 *
 * @param aSuffixes are the schematic's sheet suffixes, or null when there is no schematic to ask.
 * @param aSuffixMode is true when the project numbers reused sheets by suffix, which leaves a
 *                    reference ambiguous whenever there is no schematic to name the suffix.
 */
std::map<KIID, FOOTPRINT_REF_SPLIT> SplitRefSuffixes( const FOOTPRINTS&         aFootprints,
                                                      const SHEET_REF_SUFFIXES* aSuffixes, bool aSuffixMode );

#endif // REF_SUFFIX_SPLIT_H
