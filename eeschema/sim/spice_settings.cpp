/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "spice_settings.h"
#include <settings/parameters.h>

#include <fast_float/fast_float.h>

#include <cctype>
#include <climits>


const int spiceSettingsSchemaVersion = 0;


SPICE_SETTINGS::SPICE_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "simulator", spiceSettingsSchemaVersion, aParent, aPath ),
        m_fixIncludePaths( true )
{
    m_params.emplace_back( new PARAM<wxString>( "workbook_filename", &m_workbookFilename, "" ) );
    m_params.emplace_back( new PARAM<bool>( "fix_include_paths", &m_fixIncludePaths, true ) );
}


bool SPICE_SETTINGS::operator==( const SPICE_SETTINGS&aRhs ) const
{
    return m_workbookFilename == aRhs.m_workbookFilename
            && m_fixIncludePaths == aRhs.m_fixIncludePaths;
}


NGSPICE_SETTINGS::NGSPICE_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        SPICE_SETTINGS( aParent, aPath ),
        m_compatibilityMode( NGSPICE_COMPATIBILITY_MODE::LT_PSPICE )
{
    m_params.emplace_back( new PARAM_ENUM<NGSPICE_COMPATIBILITY_MODE>( "model_mode",
            &m_compatibilityMode, NGSPICE_COMPATIBILITY_MODE::LT_PSPICE,
            NGSPICE_COMPATIBILITY_MODE::USER_CONFIG, NGSPICE_COMPATIBILITY_MODE::HSPICE ) );

    m_params.emplace_back( new PARAM_MAP<wxString>( "options", &m_options, {} ) );
}


bool NGSPICE_SETTINGS::operator==( const SPICE_SETTINGS& aRhs ) const
{
    const NGSPICE_SETTINGS* settings = dynamic_cast<const NGSPICE_SETTINGS*>( &aRhs );

    wxCHECK( settings, false );

    return SPICE_SETTINGS::operator==( aRhs ) && m_compatibilityMode == settings->m_compatibilityMode
           && m_options == settings->m_options;
}


wxString NGSPICE_SETTINGS::GetOption( const std::string& aName ) const
{
    auto it = m_options.find( aName );

    return it != m_options.end() ? it->second : wxString();
}


void NGSPICE_SETTINGS::SetOption( const std::string& aName, const wxString& aValue )
{
    if( aValue.Strip( wxString::both ).IsEmpty() )
        m_options.erase( aName );
    else
        m_options[aName] = aValue;
}


std::optional<wxString> NGSPICE_SETTINGS::NormalizeOption( const std::string& aName, const wxString& aValue )
{
    wxString value = aValue.Strip( wxString::both );

    if( value.IsEmpty() )
        return wxString();

    if( aName == "method" )
    {
        value.MakeLower();

        if( value == wxS( "trap" ) || value == wxS( "gear" ) )
            return value;

        return std::nullopt;
    }

    if( aName == "itl1" || aName == "itl4" )
    {
        long limit = 0;

        if( !value.IsNumber() || !value.ToLong( &limit ) || limit < 1 || limit > INT_MAX )
            return std::nullopt;

        return wxString::Format( wxS( "%ld" ), limit );
    }

    // ngspice reads only a point as the decimal separator
    value.Replace( wxS( "," ), wxS( "." ) );

    std::string str( value.ToUTF8() );
    const char* begin = str.data();
    const char* end = begin + str.size();
    const char* mantissa = ( *begin == '+' || *begin == '-' ) ? begin + 1 : begin;

    // from_chars also takes inf and nan, which ngspice does not
    if( mantissa == end || !( std::isdigit( (unsigned char) *mantissa ) || *mantissa == '.' ) )
        return std::nullopt;

    double parsed = 0.0;
    auto   result = fast_float::from_chars( mantissa, end, parsed );

    if( result.ec != std::errc() )
        return std::nullopt;

    // Letters after the number are a scale factor and units, which ngspice accepts
    for( const char* c = result.ptr; c != end; ++c )
    {
        if( !std::isalpha( (unsigned char) *c ) )
            return std::nullopt;
    }

    return value;
}


wxString NGSPICE_SETTINGS::GetOptionsDirective() const
{
    wxString options;

    for( const std::string& name : SupportedOptions() )
    {
        std::optional<wxString> value = NormalizeOption( name, GetOption( name ) );

        if( value && !value->IsEmpty() )
            options += wxString::Format( wxS( " %s=%s" ), wxString::FromUTF8( name ), *value );
    }

    if( options.IsEmpty() )
        return wxEmptyString;

    return wxS( ".options" ) + options;
}


const std::vector<std::string>& NGSPICE_SETTINGS::SupportedOptions()
{
    static const std::vector<std::string> options = { "method", "reltol", "abstol", "vntol", "chgtol",
                                                      "trtol",  "gmin",   "gshunt", "rshunt", "itl1",
                                                      "itl4",   "temp",   "tnom" };

    return options;
}
