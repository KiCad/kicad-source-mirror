/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_units.h>
#include <math/util.h>      // for KiROUND
#include <macros.h>

bool EDA_UNIT_UTILS::IsImperialUnit( EDA_UNITS aUnit )
{
    switch( aUnit )
    {
    case EDA_UNITS::INCHES:
    case EDA_UNITS::MILS:
        return true;

    default:
        return false;
    }

    return false;
}


bool EDA_UNIT_UTILS::IsMetricUnit( EDA_UNITS aUnit )
{
    switch( aUnit )
    {
    case EDA_UNITS::MILLIMETRES:
        return true;

    default:
        return false;
    }

    return false;
}


int EDA_UNIT_UTILS::Mm2mils( double aVal )
{
    return KiROUND( aVal * 1000. / 25.4 );
}


int EDA_UNIT_UTILS::Mils2mm( double aVal )
{
    return KiROUND( aVal * 25.4 / 1000. );
}


void EDA_UNIT_UTILS::FetchUnitsFromString( const wxString& aTextValue, EDA_UNITS& aUnits )
{
    wxString buf( aTextValue.Strip( wxString::both ) );
    unsigned brk_point = 0;

    while( brk_point < buf.Len() )
    {
        wxChar c = buf[brk_point];

        if( !( ( c >= '0' && c <= '9' ) || ( c == '.' ) || ( c == ',' ) || ( c == '-' )
               || ( c == '+' ) ) )
            break;

        ++brk_point;
    }

    // Check the unit designator (2 ch significant)
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );

    if( unit == wxT( "mm" ) )
        aUnits = EDA_UNITS::MILLIMETRES;
    else if( unit == wxT( "mi" ) || unit == wxT( "th" ) ) // "mils" or "thou"
        aUnits = EDA_UNITS::MILS;
    else if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
        aUnits = EDA_UNITS::INCHES;
    else if( unit == wxT( "de" ) || unit == wxT( "ra" ) ) // "deg" or "rad"
        aUnits = EDA_UNITS::DEGREES;
}


wxString EDA_UNIT_UTILS::GetAbbreviatedUnitsLabel( EDA_UNITS aUnits, EDA_DATA_TYPE aType )
{
    wxString label;

    switch( aUnits )
    {
    case EDA_UNITS::MILLIMETRES: label = wxT( " mm" ); break;
    case EDA_UNITS::DEGREES: label = wxT( "°" ); break;
    case EDA_UNITS::MILS: label = wxT( " mils" ); break;
    case EDA_UNITS::INCHES: label = wxT( " in" ); break;
    case EDA_UNITS::PERCENT: label = wxT( "%" ); break;
    case EDA_UNITS::UNSCALED: break;
    default: UNIMPLEMENTED_FOR( "Unknown units" ); break;
    }

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME: label += wxT( "³" ); break;
    case EDA_DATA_TYPE::AREA: label += wxT( "²" ); break;
    case EDA_DATA_TYPE::DISTANCE: break;
    default: UNIMPLEMENTED_FOR( "Unknown measurement" ); break;
    }

    return label;
}


std::string EDA_UNIT_UTILS::FormatAngle( const EDA_ANGLE& aAngle )
{
    char temp[50];
    int  len;

    len = snprintf( temp, sizeof( temp ), "%.10g", aAngle.AsDegrees() );

    return std::string( temp, len );
}