/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <math/util.h>      // for KiROUND

#include <gerber_file_image.h>
#include <base_units.h>


/* These routines read the text string point from Text.
 * On exit, Text points the beginning of the sequence unread
 */

// conversion scale from gerber file units to Gerbview internal units
// depending on the gerber file format
// this scale list assumes gerber units are imperial.
// for metric gerber units, the imperial to metric conversion is made in read functions
#define SCALE_LIST_SIZE 9
static double scale_list[SCALE_LIST_SIZE] =
{
    1000.0 * GERB_IU_PER_MM * 0.0254,   // x.1 format (certainly useless)
    100.0 * GERB_IU_PER_MM * 0.0254,    // x.2 format (certainly useless)
    10.0 * GERB_IU_PER_MM * 0.0254,     // x.3 format
    1.0 * GERB_IU_PER_MM * 0.0254,      // x.4 format
    0.1 * GERB_IU_PER_MM * 0.0254,      // x.5 format
    0.01 * GERB_IU_PER_MM * 0.0254,     // x.6 format
    0.001 * GERB_IU_PER_MM * 0.0254,     // x.7 format  (currently the max allowed precision)
    0.0001 * GERB_IU_PER_MM * 0.0254,   // provided, but not used
    0.00001 * GERB_IU_PER_MM * 0.0254,  // provided, but not used
};


/**
 * Convert a coordinate given in floating point to GerbView's internal units
 * (currently = 10 nanometers).
 */
int scaletoIU( double aCoord, bool isMetric )
{
    int ret;

    if( isMetric )  // gerber are units in mm
        ret = KiROUND( aCoord * GERB_IU_PER_MM );
    else            // gerber are units in inches
        ret = KiROUND( aCoord * GERB_IU_PER_MM * 25.4 );

    return ret;
}


// An useful function used when reading gerber files
static bool IsNumber( char x )
{
    return ( ( x >= '0' ) && ( x <='9' ) )
           || ( x == '-' ) || ( x == '+' )  || ( x == '.' );
}


VECTOR2I GERBER_FILE_IMAGE::ReadXYCoord( char*& aText, bool aExcellonMode )
{
    VECTOR2I pos( 0, 0 );
    bool    is_float   = false;

    std::string line;

    // Reserve the anticipated length plus an optional sign and decimal
    line.reserve( std::max( m_FmtLen.x, m_FmtLen.y ) + 3 );

    // Set up return value for case where aText == nullptr
    if( !m_Relative )
        pos = m_CurrentPos;

    if( aText == nullptr )
        return pos;

    while( *aText && ( ( *aText == 'X' ) || ( *aText == 'Y' ) || ( *aText == 'A' ) ) )
    {
        double decimal_scale = 1.0;
        int    nbdigits = 0;
        int    current_coord = 0;
        char   type_coord = *aText++;

        line.clear();

        while( IsNumber( *aText ) )
        {
            if( *aText == '.' )  // Force decimal format if reading a floating point number
                is_float = true;

            // count digits only (sign and decimal point are not counted)
            if( (*aText >= '0') && (*aText <='9') )
                nbdigits++;

            line.push_back( *( aText++ ) );
        }

        double val;
        wxString text( line.data() );
        text.ToCDouble( &val );

        if( is_float )
        {
            current_coord = scaletoIU( val, m_GerbMetric );
        }
        else
        {
            int fmt_scale = (type_coord == 'X') ? m_FmtScale.x : m_FmtScale.y;

            if( m_NoTrailingZeros )
            {
                // no trailing zero format, we need to add missing zeros.
                int digit_count = (type_coord == 'X') ? m_FmtLen.x : m_FmtLen.y;

                // Truncate the extra digits if the len is more than expected
                // because the conversion to internal units expect exactly
                // digit_count digits.  Alternatively, add some additional digits
                // to pad out to the missing zeros
                if( nbdigits < digit_count || ( aExcellonMode && ( nbdigits > digit_count ) ) )
                    decimal_scale = std::pow<double>( 10, digit_count - nbdigits );
            }

            double real_scale = scale_list[fmt_scale];

            if( m_GerbMetric )
                real_scale = real_scale / 25.4;

            current_coord = KiROUND( val * real_scale * decimal_scale );
        }

        if( type_coord == 'X' )
        {
            pos.x = current_coord;
        }
        else if( type_coord == 'Y' )
        {
            pos.y = current_coord;
        }
        else if( type_coord == 'A' )
        {
            m_ArcRadius = current_coord;
            m_LastArcDataType = ARC_INFO_TYPE_RADIUS;
        }
    }

    if( m_Relative )
        pos += m_CurrentPos;

    m_CurrentPos = pos;
    return pos;
}


