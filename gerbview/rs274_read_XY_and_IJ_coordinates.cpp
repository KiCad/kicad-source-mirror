/**********************************************/
/**** rs274_read_XY_and_IJ_coordinates.cpp ****/
/**********************************************/

#include "fctsys.h"
#include "common.h"

#include "gerbview.h"
#include "macros.h"
#include "class_GERBER.h"


/* These routines read the text string point from Text.
 * After use, advanced Text the beginning of the sequence unread
 */
wxPoint GERBER_IMAGE::ReadXYCoord( char*& Text )
{
    wxPoint pos;
    int     type_coord = 0, current_coord, nbdigits;
    bool    is_float   = false;
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
                if( m_GerbMetric )
                    current_coord = wxRound( atof( line ) / 0.00254 );
                else
                    current_coord = wxRound( atof( line ) * PCB_INTERNAL_UNIT );
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
                double real_scale     = 1.0;
                double scale_list[10] =
                {
                    10000.0, 1000.0, 100.0, 10.0,
                    1,
                    0.1,     0.01,   0.001, 0.0001,0.00001
                };
                real_scale = scale_list[fmt_scale];

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;

                current_coord = wxRound( current_coord * real_scale );
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
                if( m_GerbMetric )
                    current_coord = wxRound( atof( line ) / 0.00254 );
                else
                    current_coord = wxRound( atof( line ) * PCB_INTERNAL_UNIT );
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
                double real_scale = 1.0;
                if( fmt_scale < 0 || fmt_scale > 9 )
                    fmt_scale = 4;      // select scale 1.0

                double scale_list[10] =
                {
                    10000.0, 1000.0, 100.0, 10.0,
                    1,
                    0.1,     0.01,   0.001, 0.0001,0.00001
                };
                real_scale = scale_list[fmt_scale];

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;
                current_coord = wxRound( current_coord * real_scale );
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
        if( aSkipSeparator )
            ++text;

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
        if( aSkipSeparator )
            ++text;

    return ret;
}

