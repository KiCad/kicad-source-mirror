/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "altium_props_utils.h"

#include <limits>
#include <sstream>
#include <math/util.h>

#include <wx/crt.h>
#include <wx/log.h>
#include <wx/translation.h>


int32_t ALTIUM_PROPS_UTILS::ConvertToKicadUnit( const double aValue )
{
    constexpr double int_limit = ( std::numeric_limits<int>::max() - 10 ) / 2.54;

    int32_t iu = KiROUND( std::clamp( aValue, -int_limit, int_limit ) * 2.54 );

    // Altium's internal precision is 0.1uinch.  KiCad's is 1nm.  Round to nearest 10nm to clean
    // up most rounding errors.  This allows lossless conversion of increments of 0.05mils and
    // 0.01um.
    return KiROUND( (double) iu / 10.0 ) * 10;
}


int ALTIUM_PROPS_UTILS::ReadInt( const std::map<wxString, wxString>& aProps, const wxString& aKey,
                            int aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProps.find( aKey );
    return value == aProps.end() ? aDefault : wxAtoi( value->second );
}


double ALTIUM_PROPS_UTILS::ReadDouble( const std::map<wxString, wxString>& aProps,
                                       const wxString& aKey, double aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProps.find( aKey );

    if( value == aProps.end() )
        return aDefault;

    // Locale independent str -> double conversation
    std::istringstream istr( (const char*) value->second.mb_str() );
    istr.imbue( std::locale::classic() );

    double doubleValue;
    istr >> doubleValue;
    return doubleValue;
}


bool ALTIUM_PROPS_UTILS::ReadBool( const std::map<wxString, wxString>& aProps, const wxString& aKey,
                                   bool aDefault )
{
    const std::map<wxString, wxString>::const_iterator& value = aProps.find( aKey );

    if( value == aProps.end() )
        return aDefault;
    else
        return value->second == "T" || value->second == "TRUE";
}


int32_t ALTIUM_PROPS_UTILS::ReadKicadUnit( const std::map<wxString, wxString>& aProps,
                                           const wxString& aKey, const wxString& aDefault )
{
    const wxString& value = ReadString( aProps, aKey, aDefault );

    wxString prefix;

    if( !value.EndsWith( "mil", &prefix ) )
    {
        wxLogTrace( "ALTIUM", wxT( "Unit '%s' does not end with 'mil'." ), value );
        return 0;
    }

    prefix.StartsWith( "+", &prefix );

    double mils;

    if( !prefix.ToCDouble( &mils ) )
    {
        wxLogTrace( "ALTIUM", wxT( "Cannot convert '%s' to double." ), prefix );
        return 0;
    }

    return ConvertToKicadUnit( mils * 10000 );
}


wxString ALTIUM_PROPS_UTILS::ReadString( const std::map<wxString, wxString>& aProps,
                                         const wxString& aKey, const wxString& aDefault )
{
    const auto& utf8Value = aProps.find( wxString( "%UTF8%" ) + aKey );

    if( utf8Value != aProps.end() )
        return utf8Value->second;

    const auto& value = aProps.find( aKey );

    if( value != aProps.end() )
        return value->second;

    return aDefault;
}


wxString ALTIUM_PROPS_UTILS::ReadUnicodeString( const std::map<wxString, wxString>& aProps,
                                                const wxString& aKey, const wxString& aDefault )
{
    const auto& unicodeFlag = aProps.find( wxS( "UNICODE" ) );

    if( unicodeFlag != aProps.end() && unicodeFlag->second.Contains( wxS( "EXISTS" ) ) )
    {
        const auto& unicodeValue = aProps.find( wxString( "UNICODE__" ) + aKey );

        if( unicodeValue != aProps.end() )
        {
            wxArrayString arr = wxSplit( unicodeValue->second, ',', '\0' );
            wxString      out;

            for( wxString part : arr )
                out += wxString( wchar_t( wxAtoi( part ) ) );

            return out;
        }
    }

    return ReadString( aProps, aKey, aDefault );
}
