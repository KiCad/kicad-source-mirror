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

#ifndef IMPORT_PROJ_PROPERTIES_H
#define IMPORT_PROJ_PROPERTIES_H

#include <map>
#include <string>
#include <vector>

#include <core/utf8.h>
#include <lib_id.h>
#include <wx/arrstr.h>
#include <wx/string.h>

/**
 * Property keys and codec threaded through IMPORT_PROJ_HELPER::m_properties to coordinate a
 * non-KiCad project import across the schematic and PCB editors.  Values ride the MAIL_IMPORT_FILE
 * payload, whose framing forbids '\n' inside a value, so list values join with the unit-separator
 * control character (a library nickname cannot contain it).  The codec lives here next to the
 * contract so the manager (encode) and both editor frames (decode) cannot drift.
 */
namespace IMPORT_PROJ_PROPS
{
inline constexpr char FP_CACHE_NICKNAME[] = "import_fp_cache_nickname";
inline constexpr char SOURCE_FP_LIBS[]    = "import_source_fp_libs";

/// Separator joining a list value within a single property.
inline constexpr char LIST_SEPARATOR = '\x1f';

/// Encode library nicknames into a single list-property value.
inline wxString JoinList( const wxArrayString& aItems )
{
    return wxJoin( aItems, LIST_SEPARATOR, '\0' );
}

/// Decode a list-property value back into nicknames, dropping empties.
inline std::vector<wxString> SplitList( const wxString& aValue )
{
    std::vector<wxString> out;

    for( const wxString& item : wxSplit( aValue, LIST_SEPARATOR, '\0' ) )
    {
        if( !item.IsEmpty() )
            out.push_back( item );
    }

    return out;
}

/// Read the footprint-import coordination properties out of a properties map.
inline void ReadFootprintProps( const std::map<std::string, UTF8>* aProps, wxString& aCacheNickname,
                                std::vector<wxString>& aSourceFpLibs )
{
    if( !aProps )
        return;

    if( auto it = aProps->find( FP_CACHE_NICKNAME ); it != aProps->end() )
        aCacheNickname = it->second.wx_str();

    if( auto it = aProps->find( SOURCE_FP_LIBS ); it != aProps->end() )
        aSourceFpLibs = SplitList( it->second.wx_str() );
}

/// Derive the generated footprint-cache nickname from a project or file stem.
inline wxString MakeCacheNickname( const wxString& aStem )
{
    return LIB_ID::FixIllegalChars( aStem + wxS( "-import-fps" ), true ).wx_str();
}
}

#endif // IMPORT_PROJ_PROPERTIES_H
