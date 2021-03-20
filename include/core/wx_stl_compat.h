/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_WX_STL_COMPAT_H
#define KICAD_WX_STL_COMPAT_H

#include <wx/gdicmn.h>
#include <wx/string.h>

// Some wxWidgets versions (for instance before 3.1.0) do not include
// this function, so add it if missing
#if !wxCHECK_VERSION( 3, 1, 0 )
#define USE_KICAD_WXSTRING_HASH     // for common.cpp
///< Template specialization to enable wxStrings for certain containers (e.g. unordered_map)
namespace std
{
    template<> struct hash<wxString>
    {
        size_t operator()( const wxString& s ) const;
    };
}
#endif

/// Required to use wxPoint as key type in maps
#define USE_KICAD_WXPOINT_LESS_AND_HASH // for common.cpp
namespace std
{
    template <> struct hash<wxPoint>
    {
        size_t operator() ( const wxPoint& k ) const;
    };
}

namespace std
{
    template<> struct less<wxPoint>
    {
        bool operator()( const wxPoint& aA, const wxPoint& aB ) const;
    };
}

/**
 * Helper function to print the given wxSize to a stream.
 *
 * Used for debugging functions like EDA_ITEM::Show and also in unit testing fixtures.
 */
std::ostream& operator<<( std::ostream& out, const wxSize& size );

/**
 * Helper function to print the given wxPoint to a stream.
 *
 * Used for debugging functions like EDA_ITEM::Show and also in unit testing fixtures.
 */
std::ostream& operator<<( std::ostream& out, const wxPoint& pt );

#endif // KICAD_WX_STL_COMPAT_H
