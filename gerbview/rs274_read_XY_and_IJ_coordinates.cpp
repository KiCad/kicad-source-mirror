/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <common.h>

#include <gerbview.h>
#include <macros.h>
#include <class_GERBER.h>
#include <base_units.h>


/* These routines read the text string point from Text.
 * On exit, Text points the beginning of the sequence unread
 */

// convertion scale from gerber file units to Gerbview internal units
// depending on the gerber file format
// this scale list assumes gerber units are imperial.
// for metric gerber units, the imperial to metric conversion is made in read functions
#define SCALE_LIST_SIZE 9
static double scale_list[SCALE_LIST_SIZE] =
{
    1000.0 * IU_PER_MILS,   // x.1 format (certainly useless)
    100.0 * IU_PER_MILS,    // x.2 format (certainly useless)
    10.0 * IU_PER_MILS,     // x.3 format
    1.0 * IU_PER_MILS,      // x.4 format
    0.1 * IU_PER_MILS,      // x.5 format
    0.01 * IU_PER_MILS,     // x.6 format
    0.001 * IU_PER_MILS,     // x.7 format  (currently the max allowed precision)
    0.0001 * IU_PER_MILS,   // provided, but not used
    0.00001 * IU_PER_MILS,  // provided, but not used
};

/*
 * Function scale
 * converts a coordinate given in floating point to Gerbvies internal units
 * (currently = 10 nanometers)
 */
int scaletoIU( double aCoord, bool isMetric )
{
    int ret;

    if( isMetric )  // gerber are units in mm
        ret = KiROUND( aCoord * IU_PER_MM );
    else            // gerber are units in inches
        ret = KiROUND( aCoord * IU_PER_MILS * 1000.0 );

    return ret;
}


wxPoint GERBER_IMAGE::ReadXYCoord( char*& Text )
{
    wxPoint pos;
    int     type_coord = 0, current_coord, nbdigits;
    bool    is_float   = m_DecimalFormat;
    char*   text;
    char    line[256];


    if( m_Relative )
        pos.x = pos.y = 0;
    else
        pos = m_CurrentPos;

    if( Text == NULL )
        return pos;

    text = line;
    while( *Text )
    {
        if( (*Text == 'X') || (*Text == 'Y') )
        {
            type_coord = *Text;
            Text++;
            text     = line;
            nbdigits = 0;

            while( IsNumber( *Text ) )
            {
                if( *Text == '.' )  // Force decimat format if reading a floating point number
                    is_float = true;

                // count digits only (sign and decimal point are not counted)
                if( (*Text >= '0') && (*Text <='9') )
                    nbdigits++;
                *(text++) = *(Text++);
            }

            *text = 0;

            if( is_float )
            {
                // When X or Y values are float numbers, they are given in mm or inches
                if( m_GerbMetric )  // units are mm
                    current_coord = KiROUND( atof( line ) * IU_PER_MILS / 0.0254 );
                else    // units are inches
                    current_coord = KiROUND( atof( line ) * IU_PER_MILS * 1000 );
            }
            else
            {
                int fmt_scale = (type_coord == 'X') ? m_FmtScale.x : m_FmtScale.y;

                if( m_NoTrailingZeros )
                {
                    int min_digit =
                        (type_coord == 'X') ? m_FmtLen.x : m_FmtLen.y;
                    while( nbdigits < min_digit )
                    {
                        *(text++) = '0';
                        nbdigits++;
                    }

                    *text = 0;
                }

                current_coord = atoi( line );
                double real_scale = scale_list[fmt_scale];

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;

                current_coord = KiROUND( current_coord * real_scale );
            }

            if( type_coord == 'X' )
                pos.x = current_coord;
            else if( type_coord == 'Y' )
                pos.y = current_coord;

            continue;
        }
        else
            break;
    }

    if( m_Relative )
    {
        pos.x += m_CurrentPos.x;
        pos.y += m_CurrentPos.y;
    }

    m_CurrentPos = pos;
    return pos;
}


/* Returns the current coordinate type pointed to by InnJnn Text (InnnnJmmmm)
 * These coordinates are relative, so if coordinate is absent, it's value
 * defaults to 0
 */
wxPoint GERBER_IMAGE::ReadIJCoord( char*& Text )
{
    wxPoint pos( 0, 0 );

    int     type_coord = 0, current_coord, nbdigits;
    bool    is_float   = false;
    char*   text;
    char    line[256];

    if( Text == NULL )
        return pos;

    text = line;
    while( *Text )
    {
        if( (*Text == 'I') || (*Text == 'J') )
        {
            type_coord = *Text;
            Text++;
            text     = line;
            nbdigits = 0;
            while( IsNumber( *Text ) )
            {
                if( *Text == '.' )
                    is_float = true;

                // count digits only (sign and decimal point are not counted)
                if( (*Text >= '0') && (*Text <='9') )
                    nbdigits++;

                *(text++) = *(Text++);
            }

            *text = 0;
            if( is_float )
            {
                // When X or Y values are float numbers, they are given in mm or inches
                if( m_GerbMetric )  // units are mm
                    current_coord = KiROUND( atof( line ) * IU_PER_MILS / 0.0254 );
                else    // units are inches
                    current_coord = KiROUND( atof( line ) * IU_PER_MILS * 1000 );
            }
            else
            {
                int fmt_scale =
                    (type_coord == 'I') ? m_FmtScale.x : m_FmtScale.y;

                if( m_NoTrailingZeros )
                {
                    int min_digit =
                        (type_coord == 'I') ? m_FmtLen.x : m_FmtLen.y;
                    while( nbdigits < min_digit )
                    {
                        *(text++) = '0';
                        nbdigits++;
                    }

                    *text = 0;
                }

                current_coord = atoi( line );

                double real_scale = scale_list[fmt_scale];

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;

                current_coord = KiROUND( current_coord * real_scale );
            }
            if( type_coord == 'I' )
                pos.x = current_coord;
            else if( type_coord == 'J' )
                pos.y = current_coord;

            continue;
        }
        else
            break;
    }

    m_IJPos = pos;
    return pos;
}


// Helper functions:

/**
 * Function ReadInt
 * reads an int from an ASCII character buffer.  If there is a comma after the
 * int, then skip over that.
 * @param text A reference to a character pointer from which bytes are read
 *    and the pointer is advanced for each byte read.
 * @param aSkipSeparator = true (default) to skip comma
 * @return int - The int read in.
 */
int ReadInt( char*& text, bool aSkipSeparator = true )
{
    int ret = (int) strtol( text, &text, 10 );

    if( *text == ',' || isspace( *text ) )
    {
        if( aSkipSeparator )
            ++text;
    }

    return ret;
}


/**
 * Function ReadDouble
 * reads a double from an ASCII character buffer. If there is a comma after
 * the double, then skip over that.
 * @param text A reference to a character pointer from which the ASCII double
 *          is read from and the pointer advanced for each character read.
 * @param aSkipSeparator = true (default) to skip comma
 * @return double
 */
double ReadDouble( char*& text, bool aSkipSeparator = true )
{
    double ret = strtod( text, &text );

    if( *text == ',' || isspace( *text ) )
    {
        if( aSkipSeparator )
            ++text;
    }

    return ret;
}

