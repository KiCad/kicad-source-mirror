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


#if defined( PCBNEW ) || defined( CVPCB ) || defined( EESCHEMA )
#define IU_TO_MM( x )       ( x / IU_PER_MM )
#define IU_TO_IN( x )       ( x / IU_PER_MILS / 1000 )
#define MM_TO_IU( x )       ( x * IU_PER_MM )
#define IN_TO_IU( x )       ( x * IU_PER_MILS * 1000 )
#else
#error "Cannot resolve internal units due to no definition of EESCHEMA, CVPCB or PCBNEW."
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
    return LengthDoubleToString( (double) aValue, aConvertToMils );
}

wxString LengthDoubleToString( double aValue, bool aConvertToMils )
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


double From_User_Unit( EDA_UNITS_T aUnit, double aValue )
{
    double value;

    switch( aUnit )
    {
    case MILLIMETRES:
        value = MM_TO_IU( aValue );
        break;

    case INCHES:
        value = IN_TO_IU( aValue );
        break;

    default:
    case UNSCALED_UNITS:

        value = aValue;
    }

    return value;
}




int ReturnValueFromString( EDA_UNITS_T aUnits, const wxString& aTextValue )
{
    int    Value;
    double dtmp = 0;

    // Acquire the 'right' decimal point separator
    const struct lconv* lc = localeconv();
    wxChar decimal_point = lc->decimal_point[0];
    wxString            buf( aTextValue.Strip( wxString::both ) );

    // Convert the period in decimal point
    buf.Replace( wxT( "." ), wxString( decimal_point, 1 ) );

    // An ugly fix needed by WxWidgets 2.9.1 that sometimes
    // back to a point as separator, although the separator is the comma
    // TODO: remove this line if WxWidgets 2.9.2 fixes this issue
    buf.Replace( wxT( "," ), wxString( decimal_point, 1 ) );

    // Find the end of the numeric part
    unsigned brk_point = 0;

    while( brk_point < buf.Len() )
    {
        wxChar ch = buf[brk_point];

        if( !( (ch >= '0' && ch <='9') || (ch == decimal_point) || (ch == '-') || (ch == '+') ) )
        {
            break;
        }

        ++brk_point;
    }

    // Extract the numeric part
    buf.Left( brk_point ).ToDouble( &dtmp );

    // Check the optional unit designator (2 ch significant)
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );

    if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
    {
        aUnits = INCHES;
    }
    else if( unit == wxT( "mm" ) )
    {
        aUnits = MILLIMETRES;
    }
    else if( unit == wxT( "mi" ) || unit == wxT( "th" ) ) // Mils or thous
    {
        aUnits = INCHES;
        dtmp /= 1000;
    }

    Value = From_User_Unit( aUnits, dtmp );

    return Value;
}


int ReturnValueFromTextCtrl( const wxTextCtrl& aTextCtr )
{
    int      value;
    wxString msg = aTextCtr.GetValue();

    value = ReturnValueFromString( g_UserUnit, msg );

    return value;
}


wxString& operator <<( wxString& aString, const wxPoint& aPos )
{
    aString << wxT( "@ (" ) << CoordinateToString( aPos.x );
    aString << wxT( "," ) << CoordinateToString( aPos.y );
    aString << wxT( ")" );

    return aString;
}

