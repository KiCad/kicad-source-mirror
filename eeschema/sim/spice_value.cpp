/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <math/util.h>
#include <core/kicad_algo.h>

#include <stdexcept>
#include <cmath>

#include <wx/textentry.h>
#include <wx/numformatter.h>
#include <confirm.h>
#include <common.h>
#include <locale_io.h>
#include <geometry/eda_angle.h>


void SPICE_VALUE_FORMAT::FromString( const wxString& aString )
{
    if( aString.IsEmpty() )
    {
        Precision = 3;
        Range = wxS( "~V" );
    }
    else
    {
        long val;
        aString.Left( 1 ).ToLong( &val );
        Precision = static_cast<int>( val );
        Range = aString.Right( aString.Length() - 1 );
    }
}


wxString SPICE_VALUE_FORMAT::ToString() const
{
    return wxString::Format( wxS( "%d%s" ), std::clamp( Precision, 0, 9 ), Range );
}


void SPICE_VALUE_FORMAT::UpdateUnits( const wxString& aUnits )
{
    if( Range.GetChar( 0 ) == '~' )
        Range = Range.Left( 1 ) + aUnits;
    else if( SPICE_VALUE::ParseSIPrefix( Range.GetChar( 0 ) ) != SPICE_VALUE::PFX_NONE )
        Range = Range.Left( 1 ) + aUnits;
    else
        Range = aUnits;
}


SPICE_VALUE::SPICE_VALUE( const wxString& aString ) :
        m_base( 0.0 ),
        m_prefix( PFX_NONE ),
        m_spiceStr( false )
{
    if( aString.IsEmpty() )
        return;

    char      units[8] = { 0, };
    LOCALE_IO dummy;              // Numeric values must be in "C" locale ('.' decimal separator)

    sscanf( (const char*) aString.c_str(), "%lf%7s", &m_base, units );

    if( *units == 0 )
    {
        Normalize();
        return;
    }

    m_spiceStr = true;

    for( char* bufPtr = units; *bufPtr; ++bufPtr )
        *bufPtr = tolower( *bufPtr );

    if( strcmp( units, "meg" ) == 0 )
    {
        m_prefix = PFX_MEGA;
    }
    else
    {
        switch( units[0] )
        {
            case 'f': m_prefix = PFX_FEMTO; break;
            case 'p': m_prefix = PFX_PICO; break;
            case 'n': m_prefix = PFX_NANO; break;
            case 'u': m_prefix = PFX_MICRO; break;
            case 'm': m_prefix = PFX_MILI; break;
            case 'k': m_prefix = PFX_KILO; break;
            case 'g': m_prefix = PFX_GIGA; break;
            case 't': m_prefix = PFX_TERA; break;
            default:  m_prefix = PFX_NONE; break;
        }
    }

    Normalize();
}


void SPICE_VALUE::Normalize()
{
    while( std::fabs( m_base ) >= 1000.0 )
    {
        if( m_prefix == PFX_TERA )     // this is the biggest unit available
            break;

        m_base *= 0.001;
        m_prefix = (UNIT_PREFIX)( m_prefix + 3 );
    }

    while( m_base != 0.0 && std::fabs( m_base ) < 1.000 )
    {
        if( m_prefix == PFX_FEMTO )     // this is the smallest unit available
            break;

        m_base *= 1000.0;
        m_prefix = (UNIT_PREFIX)( m_prefix - 3 );
    }
}


wxString spice_prefix( SPICE_VALUE::UNIT_PREFIX aPrefix )
{
    switch( aPrefix )
    {
    case SPICE_VALUE::PFX_FEMTO: return wxT( "f" );
    case SPICE_VALUE::PFX_PICO:  return wxT( "p" );
    case SPICE_VALUE::PFX_NANO:  return wxT( "n" );
    case SPICE_VALUE::PFX_MICRO: return wxT( "u" );
    case SPICE_VALUE::PFX_MILI:  return wxT( "m" );
    case SPICE_VALUE::PFX_NONE:  return wxEmptyString;
    case SPICE_VALUE::PFX_KILO:  return wxT( "k" );
    case SPICE_VALUE::PFX_MEGA:  return wxT( "Meg" );
    case SPICE_VALUE::PFX_GIGA:  return wxT( "G" );
    case SPICE_VALUE::PFX_TERA:  return wxT( "T" );
    }

    return wxEmptyString;
}


