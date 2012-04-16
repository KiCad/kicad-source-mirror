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



/// Scalar to convert mils to internal units.
#if defined( PCBNEW )
#if defined( USE_PCBNEW_NANOMETRES )
#define MILS_TO_IU_SCALAR   25.4e3         // Pcbnew in nanometers.
#else
#define MILS_TO_IU_SCALAR   10.0           // Pcbnew in deci-mils.
#endif
#else
#define MILS_TO_IU_SCALAR   1.0            // Eeschema and anything else.
#endif


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
 * is a helper to convert the integer coordinate \a aValue to a string in inches,
 * millimeters, or unscaled units according to the current user units setting.
 *
 * @param aValue The coordinate to convert.
 * @param aConvertToMils Convert inch values to mils if true.  This setting has no effect if
 *                       the current user unit is millimeters.
 * @return The converted string for display in user interface elements.
 */
wxString CoordinateToString( int aValue, bool aConvertToMils = false );


/**
 * Function ReturnStringFromValue
 * returns the string from \a aValue according to units (inch, mm ...) for display,
 * and the initial unit for value.
 * @param aUnit = display units (INCHES, MILLIMETRE ..)
 * @param aValue = value in Internal_Unit
 * @param aAddUnitSymbol = true to add symbol unit to the string value
 * @return A wxString object containing value and optionally the symbol unit (like 2.000 mm)
 */
wxString ReturnStringFromValue( EDA_UNITS_T aUnit, int aValue, bool aAddUnitSymbol = false );

/**
 * Function PutValueInLocalUnits
 * converts \a aValue from internal units to user units and append the units notation
 * (mm or ")then inserts the string an \a aTextCtrl.
 *
 * This function is used in dialog boxes for entering values depending on selected units.
 */
void PutValueInLocalUnits( wxTextCtrl& aTextCtr, int aValue );

#endif   // _BASE_UNITS_H_
