/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @author Wayne Stambaugh <stambaughw@gmail.com>
 * @file base_units.cpp
 * @brief Code to handle objects that require both schematic and board internal units.
 * @note This file is an ugly hack to solve the problem of formatting the base units
 *       for either schematics or boards in objects that are include in both domains.
 *       At some point in the future.  This code should be rolled back into the
 *       appropriate object and build with the correct internal unit formatting
 *       depending on the application.
 */

#include <base_units.h>
#include <common.h>
#include <kicad_string.h>
#include <math/util.h>      // for KiROUND
#include <macros.h>
#include <title_block.h>

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


double To_User_Unit( EDA_UNITS aUnit, double aValue )
{
    switch( aUnit )
    {
    case EDA_UNITS::MILLIMETRES:
        return IU_TO_MM( aValue );

    case EDA_UNITS::MILS:
        return IU_TO_MILS( aValue );

    case EDA_UNITS::INCHES:
        return IU_TO_IN( aValue );

    case EDA_UNITS::DEGREES:
        return aValue / 10.0f;

    default:
        return aValue;
    }
}

/**
 * Convert a value to a string using double notation.
 *
 * For readability, the mantissa has 0, 1, 3 or 4 digits, depending on units
 * for unit = inch the mantissa has 3 digits (Eeschema) or 4 digits
 * for unit = mil the mantissa has 0 digits (Eeschema) or 1 digits
 * for unit = mm the mantissa has 3 digits (Eeschema) or 4 digits
 * Should be used only to display info in status,
 * but not in dialogs, because 4 digits only
 * could truncate the actual value
 */

// A lower-precision (for readability) version of StringFromValue()
wxString MessageTextFromValue( EDA_UNITS aUnits, int aValue, bool aAddUnitLabel,
                               EDA_DATA_TYPE aType )
{
    return MessageTextFromValue( aUnits, double( aValue ), aAddUnitLabel, aType );
}


// A lower-precision (for readability) version of StringFromValue()
wxString MessageTextFromValue( EDA_UNITS aUnits, long long int aValue, bool aAddUnitLabel,
                               EDA_DATA_TYPE aType )
{
    return MessageTextFromValue( aUnits, double( aValue ), aAddUnitLabel, aType );
}


// A lower-precision (for readability) version of StringFromValue()
wxString MessageTextFromValue( EDA_UNITS aUnits, double aValue, bool aAddUnitLabel,
                               EDA_DATA_TYPE aType )
{
    wxString      text;
    const wxChar* format;
    double        value = aValue;

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:
        value = To_User_Unit( aUnits, value );
        // Fall through to continue computation
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::AREA:
        value = To_User_Unit( aUnits, value );
        // Fall through to continue computation
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::DISTANCE:
        value = To_User_Unit( aUnits, value );
    }

    switch( aUnits )
    {
    default:
    case EDA_UNITS::MILLIMETRES:
#if defined( EESCHEMA )
        format = wxT( "%.2f" );
#else
        format = wxT( "%.4f" );
#endif
        break;

    case EDA_UNITS::MILS:
#if defined( EESCHEMA )
        format = wxT( "%.0f" );
#else
        format = wxT( "%.2f" );
#endif
        break;

    case EDA_UNITS::INCHES:
#if defined( EESCHEMA )
        format = wxT( "%.3f" );
#else
        format = wxT( "%.4f" );
#endif
        break;

    case EDA_UNITS::DEGREES:
        // 3 digits in mantissa should be good for rotation in degree
        format = wxT( "%.3f" );
        break;

    case EDA_UNITS::UNSCALED:
        format = wxT( "%.0f" );
        break;
    }

    text.Printf( format, value );

    if( aAddUnitLabel )
    {
        text += " ";
        text += GetAbbreviatedUnitsLabel( aUnits, aType );
    }

    return text;
}


