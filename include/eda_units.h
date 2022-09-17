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
#include <base_units.h>

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
     * This should only be used for writing to files  as it ignores locale
     *
     * @note Internal angles for board items can be either degrees or tenths of degree
     *       on how KiCad is built.
     * @param aAngle A angle value to convert.
     * @return std::string object containing the converted angle.
     */
    std::string FormatAngle( const EDA_ANGLE& aAngle );

    /**
     * Converts \a aValue from internal units to a string appropriate for writing
     * to file.
     *
     * This should only be used for writing to files as it ignores locale
     *
     * @note Internal units for board items can be either deci-mils or nanometers depending
     *       on how KiCad is built.
     *
     * @param aValue A coordinate value to convert.
     * @return A std::string object containing the converted value.
     */
    std::string FormatInternalUnits( const EDA_IU_SCALE& aIuScale, int aValue );
    std::string FormatInternalUnits( const EDA_IU_SCALE& aIuScale, const wxPoint& aPoint );
    std::string FormatInternalUnits( const EDA_IU_SCALE& aIuScale, const wxSize& aSize );
    std::string FormatInternalUnits( const EDA_IU_SCALE& aIuScale, const VECTOR2I& aPoint );

    constexpr inline int Mils2IU( const EDA_IU_SCALE& aIuScale, int mils )
    {
        double x = mils * aIuScale.IU_PER_MILS;
        return int( x < 0 ? x - 0.5 : x + 0.5 );
    }

    namespace UI
    {
        /**
         * Function To_User_Unit
         * convert \a aValue in internal units to the appropriate user units defined by \a aUnit.
         *
         * @return The converted value, in double
         * @param aUnit The units to convert \a aValue to.
         * @param aValue The value in internal units to convert.
         */
        double ToUserUnit( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnit, double aValue );

        /**
         * Function StringFromValue
         * returns the string from \a aValue according to units (inch, mm ...) for display,
         * and the initial unit for value.
         *
         * For readability, the mantissa has 3 or more digits (max 8 digits),
         * the trailing 0 are removed if the mantissa has more than 3 digits
         * and some trailing 0
         * This function should be used to display values in dialogs because a value
         * entered in mm (for instance 2.0 mm) could need up to 8 digits mantissa
         * if displayed in inch to avoid truncation or rounding made just by the printf function.
         * otherwise the actual value is rounded when read from dialog and converted
         * in internal units, and therefore modified.
         *
         * @param aUnit = display units (INCHES, MILLIMETRE ..)
         * @param aValue = value in Internal_Unit
         * @param aAddUnitSymbol = true to add symbol unit to the string value
         * @return A wxString object containing value and optionally the symbol unit (like 2.000 mm)
         */
        wxString StringFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnit, double aValue,
                                  bool          aAddUnitSymbol = false,
                                  EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

        /**
         * Function MessageTextFromValue
         * is a helper to convert the \a double length \a aValue to a string in inches,
         * millimeters, or unscaled units.
         *
         * Should be used only to display a coordinate in status, but not in dialogs,
         * files, etc., because the mantissa of the number displayed has 4 digits max
         * for readability.  The actual internal value could need up to 8 digits to be
         * printed.
         *
         * Use StringFromValue() instead where precision matters.
         *
         * @param aUnits The units to show the value in.  The unit string is added to the
         *               message text.
         * @param aValue The double value to convert.
         * @param aAddUnitLabel If true, adds the unit label to the end of the string
         * @param aType Type of the unit being used (e.g. distance, area, etc.)
         * @return The converted string for display in user interface elements.
         */
        wxString MessageTextFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits, double aValue, bool aAddUnitLabel = true,
                                       EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

        wxString MessageTextFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits, int aValue,
                                       bool          aAddUnitLabel = true,
                                       EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

        wxString MessageTextFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits, long long int aValue,
                                       bool          aAddUnitLabel = true,
                                       EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

        wxString MessageTextFromValue( EDA_ANGLE aValue, bool aAddUnitLabel = true );


        /**
         * Return in internal units the value "val" given in a real unit
         * such as "in", "mm" or "deg"
         */
        double FromUserUnit( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnit, double aValue );


        /**
         * Function DoubleValueFromString
         * converts \a aTextValue to a double
         * @warning This utilizes the current locale and will break if decimal formats differ
         *
         * @param aUnits The units of \a aTextValue.
         * @param aTextValue A reference to a wxString object containing the string to convert.
         * @return A double representing that value in internal units
         */
        double DoubleValueFromString( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                      const wxString& aTextValue,
                                      EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

        double DoubleValueFromString( const wxString& aTextValue );

        /**
         * Function ValueFromString
         * converts \a aTextValue in \a aUnits to internal units used by the application.
         * @warning This utilizes the current locale and will break if decimal formats differ
         *
         * @param aUnits The units of \a aTextValue.
         * @param aTextValue A reference to a wxString object containing the string to convert.
         * @return The string from Value, according to units (inch, mm ...) for display,
         */
        long long int ValueFromString( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                       const wxString& aTextValue,
                                       EDA_DATA_TYPE aType = EDA_DATA_TYPE::DISTANCE );

        long long int ValueFromString( const wxString& aTextValue);
    }
}

#endif