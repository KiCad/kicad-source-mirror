/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

/**
 * @file rs274x.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <base_units.h>

#include <gerbview.h>
#include <class_GERBER.h>
#include <class_X2_gerber_attributes.h>

extern int ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );
extern bool GetEndOfBlock( char buff[GERBER_BUFZ], char*& text, FILE* gerber_file );


#define CODE( x, y ) ( ( (x) << 8 ) + (y) )

// See rs274xrevd_e.pdf, table 1: RS-274X parameters order of entry
// in gerber files, when a coordinate is given (like X78Y600 or I0J80):
//      Y and Y are logical coordinates
//      A and B are plotter coordiantes
//      Usually A = X, B = Y
//      But we can have A = Y, B = X and/or offset, mirror, scale;
// Also:
//  Image is what you must plot (the entire data of the file).
//  Layer is just a set of data blocks with their parameters. An image can have more than one
//   layer so a gerber layer is not like a board layer or the graphic layers used in GerbView
//   to show a file.
enum RS274X_PARAMETERS {
    // Directive parameters: single usage recommended
    // Must be at the beginning of the file
    AXIS_SELECT   = CODE( 'A', 'S' ),           // Default: A=X, B=Y
    FORMAT_STATEMENT = CODE( 'F', 'S' ),        // no default: this command must exists
    MIRROR_IMAGE  = CODE( 'M', 'I' ),           // Default: mo mirror
    MODE_OF_UNITS = CODE( 'M', 'O' ),           // Default:  inch
    INCH   = CODE( 'I', 'N' ),
    MILLIMETER = CODE( 'M', 'M' ),
    OFFSET = CODE( 'O', 'F' ),                  // Default: A = 0, B = 0
    SCALE_FACTOR   = CODE( 'S', 'F' ),          // Default:  A = 1.0, B = 1.0

    // Image parameters:
    // commands used only once at the beginning of the file
    IMAGE_JUSTIFY  = CODE( 'I', 'J' ),          // Default: no justification
    IMAGE_NAME     = CODE( 'I', 'N' ),          // Default: void
    IMAGE_OFFSET   = CODE( 'I', 'O' ),          // Default: A = 0, B = 0
    IMAGE_POLARITY = CODE( 'I', 'P' ),          // Default: Positive
    IMAGE_ROTATION = CODE( 'I', 'R' ),          // Default: 0
    PLOTTER_FILM   = CODE( 'P', 'M' ),

    // Aperture parameters:
    // Usually for the whole file
    AP_DEFINITION   = CODE( 'A', 'D' ),
    AP_MACRO = CODE( 'A', 'M' ),

    // X2 extention attribute commands
    // Mainly are found standard attributes and user attributes
    // standard attributes commands are:
    // TF (file attribute)
    // TA (aperture attribute) and TD (delete aperture attribute)
    FILE_ATTRIBUTE   = CODE( 'T', 'F' ),

    // Layer specific parameters
    // May be used singly or may be layer specfic
    // theses parameters are at the beginning of the file or layer
    // and reset some layer parameters (like interpolation)
    LAYER_NAME      = CODE( 'L', 'N' ),         // Default: Positive
    LAYER_POLARITY  = CODE( 'L', 'P' ),
    KNOCKOUT = CODE( 'K', 'O' ),                // Default: off
    STEP_AND_REPEAT = CODE( 'S', 'R' ),         //  Default: A = 1, B = 1
    ROTATE = CODE( 'R', 'O' ),                  //  Default: 0

    // Miscellaneous parameters:
    INCLUDE_FILE   = CODE( 'I', 'F' )
};


/**
 * Function ReadXCommand
 * reads in two bytes of data and assembles them into an int with the first
 * byte in the sequence put into the most significant part of a 16 bit value
 * and the second byte put into the least significant part of the 16 bit value.
 * @param text A reference to a pointer to read bytes from and to advance as
 *             they are read.
 * @return int - with 16 bits of data in the ls bits, upper bits zeroed.
 */
static int ReadXCommand( char*& text )
{
    int result;

    if( text && *text )
        result = *text++ << 8;
    else
        return -1;

    if( text && *text )
        result += *text++;
    else
        return -1;

    return result;
}