wxString si_prefix( SPICE_VALUE::UNIT_PREFIX aPrefix )
{
    switch( aPrefix )
    {
    case SPICE_VALUE::PFX_FEMTO: return wxT( "f" );
    case SPICE_VALUE::PFX_PICO:  return wxT( "p" );
    case SPICE_VALUE::PFX_NANO:  return wxT( "n" );
    case SPICE_VALUE::PFX_MICRO: return wxT( "u" );
    case SPICE_VALUE::PFX_MILI:  return wxT( "m" );
    case SPICE_VALUE::PFX_NONE:  return wxEmptyString;
    case SPICE_VALUE::PFX_KILO:  return wxT( "K" );
    case SPICE_VALUE::PFX_MEGA:  return wxT( "M" );
    case SPICE_VALUE::PFX_GIGA:  return wxT( "G" );
    case SPICE_VALUE::PFX_TERA:  return wxT( "T" );
    }

    return wxEmptyString;
}


SPICE_VALUE::UNIT_PREFIX SPICE_VALUE::ParseSIPrefix( wxChar c )
{
    switch( c )
    {
    case 'f': return SPICE_VALUE::PFX_FEMTO;
    case 'p': return SPICE_VALUE::PFX_PICO;
    case 'n': return SPICE_VALUE::PFX_NANO;
    case 'u': return SPICE_VALUE::PFX_MICRO;
    case 'm': return SPICE_VALUE::PFX_MILI;
    case 'K': return SPICE_VALUE::PFX_KILO;
    case 'M': return SPICE_VALUE::PFX_MEGA;
    case 'G': return SPICE_VALUE::PFX_GIGA;
    case 'T': return SPICE_VALUE::PFX_TERA;
    default:  return SPICE_VALUE::PFX_NONE;
    }
}


double SPICE_VALUE::ToNormalizedDouble( wxString* aPrefix )
{
    Normalize();

    *aPrefix = spice_prefix( m_prefix );
    return m_base;
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
    StripZeros( res );

    return res;
}


wxString SPICE_VALUE::ToString( const SPICE_VALUE_FORMAT& aFormat )
{
    wxString range( aFormat.Range );

    if( range.EndsWith( wxS( "°" ) ) )
    {
        EDA_ANGLE angle( m_base * std::pow( 10, (int) m_prefix ), DEGREES_T );
        angle.Normalize180();
        return wxString::FromCDouble( angle.AsDegrees(), aFormat.Precision ) + wxS( "°" );
    }

    if( range.StartsWith( wxS( "~" ) ) )
    {
        Normalize();
        range = si_prefix( m_prefix ) + range.Right( range.Length() - 1 );
    }
    else
    {
        SPICE_VALUE::UNIT_PREFIX rangePrefix = ParseSIPrefix( range[0] );
        m_base = m_base * std::pow( 10, m_prefix - rangePrefix );
        m_prefix = rangePrefix;
    }

    double mantissa = m_base;
    int    scale = 0;

    while( std::fabs( mantissa ) >= 10.0 )
    {
        mantissa *= 0.1;
        scale += 1;
    }

    while( mantissa != 0.0 && std::fabs( mantissa ) < 1.0 )
    {
        mantissa *= 10;
        scale -= 1;
    }

    mantissa = KiROUND( mantissa * std::pow( 10, aFormat.Precision - 1 ) );
    mantissa *= std::pow( 10, scale - aFormat.Precision + 1 );

    wxString res = wxString::FromCDouble( mantissa, std::max( 0, aFormat.Precision - scale - 1 ) );

    // If we have an excessively long number, switch to scientific notation
    if( ssize_t( res.length() ) > aFormat.Precision + static_cast<long long>( scale ) + 1 )
        res = wxString::FromCDouble( mantissa );

    return res + range;
}


wxString SPICE_VALUE::ToSpiceString() const
{
    wxString res = wxString::FromCDouble( m_base );
    StripZeros( res );
    res += spice_prefix( m_prefix );

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


void SPICE_VALUE::StripZeros( wxString& aString )
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
        DisplayError( aParent, wxString::Format( _( "'%s' is not a valid SPICE value." ),
                                                 text->GetValue() ) );

        return false;
    }

    if( svalue != text->GetValue() )
        text->SetValue( svalue );

    return true;
}
