/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2018 KiCad Developers, see change_log.txt for contributors.
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

#include <macros.h>
#include <base_struct.h>
#include <title_block.h>
#include <common.h>
#include <base_units.h>
#include "libeval/numeric_evaluator.h"


#if defined( PCBNEW ) || defined( CVPCB ) || defined( EESCHEMA ) || defined( GERBVIEW ) || defined( PL_EDITOR )
#define IU_TO_MM( x )       ( x / IU_PER_MM )
#define IU_TO_IN( x )       ( x / IU_PER_MILS / 1000 )
#define IU_TO_MILS( x )     ( x / IU_PER_MILS )
#define MM_TO_IU( x )       ( x * IU_PER_MM )
#define IN_TO_IU( x )       ( x * IU_PER_MILS * 1000 )
#define MILS_TO_IU( x )     ( x * IU_PER_MILS )
#else
#error "Cannot resolve internal units due to no definition of EESCHEMA, CVPCB or PCBNEW."
#endif


// Helper function to print a float number without using scientific notation
// and no trailing 0
// So we cannot always just use the %g or the %f format to print a fp number
// this helper function uses the %f format when needed, or %g when %f is
// not well working and then removes trailing 0

std::string Double2Str( double aValue )
{
    char    buf[50];
    int     len;

    if( aValue != 0.0 && fabs( aValue ) <= 0.0001 )
    {
        // For these small values, %f works fine,
        // and %g gives an exponent
        len = sprintf( buf,  "%.16f", aValue );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        if( buf[len] == '.' )
            buf[len] = '\0';
        else
            ++len;
    }
    else
    {
        // For these values, %g works fine, and sometimes %f
        // gives a bad value (try aValue = 1.222222222222, with %.16f format!)
        len = sprintf( buf, "%.16g", aValue );
    }

    return std::string( buf, len );
}


double To_User_Unit( EDA_UNITS_T aUnit, double aValue, bool aUseMils )
{
    switch( aUnit )
    {
    case MILLIMETRES:
        return IU_TO_MM( aValue );

    case INCHES:
        if( aUseMils )
            return IU_TO_MILS( aValue );
        else
            return IU_TO_IN( aValue );

    case DEGREES:
        return aValue / 10.0f;

    default:
        return aValue;
    }
}

/* Convert a value to a string using double notation.
 * For readability, the mantissa has 0, 1, 3 or 4 digits, depending on units
 * for unit = inch the mantissa has 3 digits (Eeschema) or 4 digits
 * for unit = mil the mantissa has 0 digits (Eeschema) or 1 digits
 * for unit = mm the mantissa has 3 digits (Eeschema) or 4 digits
 * Should be used only to display info in status,
 * but not in dialogs, because 4 digits only
 * could truncate the actual value
 */

// A lower-precision (for readability) version of StringFromValue()
wxString MessageTextFromValue( EDA_UNITS_T aUnits, int aValue, bool aUseMils )
{
    return MessageTextFromValue( aUnits, (double) aValue, aUseMils );
}


