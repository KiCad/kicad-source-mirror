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


#ifndef EDA_UNITS_H
#define EDA_UNITS_H

#include <wx/string.h>
#include <geometry/eda_angle.h>

/**
 * The type of unit.
 */
enum class EDA_DATA_TYPE
{
    DISTANCE = 0,
    AREA     = 1,
    VOLUME   = 2
};

enum class EDA_UNITS
{
    INCHES      = 0,
    MILLIMETRES = 1,
    UNSCALED    = 2,
    DEGREES     = 3,
    PERCENT     = 4,
    MILS        = 5,
};

namespace EDA_UNIT_UTILS
{
    bool IsImperialUnit( EDA_UNITS aUnit );

    bool IsMetricUnit( EDA_UNITS aUnit );

    /**
     *  Convert mm to mils.
     */
    int Mm2mils( double aVal );

    /**
     *  Convert mils to mm.
     */
    int Mils2mm( double aVal );

    /**
     * Writes any unit info found in the string to aUnits.
     */
    void FetchUnitsFromString( const wxString& aTextValue, EDA_UNITS& aUnits );

    /**
     * Get the units string for a given units type.
     *
     * @param aUnits - The units requested.
     * @param aType - The data type of the unit (e.g. distance, area, etc.)
     * @return The human readable units string.
     */
    wxString GetAbbreviatedUnitsLabel( EDA_UNITS aUnit, EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

    /**
     * Converts \a aAngle from board units to a string appropriate for writing to file.
     *
     * @note Internal angles for board items can be either degrees or tenths of degree
     *       on how KiCad is built.
     * @param aAngle A angle value to convert.
     * @return std::string object containing the converted angle.
     */
    std::string FormatAngle( const EDA_ANGLE& aAngle );
}

#endif