/**
 * Convert a value to a string using double notation.
 *
 * For readability, the mantissa has 3 or more digits,
 * the trailing 0 are removed if the mantissa has more than 3 digits
 * and some trailing 0
 * This function should be used to display values in dialogs because a value
 * entered in mm (for instance 2.0 mm) could need up to 8 digits mantissa
 * if displayed in inch to avoid truncation or rounding made just by the printf function.
 * otherwise the actual value is rounded when read from dialog and converted
 * in internal units, and therefore modified.
 */
wxString StringFromValue( EDA_UNITS aUnits, double aValue, bool aAddUnitSymbol,
                          EDA_DATA_TYPE aType )
{
    double  value_to_print = aValue;

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:
        value_to_print = To_User_Unit( aUnits, value_to_print );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::AREA:
        value_to_print = To_User_Unit( aUnits, value_to_print );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::DISTANCE:
        value_to_print = To_User_Unit( aUnits, value_to_print );
    }


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
        if( aUnits == EDA_UNITS::MILS )
            len = sprintf( buf, "%.7g", value_to_print );
        else
            len = sprintf( buf, "%.10g", value_to_print );
    }

    wxString    stringValue( buf, wxConvUTF8 );

    if( aAddUnitSymbol )
    {
        switch( aUnits )
        {
        case EDA_UNITS::MILLIMETRES:
            stringValue += wxT( " mm" );
            break;

        case EDA_UNITS::DEGREES:
            stringValue += wxT( " deg" );
            break;

        case EDA_UNITS::MILS:
            stringValue += wxT( " mils" );
            break;

        case EDA_UNITS::INCHES:
            stringValue += wxT( " in" );
            break;

        case EDA_UNITS::PERCENT:
            stringValue += wxT( "%" );
            break;

        case EDA_UNITS::UNSCALED:
            break;
        }
    }

    return stringValue;
}


double From_User_Unit( EDA_UNITS aUnits, double aValue )
{
    switch( aUnits )
    {
    case EDA_UNITS::MILLIMETRES:
        return MM_TO_IU( aValue );

    case EDA_UNITS::MILS:
        return MILS_TO_IU( aValue );

    case EDA_UNITS::INCHES:
        return IN_TO_IU( aValue );

    case EDA_UNITS::DEGREES:
        // Convert to "decidegrees"
        return aValue * 10;

    default:
    case EDA_UNITS::UNSCALED:
    case EDA_UNITS::PERCENT:
        return aValue;
    }
}


double DoubleValueFromString( EDA_UNITS aUnits, const wxString& aTextValue, EDA_DATA_TYPE aType )
{
    double dtmp = 0;

    // Acquire the 'right' decimal point separator
    const struct lconv* lc = localeconv();

    wxChar      decimal_point = lc->decimal_point[0];
    wxString    buf( aTextValue.Strip( wxString::both ) );

    // Convert any entered decimal point separators to the 'right' one
    buf.Replace( wxT( "." ), wxString( decimal_point, 1 ) );
    buf.Replace( wxT( "," ), wxString( decimal_point, 1 ) );

    // Find the end of the numeric part
    unsigned brk_point = 0;

    while( brk_point < buf.Len() )
    {
        wxChar ch = buf[brk_point];

        if( !( (ch >= '0' && ch <= '9') || (ch == decimal_point) || (ch == '-') || (ch == '+') ) )
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

    if( aUnits == EDA_UNITS::MILLIMETRES || aUnits == EDA_UNITS::MILS
      || aUnits == EDA_UNITS::INCHES )
    {
        if( unit == wxT( "mm" ) )
        {
            aUnits = EDA_UNITS::MILLIMETRES;
        }
        else if( unit == wxT( "mi" ) || unit == wxT( "th" ) )
        {
            aUnits = EDA_UNITS::MILS;
        }
        else if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
        {
            aUnits = EDA_UNITS::INCHES;
        }
        else if( unit == "oz" ) // 1 oz = 1.37 mils
        {
            aUnits = EDA_UNITS::MILS;
            dtmp *= 1.37;
        }
    }
    else if( aUnits == EDA_UNITS::DEGREES )
    {
        if( unit == wxT( "ra" ) ) // Radians
        {
            dtmp *= 180.0f / M_PI;
        }
    }

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:
        dtmp = From_User_Unit( aUnits, dtmp );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::AREA:
        dtmp = From_User_Unit( aUnits, dtmp );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::DISTANCE:
        dtmp = From_User_Unit( aUnits, dtmp );
    }

    return dtmp;
}