// A lower-precision (for readability) version of StringFromValue()
wxString MessageTextFromValue( EDA_UNITS_T aUnits, double aValue, bool aUseMils )
{
    wxString      text;
    const wxChar* format;
    double        value = To_User_Unit( aUnits, aValue, aUseMils );

    if( aUnits == INCHES )
    {
        if( aUseMils )
        {
#if defined( EESCHEMA )
            format = wxT( "%.0f" );
#else
            format = wxT( "%.1f" );
#endif
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
    text += " ";

    text += GetAbbreviatedUnitsLabel( aUnits, aUseMils );

    return text;
}

/* Remove trailing 0 from a string containing a converted float number.
 * the trailing 0 are removed if the mantissa has more
 * than aTrailingZeroAllowed digits and some trailing 0
 */
void StripTrailingZeros( wxString& aStringValue, unsigned aTrailingZeroAllowed )
{
    struct lconv * lc = localeconv();
    char sep = lc->decimal_point[0];
    unsigned sep_pos = aStringValue.Find( sep );

    if( sep_pos > 0 )
    {
        // We want to keep at least aTrailingZeroAllowed digits after the separator
        unsigned min_len = sep_pos + aTrailingZeroAllowed + 1;

        while( aStringValue.Len() > min_len )
        {
            if( aStringValue.Last() == '0' )
                aStringValue.RemoveLast();
            else
                break;
        }
    }
}


/* Convert a value to a string using double notation.
 * For readability, the mantissa has 3 or more digits,
 * the trailing 0 are removed if the mantissa has more than 3 digits
 * and some trailing 0
 * This function should be used to display values in dialogs because a value
 * entered in mm (for instance 2.0 mm) could need up to 8 digits mantissa
 * if displayed in inch to avoid truncation or rounding made just by the printf function.
 * otherwise the actual value is rounded when read from dialog and converted
 * in internal units, and therefore modified.
 */
wxString StringFromValue( EDA_UNITS_T aUnits, int aValue, bool aAddUnitSymbol, bool aUseMils )
{
    double  value_to_print = To_User_Unit( aUnits, aValue, aUseMils );

#if defined( EESCHEMA )
    wxString    stringValue = wxString::Format( wxT( "%.3f" ), value_to_print );

    // Strip trailing zeros. However, keep at least 3 digits in mantissa
    // For readability
    StripTrailingZeros( stringValue, 3 );

#else

    char    buf[50];
    int     len;

    if( value_to_print != 0.0 && fabs( value_to_print ) <= 0.0001 )
    {
        len = sprintf( buf, "%.10f", value_to_print );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        if( buf[len]=='.' || buf[len]==',' )
            buf[len] = '\0';
        else
            ++len;
    }
    else
    {
        if( aUnits == INCHES && aUseMils )
            len = sprintf( buf, "%.7g", value_to_print );
        else
            len = sprintf( buf, "%.10g", value_to_print );
    }

    wxString    stringValue( buf, wxConvUTF8 );

#endif

    if( aAddUnitSymbol )
    {
        switch( aUnits )
        {
        case INCHES:
            if( aUseMils )
                stringValue += wxT( " mils" );
            else
                stringValue += wxT( " in" );
            break;

        case MILLIMETRES:
            stringValue += wxT( " mm" );
            break;

        case DEGREES:
            stringValue += wxT( " deg" );
            break;

        case PERCENT:
            stringValue += wxT( "%" );
            break;

        case UNSCALED_UNITS:
            break;
        }
    }

    return stringValue;
}


double From_User_Unit( EDA_UNITS_T aUnits, double aValue, bool aUseMils )
{
    switch( aUnits )
    {
    case MILLIMETRES:
        return MM_TO_IU( aValue );

    case INCHES:
        if( aUseMils )
            return MILS_TO_IU( aValue );
        else
            return IN_TO_IU( aValue );

    case DEGREES:
        // Convert to "decidegrees"
        return aValue * 10;

    default:
    case UNSCALED_UNITS:
    case PERCENT:
        return aValue;
    }
}


double DoubleValueFromString( EDA_UNITS_T aUnits, const wxString& aTextValue, bool aUseMils )
{
    double value;
    double dtmp = 0;

    // Acquire the 'right' decimal point separator
    const struct lconv* lc = localeconv();

    wxChar      decimal_point = lc->decimal_point[0];
    wxString    buf( aTextValue.Strip( wxString::both ) );

    // Convert the period in decimal point
    buf.Replace( wxT( "." ), wxString( decimal_point, 1 ) );

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
    buf.Left( brk_point );

    buf.ToDouble( &dtmp );

    // Check the optional unit designator (2 ch significant)
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );

    if( aUnits == INCHES || aUnits == MILLIMETRES )
    {
        if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
        {
            aUnits = INCHES;
            aUseMils = false;
        }
        else if( unit == wxT( "mm" ) )
        {
            aUnits = MILLIMETRES;
        }
        else if( unit == wxT( "mi" ) || unit == wxT( "th" ) )  // "mils" or "thou"
        {
            aUnits = INCHES;
            aUseMils = true;
        }
    }
    else if( aUnits == DEGREES )
    {
        if( unit == wxT( "ra" ) ) // Radians
        {
            dtmp *= 180.0f / M_PI;
        }
    }

    value = From_User_Unit( aUnits, dtmp, aUseMils );

    return value;
}


void FetchUnitsFromString( const wxString& aTextValue, EDA_UNITS_T& aUnits, bool& aUseMils )
{
    wxString buf( aTextValue.Strip( wxString::both ) );
    unsigned brk_point = 0;

    while( brk_point < buf.Len() )
    {
        wxChar c = buf[brk_point];

        if( !( (c >= '0' && c <='9') || (c == '.') || (c == ',') || (c == '-') || (c == '+') ) )
            break;

        ++brk_point;
    }

    // Check the unit designator (2 ch significant)
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );

    if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
    {
        aUnits = INCHES;
        aUseMils = false;
    }
    else if( unit == wxT( "mm" ) )
    {
        aUnits = MILLIMETRES;
    }
    else if( unit == wxT( "mi" ) || unit == wxT( "th" ) )  // "mils" or "thou"
    {
        aUnits = INCHES;
        aUseMils = true;
    }
    else if( unit == wxT( "de" ) || unit == wxT( "ra" ) )  // "deg" or "rad"
    {
        aUnits = DEGREES;
    }
}


