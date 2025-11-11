/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "pin_numbers.h"
#include <wx/crt.h>
#include <wx/translation.h>


wxString PIN_NUMBERS::getNextSymbol( const wxString& str, wxString::size_type& cursor )
{
    if( str.size() <= cursor )
        return wxEmptyString;

    wxString::size_type begin = cursor;

    wxChar c = str[cursor];

    // Need to check that there is a digit in the string before we parse it as a numeric
    if( ( wxIsdigit( c ) || ( ( c == '+' || c == '-' )  && ( cursor < str.size() - 1 )
                            && wxIsdigit( str[cursor + 1] ) ) ) )
    {
        // number, possibly with sign
        while( ++cursor < str.size() )
        {
            c = str[cursor];

            if( wxIsdigit( c ) || c == 'v' || c == 'V' || c == '.' )
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

            if( wxIsdigit( c ) )
                break;
            else
                continue;
        }
    }

    return str.substr( begin, cursor - begin );
}


wxString PIN_NUMBERS::GetSummary() const
{
    wxString ret;

    const_iterator i = begin();

    if( i == end() )
        return ret;

    const_iterator begin_of_range = i;
    const_iterator last;

    for( ;; )
    {
        last = i;
        ++i;

        int rc = ( i != end() ) ? Compare( *last, *i ) : -2;

        assert( rc == -1 || rc == -2 );

        if( rc == -1 )
            // adjacent elements
            continue;

        ret += *begin_of_range;

        if( begin_of_range != last )
        {
            ret += '-';
            ret += *last;
        }

        if( i == end() )
            break;

        begin_of_range = i;
        ret += ',';
    }

    return ret;
}


wxString PIN_NUMBERS::GetDuplicates() const
{
    wxString ret;

    for( const wxString& pinNumber : duplicate_pins )
    {
        ret += pinNumber;
        ret += ',';
    }

    // Remove the trailing comma
    ret.RemoveLast();

    if( ret.IsEmpty() )
        ret = _( "none" );

    return ret;
}


int PIN_NUMBERS::Compare( const wxString& lhs, const wxString& rhs )
{
    wxString::size_type cursor1 = 0;
    wxString::size_type cursor2 = 0;

    wxString symbol1, symbol2;

    for( ; ; )
    {
        symbol1 = getNextSymbol( lhs, cursor1 );
        symbol2 = getNextSymbol( rhs, cursor2 );

        if( symbol1.empty() && symbol2.empty() )
            return 0;

        if( symbol1.empty() )
            return -2;

        if( symbol2.empty() )
            return 2;

        bool sym1_isnumeric = symbol1.find_first_of( wxS( "0123456789" ) ) != wxString::npos;
        bool sym2_isnumeric = symbol2.find_first_of( wxS( "0123456789" ) ) != wxString::npos;

        if( sym1_isnumeric )
        {
            if( sym2_isnumeric )
            {
                // numeric comparison
                wxString::size_type v1 = symbol1.find_first_of( wxS( "vV" ) );

                if( v1 != wxString::npos )
                    symbol1[v1] = '.';

                wxString::size_type v2 = symbol2.find_first_of( wxS( "vV" ) );

                if( v2 != wxString::npos )
                    symbol2[v2] = '.';

                double val1, val2;

                symbol1.ToCDouble( &val1 );
                symbol2.ToCDouble( &val2 );

                if( val1 < val2 )
                {
                    if( val1 == val2 - 1 )
                        return -1;
                    else
                        return -2;
                }

                if( val1 > val2 )
                {
                    if( val1 == val2 + 1 )
                        return 1;
                    else
                        return 2;
                }
            }
            else
            {
                return -2;
            }
        }
        else
        {
            if( sym2_isnumeric )
                return 2;

            int res = symbol1.Cmp( symbol2 );

            if( res != 0 )
                return res;
        }
    }
}
