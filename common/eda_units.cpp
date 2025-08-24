/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_units.h>
#include <fmt/core.h>
#include <math/util.h>      // for KiROUND
#include <macros.h>
#include <charconv>
#include <wx/translation.h>


static void removeTrailingZeros( wxString& aText )
{
    int len = aText.length();
    int removeLast = 0;

    while( --len > 0 && aText[len] == '0' )
        removeLast++;

    if( len >= 0 && ( aText[len] == '.' || aText[len] == ',' ) )
        removeLast++;

    aText = aText.RemoveLast( removeLast );
}


bool EDA_UNIT_UTILS::IsImperialUnit( EDA_UNITS aUnit )
{
    switch( aUnit )
    {
    case EDA_UNITS::INCH:
    case EDA_UNITS::MILS:
        return true;

    default:
        return false;
    }
}


bool EDA_UNIT_UTILS::IsMetricUnit( EDA_UNITS aUnit )
{
    switch( aUnit )
    {
    case EDA_UNITS::UM:
    case EDA_UNITS::MM:
    case EDA_UNITS::CM:
        return true;

    default:
        return false;
    }
}


int EDA_UNIT_UTILS::Mm2mils( double aVal )
{
    return KiROUND( aVal * 1000. / 25.4 );
}


int EDA_UNIT_UTILS::Mils2mm( double aVal )
{
    return KiROUND( aVal * 25.4 / 1000. );
}


bool EDA_UNIT_UTILS::FetchUnitsFromString( const wxString& aTextValue, EDA_UNITS& aUnits )
{
    wxString buf( aTextValue.Strip( wxString::both ) );
    unsigned brk_point = 0;

    while( brk_point < buf.Len() )
    {
        wxChar c = buf[brk_point];

        if( !( ( c >= '0' && c <= '9' ) || ( c == '.' ) || ( c == ',' ) || ( c == '-' )
               || ( c == '+' ) ) )
            break;

        ++brk_point;
    }

    // Check the unit designator (2 ch significant)
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );

    //check for um, μm (µ is MICRO SIGN) and µm (µ is GREEK SMALL LETTER MU) for micrometre
    if( unit == wxT( "um" ) || unit == wxT( "\u00B5m" ) || unit == wxT( "\u03BCm" ) )
        aUnits = EDA_UNITS::UM;
    else if( unit == wxT( "mm" ) )
        aUnits = EDA_UNITS::MM;
    if( unit == wxT( "cm" ) )
        aUnits = EDA_UNITS::CM;
    else if( unit == wxT( "mi" ) || unit == wxT( "th" ) ) // "mils" or "thou"
        aUnits = EDA_UNITS::MILS;
    else if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
        aUnits = EDA_UNITS::INCH;
    else if( unit == wxT( "de" ) || unit == wxT( "ra" ) ) // "deg" or "rad"
        aUnits = EDA_UNITS::DEGREES;
    else if( unit == wxT("fs" ) )
        aUnits = EDA_UNITS::FS;
    else if( unit == wxT( "ps" ) )
    {
        wxString timeUnit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 5 ).Lower() );

        if( timeUnit == wxT( "ps" ) )
            aUnits = EDA_UNITS::PS;
        else if( timeUnit == wxT( "ps/in" ) )
            aUnits = EDA_UNITS::PS_PER_INCH;
        else if( timeUnit == wxT( "ps/cm" ) )
            aUnits = EDA_UNITS::PS_PER_CM;
        else if( timeUnit == wxT( "ps/mm" ) )
            aUnits = EDA_UNITS::PS_PER_MM;
        else
            return false;
    }
    else
        return false;

    return true;
}