VECTOR2I GERBER_FILE_IMAGE::ReadIJCoord( char*& aText )
{
    VECTOR2I pos( 0, 0 );
    bool    is_float   = false;

    std::string line;

    // Reserve the anticipated length plus an optional sign and decimal
    line.reserve( std::max( m_FmtLen.x, m_FmtLen.y ) + 3 );

    if( aText == nullptr )
        return pos;

    while( *aText && ( ( *aText == 'I' ) || ( *aText == 'J' ) ) )
    {
        double decimal_scale = 1.0;
        int    nbdigits = 0;
        int    current_coord = 0;
        char   type_coord = *aText++;

        line.clear();

        while( IsNumber( *aText ) )
        {
            if( *aText == '.' )  // Force decimal format if reading a floating point number
                is_float = true;

            // count digits only (sign and decimal point are not counted)
            if( (*aText >= '0') && (*aText <='9') )
                nbdigits++;

            line.push_back( *( aText++ ) );
        }

        double val;
        wxString text( line.data() );
        text.Trim( true ).Trim( false );
        text.ToCDouble( &val );

        if( is_float )
        {
            current_coord = scaletoIU( val, m_GerbMetric );
        }
        else
        {
            int fmt_scale = ( type_coord == 'I' ) ? m_FmtScale.x : m_FmtScale.y;

            if( m_NoTrailingZeros )
            {
                // no trailing zero format, we need to add missing zeros.
                int digit_count = ( type_coord == 'I' ) ? m_FmtLen.x : m_FmtLen.y;

                // Truncate the extra digits if the len is more than expected
                // because the conversion to internal units expect exactly
                // digit_count digits.  Alternatively, add some additional digits
                // to pad out to the missing zeros
                if( nbdigits < digit_count )
                    decimal_scale = std::pow<double>( 10, digit_count - nbdigits );
            }

            double real_scale = scale_list[fmt_scale];

            if( m_GerbMetric )
                real_scale = real_scale / 25.4;

            current_coord = KiROUND( val * real_scale * decimal_scale );
        }

        if( type_coord == 'I' )
        {
            pos.x = current_coord;
        }
        else if( type_coord == 'J' )
        {
            pos.y = current_coord;
        }
    }

    m_IJPos = pos;
    m_LastArcDataType = ARC_INFO_TYPE_CENTER;
    m_LastCoordIsIJPos = true;

    return pos;
}


// Helper functions:

/**
 * Read an integer from an ASCII character buffer.
 *
 * If there is a comma after the integer, then skip over that.
 *
 * @param text is a reference to a character pointer from which bytes are read
 *        and the pointer is advanced for each byte read.
 * @param aSkipSeparator set to true (default) to skip comma.
 * @return The integer read in.
 */
int ReadInt( char*& text, bool aSkipSeparator = true )
{
    int ret;

    // For strtol, a string starting by 0X or 0x is a valid number in hexadecimal or octal.
    // However, 'X'  is a separator in Gerber strings with numbers.
    // We need to detect that
    if( strncasecmp( text, "0X", 2 ) == 0 )
    {
        text++;
        ret = 0;
    }
    else
    {
        ret = (int) strtol( text, &text, 10 );
    }

    if( *text == ',' || isspace( *text ) )
    {
        if( aSkipSeparator )
            ++text;
    }

    return ret;
}


/**
 * Read a double precision floating point number from an ASCII character buffer.
 *
 * If there is a comma after the number, then skip over that.
 *
 * @param text is a reference to a character pointer from which the ASCII double
 *             is read from and the pointer advanced for each character read.
 * @param aSkipSeparator set to true (default) to skip comma.
 * @return number read.
 */

double ReadDouble( char*& text, bool aSkipSeparator = true )
{
    double ret;

    // For strtod, a string starting by 0X or 0x is a valid number in hexadecimal or octal.
    // However, 'X'  is a separator in Gerber strings with numbers.
    // We need to detect that
    if( strncasecmp( text, "0X", 2 ) == 0 )
    {
        text++;
        ret = 0.0;
    }
    else
    {
        wxString line( text );
        line.Trim( false );

        // Warning: in locales using ',' as separator, wxString::ToCDouble accept both '.' and ','
        // as separator (wxWidgets 3.2.6 bug?, look fixed in 3.2.8). So because ',' is used also
        // to separe 2 operands in Gerber strings, remove the first ',' that is a operand separator,
        // not a float separator
        line.Replace(","," ", false);
        line.ToCDouble( &ret );
        // Find the end of the float number. The float number contains only chars
        // "0123456789." but can start by a '+' or '-' char.
        // others chars (usually '+' '-' '$' ',' ) are separators between operands and are not members
        // of the current float number
        if( ( line[0] == '+' || line[0] == '-' ) && line.Length() > 1 && line[1] != '$' )
        {
            // It is the sign of a number, not an operator. Remove it to find the last digit
            line[0] = '0';
        }

        auto endpos = line.find_first_not_of( "0123456789." );

        if( endpos != wxString::npos )
        {
            // Advance the text pointer to the end of the number
            text += endpos;
        }
        else
        {
            // If no non-number characters found, advance to the end of the string
            text += line.length();
        }
    }

    if( *text == ',' || isspace( *text ) )
    {
        if( aSkipSeparator )
            ++text;
    }

    return ret;
}

