/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pads_unit_converter.h"

#include <cmath>

PADS_UNIT_CONVERTER::PADS_UNIT_CONVERTER() :
        m_unitType( PADS_UNIT_TYPE::MILS ),
        m_scaleFactor( MILS_TO_NM ),
        m_basicUnitsMode( false ),
        m_basicUnitsScale( BASIC_TO_NM )
{
}


void PADS_UNIT_CONVERTER::SetBaseUnits( PADS_UNIT_TYPE aUnitType )
{
    m_unitType = aUnitType;
    updateScaleFactor();
}


void PADS_UNIT_CONVERTER::SetBasicUnitsMode( bool aEnabled )
{
    m_basicUnitsMode = aEnabled;
    updateScaleFactor();
}


void PADS_UNIT_CONVERTER::SetBasicUnitsScale( double aScale )
{
    m_basicUnitsScale = aScale;

    if( m_basicUnitsMode )
        updateScaleFactor();
}


bool PADS_UNIT_CONVERTER::ParseFileHeader( const std::string& aHeader )
{
    // Look for BASIC indicator in header like "!PADS-POWERPCB-V9.0-BASIC!"
    if( aHeader.find( "BASIC" ) != std::string::npos ||
        aHeader.find( "basic" ) != std::string::npos )
    {
        SetBasicUnitsMode( true );
        return true;
    }

    // Look for explicit unit type indicators
    if( aHeader.find( "MILS" ) != std::string::npos ||
        aHeader.find( "mils" ) != std::string::npos )
    {
        SetBasicUnitsMode( false );
        SetBaseUnits( PADS_UNIT_TYPE::MILS );
        return true;
    }

    if( aHeader.find( "METRIC" ) != std::string::npos ||
        aHeader.find( "metric" ) != std::string::npos )
    {
        SetBasicUnitsMode( false );
        SetBaseUnits( PADS_UNIT_TYPE::METRIC );
        return true;
    }

    if( aHeader.find( "INCHES" ) != std::string::npos ||
        aHeader.find( "inches" ) != std::string::npos )
    {
        SetBasicUnitsMode( false );
        SetBaseUnits( PADS_UNIT_TYPE::INCHES );
        return true;
    }

    return false;
}


std::optional<PADS_UNIT_TYPE> PADS_UNIT_CONVERTER::ParseUnitCode( const std::string& aUnitCode )
{
    if( aUnitCode.empty() )
        return std::nullopt;

    // "M" and "D" (default) both mean MILS
    if( aUnitCode == "M" || aUnitCode == "m" || aUnitCode == "D" || aUnitCode == "d" ||
        aUnitCode == "MILS" || aUnitCode == "mils" || aUnitCode == "MIL" || aUnitCode == "mil" )
    {
        return PADS_UNIT_TYPE::MILS;
    }

    // "MM" means METRIC (millimeters)
    if( aUnitCode == "MM" || aUnitCode == "mm" ||
        aUnitCode == "METRIC" || aUnitCode == "metric" )
    {
        return PADS_UNIT_TYPE::METRIC;
    }

    // "I" means INCHES
    if( aUnitCode == "I" || aUnitCode == "i" ||
        aUnitCode == "INCHES" || aUnitCode == "inches" || aUnitCode == "INCH" || aUnitCode == "inch" )
    {
        return PADS_UNIT_TYPE::INCHES;
    }

    // "N" means no override
    if( aUnitCode == "N" || aUnitCode == "n" )
        return std::nullopt;

    return std::nullopt;
}


bool PADS_UNIT_CONVERTER::PushUnitOverride( const std::string& aUnitCode )
{
    std::optional<PADS_UNIT_TYPE> unitType = ParseUnitCode( aUnitCode );

    if( !unitType.has_value() )
        return false;

    m_unitOverrideStack.push_back( *unitType );
    updateScaleFactor();
    return true;
}


void PADS_UNIT_CONVERTER::PopUnitOverride()
{
    if( !m_unitOverrideStack.empty() )
    {
        m_unitOverrideStack.pop_back();
        updateScaleFactor();
    }
}


void PADS_UNIT_CONVERTER::updateScaleFactor()
{
    if( m_basicUnitsMode )
    {
        m_scaleFactor = m_basicUnitsScale;
        return;
    }

    // Use override if present, otherwise use base unit type
    PADS_UNIT_TYPE effectiveType = m_unitOverrideStack.empty()
                                           ? m_unitType
                                           : m_unitOverrideStack.back();

    switch( effectiveType )
    {
    case PADS_UNIT_TYPE::MILS:
        m_scaleFactor = MILS_TO_NM;
        break;

    case PADS_UNIT_TYPE::METRIC:
        m_scaleFactor = MM_TO_NM;
        break;

    case PADS_UNIT_TYPE::INCHES:
        m_scaleFactor = INCHES_TO_NM;
        break;
    }
}


int64_t PADS_UNIT_CONVERTER::ToNanometers( double aValue ) const
{
    return static_cast<int64_t>( std::round( aValue * m_scaleFactor ) );
}


int64_t PADS_UNIT_CONVERTER::ToNanometersSize( double aValue ) const
{
    return static_cast<int64_t>( std::round( aValue * m_scaleFactor ) );
}
