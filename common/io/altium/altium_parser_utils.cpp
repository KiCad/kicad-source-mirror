/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "altium_parser_utils.h"

#include <string_utils.h>
#include <lib_id.h>
#include <trigo.h>
#include <math/util.h>

LIB_ID AltiumToKiCadLibID( const wxString& aLibName, const wxString& aLibReference )
{
    wxString libName = LIB_ID::FixIllegalChars( aLibName, true );
    wxString libReference = EscapeString( aLibReference, CTX_LIBID );

    wxString key = !libName.empty() ? ( libName + ":" + libReference ) : libReference;

    LIB_ID libId;
    libId.Parse( key, true );

    return libId;
}


wxString AltiumPropertyToKiCadString( const wxString& aString )
{
    wxString  output;
    wxString  tempString;
    bool      hasPrev = false;
    wxUniChar prev;

    for( wxString::const_iterator it = aString.begin(); it != aString.end(); ++it )
    {
        char ch = 0;

        if( (*it).GetAsChar( &ch ) )
        {
            if( ch == '\\' )
            {
                if( hasPrev )
                {
                    tempString += prev;
                    hasPrev = false;
                }

                continue; // Backslash is ignored and not added to the output
            }
        }

        if( hasPrev ) // Two letters in a row with no backslash
        {
            if( !tempString.empty() )
            {
                output += "~{" + tempString + "}";
                tempString.Clear();
            }

            output += prev;
        }

        prev = *it;
        hasPrev = true;
    }

    // Append any leftover escaped string
    if( !tempString.IsEmpty() )
        output += "~{" + tempString + "}";

    if( hasPrev )
        output += prev;

    return output;
}


// https://www.altium.com/documentation/altium-designer/sch-obj-textstringtext-string-ad#!special-strings
wxString AltiumSchSpecialStringsToKiCadVariables( const wxString&                     aString,
                                                  const std::map<wxString, wxString>& aOverrides )
{
    if( aString.IsEmpty() || aString.at( 0 ) != '=' )
    {
        return aString;
    }

    wxString result;

    size_t start = 1;
    size_t delimiter = 0;
    size_t escaping_start = 0;
    do
    {
        delimiter = aString.find( "+", start );
        escaping_start = aString.find( "'", start );

        if( escaping_start < delimiter )
        {
            size_t text_start = escaping_start + 1;
            size_t escaping_end = aString.find( "'", text_start );

            if( escaping_end == wxString::npos )
            {
                escaping_end = aString.size();
            }

            result += aString.substr( text_start, escaping_end - text_start );

            start = escaping_end + 1;
        }
        else
        {
            wxString specialString = aString.substr( start, delimiter - start ).Trim().Trim( false );

            if( specialString.StartsWith( "\"" ) && specialString.EndsWith( "\"" ) )
                specialString = specialString.Mid( 1, specialString.Length() - 2 );

            if( !specialString.IsEmpty() )
            {
                // Note: Altium variable references are case-insensitive.  KiCad matches
                // case-sensitive OR to all-upper-case, so make the references all-upper-case.
                specialString.UpperCase();

                auto overrideIt = aOverrides.find( specialString );

                if( overrideIt != aOverrides.end() )
                    specialString = overrideIt->second;

                result += wxString::Format( wxT( "${%s}" ), specialString );
            }

            start = delimiter + 1;
        }
    } while( delimiter != wxString::npos );

    return result;
}

// https://www.altium.com/documentation/altium-designer/text-objects-pcb
wxString AltiumPcbSpecialStringsToKiCadStrings( const wxString&                     aString,
                                                const std::map<wxString, wxString>& aOverrides )
{
    if( aString.IsEmpty() )
        return aString;

    // special case: string starts with dot -> whole string is special string
    if( aString.at( 0 ) == '.' )
    {
        wxString specialString = aString.substr( 1 );

        specialString.UpperCase(); // matching is implemented using upper case strings

        auto overrideIt = aOverrides.find( specialString );

        if( overrideIt != aOverrides.end() )
            specialString = overrideIt->second;

        return wxString::Format( wxT( "${%s}" ), specialString );
    }

    // TODO: implement Concatenated special strings using apostrophe "'".

    return aString;
}

wxString AltiumPinNamesToKiCad( wxString& aString )
{
    if( aString.IsEmpty() )
        return wxEmptyString;

    wxString rest;

    if( aString.StartsWith( '\\', &rest ) && !rest.Contains( '\\' ) )
        return "~{" + rest + "}";

    return AltiumPropertyToKiCadString( aString );
}

VECTOR2I AltiumGetEllipticalPos( double aMajor, double aMinor, double aAngleRadians )
{
    if( aMajor == 0 || aMinor == 0 )
        return VECTOR2I( 0, 0 );

    double numerator = aMajor * aMinor;
    double majorTerm = aMajor * sin( aAngleRadians );
    double minorTerm = aMinor * cos( aAngleRadians );
    double denominator = sqrt( majorTerm * majorTerm + minorTerm * minorTerm );

    double radius = numerator / denominator;

    VECTOR2I retval( KiROUND( radius * cos( aAngleRadians ) ), KiROUND( radius * sin( aAngleRadians ) ) );

    return retval;

}