int ValueFromString( EDA_UNITS_T aUnits, const wxString& aTextValue, bool aUseMils )
{
    double value = DoubleValueFromString( aUnits, aTextValue, aUseMils );
    return KiROUND( value );
}


/**
 * Function AngleToStringDegrees
 * is a helper to convert the \a double \a aAngle (in internal unit)
 * to a string in degrees
 */
wxString AngleToStringDegrees( double aAngle )
{
    wxString text;

    text.Printf( wxT( "%.3f" ), aAngle/10.0 );
    StripTrailingZeros( text, 1 );

    return text;
}


wxString GetAbbreviatedUnitsLabel( EDA_UNITS_T aUnit, bool aUseMils )
{
    switch( aUnit )
    {
    case INCHES:
        if( aUseMils )
            return _( "mils" );
        else
            return _( "in" );

    case MILLIMETRES:
        return _( "mm" );

    case PERCENT:
        return _( "%" );

    case UNSCALED_UNITS:
        return wxEmptyString;

    case DEGREES:
        return _( "deg" );

    default:
        return wxT( "??" );
    }
}


std::string FormatInternalUnits( int aValue )
{
    char    buf[50];
    double  engUnits = aValue;
    int     len;

#ifndef EESCHEMA
    engUnits /= IU_PER_MM;
#endif

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        len = snprintf( buf, sizeof(buf), "%.10f", engUnits );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

#ifndef EESCHEMA
        if( buf[len] == '.' )
            buf[len] = '\0';
        else
#endif
            ++len;
    }
    else
    {
        len = snprintf( buf, sizeof(buf), "%.10g", engUnits );
    }

    return std::string( buf, len );
}


std::string FormatAngle( double aAngle )
{
    char temp[50];
    int len;

    len = snprintf( temp, sizeof(temp), "%.10g", aAngle / 10.0 );

    return std::string( temp, len );
}


std::string FormatInternalUnits( const wxPoint& aPoint )
{
    return FormatInternalUnits( aPoint.x ) + " " + FormatInternalUnits( aPoint.y );
}


std::string FormatInternalUnits( const VECTOR2I& aPoint )
{
    return FormatInternalUnits( aPoint.x ) + " " + FormatInternalUnits( aPoint.y );
}


std::string FormatInternalUnits( const wxSize& aSize )
{
    return FormatInternalUnits( aSize.GetWidth() ) + " " + FormatInternalUnits( aSize.GetHeight() );
}

