/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
 *
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

/**
 * @author Wayne Stambaugh <stambaughw@verizon.net>
 * @file base_units.h
 * @brief Implementation of conversion functions that require both schematic and board
 *        internal units.
 */

#ifndef _BASE_UNITS_H_
#define _BASE_UNITS_H_


#include <common.h>
#include <convert_to_biu.h>

/** Helper function Double2Str to print a float number without
 * using scientific notation and no trailing 0
 * We want to avoid scientific notation in S-expr files (not easy to read)
 * for floating numbers.
 * So we cannot always just use the %g or the %f format to print a fp number
 * this helper function uses the %f format when needed, or %g when %f is
 * not well working and then removes trailing 0
 */
std::string Double2Str( double aValue );

/**
 * Function StripTrailingZeros
 * Remove trailing 0 from a string containing a converted float number.
 * The trailing 0 are removed if the mantissa has more
 * than aTrailingZeroAllowed digits and some trailing 0
 */
void StripTrailingZeros( wxString& aStringValue, unsigned aTrailingZeroAllowed = 1 );


/**
 * Function To_User_Unit
 * convert \a aValue in internal units to the appropriate user units defined by \a aUnit.
 *
 * @return The converted value, in double
 * @param aUnit The units to convert \a aValue to.
 * @param aValue The value in internal units to convert.
 */
double To_User_Unit( EDA_UNITS_T aUnit, double aValue );

/**
 * Function CoordinateToString
 * is a helper to convert the \a integer coordinate \a aValue to a string in inches,
 * millimeters, or unscaled units according to the current user units setting.
 *
 * Should be used only to display a coordinate in status, but not in dialogs,
 * because the mantissa of the number displayed has 4 digits max for readability.
 * (i.e. the value shows the decimils or the microns )
 * However the actual internal value could need up to 8 digits to be printed
 *
 * @param aValue The integer coordinate to convert.
 * @param aConvertToMils Convert inch values to mils if true.  This setting has no effect if
 *                       the current user unit is millimeters.
 * @return The converted string for display in user interface elements.
 */
wxString CoordinateToString( int aValue, bool aConvertToMils = false );

/**
 * Function AngleToStringDegrees
 * is a helper to convert the \a double \a aAngle (in internal unit)
 * to a string in degrees
 */
wxString AngleToStringDegrees( double aAngle );

/**
 * Function LenghtDoubleToString
 * is a helper to convert the \a double length \a aValue to a string in inches,
 * millimeters, or unscaled units according to the current user units setting.
 *
 * Should be used only to display a coordinate in status, but not in dialogs,
 * because the mantissa of the number displayed has 4 digits max for readability.
 * (i.e. the value shows the decimils or the microns )
 * However the actual internal value could need up to 8 digits to be printed
 *
 * @param aValue The double value to convert.
 * @param aConvertToMils Convert inch values to mils if true.  This setting has no effect if
 *                       the current user unit is millimeters.
 * @return The converted string for display in user interface elements.
 */
wxString LengthDoubleToString( double aValue, bool aConvertToMils = false );

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
wxString StringFromValue( EDA_UNITS_T aUnit, int aValue, bool aAddUnitSymbol = false );

/**
 * Operator << overload
 * outputs a point to the argument string in a format resembling
 * "@ (x,y)
 * @param aString Where to put the text describing the point value
 * @param aPoint  The point to output.
 * @return wxString& - the input string
 */
wxString& operator <<( wxString& aString, const wxPoint& aPoint );

/**
 * Function PutValueInLocalUnits
 * converts \a aValue from internal units to user units and append the units notation
 * (mm or ")then inserts the string an \a aTextCtrl.
 *
 * This function is used in dialog boxes for entering values depending on selected units.
 */
void PutValueInLocalUnits( wxTextCtrl& aTextCtr, int aValue );

/**
 * Return in internal units the value "val" given in inch or mm
 */
double From_User_Unit( EDA_UNITS_T aUnit, double aValue );

/**
 * Function ValueFromString
 * converts \a aTextValue in \a aUnits to internal units used by the application.
 *
 * @param aUnits The units of \a aTextValue.
 * @param aTextValue A reference to a wxString object containing the string to convert.
 * @return The string from Value, according to units (inch, mm ...) for display,
 */
int ValueFromString( EDA_UNITS_T aUnits, const wxString& aTextValue );

/**
 * Function ValueFromString

 * converts \a aTextValue in \a aUnits to internal units used by the application,
 * unit type will be obtained from g_UserUnit.
 *
 * @param aTextValue A reference to a wxString object containing the string to convert.
 * @return The string from Value, according to units (inch, mm ...) for display,
 */

int ValueFromString( const wxString& aTextValue );

/**
 * Convert the number Value in a string according to the internal units
 *  and the selected unit (g_UserUnit) and put it in the wxTextCtrl TextCtrl
 */
int ValueFromTextCtrl( const wxTextCtrl& aTextCtr );

#endif   // _BASE_UNITS_H_