wxString EDA_UNIT_UTILS::GetText( EDA_UNITS aUnits, EDA_DATA_TYPE aType )
{
    wxString label;

    switch( aUnits )
    {
    case EDA_UNITS::UM:          label = wxT( " \u00B5m" ); break; //00B5 for µ
    case EDA_UNITS::MM:          label = wxT( " mm" );      break;
    case EDA_UNITS::CM:          label = wxT( " cm" );      break;
    case EDA_UNITS::DEGREES:     label = wxT( "°" );        break;
    case EDA_UNITS::MILS:        label = wxT( " mils" );    break;
    case EDA_UNITS::INCH:        label = wxT( " in" );      break;
    case EDA_UNITS::PERCENT:     label = wxT( "%" );        break;
    case EDA_UNITS::FS:          label = wxT( " fs" );      break;
    case EDA_UNITS::PS:          label = wxT( " ps" );      break;
    case EDA_UNITS::PS_PER_INCH: label = wxT( " ps/in" );   break;
    case EDA_UNITS::PS_PER_CM:   label = wxT( " ps/cm");    break;
    case EDA_UNITS::PS_PER_MM:   label = wxT( " ps/mm");    break;
    case EDA_UNITS::UNSCALED:                               break;
    default: UNIMPLEMENTED_FOR( wxS( "Unknown units" ) );   break;
    }

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:      label += wxT( "³" );       break;
    case EDA_DATA_TYPE::AREA:        label += wxT( "²" );       break;
    case EDA_DATA_TYPE::DISTANCE:                               break;
    case EDA_DATA_TYPE::TIME:                                   break;
    case EDA_DATA_TYPE::LENGTH_DELAY:                           break;
    default: UNIMPLEMENTED_FOR( wxS( "Unknown measurement" ) ); break;
    }

    return label;
}


wxString EDA_UNIT_UTILS::GetLabel( EDA_UNITS aUnits, EDA_DATA_TYPE aType )
{
    return GetText( aUnits, aType ).Trim( false );
}


std::string EDA_UNIT_UTILS::FormatAngle( const EDA_ANGLE& aAngle )
{
    std::string temp = fmt::format( "{:.10g}", aAngle.AsDegrees() );

    return temp;
}


std::string EDA_UNIT_UTILS::FormatInternalUnits( const EDA_IU_SCALE& aIuScale, int aValue )
{
    std::string buf;
    double engUnits = aValue;

    engUnits /= aIuScale.IU_PER_MM;

    if( engUnits != 0.0 && fabs( engUnits ) <= 0.0001 )
    {
        buf = fmt::format( "{:.10f}", engUnits );

        // remove trailing zeros
        while( !buf.empty() && buf[buf.size() - 1] == '0' )
        {
            buf.pop_back();
        }

        // if the value was really small
        // we may have just stripped all the zeros after the decimal
        if( buf[buf.size() - 1] == '.' )
        {
            buf.pop_back();
        }
    }
    else
    {
        buf = fmt::format( "{:.10g}", engUnits );
    }

    return buf;
}


std::string EDA_UNIT_UTILS::FormatInternalUnits( const EDA_IU_SCALE& aIuScale,
                                                 const VECTOR2I&     aPoint )
{
    return FormatInternalUnits( aIuScale, aPoint.x ) + " "
           + FormatInternalUnits( aIuScale, aPoint.y );
}


#if 0   // No support for std::from_chars on MacOS yet

bool EDA_UNIT_UTILS::ParseInternalUnits( const std::string& aInput, const EDA_IU_SCALE& aIuScale,
                                         int& aOut )
{
    double value;

    if( std::from_chars( aInput.data(), aInput.data() + aInput.size(), value ).ec != std::errc() )
        return false;

    aOut = value * aIuScale.IU_PER_MM;
    return true;
}


bool EDA_UNIT_UTILS::ParseInternalUnits( const std::string& aInput, const EDA_IU_SCALE& aIuScale,
                                         VECTOR2I& aOut )
{
    size_t pos = aInput.find( ' ' );

    if( pos == std::string::npos )
        return false;

    std::string first = aInput.substr( 0, pos );
    std::string second = aInput.substr( pos + 1 );

    VECTOR2I vec;

    if( !ParseInternalUnits( first, aIuScale, vec.x ) )
        return false;

    if( !ParseInternalUnits( second, aIuScale, vec.y ) )
        return false;

    aOut = vec;

    return true;
}

#endif


