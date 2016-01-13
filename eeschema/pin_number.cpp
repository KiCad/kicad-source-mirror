/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pin_number.h"

namespace {

wxString GetNextComponent( const wxString& str, wxString::size_type& cursor )
{
    if( str.size() <= cursor )
        return wxEmptyString;

    wxString::size_type begin = cursor;

    wxUniChar c = str[cursor];

    if( isdigit( c ) || c == '+' || c == '-' )
    {
        // number, possibly with sign
        while( ++cursor < str.size() )
        {
            c = str[cursor];

            if( isdigit( c ) || c == 'v' || c == 'V' )
                continue;
            else
                break;
        }
    }
    else
    {
        while( ++cursor < str.size() )
        {
            c = str[cursor];

            if( isdigit( c ) )
                break;
            else
                continue;
        }
    }

    return str.substr( begin, cursor - begin );
}

}


int PinNumbers::Compare( const PinNumber& lhs, const PinNumber& rhs )
{
    wxString::size_type cursor1 = 0;
    wxString::size_type cursor2 = 0;

    wxString comp1, comp2;

    for( ; ; )
    {
        comp1 = GetNextComponent( lhs, cursor1 );
        comp2 = GetNextComponent( rhs, cursor2 );

        if( comp1.empty() && comp2.empty() )
            return 0;

        if( comp1.empty() )
            return -1;

        if( comp2.empty() )
            return 1;

        wxUniChar c1    = comp1[0];
        wxUniChar c2    = comp2[0];

        if( isdigit( c1 ) || c1 == '-' || c1 == '+' )
        {
            if( isdigit( c2 ) || c2 == '-' || c2 == '+' )
            {
                // numeric comparison
                wxString::size_type v1 = comp1.find_first_of( "vV" );

                if( v1 != wxString::npos )
                    comp1[v1] = '.';

                wxString::size_type v2 = comp2.find_first_of( "vV" );

                if( v2 != wxString::npos )
                    comp2[v2] = '.';

                double val1, val2;

                comp1.ToDouble( &val1 );
                comp2.ToDouble( &val2 );

                if( val1 < val2 )
                    return -1;

                if( val1 > val2 )
                    return 1;
            }
            else
                return -1;
        }
        else
        {
            if( isdigit( c2 ) || c2 == '-' || c2 == '+' )
                return 1;

            int res = comp1.Cmp( comp2 );

            if( res != 0 )
                return res;
        }
    }
}
