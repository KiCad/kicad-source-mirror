/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "spice_value.h"

#include <stdexcept>
#include <cmath>

#include <wx/textentry.h>
#include <wx/numformatter.h>
#include <confirm.h>
#include <common.h>

SPICE_VALUE::SPICE_VALUE( const wxString& aString )
{
    char buf[8] = { 0, };

    if( aString.IsEmpty() )
        throw std::invalid_argument( "Spice value cannot be empty" );

    LOCALE_IO dummy;    // All numeric values should be in "C" locale(decimal separator = .)

    if( sscanf( (const char*) aString.c_str(), "%lf%7s", &m_base, buf ) == 0 )
        throw std::invalid_argument( "Invalid Spice value string" );

    if( *buf == 0 )
    {
        m_prefix = PFX_NONE;
        m_spiceStr = false;
        Normalize();
        return;
    }

    m_spiceStr = true;

    for( char* bufPtr = buf; *bufPtr; ++bufPtr )
        *bufPtr = tolower( *bufPtr );

    if( !strcmp( buf, "meg" ) )
    {
        m_prefix = PFX_MEGA;
    }
    else
    {
        switch( buf[0] )
        {
            case 'f': m_prefix = PFX_FEMTO; break;
            case 'p': m_prefix = PFX_PICO; break;
            case 'n': m_prefix = PFX_NANO; break;
            case 'u': m_prefix = PFX_MICRO; break;
            case 'm': m_prefix = PFX_MILI; break;
            case 'k': m_prefix = PFX_KILO; break;
            case 'g': m_prefix = PFX_GIGA; break;
            case 't': m_prefix = PFX_TERA; break;

            default:
                throw std::invalid_argument( "Invalid unit prefix" );
        }
    }

    Normalize();
}


void SPICE_VALUE::Normalize()
{
    while( std::fabs( m_base ) >= 1000.0 )
    {
        m_base *= 0.001;
        m_prefix = (UNIT_PREFIX)( m_prefix + 3 );
    }

    while( m_base != 0.0 && std::fabs( m_base ) < 1.000 )
    {
        m_base *= 1000.0;
        m_prefix = (UNIT_PREFIX)( m_prefix - 3 );
    }
}


double SPICE_VALUE::ToDouble() const
{
    double res = m_base;

    if( m_prefix != PFX_NONE )
        res *= std::pow( 10, (int) m_prefix );

    return res;
}

wxString SPICE_VALUE::ToString() const
{
    wxString res( wxString::Format( "%.3f", ToDouble() ) );
    stripZeros( res );

    return res;
}


wxString SPICE_VALUE::ToSpiceString() const
{
    wxString res = wxString::FromCDouble( m_base );
    stripZeros( res );

    switch( m_prefix )
    {
        case PFX_FEMTO: res += "f"; break;
        case PFX_PICO:  res += "p"; break;
        case PFX_NANO:  res += "n"; break;
        case PFX_MICRO: res += "u"; break;
        case PFX_MILI:  res += "m"; break;
        case PFX_NONE:  break;
        case PFX_KILO:  res += "k"; break;
        case PFX_MEGA:  res += "Meg"; break;
        case PFX_GIGA:  res += "G"; break;
        case PFX_TERA:  res += "T"; break;
    }

    return res;
}


SPICE_VALUE SPICE_VALUE::operator+( const SPICE_VALUE& aOther ) const
{
    int prefixDiff = m_prefix - aOther.m_prefix;
    SPICE_VALUE res;
    res.m_spiceStr = m_spiceStr || aOther.m_spiceStr;

    // Convert both numbers to a common prefix
    if( prefixDiff > 0 )
    {
        // Switch to the aOther prefix
        res.m_base =  ( m_base * std::pow( 10, prefixDiff ) ) + aOther.m_base;
        res.m_prefix = aOther.m_prefix;
    }
    else if( prefixDiff < 0 )
    {
        // Use the current prefix
        res.m_base = m_base + ( aOther.m_base * std::pow( 10, -prefixDiff ) );
        res.m_prefix = m_prefix;
    }
    else
    {
        res.m_base = m_base + aOther.m_base;
        res.m_prefix = m_prefix; // == aOther.m_prefix
    }

    res.Normalize();

    return res;
}


SPICE_VALUE SPICE_VALUE::operator-( const SPICE_VALUE& aOther ) const
{
    int prefixDiff = m_prefix - aOther.m_prefix;
    SPICE_VALUE res;
    res.m_spiceStr = m_spiceStr || aOther.m_spiceStr;

    // Convert both numbers to a common prefix
    if( prefixDiff > 0 )
    {
        // Switch to the aOther prefix
        res.m_base = m_base * std::pow( 10, prefixDiff ) - aOther.m_base;
        res.m_prefix = aOther.m_prefix;
    }
    else if( prefixDiff < 0 )
    {
        // Use the current prefix
        res.m_base = m_base - aOther.m_base * std::pow( 10, -prefixDiff );
        res.m_prefix = m_prefix;
    }
    else
    {
        res.m_base = m_base - aOther.m_base;
        res.m_prefix = m_prefix; // == aOther.m_prefix
    }

    res.Normalize();

    return res;
}


SPICE_VALUE SPICE_VALUE::operator*( const SPICE_VALUE& aOther ) const
{
    SPICE_VALUE res( m_base * aOther.m_base, (UNIT_PREFIX)( m_prefix + aOther.m_prefix ) );
    res.m_spiceStr = m_spiceStr || aOther.m_spiceStr;
    res.Normalize();

    return res;
}


SPICE_VALUE SPICE_VALUE::operator/( const SPICE_VALUE& aOther ) const
{
    SPICE_VALUE res( m_base / aOther.m_base, (UNIT_PREFIX)( m_prefix - aOther.m_prefix ) );
    res.m_spiceStr = m_spiceStr || aOther.m_spiceStr;
    res.Normalize();

    return res;
}


void SPICE_VALUE::stripZeros( wxString& aString )
{
    if ( aString.Find( ',' ) >= 0 || aString.Find( '.' ) >= 0 )
    {
        while( aString.EndsWith( '0' ) )
            aString.RemoveLast();

        if( aString.EndsWith( '.' ) || aString.EndsWith( ',' ) )
            aString.RemoveLast();
    }
}


bool SPICE_VALIDATOR::Validate( wxWindow* aParent )
{
    wxTextEntry* const text = GetTextEntry();

    if( !text )
        return false;

    if( text->IsEmpty() )
    {
        if( m_emptyAllowed )
            return true;

        DisplayError( aParent, wxString::Format( _( "Please, fill required fields" ) ) );
        return false;
    }

    wxString svalue = text->GetValue();

    // In countries where the decimal separator is not a point, if the user
    // has not used a point, replace the decimal separator by the point, as needed
    // by spice simulator which uses the "C" decimal separator
    svalue.Replace(",", "." );

    try
    {
        // If SPICE_VALUE can be constructed, then it is a valid Spice value
        SPICE_VALUE val( svalue );
    }
    catch( ... )
    {
        DisplayError( aParent,
                wxString::Format( _( "'%s' is not a valid Spice value" ), text->GetValue() ) );

        return false;
    }

    if( svalue != text->GetValue() )
        text->SetValue( svalue );

    return true;
}