bool GERBER_IMAGE::ReadRS274XCommand( char buff[GERBER_BUFZ], char*& text )
{
    bool ok = true;
    int  code_command;

    text++;

    for( ; ; )
    {
        while( *text )
        {
            switch( *text )
            {
            case '%':       // end of command
                text++;
                m_CommandState = CMD_IDLE;
                goto exit;  // success completion

            case ' ':
            case '\r':
            case '\n':
                text++;
                break;

            case '*':
                text++;
                break;

            default:
                code_command = ReadXCommand( text );
                ok = ExecuteRS274XCommand( code_command, buff, text );
                if( !ok )
                    goto exit;
                break;
            }
        }

        // end of current line, read another one.
        if( fgets( buff, GERBER_BUFZ, m_Current_File ) == NULL )
        {
            // end of file
            ok = false;
            break;
        }

        text = buff;
    }

exit:
    return ok;
}


bool GERBER_IMAGE::ExecuteRS274XCommand( int       command,
                                   char buff[GERBER_BUFZ],
                                   char*&    text )
{
    int      code;
    int      seq_len;    // not used, just provided
    int      seq_char;
    bool     ok = true;
    char     line[GERBER_BUFZ];
    wxString msg;
    double   fcoord;
    bool     x_fmt_known = false;
    bool     y_fmt_known = false;

    // conv_scale = scaling factor from inch to Internal Unit
    double   conv_scale = IU_PER_MILS * 1000;
    if( m_GerbMetric )
        conv_scale /= 25.4;

//    DBG( printf( "%22s: Command <%c%c>\n", __func__, (command >> 8) & 0xFF, command & 0xFF ); )

    switch( command )
    {
    case FORMAT_STATEMENT:
        seq_len = 2;

        while( *text != '*' )
        {
            switch( *text )
            {
            case ' ':
                text++;
                break;

            case 'L':       // No Leading 0
                m_DecimalFormat = false;
                m_NoTrailingZeros = false;
                text++;
                break;

            case 'T':       // No trailing 0
                m_DecimalFormat = false;
                m_NoTrailingZeros = true;
                text++;
                break;

            case 'A':       // Absolute coord
                m_Relative = false;
                text++;
                break;

            case 'I':       // Relative coord
                m_Relative = true;
                text++;
                break;

            case 'G':
            case 'N':       // Sequence code (followed by one digit: the sequence len)
                            // (sometimes found before the X,Y sequence)
                            // Obscure option
                text++;
                seq_char = *text++;
                if( (seq_char >= '0') && (seq_char <= '9') )
                    seq_len = seq_char - '0';
                break;

            case 'D':
            case 'M':       // Sequence code (followed by one digit: the sequence len)
                            // (sometimes found after the X,Y sequence)
                            // Obscure option
                code = *text++;
                if( ( *text >= '0' ) && ( *text<= '9' ) )
                    text++;     // skip the digit
                else if( code == 'D' )
                    // Decimal format: sometimes found, but not really documented
                    m_DecimalFormat = true;
                break;

            case 'X':
            case 'Y':
            {
                code = *(text++);
                char ctmp = *(text++) - '0';
                if( code == 'X' )
                {
                    x_fmt_known = true;
                    // number of digits after the decimal point (0 to 7 allowed)
                    m_FmtScale.x = *text - '0';
                    m_FmtLen.x   = ctmp + m_FmtScale.x;

                    // m_FmtScale is 0 to 7
                    // (Old Gerber specification was 0 to 6)
                    if( m_FmtScale.x < 0 )
                        m_FmtScale.x = 0;
                    if( m_FmtScale.x > 7 )
                        m_FmtScale.x = 7;
                }
                else
                {
                    y_fmt_known = true;
                    m_FmtScale.y = *text - '0';
                    m_FmtLen.y   = ctmp + m_FmtScale.y;
                    if( m_FmtScale.y < 0 )
                        m_FmtScale.y = 0;
                    if( m_FmtScale.y > 7 )
                        m_FmtScale.y = 7;
                }
                text++;
            }
            break;

            case '*':
                break;

            default:
                msg.Printf( wxT( "Unknown id (%c) in FS command" ),
                           *text );
                ReportMessage( msg );
                GetEndOfBlock( buff, text, m_Current_File );
                ok = false;
                break;
            }
        }
        if( !x_fmt_known || !y_fmt_known )
            ReportMessage( wxT( "RS274X: Format Statement (FS) without X or Y format" ) );

        break;

    case AXIS_SELECT:       // command ASAXBY*% or %ASAYBX*%
        m_SwapAxis = false;
        if( strnicmp( text, "AYBX", 4 ) == 0 )
            m_SwapAxis = true;
        break;

    case MIRROR_IMAGE:      // command %MIA0B0*%, %MIA0B1*%, %MIA1B0*%, %MIA1B1*%
        m_MirrorA = m_MirrorB = 0;
        while( *text && *text != '*' )
        {
            switch( *text )
            {
            case 'A':       // Mirror A axis ?
                text++;
                if( *text == '1' )
                    m_MirrorA = true;
                break;

            case 'B':       // Mirror B axis ?
                text++;
                if( *text == '1' )
                    m_MirrorB = true;
                break;

            default:
                text++;
                break;
            }
        }
        break;

    case MODE_OF_UNITS:
        code = ReadXCommand( text );
        if( code == INCH )
            m_GerbMetric = false;
        else if( code == MILLIMETER )
            m_GerbMetric = true;
        conv_scale = m_GerbMetric ? IU_PER_MILS / 25.4 : IU_PER_MILS;
        break;

    case FILE_ATTRIBUTE:    // Command %TF ...
        m_IsX2_file = true;
    {
        X2_ATTRIBUTE dummy;
        dummy.ParseAttribCmd( m_Current_File, buff, GERBER_BUFZ, text );
        if( dummy.IsFileFunction() )
        {
            delete m_FileFunction;
            m_FileFunction = new X2_ATTRIBUTE_FILEFUNCTION( dummy );
        }
        else if( dummy.IsFileMD5() )
        {
            m_MD5_value = dummy.GetPrm( 1 );
        }
        else if( dummy.IsFilePart() )
        {
            m_PartString = dummy.GetPrm( 1 );
        }
     }
        break;

    case OFFSET:        // command: OFAnnBnn (nn = float number) = layer Offset
        m_Offset.x = m_Offset.y = 0;
        while( *text != '*' )
        {
            switch( *text )
            {
            case 'A':       // A axis offset in current unit (inch or mm)
                text++;
                fcoord     = ReadDouble( text );
                m_Offset.x = KiROUND( fcoord * conv_scale );
                break;

            case 'B':       // B axis offset in current unit (inch or mm)
                text++;
                fcoord     = ReadDouble( text );
                m_Offset.y = KiROUND( fcoord * conv_scale );
                break;
            }
        }
        break;

    case SCALE_FACTOR:
        m_Scale.x = m_Scale.y = 1.0;
        while( *text != '*' )
        {
            switch( *text )
            {
            case 'A':       // A axis scale
                text++;
                m_Scale.x = ReadDouble( text );
                break;

            case 'B':       // B axis scale
                text++;
                m_Scale.y = ReadDouble( text );
                break;
            }
        }
        break;

    case IMAGE_OFFSET:  // command: IOAnnBnn (nn = float number) = Image Offset
        m_ImageOffset.x = m_ImageOffset.y = 0;
        while( *text != '*' )
        {
            switch( *text )
            {
            case 'A':       // A axis offset in current unit (inch or mm)
                text++;
                fcoord     = ReadDouble( text );
                m_ImageOffset.x = KiROUND( fcoord * conv_scale );
                break;

            case 'B':       // B axis offset in current unit (inch or mm)
                text++;
                fcoord     = ReadDouble( text );
                m_ImageOffset.y = KiROUND( fcoord * conv_scale );
                break;
            }
        }
        break;

    case IMAGE_ROTATION:    // command IR0* or IR90* or IR180* or IR270*
        if( strnicmp( text, "0*", 2 ) == 0 )
            m_ImageRotation = 0;
        if( strnicmp( text, "90*", 2 ) == 0 )
            m_ImageRotation = 90;
        if( strnicmp( text, "180*", 2 ) == 0 )
            m_ImageRotation = 180;
        if( strnicmp( text, "270*", 2 ) == 0 )
            m_ImageRotation = 270;
        else
            ReportMessage( _( "RS274X: Command \"IR\" rotation value not allowed" ) );
        break;

    case STEP_AND_REPEAT:   // command SR, like %SRX3Y2I5.0J2*%
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Start a new Gerber layer
        GetLayerParams().m_StepForRepeat.x = 0.0;
        GetLayerParams().m_StepForRepeat.x = 0.0;       // offset for Step and Repeat command
        GetLayerParams().m_XRepeatCount = 1;
        GetLayerParams().m_YRepeatCount = 1;            // The repeat count
        GetLayerParams().m_StepForRepeatMetric = m_GerbMetric;  // the step units
        while( *text && *text != '*' )
        {
            switch( *text )
            {
            case 'I':       // X axis offset
                text++;
                GetLayerParams().m_StepForRepeat.x = ReadDouble( text );
                break;

            case 'J':       // Y axis offset
                text++;
                GetLayerParams().m_StepForRepeat.y = ReadDouble( text );
                break;

            case 'X':       // X axis repeat count
                text++;
                GetLayerParams().m_XRepeatCount = ReadInt( text );
                break;

            case 'Y':       // Y axis offset
                text++;
                GetLayerParams().m_YRepeatCount = ReadInt( text );
                break;
            default:
                text++;
                break;
            }
        }
        break;

    case IMAGE_JUSTIFY: // Command IJAnBn*
        m_ImageJustifyXCenter = false;          // Image Justify Center on X axis (default = false)
        m_ImageJustifyYCenter = false;          // Image Justify Center on Y axis (default = false)
        m_ImageJustifyOffset = wxPoint(0,0);    // Image Justify Offset on XY axis (default = 0,0)
        while( *text && *text != '*' )
        {
            // IJ command is (for A or B axis) AC or AL or A<coordinate>
            switch( *text )
            {
            case 'A':       // A axis justify
                text++;
                if( *text == 'C' )
                {
                    m_ImageJustifyXCenter = true;
                    text++;
                }
                else if( *text == 'L' )
                {
                    m_ImageJustifyXCenter = true;
                    text++;
                }
                else m_ImageJustifyOffset.x = KiROUND( ReadDouble( text ) * conv_scale);
                break;

            case 'B':       // B axis justify
                text++;
                if( *text == 'C' )
                {
                    m_ImageJustifyYCenter = true;
                    text++;
                }
                else if( *text == 'L' )
                {
                    m_ImageJustifyYCenter = true;
                    text++;
                }
                else m_ImageJustifyOffset.y = KiROUND( ReadDouble( text ) * conv_scale);
                break;
            default:
                text++;
                break;
            }
        }
        if( m_ImageJustifyXCenter )
            m_ImageJustifyOffset.x = 0;
        if( m_ImageJustifyYCenter )
            m_ImageJustifyOffset.y = 0;
        break;

    case KNOCKOUT:
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Start a new Gerber layer
        msg = _( "RS274X: Command KNOCKOUT ignored by GerbView" ) ;
        ReportMessage( msg );
        break;

    case PLOTTER_FILM:  // Command PF <string>
        // This is an info about film that must be used to plot this file
        // Has no meaning here. We just display this string
        msg = wxT( "Plotter Film info:<br>" );
        while( *text != '*' )
        {
           msg.Append( *text++ );
        }
        ReportMessage( msg );
        break;

    case ROTATE:        // Layer rotation: command like %RO45*%
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Start a new Gerber layer
        m_LocalRotation =ReadDouble( text );             // Store layer rotation in degrees
        break;

    case IMAGE_NAME:
        m_ImageName.Empty();
        while( *text != '*' )
        {
            m_ImageName.Append( *text++ );
        }

        break;

    case LAYER_NAME:
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Start a new Gerber layer
        GetLayerParams( ).m_LayerName.Empty();
        while( *text != '*' )
        {
            GetLayerParams( ).m_LayerName.Append( *text++ );
        }

        break;

    case IMAGE_POLARITY:
        if( strnicmp( text, "NEG", 3 ) == 0 )
            m_ImageNegative = true;
        else
            m_ImageNegative = false;
        DBG( printf( "%22s: IMAGE_POLARITY m_ImageNegative=%s\n", __func__,
                   m_ImageNegative ? "true" : "false" ); )
        break;

    case LAYER_POLARITY:
        if( *text == 'C' )
            GetLayerParams().m_LayerNegative = true;

        else
            GetLayerParams().m_LayerNegative = false;
        DBG( printf( "%22s: LAYER_POLARITY m_LayerNegative=%s\n", __func__,
                   GetLayerParams().m_LayerNegative ? "true" : "false" ); )
        break;

    case INCLUDE_FILE:
        if( m_FilesPtr >= INCLUDE_FILES_CNT_MAX )
        {
            ok = false;
            ReportMessage( _( "Too many include files!!" ) );
            break;
        }

        strncpy( line, text, sizeof(line)-1 );
        line[sizeof(line)-1] = '\0';

        strtok( line, "*%%\n\r" );
        m_FilesList[m_FilesPtr] = m_Current_File;

        m_Current_File = fopen( line, "rt" );
        if( m_Current_File == 0 )
        {
            msg.Printf( wxT( "include file <%s> not found." ), line );
            ReportMessage( msg );
            ok = false;
            m_Current_File = m_FilesList[m_FilesPtr];
            break;
        }
        m_FilesPtr++;
        break;

    case AP_MACRO:  // lines like %AMMYMACRO*
                    // 5,1,8,0,0,1.08239X$1,22.5*
                    // %
        /*ok = */ReadApertureMacro( buff, text, m_Current_File );
        break;

    case AP_DEFINITION:

        /* input example:  %ADD30R,0.081800X0.101500*%
         * Aperture definition has 4 options: C, R, O, P
         * (Circle, Rect, Oval, regular Polygon)
         * and shapes can have a hole (round or rectangular).
         * All optional parameters values start by X
         * at this point, text points to 2nd 'D'
         */
        if( *text++ != 'D' )
        {
            ok = false;
            break;
        }

        m_Has_DCode = true;

        code = ReadInt( text );

        D_CODE* dcode;
        dcode = GetDCODE( code );
        if( dcode == NULL )
            break;

        // at this point, text points to character after the ADD<num>,
        // i.e. R in example above.  If text[0] is one of the usual
        // apertures: (C,R,O,P), there is a comma after it.
        if( text[1] == ',' )
        {
            char stdAperture = *text;

            text += 2;              // skip "C," for example

            dcode->m_Size.x = KiROUND( ReadDouble( text ) * conv_scale );
            dcode->m_Size.y = dcode->m_Size.x;

            switch( stdAperture )   // Aperture desceiption has optional parameters. Read them
            {
            case 'C':               // Circle
                dcode->m_Shape = APT_CIRCLE;
                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.x = dcode->m_Drill.y =
                                           KiROUND( ReadDouble( text ) * conv_scale );
                    dcode->m_DrillShape = APT_DEF_ROUND_HOLE;
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.y =
                        KiROUND( ReadDouble( text ) * conv_scale );

                    dcode->m_DrillShape = APT_DEF_RECT_HOLE;
                }
                dcode->m_Defined = true;
                break;

            case 'O':               // oval
            case 'R':               // rect
                dcode->m_Shape = (stdAperture == 'O') ? APT_OVAL : APT_RECT;

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Size.y =
                        KiROUND( ReadDouble( text ) * conv_scale );
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.x = KiROUND( ReadDouble( text ) * conv_scale );
                    dcode->m_Drill.y = dcode->m_Drill.x;
                    dcode->m_DrillShape = APT_DEF_ROUND_HOLE;
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.y =
                        KiROUND( ReadDouble( text ) * conv_scale );
                    dcode->m_DrillShape = APT_DEF_RECT_HOLE;
                }
                dcode->m_Defined = true;
                break;

            case 'P':

                /* Regular polygon: a command line like %ADD12P,0.040X10X25X0.025X0.025X0.0150*%
                 * params are: <diameter>, X<edge count>, X<Rotation>, X<X hole dim>, X<Y hole dim>
                 */
                dcode->m_Shape = APT_POLYGON;
                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_EdgesCount = ReadInt( text );
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Rotation = ReadDouble( text );
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.x = KiROUND( ReadDouble( text ) * conv_scale );
                    dcode->m_Drill.y = dcode->m_Drill.x =
                    dcode->m_DrillShape = APT_DEF_ROUND_HOLE;
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.y = KiROUND( ReadDouble( text ) * conv_scale );
                    dcode->m_DrillShape = APT_DEF_RECT_HOLE;
                }
                dcode->m_Defined = true;
                break;
            }
        }
        else    // text[0] starts an aperture macro name
        {
            APERTURE_MACRO am_lookup;

            while( *text && *text != '*' && *text != ',' )
                am_lookup.name.Append( *text++ );

            // When an aperture definition is like %AMLINE2* 22,1,$1,$2,0,0,-45*
            // the ADDxx<MACRO_NAME> command has parameters, like %ADD14LINE2,0.8X0.5*%
            if( *text == ',' )
            {   // Read aperture macro parameters and store them
                text++;     // text points the first parameter
                while( *text && *text != '*' )
                {
                    double param = ReadDouble( text );
                    dcode->AppendParam( param );
                    while( isspace( *text ) ) text++;
                    if( *text == 'X' )
                        ++text;
                }
            }

            // lookup the aperture macro here.
            APERTURE_MACRO* pam = FindApertureMacro( am_lookup );
            if( !pam )
            {
                msg.Printf( wxT( "RS274X: aperture macro %s not found\n" ),
                           TO_UTF8( am_lookup.name ) );
                ReportMessage( msg );
                ok = false;
                break;
            }

            dcode->m_Shape = APT_MACRO;
            dcode->SetMacro( (APERTURE_MACRO*) pam );
        }
        break;

    default:
        ok = false;
        break;
    }

    (void) seq_len;     // quiet g++, or delete the unused variable.

    ok = GetEndOfBlock( buff, text, m_Current_File );

    return ok;
}