void FetchUnitsFromString( const wxString& aTextValue, EDA_UNITS& aUnits )
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

    if( unit == wxT( "mm" ) )
        aUnits = EDA_UNITS::MILLIMETRES;
    else if( unit == wxT( "mi" ) || unit == wxT( "th" ) ) // "mils" or "thou"
        aUnits = EDA_UNITS::MILS;
    else if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
        aUnits = EDA_UNITS::INCHES;
    else if( unit == wxT( "de" ) || unit == wxT( "ra" ) ) // "deg" or "rad"
        aUnits = EDA_UNITS::DEGREES;
}


long long int ValueFromString( EDA_UNITS aUnits, const wxString& aTextValue, EDA_DATA_TYPE aType )
{
    double value = DoubleValueFromString( aUnits, aTextValue, aType );
    return KiROUND<double, long long int>( value );
}


/**
 * A helper to convert \a aAngle in deci-degrees to a string in degrees.
 */
wxString AngleToStringDegrees( double aAngle )
{
    wxString text;

    text.Printf( wxT( "%.3f" ), aAngle / 10.0 );
    StripTrailingZeros( text, 1 );

    return text;
}


wxString GetAbbreviatedUnitsLabel( EDA_UNITS aUnit, EDA_DATA_TYPE aType )
{
    switch( aUnit )
    {
    case EDA_UNITS::MILLIMETRES:
        switch( aType )
        {
        default:
            wxASSERT( 0 );
            KI_FALLTHROUGH;
        case EDA_DATA_TYPE::DISTANCE:
            return _( "mm" );
        case EDA_DATA_TYPE::AREA:
            return _( "sq. mm" );
        case EDA_DATA_TYPE::VOLUME:
            return _( "cu. mm" );
        }

    case EDA_UNITS::MILS:
        switch( aType )
        {
        default:
            wxASSERT( 0 );
            KI_FALLTHROUGH;
        case EDA_DATA_TYPE::DISTANCE:
            return _( "mils" );
        case EDA_DATA_TYPE::AREA:
            return _( "sq. mils" );
        case EDA_DATA_TYPE::VOLUME:
            return _( "cu. mils" );
        }

    case EDA_UNITS::INCHES:
        switch( aType )
        {
        default:
            wxASSERT( 0 );
            KI_FALLTHROUGH;
        case EDA_DATA_TYPE::DISTANCE:
            return _( "in" );
        case EDA_DATA_TYPE::AREA:
            return _( "sq. in" );
        case EDA_DATA_TYPE::VOLUME:
            return _( "cu. in" );
        }

    case EDA_UNITS::PERCENT:
        return _( "%" );

    case EDA_UNITS::UNSCALED:
        return wxEmptyString;

    case EDA_UNITS::DEGREES:
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

    engUnits /= IU_PER_MM;

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        len = snprintf( buf, sizeof(buf), "%.10f", engUnits );

        // Make sure snprintf() didn't fail and the locale numeric separator is correct.
        wxCHECK( len >= 0 && len < 50 && strchr( buf, ',' ) == NULL, std::string( "" ) );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        if( buf[len] == '.' )
            buf[len] = '\0';
        else
            ++len;
    }
    else
    {
        len = snprintf( buf, sizeof(buf), "%.10g", engUnits );

        // Make sure snprintf() didn't fail and the locale numeric separator is correct.
        wxCHECK( len >= 0 && len < 50 && strchr( buf, ',' ) == NULL , std::string( "" ) );
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