#define IU_TO_MM( x, scale ) ( x / scale.IU_PER_MM )
#define IU_TO_IN( x, scale ) ( x / scale.IU_PER_MILS / 1000 )
#define IU_TO_MILS( x, scale ) ( x / scale.IU_PER_MILS )
#define IU_TO_PS( x, scale ) ( x / scale.IU_PER_PS )
#define IU_TO_PS_PER_MM( x, scale ) ( x / scale.IU_PER_PS_PER_MM )
#define MM_TO_IU( x, scale ) ( x * scale.IU_PER_MM )
#define IN_TO_IU( x, scale ) ( x * scale.IU_PER_MILS * 1000 )
#define MILS_TO_IU( x, scale ) ( x * scale.IU_PER_MILS )
#define PS_TO_IU( x, scale ) ( x * scale.IU_PER_PS )
#define PS_PER_MM_TO_IU( x, scale ) ( x * scale.IU_PER_PS_PER_MM )


double EDA_UNIT_UTILS::UI::ToUserUnit( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnit,
                                       double aValue )
{
    switch( aUnit )
    {
    case EDA_UNITS::UM:          return IU_TO_MM( aValue, aIuScale ) * 1000;
    case EDA_UNITS::MM:          return IU_TO_MM( aValue, aIuScale );
    case EDA_UNITS::CM:          return IU_TO_MM( aValue, aIuScale ) / 10;
    case EDA_UNITS::MILS:        return IU_TO_MILS( aValue, aIuScale );
    case EDA_UNITS::INCH:        return IU_TO_IN( aValue, aIuScale );
    case EDA_UNITS::DEGREES:     return aValue;
    case EDA_UNITS::FS:          return IU_TO_PS( aValue, aIuScale ) * 1000.0;
    case EDA_UNITS::PS:          return IU_TO_PS( aValue, aIuScale );
    case EDA_UNITS::PS_PER_INCH: return IU_TO_PS_PER_MM( aValue, aIuScale ) * 25.4;
    case EDA_UNITS::PS_PER_CM:   return IU_TO_PS_PER_MM( aValue, aIuScale ) * 10;
    case EDA_UNITS::PS_PER_MM:   return IU_TO_PS_PER_MM( aValue, aIuScale );
    default:                     return aValue;
    }
}


wxString EDA_UNIT_UTILS::UI::StringFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                              double aValue, bool aAddUnitsText,
                                              EDA_DATA_TYPE aType )
{
    double value_to_print = aValue;
    bool   is_eeschema = ( aIuScale.IU_PER_MM == SCH_IU_PER_MM );

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:
        value_to_print = ToUserUnit( aIuScale, aUnits, value_to_print );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::AREA:
        value_to_print = ToUserUnit( aIuScale, aUnits, value_to_print );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::DISTANCE:
        value_to_print = ToUserUnit( aIuScale, aUnits, value_to_print );
        break;

    case EDA_DATA_TYPE::UNITLESS:
        break;

    case EDA_DATA_TYPE::TIME:
        value_to_print = ToUserUnit( aIuScale, aUnits, value_to_print );
        break;

    case EDA_DATA_TYPE::LENGTH_DELAY:
        value_to_print = ToUserUnit( aIuScale, aUnits, value_to_print );
        break;
    }

    const wxChar* format = nullptr;

    switch( aUnits )
    {
    case EDA_UNITS::MILS:        format = is_eeschema ? wxT( "%.3f" ) : wxT( "%.5f" ); break;
    case EDA_UNITS::INCH:        format = is_eeschema ? wxT( "%.6f" ) : wxT( "%.8f" ); break;
    case EDA_UNITS::DEGREES:     format = wxT( "%.4f" );                               break;
    case EDA_UNITS::PS_PER_INCH: format = wxT( "%.4f" );                               break;
    case EDA_UNITS::PS_PER_CM:   format = wxT( "%.3f" );                               break;
    case EDA_UNITS::PS_PER_MM:   format = wxT( "%.3f" );                               break;
    default:                     format = wxT( "%.10f" );                              break;
    }

    wxString text;
    text.Printf( format, value_to_print );
    removeTrailingZeros( text );

    if( value_to_print != 0.0 && ( text == wxS( "0" ) || text == wxS( "-0" ) ) )
    {
        text.Printf( wxS( "%.10f" ), value_to_print );
        removeTrailingZeros( text );
    }

    if( aAddUnitsText )
        text << EDA_UNIT_UTILS::GetText( aUnits, aType );

    return text;
}