bool GetEndOfBlock( char buff[GERBER_BUFZ], char*& text, FILE* gerber_file )
{
    for( ; ; )
    {
        while( (text < buff + GERBER_BUFZ) && *text )
        {
            if( *text == '*' )
                return true;

            if( *text == '%' )
                return true;

            text++;
        }

        if( fgets( buff, GERBER_BUFZ, gerber_file ) == NULL )
            break;

        text = buff;
    }

    return false;
}


/**
 * Function GetNextLine
 * test for an end of line
 * if an end of line is found:
 *   read a new line
 * @param aBuff = buffer (size = GERBER_BUFZ) to fill with a new line
 * @param aText = pointer to the last useful char in aBuff
 *          on return: points the beginning of the next line.
 * @param aFile = the opened GERBER file to read
 * @return a pointer to the beginning of the next line or NULL if end of file
*/
static char* GetNextLine(  char aBuff[GERBER_BUFZ], char* aText, FILE* aFile  )
{
    for( ; ; )
    {
        switch (*aText )
        {
            case ' ':     // skip blanks
            case '\n':
            case '\r':    // Skip line terminators
                ++aText;
                break;

            case 0:    // End of text found in aBuff: Read a new string
                if( fgets( aBuff, GERBER_BUFZ, aFile ) == NULL )
                    return NULL;
                aText = aBuff;
                return aText;

            default:
                return aText;
        }
    }
    return aText;
}


