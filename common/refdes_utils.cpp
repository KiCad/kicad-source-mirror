/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <refdes_utils.h>

#include <string_utils.h>

#include <algorithm>
#include <cctype>


namespace UTIL
{

wxString GetRefDesPrefix( const wxString& aRefDes )
{
    // find the first non-digit, non-question-mark character from the back
    auto res = std::find_if( aRefDes.rbegin(), aRefDes.rend(),
            []( wxUniChar aChr )
            {
                return aChr != '?' && !std::isdigit( aChr );
            } );

    return { aRefDes.begin(), res.base() };
}


wxString GetRefDesUnannotated( const wxString& aSource )
{
   return UTIL::GetRefDesPrefix( aSource ) + wxT( "?" );
}


int GetRefDesNumber( const wxString& aRefDes )
{
    int    retval = -1; // negative to indicate not found
    size_t firstnum = aRefDes.find_first_of( wxS( "0123456789" ) );

    if( firstnum != wxString::npos )
    {
        wxString candidateValue = aRefDes.Mid( firstnum );
        long     result;

        if( !candidateValue.ToLong( &result ) )
            retval = -1;
        else
            retval = static_cast<int>( result );
    }

    return retval;
}

} // namespace UTIL