// A lower-precision (for readability) version of StringFromValue()
wxString EDA_UNIT_UTILS::UI::MessageTextFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                                   int aValue,
                                                   bool aAddUnitLabel,
                                                   EDA_DATA_TYPE aType )
{
    return MessageTextFromValue( aIuScale, aUnits, double( aValue ), aAddUnitLabel, aType );
}


// A lower-precision (for readability) version of StringFromValue()
wxString EDA_UNIT_UTILS::UI::MessageTextFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                                   long long int aValue,
                                                   bool aAddUnitLabel,
                                                   EDA_DATA_TYPE aType )
{
    return MessageTextFromValue( aIuScale, aUnits, double( aValue ), aAddUnitLabel, aType );
}


wxString EDA_UNIT_UTILS::UI::MessageTextFromValue( EDA_ANGLE aValue, bool aAddUnitLabel )
{
    if( aAddUnitLabel )
        return wxString::Format( wxT( "%.1f°" ), aValue.AsDegrees() );
    else
        return wxString::Format( wxT( "%.1f" ), aValue.AsDegrees() );
}


// A lower-precision (for readability) version of StringFromValue()
wxString EDA_UNIT_UTILS::UI::MessageTextFromValue( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                                   double aValue, bool aAddUnitsText,
                                                   EDA_DATA_TYPE aType )
{
    wxString      text;
    const wxChar* format;
    double        value = aValue;
    bool          short_form = ( aIuScale.IU_PER_MM == SCH_IU_PER_MM )
                                || ( aType == EDA_DATA_TYPE::VOLUME || aType == EDA_DATA_TYPE::AREA );

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:
        value = ToUserUnit( aIuScale, aUnits, value );
        // Fall through to continue computation
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::AREA:
        value = ToUserUnit( aIuScale, aUnits, value );
        // Fall through to continue computation
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::DISTANCE:
        value = ToUserUnit( aIuScale, aUnits, value );
        break;

    case EDA_DATA_TYPE::UNITLESS:
        break;

    case EDA_DATA_TYPE::TIME:
        value = ToUserUnit( aIuScale, aUnits, value );
        break;

    case EDA_DATA_TYPE::LENGTH_DELAY:
        value = ToUserUnit( aIuScale, aUnits, value );
        break;
    }

    switch( aUnits )
    {
    default:
    case EDA_UNITS::UM:          format = short_form ? wxT( "%.0f" ) : wxT( "%.1f" ); break;
    case EDA_UNITS::MM:          format = short_form ? wxT( "%.3f" ) : wxT( "%.4f" ); break;
    case EDA_UNITS::CM:          format = short_form ? wxT( "%.3f" ) : wxT( "%.5f" ); break;
    case EDA_UNITS::MILS:        format = short_form ? wxT( "%.0f" ) : wxT( "%.2f" ); break;
    case EDA_UNITS::INCH:        format = short_form ? wxT( "%.3f" ) : wxT( "%.4f" ); break;
    case EDA_UNITS::DEGREES:     format = wxT( "%.3f" );                              break;
    case EDA_UNITS::UNSCALED:    format = wxT( "%.0f" );                              break;
    case EDA_UNITS::FS:          format = wxT( "%.4f" );                              break;
    case EDA_UNITS::PS:          format = wxT( "%.2f" );                              break;
    case EDA_UNITS::PS_PER_INCH: format = wxT( "%.2f" );                              break;
    case EDA_UNITS::PS_PER_CM:   format = wxT( "%.2f" );                              break;
    case EDA_UNITS::PS_PER_MM:   format = wxT( "%.2f" );                              break;
    }

    text.Printf( format, value );

    // Check if the formatted value shows only zeros but the actual value is non-zero
    // If so, use scientific notation instead
    if( value != 0.0 )
    {
        bool showsOnlyZeros = true;

        // Check if the text contains only zeros (allowing for decimal point, minus sign, etc.)
        for( auto ch : text )
        {
            if( ch >= '1' && ch <= '9' )
            {
                showsOnlyZeros = false;
                break;
            }
        }

        if( showsOnlyZeros )
        {
            text.Printf( wxT( "%.3e" ), value );
        }
    }

    // Trim to 2-1/2 digits after the decimal place for short-form mm
    if( short_form && aUnits == EDA_UNITS::MM )
    {
        struct lconv* lc = localeconv();
        int           length = (int) text.Length();

        if( length > 4 && text[length - 4] == *lc->decimal_point && text[length - 1] == '0' )
            text = text.Left( length - 1 );
    }

    if( aAddUnitsText )
        text += EDA_UNIT_UTILS::GetText( aUnits, aType );

    return text;
}