bool GERBER_IMAGE::ReadApertureMacro( char buff[GERBER_BUFZ],
                                char*&    text,
                                FILE*     gerber_file )
{
    wxString       msg;
    APERTURE_MACRO am;

    // read macro name
    while( *text )
    {
        if( *text == '*' )
        {
            ++text;
            break;
        }

        am.name.Append( *text++ );
    }

    // Read aperture macro parameters
    for( ; ; )
    {
        if( *text == '*' )
            ++text;

        text = GetNextLine( buff, text, gerber_file );  // Get next line
        if( text == NULL )  // End of File
            return false;

        // text points the beginning of a new line.

        // Test for the last line in aperture macro lis:
        // last line is % or *% sometime found.
        if( *text == '*' )
            ++text;
        if( *text == '%' )
            break;      // exit with text still pointing at %

        int paramCount = 0;
        int primitive_type = AMP_UNKNOWN;
        // Test for a valid symbol at the beginning of a description:
        // it can be: a parameter declaration like $1=$2/4
        // or a digit (macro primitive selection)
        // all other symbols are illegal.
        if( *text == '$' )  // local parameter declaration, inside the aperture macro
        {
            am.m_localparamStack.push_back( AM_PARAM() );
            AM_PARAM& param = am.m_localparamStack.back();
            text = GetNextLine(  buff, text, gerber_file );
            if( text == NULL)   // End of File
                return false;
            param.ReadParam( text );
            continue;
        }
        else if( !isdigit(*text)  )     // Ill. symbol
        {
            msg.Printf( wxT( "RS274X: Aperture Macro \"%s\": ill. symbol, line: \"%s\"" ),
                        GetChars( am.name ), GetChars( FROM_UTF8( buff ) ) );
            ReportMessage( msg );
            primitive_type = AMP_COMMENT;
        }
        else
            primitive_type = ReadInt( text );

        switch( primitive_type )
        {
        case AMP_COMMENT:     // lines starting by 0 are a comment
            paramCount = 0;
            // Skip comment
            while( *text && (*text != '*') )
                text++;
            break;

        case AMP_CIRCLE:
            paramCount = 4;
            break;

        case AMP_LINE2:
        case AMP_LINE20:
            paramCount = 7;
            break;

        case AMP_LINE_CENTER:
        case AMP_LINE_LOWER_LEFT:
            paramCount = 6;
            break;

        case AMP_EOF:
            paramCount = 0;
            break;

        case AMP_OUTLINE:
            paramCount = 4;
            break;

        case AMP_POLYGON:
            paramCount = 6;
            break;

        case AMP_MOIRE:
            paramCount = 9;
            break;

        case AMP_THERMAL:
            paramCount = 6;
            break;

        default:
            // @todo, there needs to be a way of reporting the line number
            msg.Printf( wxT( "RS274X: Aperture Macro \"%s\": Invalid primitive id code %d, line: \"%s\"" ),
                        GetChars( am.name ), primitive_type,  GetChars( FROM_UTF8( buff ) ) );
            ReportMessage( msg );
            return false;
        }

        AM_PRIMITIVE prim( m_GerbMetric );
        prim.primitive_id = (AM_PRIMITIVE_ID) primitive_type;
        int i;

        for( i = 0; i < paramCount && *text && *text != '*'; ++i )
        {
            prim.params.push_back( AM_PARAM() );

            AM_PARAM& param = prim.params.back();

            text = GetNextLine(  buff, text, gerber_file );

            if( text == NULL)   // End of File
                return false;

            param.ReadParam( text );
        }

        if( i < paramCount )
        {
            // maybe some day we can throw an exception and track a line number
            msg.Printf( wxT( "RS274X: read macro descr type %d: read %d parameters, insufficient parameters\n" ),
                        prim.primitive_id, i );
            ReportMessage( msg );

        }
        // there are more parameters to read if this is an AMP_OUTLINE
        if( prim.primitive_id == AMP_OUTLINE )
        {
            // so far we have read [0]:exposure, [1]:#points, [2]:X start, [3]: Y start
            // Now read all the points, plus trailing rotation in degrees.

            // params[1] is a count of polygon points, so it must be given
            // in advance, i.e. be immediate.
            wxASSERT( prim.params[1].IsImmediate() );

            paramCount = (int) prim.params[1].GetValue( 0 ) * 2 + 1;

            for( int i = 0; i < paramCount && *text != '*'; ++i )
            {
                prim.params.push_back( AM_PARAM() );

                AM_PARAM& param = prim.params.back();

                text = GetNextLine(  buff, text, gerber_file );

                if( text == NULL )  // End of File
                    return false;

                param.ReadParam( text );
            }
        }

        am.primitives.push_back( prim );
    }

    m_aperture_macros.insert( am );

    return true;
}

