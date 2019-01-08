/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string>

#include <math/vector2d.h>
#include <common.h>
#include <convert_to_biu.h>

//TODO: Abstract Base Units to a single class

/**
 * Used for holding indeterminate values, such as with multiple selections
 * holding different values or controls which do not wish to set a value.
 */
#define INDETERMINATE wxString( "<...>" )


/// Convert mm to mils.
inline int Mm2mils( double x ) { return KiROUND( x * 1000./25.4 ); }

/// Convert mils to mm.
inline int Mils2mm( double x ) { return KiROUND( x * 25.4 / 1000. ); }

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
 * @param aUseMils Indicates mils should be used for imperial units (inches).
 */
double To_User_Unit( EDA_UNITS_T aUnit, double aValue, bool aUseMils = false );

/**
 * Function AngleToStringDegrees
 * is a helper to convert the \a double \a aAngle (in internal unit)
 * to a string in degrees
 */
wxString AngleToStringDegrees( double aAngle );

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
 * @param aUseMils Convert inch values to mils if true.
 * @return The converted string for display in user interface elements.
 */
wxString MessageTextFromValue( EDA_UNITS_T aUnits, double aValue, bool aUseMils = false );

wxString MessageTextFromValue( EDA_UNITS_T aUnits, int aValue, bool aUseMils = false );

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
 * @param aUseMils Indicates mils should be used for imperial units (inches).
 * @return A wxString object containing value and optionally the symbol unit (like 2.000 mm)
 */
wxString StringFromValue( EDA_UNITS_T aUnit, int aValue, bool aAddUnitSymbol = false,
                          bool aUseMils = false );

/**
 * Return in internal units the value "val" given in a real unit
 * such as "in", "mm" or "deg"
 */
double From_User_Unit( EDA_UNITS_T aUnit, double aValue, bool aUseMils = false );


/**
 * Function DoubleValueFromString
 * converts \a aTextValue to a double
 * @param aUnits The units of \a aTextValue.
 * @param aTextValue A reference to a wxString object containing the string to convert.
 * @param aUseMils Indicates mils should be used for imperial units (inches).
 * @return A double representing that value in internal units
 */
double DoubleValueFromString( EDA_UNITS_T aUnits, const wxString& aTextValue,
                              bool aUseMils = false );

/**
 * Function ValueFromString
 * converts \a aTextValue in \a aUnits to internal units used by the application.
 *
 * @param aUnits The units of \a aTextValue.
 * @param aTextValue A reference to a wxString object containing the string to convert.
 * @param aUseMils Indicates mils should be used for imperial units (inches).
 * @return The string from Value, according to units (inch, mm ...) for display,
 */
int ValueFromString( EDA_UNITS_T aUnits, const wxString& aTextValue, bool aUseMils = false );

/**
 * Function FetchUnitsFromString
 * writes any unit info found in the string to aUnits and aUseMils.
 */
void FetchUnitsFromString( const wxString& aTextValue, EDA_UNITS_T& aUnits, bool& aUseMils );

/**
 * Get the units string for a given units type.
 *
 * @param aUnits - The units requested.
 * @return The human readable units string.
 */
wxString GetAbbreviatedUnitsLabel( EDA_UNITS_T aUnit, bool aUseMils = false );

/**
 * Function FormatInternalUnits
 * converts \a aValue from internal units to a string appropriate for writing
 * to file.
 *
 * @note Internal units for board items can be either deci-mils or nanometers depending
 *       on how KiCad is built.
 *
 * @param aValue A coordinate value to convert.
 * @return A std::string object containing the converted value.
 */
std::string FormatInternalUnits( int aValue );

/**
 * Function FormatAngle
 * converts \a aAngle from board units to a string appropriate for writing to file.
 *
 * @note Internal angles for board items can be either degrees or tenths of degree
 *       on how KiCad is built.
 * @param aAngle A angle value to convert.
 * @return A std::string object containing the converted angle.
 */
std::string FormatAngle( double aAngle );

std::string FormatInternalUnits( const wxPoint& aPoint );

std::string FormatInternalUnits( const wxSize& aSize );

std::string FormatInternalUnits( const VECTOR2I& aPoint );


#endif   // _BASE_UNITS_H_