wxString EDA_UNIT_UTILS::UI::MessageTextFromMinOptMax( const EDA_IU_SCALE& aIuScale,
                                                       EDA_UNITS aUnits,
                                                       const MINOPTMAX<int>& aValue )
{
    wxString msg;

    if( aValue.HasMin() && aValue.Min() > 0 )
    {
        msg += _( "min" ) + wxS( " " ) + MessageTextFromValue( aIuScale, aUnits, aValue.Min() );
    }

    if( aValue.HasOpt() )
    {
        if( !msg.IsEmpty() )
            msg += wxS( "; " );

        msg += _( "opt" ) + wxS( " " ) + MessageTextFromValue( aIuScale, aUnits, aValue.Opt() );
    }

    if( aValue.HasMax() )
    {
        if( !msg.IsEmpty() )
            msg += wxS( "; " );

        msg += _( "max" ) + wxS( " " ) + MessageTextFromValue( aIuScale, aUnits, aValue.Max() );
    }

    return msg;
};


double EDA_UNIT_UTILS::UI::FromUserUnit( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                         double aValue )
{
    switch( aUnits )
    {
    case EDA_UNITS::UM:          return MM_TO_IU( aValue / 1000.0, aIuScale );
    case EDA_UNITS::MM:          return MM_TO_IU( aValue, aIuScale );
    case EDA_UNITS::CM:          return MM_TO_IU( aValue * 10, aIuScale );
    case EDA_UNITS::MILS:        return MILS_TO_IU( aValue, aIuScale );
    case EDA_UNITS::INCH:        return IN_TO_IU( aValue, aIuScale );
    case EDA_UNITS::FS:          return PS_TO_IU( aValue / 1000.0, aIuScale );
    case EDA_UNITS::PS:          return PS_TO_IU( aValue, aIuScale );
    case EDA_UNITS::PS_PER_INCH: return PS_PER_MM_TO_IU( aValue / 25.4, aIuScale );
    case EDA_UNITS::PS_PER_CM:   return PS_PER_MM_TO_IU( aValue / 10, aIuScale );
    case EDA_UNITS::PS_PER_MM:   return PS_PER_MM_TO_IU( aValue, aIuScale );
    default:
    case EDA_UNITS::DEGREES:
    case EDA_UNITS::UNSCALED:
    case EDA_UNITS::PERCENT:     return aValue;
    }
}


double EDA_UNIT_UTILS::UI::DoubleValueFromString( const wxString& aTextValue )
{
    double dtmp = 0;

    // Acquire the 'right' decimal point separator
    const struct lconv* lc = localeconv();

    wxChar   decimal_point = lc->decimal_point[0];
    wxString buf( aTextValue.Strip( wxString::both ) );

    // Convert any entered decimal point separators to the 'right' one
    buf.Replace( wxT( "." ), wxString( decimal_point, 1 ) );
    buf.Replace( wxT( "," ), wxString( decimal_point, 1 ) );

    // Find the end of the numeric part
    unsigned brk_point = 0;

    while( brk_point < buf.Len() )
    {
        wxChar ch = buf[brk_point];

        if( !( ( ch >= '0' && ch <= '9' ) || ( ch == decimal_point ) || ( ch == '-' )
               || ( ch == '+' ) ) )
        {
            break;
        }

        ++brk_point;
    }

    // Extract the numeric part
    buf.Left( brk_point ).ToDouble( &dtmp );

    return dtmp;
}


