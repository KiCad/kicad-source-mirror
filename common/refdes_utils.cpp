/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <kicad_string.h>

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


int RefDesStringCompare( const wxString& aFirst, const wxString& aSecond )
{
    // Compare unescaped text
    wxString strFWord = UnescapeString( aFirst );
    wxString strSWord = UnescapeString( aSecond );

    // The different sections of the two strings
    wxString strFWordBeg, strFWordMid, strFWordEnd;
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    // Split the two strings into separate parts
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    // Compare the Beginning section of the strings
    int isEqual = strFWordBeg.Cmp( strSWordBeg );

    if( isEqual > 0 )
        return 1;
    else if( isEqual < 0 )
        return -1;
    else
    {
        // If the first sections are equal compare their digits
        long lFirstDigit = 0;
        long lSecondDigit = 0;

        strFWordMid.ToLong( &lFirstDigit );
        strSWordMid.ToLong( &lSecondDigit );

        if( lFirstDigit > lSecondDigit )
            return 1;
        else if( lFirstDigit < lSecondDigit )
            return -1;
        // If the first two sections are equal compare the endings
        else
            return strFWordEnd.Cmp( strSWordEnd );
    }
}


} // namespace UTIL