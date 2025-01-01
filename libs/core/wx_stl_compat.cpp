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

#include <ostream>
#include <core/wx_stl_compat.h>

// add this only if it is not in wxWidgets (for instance before 3.1.0)
#ifdef USE_KICAD_WXSTRING_HASH
size_t std::hash<wxString>::operator()( const wxString& s ) const
{
    return std::hash<std::wstring>{}( s.ToStdWstring() );
}
#endif

#ifdef USE_KICAD_WXPOINT_LESS_AND_HASH
bool std::less<wxPoint>::operator()( const wxPoint& aA, const wxPoint& aB ) const
{
    if( aA.x == aB.x )
        return aA.y < aB.y;

    return aA.x < aB.x;
}
#endif


wxString TowxString( const std::string_view& view )
{
    return wxString( view.data(), view.length() );
}


std::ostream& operator<<( std::ostream& out, const wxSize& size )
{
    out << " width=\"" << size.GetWidth() << "\" height=\"" << size.GetHeight() << "\"";
    return out;
}


std::ostream& operator<<( std::ostream& out, const wxPoint& pt )
{
    out << " x=\"" << pt.x << "\" y=\"" << pt.y << "\"";
    return out;
}