double EDA_UNIT_UTILS::UI::DoubleValueFromString( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                                  const wxString& aTextValue, EDA_DATA_TYPE aType )
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
            break;

        ++brk_point;
    }

    // Extract the numeric part
    buf.Left( brk_point ).ToDouble( &dtmp );

    // Check the optional unit designator (2 ch significant)
    wxString unit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 2 ).Lower() );

    if( aUnits == EDA_UNITS::UM
            || aUnits == EDA_UNITS::MM
            || aUnits == EDA_UNITS::CM
            || aUnits == EDA_UNITS::MILS
            || aUnits == EDA_UNITS::INCH )
    {
        //check for um, μm (µ is MICRO SIGN) and µm (µ is GREEK SMALL LETTER MU) for micrometre
        if( unit == wxT( "um" ) || unit == wxT( "\u00B5m" ) || unit == wxT( "\u03BCm" ) )
        {
            aUnits = EDA_UNITS::UM;
        }
        else if( unit == wxT( "mm" ) )
        {
            aUnits = EDA_UNITS::MM;
        }
        else if( unit == wxT( "cm" ) )
        {
            aUnits = EDA_UNITS::CM;
        }
        else if( unit == wxT( "mi" ) || unit == wxT( "th" ) )
        {
            aUnits = EDA_UNITS::MILS;
        }
        else if( unit == wxT( "in" ) || unit == wxT( "\"" ) )
        {
            aUnits = EDA_UNITS::INCH;
        }
        else if( unit == wxT( "oz" ) ) // 1 oz = 1.37 mils
        {
            aUnits = EDA_UNITS::MILS;
            dtmp *= 1.37;
        }
    }
    else if( aUnits == EDA_UNITS::DEGREES )
    {
        if( unit == wxT( "ra" ) ) // Radians
            dtmp *= 180.0f / M_PI;
    }
    else if( aUnits == EDA_UNITS::FS || aUnits == EDA_UNITS::PS || aUnits == EDA_UNITS::PS_PER_INCH
             || aUnits == EDA_UNITS::PS_PER_CM || aUnits == EDA_UNITS::PS_PER_MM )

    {
        wxString timeUnit( buf.Mid( brk_point ).Strip( wxString::leading ).Left( 5 ).Lower() );

        if( timeUnit == wxT( "fs" ) )
            aUnits = EDA_UNITS::FS;
        if( timeUnit == wxT( "ps" ) )
            aUnits = EDA_UNITS::PS;
        else if( timeUnit == wxT( "ps/in" ) )
            aUnits = EDA_UNITS::PS_PER_INCH;
        else if( timeUnit == wxT( "ps/cm" ) )
            aUnits = EDA_UNITS::PS_PER_CM;
        else if( timeUnit == wxT( "ps/mm" ) )
            aUnits = EDA_UNITS::PS_PER_MM;
    }

    switch( aType )
    {
    case EDA_DATA_TYPE::VOLUME:
        dtmp = FromUserUnit( aIuScale, aUnits, dtmp );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::AREA:
        dtmp = FromUserUnit( aIuScale, aUnits, dtmp );
        KI_FALLTHROUGH;

    case EDA_DATA_TYPE::DISTANCE:
        dtmp = FromUserUnit( aIuScale, aUnits, dtmp );
        break;

    case EDA_DATA_TYPE::UNITLESS:
        break;

    case EDA_DATA_TYPE::TIME:
        dtmp = FromUserUnit( aIuScale, aUnits, dtmp );
        break;

    case EDA_DATA_TYPE::LENGTH_DELAY:
        dtmp = FromUserUnit( aIuScale, aUnits, dtmp );
        break;
    }

    return dtmp;
}


long long int EDA_UNIT_UTILS::UI::ValueFromString( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                                   const wxString& aTextValue, EDA_DATA_TYPE aType )
{
    double value = DoubleValueFromString( aIuScale, aUnits, aTextValue, aType );

    return KiROUND<double, long long int>( value );
}


long long int EDA_UNIT_UTILS::UI::ValueFromString( const wxString& aTextValue )
{
    double value = DoubleValueFromString( aTextValue );

    return KiROUND<double, long long int>( value );
}
