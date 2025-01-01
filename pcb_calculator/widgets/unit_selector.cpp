/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
  * @file UnitSelector.cpp
  * a wxChoiceBox to select units in Pcb_Calculator
  */

#include "unit_selector.h"
#include "units_scales.h"

UNIT_SELECTOR_LEN::UNIT_SELECTOR_LEN( wxWindow *parent, wxWindowID id,
                                      const wxPoint& pos, const wxSize& size,
                                      const wxArrayString& choices, long style ) :
        UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "mm" ) );
    Append( _( "um" ) );
    Append( _( "cm" ) );
    Append( _( "mil" ) );
    Append( _( "inch" ) );
}


/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (meter)
 */
double UNIT_SELECTOR_LEN::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_MM;     break;
    case 1: return UNIT_MICRON; break;
    case 2: return UNIT_CM;     break;
    case 3: return UNIT_MIL;    break;
    case 4: return UNIT_INCH;   break;
    }
    return 1.0;
}


UNIT_SELECTOR_THICKNESS::UNIT_SELECTOR_THICKNESS( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( wxT( "mm" ) );
    Append( wxT( "µm" ) );
    Append( wxT( "cm" ) );
    Append( wxT( "mil" ) );
    Append( wxT( "inch" ) );
    Append( wxT( "oz/ft²" ) );
}


/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (meter) including copper oz/ft^2
 */
double UNIT_SELECTOR_THICKNESS::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_MM;     break;
    case 1: return UNIT_MICRON; break;
    case 2: return UNIT_CM;     break;
    case 3: return UNIT_MIL;    break;
    case 4: return UNIT_INCH;   break;
    case 5: return UNIT_OZSQFT; break;
    }
    return 1.0;
}


UNIT_SELECTOR_FREQUENCY::UNIT_SELECTOR_FREQUENCY( wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        const wxArrayString& choices, long style ):
    UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "GHz" ) );
    Append( _( "MHz" ) );
    Append( _( "kHz" ) );
    Append( _( "Hz" ) );
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (herz )
 */
double UNIT_SELECTOR_FREQUENCY::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_GHZ;
    case 1: return UNIT_MHZ;
    case 2: return UNIT_KHZ;
    case 3: return 1.0;
    }
    return 1.0;
}


UNIT_SELECTOR_ANGLE::UNIT_SELECTOR_ANGLE( wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        const wxArrayString& choices, long style ) :
    UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "rad" ) );
    Append( _( "deg" ) );
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( radian )
 */
double UNIT_SELECTOR_ANGLE::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_RADIAN; break;
    case 1: return UNIT_DEGREE; break;
    }
    return 1.0;
}


UNIT_SELECTOR_RESISTOR::UNIT_SELECTOR_RESISTOR( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( wxT( "Ω" ) );
    Append( wxT( "kΩ" ) );
}


/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( ohm )
 */
double UNIT_SELECTOR_RESISTOR::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_OHM;    break;
    case 1: return UNIT_KOHM;   break;
    }
    return 1.0;
}


UNIT_SELECTOR_LINEAR_RESISTANCE::UNIT_SELECTOR_LINEAR_RESISTANCE( wxWindow *parent, wxWindowID id,
                  const wxPoint& pos, const wxSize& size,
                  const wxArrayString& choices, long style )
                : UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( wxT( "Ω/m" ) );
    Append( wxT( "Ω/km" ) );
    Append( wxT( "Ω/ft" ) );
    Append( wxT( "Ω/1000ft" ) );
}


/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( ohm )
 */
double UNIT_SELECTOR_LINEAR_RESISTANCE::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_OHM_PER_METER; break;
    case 1: return UNIT_OHM_PER_KILOMETER; break;
    case 2: return UNIT_OHM_PER_FEET; break;
    case 3: return UNIT_OHM_PER_1000FEET; break;
    }
    return 1.0;
}


UNIT_SELECTOR_LEN_CABLE::UNIT_SELECTOR_LEN_CABLE( wxWindow *parent, wxWindowID id,
                                      const wxPoint& pos, const wxSize& size,
                                      const wxArrayString& choices, long style ) :
        UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "cm" ) );
    Append( _( "m" ) );
    Append( _( "km" ) );
    Append( _( "inch" ) );
    Append( _( "feet" ) );
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( m )
 */
double UNIT_SELECTOR_LEN_CABLE::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_CM;     break;
    case 1: return UNIT_M;      break;
    case 2: return UNIT_KM;     break;
    case 3: return UNIT_INCH;    break;
    case 4: return UNIT_FEET;   break;
    }
    return 1.0;
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( V )
 */
double UNIT_SELECTOR_VOLTAGE::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_MILLIVOLT; break;
    case 1: return UNIT_VOLT; break;
    }
    return 1.0;
}

UNIT_SELECTOR_VOLTAGE::UNIT_SELECTOR_VOLTAGE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                              const wxSize& size, const wxArrayString& choices,
                                              long style ) :
        UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "mV" ) );
    Append( _( "V" ) );
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units ( W )
 */
double UNIT_SELECTOR_POWER::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_MILLIWATT; break;
    case 1: return UNIT_WATT; break;
    }
    return 1.0;
}

UNIT_SELECTOR_POWER::UNIT_SELECTOR_POWER( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                          const wxSize& size, const wxArrayString& choices,
                                          long style ) :
        UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "mW" ) );
    Append( _( "W" ) );
}

UNIT_SELECTOR_SPEED::UNIT_SELECTOR_SPEED( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                          const wxSize& size, const wxArrayString& choices,
                                          long style ) :
        UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "m/s" ) );
    Append( _( "ft/s" ) );
    Append( _( "km/h" ) );
    Append( _( "mi/h" ) );
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (meter per second)
 */
double UNIT_SELECTOR_SPEED::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_METER_PER_SECOND; break;
    case 1: return UNIT_FEET_PER_SECOND; break;
    case 2: return UNIT_KILOMETER_PER_HOUR; break;
    case 3: return UNIT_MILES_PER_HOUR; break;
    }
    return 1.0;
}

UNIT_SELECTOR_TIME::UNIT_SELECTOR_TIME( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, const wxArrayString& choices,
                                        long style ) :
        UNIT_SELECTOR( parent, id, pos, size, choices, style )
{
    Append( _( "ns" ) );
    Append( _( "ps" ) );
}

/*
 * Function GetUnitScale
 * return the scaling factor to convert users units
 * to normalized units (second)
 */
double UNIT_SELECTOR_TIME::GetUnitScale()
{
    switch( GetCurrentSelection() )
    {
    case 0: return UNIT_NSECOND; break;
    case 1: return UNIT_PSECOND; break;
    }
    return 1.0;
}