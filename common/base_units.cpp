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
 * @file base_units.cpp
 * @brief Code to handle objects that require both schematic and board internal units.
 * @note This file is an ugly hack to solve the problem of formatting the base units
 *       for either schematics or boards in objects that are include in both domains.
 *       At some point in the future.  This code should be rolled back into the
 *       appropriate object and build with the correct internal unit formatting
 *       depending on the application.
 */

#include <base_struct.h>
#include <class_title_block.h>
#include <common.h>
#include <base_units.h>


#if defined( PCBNEW )
#if defined( USE_PCBNEW_NANOMETRES )
#define IU_TO_MM( x )       ( x * 1e-6 )
#define IU_TO_IN( x )       ( ( x * 1e-6 ) / 25.4 )
#else
#define IU_TO_MM( x )       ( ( x * 0.0001 ) * 25.4 )
#define IU_TO_IN( x )       ( x * 0.0001 )
#endif
#elif defined( EESCHEMA )
#define IU_TO_MM( x )       ( ( x * 0.001 ) * 25.4 )
#define IU_TO_IN( x )       ( x * 0.001 )
#else
#error "Cannot resolve internal units due to no definition of EESCHEMA or PCBNEW."
#endif


double To_User_Unit( EDA_UNITS_T aUnit, double aValue )
{
    switch( aUnit )
    {
    case MILLIMETRES:
        return IU_TO_MM( aValue );

    case INCHES:
        return IU_TO_IN( aValue );

    default:
        return aValue;
    }
}


wxString CoordinateToString( int aValue, bool aConvertToMils )
{
    wxString      text;
    const wxChar* format;
    double        value = To_User_Unit( g_UserUnit, aValue );

    if( g_UserUnit == INCHES )
    {
        if( aConvertToMils )
        {
#if defined( EESCHEMA )
            format = wxT( "%.0f" );
#else
            format = wxT( "%.1f" );
#endif
            if( aConvertToMils )
                value *= 1000;
        }
        else
        {
#if defined( EESCHEMA )
            format = wxT( "%.3f" );
#else
            format = wxT( "%.4f" );
#endif
        }
    }
    else
    {
#if defined( EESCHEMA )
        format = wxT( "%.2f" );
#else
        format = wxT( "%.3f" );
#endif
    }

    text.Printf( format, value );

    if( g_UserUnit == INCHES )
        text += ( aConvertToMils ) ? _( " mils" ) : _( " in" );
    else
        text += _( " mm" );

    return text;
}


wxString ReturnStringFromValue( EDA_UNITS_T aUnit, int aValue, bool aAddUnitSymbol )
{
    wxString StringValue;
    double   value_to_print;

    value_to_print = To_User_Unit( aUnit, aValue );

#if defined( PCBNEW )
    StringValue.Printf( wxT( "%.4f" ), value_to_print );
#else
    StringValue.Printf( wxT( "%.3f" ), value_to_print );
#endif

    if( aAddUnitSymbol )
    {
        switch( aUnit )
        {
        case INCHES:
            StringValue += _( " \"" );
            break;

        case MILLIMETRES:
            StringValue += _( " mm" );
            break;

        case UNSCALED_UNITS:
            break;
        }
    }

    return StringValue;
}


void PutValueInLocalUnits( wxTextCtrl& aTextCtr, int aValue )
{
    wxString msg = ReturnStringFromValue( g_UserUnit, aValue );

    aTextCtr.SetValue( msg );
}
