/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2018 Jean-Pierre Charras  jp.charras at wanadoo.fr
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


#include <base_units.h>
#include <math/util.h>      // for KiROUND

#include <gerbview.h>
#include <gerber_file_image.h>
#include <core/ignore.h>
#include <macros.h>
#include <string_utils.h>
#include <X2_gerber_attributes.h>
#include <gbr_metadata.h>
#include <wx/log.h>

extern int ReadInt( char*& text, bool aSkipSeparator = true );
extern double ReadDouble( char*& text, bool aSkipSeparator = true );


#define CODE( x, y ) ( ( (x) << 8 ) + (y) )

// See rs274xrevd_e.pdf, table 1: RS-274X parameters order of entry
// in gerber files, when a coordinate is given (like X78Y600 or I0J80):
//      Y and Y are logical coordinates
//      A and B are plotter coordinates
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
    // commands used only once at the beginning of the file, and are deprecated
    IMAGE_JUSTIFY  = CODE( 'I', 'J' ),          // Default: no justification
    IMAGE_NAME     = CODE( 'I', 'N' ),          // Default: void
    IMAGE_OFFSET   = CODE( 'I', 'O' ),          // Default: A = 0, B = 0
    IMAGE_POLARITY = CODE( 'I', 'P' ),          // Default: Positive
    IMAGE_ROTATION = CODE( 'I', 'R' ),          // Default: 0

    // Aperture parameters:
    // Usually for the whole file
    AP_DEFINITION   = CODE( 'A', 'D' ),
    AP_MACRO = CODE( 'A', 'M' ),

    // X2 extension attribute commands
    // Mainly are found standard attributes and user attributes
    // standard attributes commands are:
    // TF (file attribute) TO (net attribute)
    // TA (aperture attribute) and TD (delete aperture attribute)
    FILE_ATTRIBUTE   = CODE( 'T', 'F' ),

    // X2 extension Net attribute info
    // Net attribute options are:
    // TO (net attribute data): TO.CN or TO.P TO.N or TO.C
    NET_ATTRIBUTE   = CODE( 'T', 'O' ),

    // X2 extension Aperture attribute TA
    APERTURE_ATTRIBUTE   = CODE( 'T', 'A' ),

    // TD (delete aperture/object attribute):
    // Delete aperture attribute added by %TA or Oblect attribute added b %TO
    // TD (delete all) or %TD<attr name> to delete <attr name>.
    // eg: TD.P or TD.N or TD.C ...
    REMOVE_APERTURE_ATTRIBUTE   = CODE( 'T', 'D' ),

    // Layer specific parameters
    // May be used singly or may be layer specific
    // These parameters are at the beginning of the file or layer
    // and reset some layer parameters (like interpolation)
    KNOCKOUT = CODE( 'K', 'O' ),                // Default: off
    STEP_AND_REPEAT = CODE( 'S', 'R' ),         //  Default: A = 1, B = 1
    ROTATE = CODE( 'R', 'O' ),                  //  Default: 0

    LOAD_POLARITY  = CODE( 'L', 'P' ),          //LPC or LPD. Default: Dark (LPD)
    LOAD_NAME      = CODE( 'L', 'N' ),          // Deprecated: equivalent to G04
};


int GERBER_FILE_IMAGE::ReadXCommandID( char*& text )
{
    /* reads  two bytes of data and assembles them into an int with the first
     * byte in the sequence put into the most significant part of a 16 bit value
     */
    int result;
    int currbyte;

    if( text && *text )
    {
        currbyte = *text++;
        result = ( currbyte & 0xFF ) << 8;
    }
    else
        return -1;

    if( text && *text )
    {
        currbyte = *text++;
        result += currbyte & 0xFF;
    }
    else
        return -1;

    return result;
}


bool GERBER_FILE_IMAGE::ReadRS274XCommand( char *aBuff, unsigned int aBuffSize, char*& aText )
{
    bool ok = true;
    int  code_command;

    aText++;

    for( ; ; )
    {
        while( *aText )
        {
            switch( *aText )
            {
            case '%':       // end of command
                aText++;
                m_CommandState = CMD_IDLE;
                goto exit;  // success completion

            case ' ':
            case '\r':
            case '\n':
                aText++;
                break;

            case '*':
                aText++;
                break;

            default:
                code_command = ReadXCommandID( aText );
                ok = ExecuteRS274XCommand( code_command, aBuff, aBuffSize, aText );

                if( !ok )
                    goto exit;

                break;
            }
        }

        // end of current line, read another one.
        if( fgets( aBuff, aBuffSize, m_Current_File ) == nullptr )
        {
            // end of file
            ok = false;
            break;
        }

        m_LineNum++;
        aText = aBuff;
    }

exit:
    return ok;
}


bool GERBER_FILE_IMAGE::ExecuteRS274XCommand( int aCommand, char* aBuff,
                                              unsigned int aBuffSize, char*& aText )
{
    int      code;
    int      seq_len;    // not used, just provided
    int      seq_char;
    bool     ok = true;
    wxString msg;
    double   fcoord;
    bool     x_fmt_known = false;
    bool     y_fmt_known = false;

    // conv_scale = scaling factor from inch to Internal Unit
    double   conv_scale = gerbIUScale.IU_PER_MILS * 1000;

    X2_ATTRIBUTE dummy;

    if( m_GerbMetric )
        conv_scale /= 25.4;

    switch( aCommand )
    {
    case FORMAT_STATEMENT:
        seq_len = 2;

        while( *aText != '*' )
        {
            switch( *aText )
            {
            case ' ':
                aText++;
                break;

            case 'D':       // Non-standard option for all zeros (leading + tailing)
                msg.Printf( _( "RS274X: Invalid GERBER format command '%c' at line %d: '%s'" ),
                        'D', m_LineNum, aBuff );
                AddMessageToList( msg );
                msg.Printf( _("GERBER file '%s' may not display as intended." ),
                        m_FileName.ToAscii() );
                AddMessageToList( msg );
                KI_FALLTHROUGH;

            case 'L':       // No Leading 0
                m_NoTrailingZeros = false;
                aText++;
                break;

            case 'T':       // No trailing 0
                m_NoTrailingZeros = true;
                aText++;
                break;

            case 'A':       // Absolute coord
                m_Relative = false;
                aText++;
                break;

            case 'I':       // Relative coord
                m_Relative = true;
                aText++;
                break;

            case 'G':
            case 'N':       // Sequence code (followed by one digit: the sequence len)
                            // (sometimes found before the X,Y sequence)
                            // Obscure option
                aText++;
                seq_char = *aText++;

                if( (seq_char >= '0') && (seq_char <= '9') )
                    seq_len = seq_char - '0';

                break;

            case 'M':       // Sequence code (followed by one digit: the sequence len)
                            // (sometimes found after the X,Y sequence)
                            // Obscure option
                aText++;
                code = *aText;

                if( ( code >= '0' ) && ( code <= '9' ) )
                    aText++;     // skip the digit

                break;

            case 'X':
            case 'Y':
            {
                code = *(aText++);
                char ctmp = *(aText++) - '0';

                if( code == 'X' )
                {
                    x_fmt_known = true;
                    // number of digits after the decimal point (0 to 7 allowed)
                    m_FmtScale.x = *aText - '0';
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
                    m_FmtScale.y = *aText - '0';
                    m_FmtLen.y   = ctmp + m_FmtScale.y;

                    if( m_FmtScale.y < 0 )
                        m_FmtScale.y = 0;

                    if( m_FmtScale.y > 7 )
                        m_FmtScale.y = 7;
                }

                aText++;
                break;
            }

            case '*':
                break;

            default:
                msg.Printf( wxT( "Unknown id (%c) in FS command" ), *aText );
                AddMessageToList( msg );
                GetEndOfBlock( aBuff, aBuffSize, aText, m_Current_File );
                ok = false;
                break;
            }
        }

        if( !x_fmt_known || !y_fmt_known )
            AddMessageToList( wxT( "RS274X: Format Statement (FS) without X or Y format" ) );

        break;

    case AXIS_SELECT:       // command ASAXBY*% or %ASAYBX*%
        m_SwapAxis = false;

        if( strncasecmp( aText, "AYBX", 4 ) == 0 )
            m_SwapAxis = true;

        break;

    case MIRROR_IMAGE:      // command %MIA0B0*%, %MIA0B1*%, %MIA1B0*%, %MIA1B1*%
        m_MirrorA = m_MirrorB = false;

        while( *aText && *aText != '*' )
        {
            switch( *aText )
            {
            case 'A':       // Mirror A axis ?
                aText++;

                if( *aText == '1' )
                    m_MirrorA = true;

                break;

            case 'B':       // Mirror B axis ?
                aText++;

                if( *aText == '1' )
                    m_MirrorB = true;

                break;

            default:
                aText++;
                break;
            }
        }
        break;

    case MODE_OF_UNITS:
        code = ReadXCommandID( aText );

        if( code == INCH )
            m_GerbMetric = false;
        else if( code == MILLIMETER )
            m_GerbMetric = true;

        conv_scale = m_GerbMetric ? gerbIUScale.IU_PER_MILS / 25.4 : gerbIUScale.IU_PER_MILS;
        break;

    case FILE_ATTRIBUTE:    // Command %TF ...
        dummy.ParseAttribCmd( m_Current_File, aBuff, aBuffSize, aText, m_LineNum );

        if( dummy.IsFileFunction() )
        {
            delete m_FileFunction;
            m_FileFunction = new X2_ATTRIBUTE_FILEFUNCTION( dummy );

            // Don't set this until we get a file function; other code expects m_IsX2_file == true
            // to mean that we have a valid m_FileFunction
            m_IsX2_file = true;
        }
        else if( dummy.IsFileMD5() )
        {
            m_MD5_value = dummy.GetPrm( 1 );
        }
        else if( dummy.IsFilePart() )
        {
            m_PartString = dummy.GetPrm( 1 );
        }

        break;

    case APERTURE_ATTRIBUTE:    // Command %TA
        dummy.ParseAttribCmd( m_Current_File, aBuff, aBuffSize, aText, m_LineNum );

        if( dummy.GetAttribute() == wxT( ".AperFunction" ) )
        {
            m_AperFunction = dummy.GetPrm( 1 );

            // A few function values can have other parameters. Add them
            for( int ii = 2; ii < dummy.GetPrmCount(); ii++ )
                m_AperFunction << wxT( "," ) << dummy.GetPrm( ii );
        }

        break;

    case NET_ATTRIBUTE:    // Command %TO currently %TO.P %TO.N and %TO.C
        dummy.ParseAttribCmd( m_Current_File, aBuff, aBuffSize, aText, m_LineNum );

        if( dummy.GetAttribute() == wxT( ".N" ) )
        {
            m_NetAttributeDict.m_NetAttribType |= GBR_NETLIST_METADATA::GBR_NETINFO_NET;
            m_NetAttributeDict.m_Netname = FormatStringFromGerber( dummy.GetPrm( 1 ) );
        }
        else if( dummy.GetAttribute() == wxT( ".C" ) )
        {
            m_NetAttributeDict.m_NetAttribType |= GBR_NETLIST_METADATA::GBR_NETINFO_CMP;
            m_NetAttributeDict.m_Cmpref = FormatStringFromGerber( dummy.GetPrm( 1 ) );
        }
        else if( dummy.GetAttribute() == wxT( ".P" ) )
        {
            m_NetAttributeDict.m_NetAttribType |= GBR_NETLIST_METADATA::GBR_NETINFO_PAD;
            m_NetAttributeDict.m_Cmpref = FormatStringFromGerber( dummy.GetPrm( 1 ) );
            m_NetAttributeDict.m_Padname.SetField( FormatStringFromGerber( dummy.GetPrm( 2 ) ), true, true );

            if( dummy.GetPrmCount() > 3 )
            {
                m_NetAttributeDict.m_PadPinFunction.SetField( FormatStringFromGerber( dummy.GetPrm( 3 ) ),
                                                              true, true );
            }
            else
            {
                m_NetAttributeDict.m_PadPinFunction.Clear();
            }
        }

        break;

    case REMOVE_APERTURE_ATTRIBUTE:    // Command %TD ...
        dummy.ParseAttribCmd( m_Current_File, aBuff, aBuffSize, aText, m_LineNum );
        RemoveAttribute( dummy );

        break;

    case OFFSET:        // command: OFAnnBnn (nn = float number) = layer Offset
        m_Offset.x = m_Offset.y = 0;

        while( *aText != '*' )
        {
            switch( *aText )
            {
            case 'A':       // A axis offset in current unit (inch or mm)
                aText++;
                fcoord     = ReadDouble( aText );
                m_Offset.x = KiROUND( fcoord * conv_scale );
                break;

            case 'B':       // B axis offset in current unit (inch or mm)
                aText++;
                fcoord     = ReadDouble( aText );
                m_Offset.y = KiROUND( fcoord * conv_scale );
                break;
            }
        }

        break;

    case SCALE_FACTOR:
        m_Scale.x = m_Scale.y = 1.0;

        while( *aText != '*' )
        {
            switch( *aText )
            {
            case 'A':       // A axis scale
                aText++;
                m_Scale.x = ReadDouble( aText );
                break;

            case 'B':       // B axis scale
                aText++;
                m_Scale.y = ReadDouble( aText );
                break;
            }
        }

        break;

    case IMAGE_OFFSET:  // command: IOAnnBnn (nn = float number) = Image Offset
        m_ImageOffset.x = m_ImageOffset.y = 0;

        while( *aText != '*' )
        {
            switch( *aText )
            {
            case 'A':       // A axis offset in current unit (inch or mm)
                aText++;
                fcoord     = ReadDouble( aText );
                m_ImageOffset.x = KiROUND( fcoord * conv_scale );
                break;

            case 'B':       // B axis offset in current unit (inch or mm)
                aText++;
                fcoord     = ReadDouble( aText );
                m_ImageOffset.y = KiROUND( fcoord * conv_scale );
                break;
            }
        }

        break;

    case IMAGE_ROTATION:    // command IR0* or IR90* or IR180* or IR270*
        if( strncasecmp( aText, "0*", 2 ) == 0 )
            m_ImageRotation = 0;
        else if( strncasecmp( aText, "90*", 3 ) == 0 )
            m_ImageRotation = 90;
        else if( strncasecmp( aText, "180*", 4 ) == 0 )
            m_ImageRotation = 180;
        else if( strncasecmp( aText, "270*", 4 ) == 0 )
            m_ImageRotation = 270;
        else
            AddMessageToList( _( "RS274X: Command \"IR\" rotation value not allowed" ) );

        break;

    case STEP_AND_REPEAT:   // command SR, like %SRX3Y2I5.0J2*%
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;       // Start a new Gerber layer
        GetLayerParams().m_StepForRepeat.x = 0.0;
        GetLayerParams().m_StepForRepeat.x = 0.0;       // offset for Step and Repeat command
        GetLayerParams().m_XRepeatCount = 1;
        GetLayerParams().m_YRepeatCount = 1;            // The repeat count
        GetLayerParams().m_StepForRepeatMetric = m_GerbMetric;  // the step units

        while( *aText && *aText != '*' )
        {
            switch( *aText )
            {
            case 'I':       // X axis offset
                aText++;
                GetLayerParams().m_StepForRepeat.x = ReadDouble( aText );
                break;

            case 'J':       // Y axis offset
                aText++;
                GetLayerParams().m_StepForRepeat.y = ReadDouble( aText );
                break;

            case 'X':       // X axis repeat count
                aText++;
                GetLayerParams().m_XRepeatCount = ReadInt( aText );
                break;

            case 'Y':       // Y axis offset
                aText++;
                GetLayerParams().m_YRepeatCount = ReadInt( aText );
                break;

            default:
                aText++;
                break;
            }
        }

        break;

    case IMAGE_JUSTIFY: // Command IJAnBn*
        m_ImageJustifyXCenter = false;           // Image Justify Center on X axis (default = false)
        m_ImageJustifyYCenter = false;           // Image Justify Center on Y axis (default = false)
        m_ImageJustifyOffset = VECTOR2I( 0, 0 ); // Image Justify Offset on XY axis (default = 0,0)

        while( *aText && *aText != '*' )
        {
            // IJ command is (for A or B axis) AC or AL or A<coordinate>
            switch( *aText )
            {
            case 'A':       // A axis justify
                aText++;

                if( *aText == 'C' )
                {
                    m_ImageJustifyXCenter = true;
                    aText++;
                }
                else if( *aText == 'L' )
                {
                    m_ImageJustifyXCenter = true;
                    aText++;
                }
                else
                {
                    m_ImageJustifyOffset.x = KiROUND( ReadDouble( aText ) * conv_scale);
                }

                break;

            case 'B':       // B axis justify
                aText++;

                if( *aText == 'C' )
                {
                    m_ImageJustifyYCenter = true;
                    aText++;
                }
                else if( *aText == 'L' )
                {
                    m_ImageJustifyYCenter = true;
                    aText++;
                }
                else
                {
                    m_ImageJustifyOffset.y = KiROUND( ReadDouble( aText ) * conv_scale);
                }

                break;

            default:
                aText++;
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
        AddMessageToList( msg );
        break;

    case ROTATE:        // Layer rotation: command like %RO45*%
        m_Iterpolation  = GERB_INTERPOL_LINEAR_1X;       // Start a new Gerber layer
        m_LocalRotation = ReadDouble( aText );           // Store layer rotation in degrees
        break;

    case IMAGE_NAME:
        m_ImageName.Empty();

        while( *aText != '*' )
            m_ImageName.Append( *aText++ );

        break;

    case LOAD_NAME:
        // %LN is a (deprecated) equivalentto G04: a comment
        while( *aText && *aText != '*' )
            aText++; // Skip text

        break;

    case IMAGE_POLARITY:
        // Note: these commands IPPOS and IPNEG are deprecated since 2012.
        if( strncasecmp( aText, "NEG", 3 ) == 0 )
        {
            m_ImageNegative = true;
            // IPPOS Gerber command is deprecated since 2012.
            // in Gerber doc 2024, the advice is: warn user and skip it.
            AddMessageToList( _( "IPNEG Gerber command is deprecated since 2012. Skip it" ) );
        }
        else
        {
            m_ImageNegative = false;
            // IPPOS Gerber command is deprecated since 2012.
            // However this is the default for a Gerber file, and does not have
            // actual effect. Just skip it.
        }

        break;

    case LOAD_POLARITY:
        if( *aText == 'C' )
            GetLayerParams().m_LayerNegative = true;
        else
            GetLayerParams().m_LayerNegative = false;

        break;

    case AP_MACRO:  // lines like %AMMYMACRO*
                    // 5,1,8,0,0,1.08239X$1,22.5*
                    // %
        /*ok = */ReadApertureMacro( aBuff, aBuffSize, aText, m_Current_File );
        break;

    case AP_DEFINITION:
        /* input example:  %ADD30R,0.081800X0.101500*%
         * Aperture definition has 4 options: C, R, O, P
         * (Circle, Rect, Oval, regular Polygon)
         * and shapes can have a hole (round or rectangular).
         * All optional parameters values start by X
         * at this point, text points to 2nd 'D'
         */
        if( *aText++ != 'D' )
        {
            ok = false;
            break;
        }

        m_Has_DCode = true;

        code = ReadInt( aText );

        D_CODE* dcode;
        dcode = GetDCODEOrCreate( code );

        if( dcode == nullptr )
            break;

        dcode->m_AperFunction = m_AperFunction;

        // at this point, text points to character after the ADD<num>,
        // i.e. R in example above.  If aText[0] is one of the usual
        // apertures: (C,R,O,P), there is a comma after it.
        if( aText[1] == ',' )
        {
            char stdAperture = *aText;

            aText += 2;              // skip "C," for example

            // First parameter is the size X:
            dcode->m_Size.x = KiROUND( ReadDouble( aText ) * conv_scale );
            dcode->m_Size.y = dcode->m_Size.x;

            switch( stdAperture )   // Aperture desceiption has optional parameters. Read them
            {
            case 'C':               // Circle
                dcode->m_ApertType = APT_CIRCLE;
                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' )
                {
                    aText++;
                    dcode->m_Drill.x = dcode->m_Drill.y =
                                           KiROUND( ReadDouble( aText ) * conv_scale );
                    dcode->m_DrillShape = APT_DEF_ROUND_HOLE;
                }

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' )
                {
                    aText++;
                    dcode->m_Drill.y =
                        KiROUND( ReadDouble( aText ) * conv_scale );

                    dcode->m_DrillShape = APT_DEF_RECT_HOLE;
                }

                dcode->m_Defined = true;
                break;

            case 'O':               // oval
            case 'R':               // rect
                dcode->m_ApertType = (stdAperture == 'O') ? APT_OVAL : APT_RECT;

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' ) // Second parameter: size Y
                {
                    aText++;
                    dcode->m_Size.y = KiROUND( ReadDouble( aText ) * conv_scale );
                }

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' ) // third parameter: drill size (or drill size X)
                {
                    aText++;
                    dcode->m_Drill.x = KiROUND( ReadDouble( aText ) * conv_scale );
                    dcode->m_Drill.y = dcode->m_Drill.x;
                    dcode->m_DrillShape = APT_DEF_ROUND_HOLE;
                }

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' ) // fourth parameter: drill size Y
                {
                    aText++;
                    dcode->m_Drill.y = KiROUND( ReadDouble( aText ) * conv_scale );
                    dcode->m_DrillShape = APT_DEF_RECT_HOLE;
                }

                dcode->m_Defined = true;
                break;

            case 'P':

                /* Regular polygon: a command line like %ADD12P,0.040X10X25X0.025X0.025X0.0150*%
                 * params are: <diameter>, X<edge count>, X<Rotation>, X<X hole dim>, X<Y hole dim>
                 */
                dcode->m_ApertType = APT_POLYGON;

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' )
                {
                    aText++;
                    dcode->m_EdgesCount = ReadInt( aText );
                }

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' )
                {
                    aText++;
                    dcode->m_Rotation = EDA_ANGLE( ReadDouble( aText ), DEGREES_T );
                }

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' )
                {
                    aText++;
                    dcode->m_Drill.x = KiROUND( ReadDouble( aText ) * conv_scale );
                    dcode->m_Drill.y = dcode->m_Drill.x;
                    dcode->m_DrillShape = APT_DEF_ROUND_HOLE;
                }

                while( *aText == ' ' )
                    aText++;

                if( *aText == 'X' )
                {
                    aText++;
                    dcode->m_Drill.y = KiROUND( ReadDouble( aText ) * conv_scale );
                    dcode->m_DrillShape = APT_DEF_RECT_HOLE;
                }

                dcode->m_Defined = true;
                break;
            }
        }
        else    // aText[0] starts an aperture macro name
        {
            APERTURE_MACRO am_lookup;

            while( *aText && *aText != '*' && *aText != ',' )
                am_lookup.m_AmName.Append( *aText++ );

            // When an aperture definition is like %AMLINE2* 22,1,$1,$2,0,0,-45*
            // the ADDxx<MACRO_NAME> command has parameters, like %ADD14LINE2,0.8X0.5*%
            if( *aText == ',' )
            {   // Read aperture macro parameters and store them
                aText++;     // aText points the first parameter

                while( *aText && *aText != '*' )
                {
                    double param = ReadDouble( aText );
                    dcode->AppendParam( param );

                    if( !( isspace( *aText ) || *aText == 'X' || *aText == 'x' || *aText == '*' ) )
                    {
                        msg.Printf( wxT( "RS274X: aperture macro %s has invalid template "
                                         "parameters\n" ),
                                    TO_UTF8( am_lookup.m_AmName ) );
                        AddMessageToList( msg );
                        ok = false;
                        break;
                    }

                    while( isspace( *aText ) )
                        aText++;

                    // Skip 'X' separator:
                    if( *aText == 'X' || *aText == 'x' )
                        aText++;
                }
            }

            // lookup the aperture macro here.
            APERTURE_MACRO* pam = FindApertureMacro( am_lookup );

            if( !pam )
            {
                msg.Printf( wxT( "RS274X: aperture macro %s not found\n" ),
                           TO_UTF8( am_lookup.m_AmName ) );
                AddMessageToList( msg );
                ok = false;
                break;
            }

            dcode->m_ApertType = APT_MACRO;
            dcode->SetMacro( pam );
            dcode->m_Defined = true;
        }

        break;

    default:
        ok = false;
        break;
    }

    ignore_unused( seq_len );

    if( !GetEndOfBlock( aBuff, aBuffSize, aText, m_Current_File ) )
        ok = false;

    return ok;
}


bool GERBER_FILE_IMAGE::GetEndOfBlock( char* aBuff, unsigned int aBuffSize, char*& aText, FILE* gerber_file )
{
    for( ; ; )
    {
        while( (aText < aBuff + aBuffSize) && *aText )
        {
            if( *aText == '*' )
                return true;

            if( *aText == '%' )
                return true;

            aText++;
        }

        if( fgets( aBuff, aBuffSize, gerber_file ) == nullptr )
            break;

        m_LineNum++;
        aText = aBuff;
    }

    return false;
}


char* GERBER_FILE_IMAGE::GetNextLine( char *aBuff, unsigned int aBuffSize, char* aText, FILE* aFile )
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
            if( fgets( aBuff, aBuffSize, aFile ) == nullptr )
                return nullptr;

            m_LineNum++;
            aText = aBuff;
            return aText;

        default:
            return aText;
        }
    }
}


bool GERBER_FILE_IMAGE::ReadApertureMacro( char *aBuff, unsigned int aBuffSize, char*& aText,
                                           FILE* gerber_file )
{
    wxString       msg;
    APERTURE_MACRO am;

    // read macro name
    while( *aText )
    {
        if( *aText == '*' )
        {
            ++aText;
            break;
        }

        am.m_AmName.Append( *aText++ );
    }

    // Read aperture macro parameters
    for( ; ; )
    {
        if( *aText == '*' )
            ++aText;

        aText = GetNextLine( aBuff, aBuffSize, aText, gerber_file );

        if( aText == nullptr )  // End of File
            return false;

        // aText points the beginning of a new line.

        // Test for the last line in aperture macro lis:
        // last line is % or *% sometime found.
        if( *aText == '*' )
            ++aText;

        if( *aText == '%' )
            break;      // exit with aText still pointing at %

        int paramCount = 0; // will be set to the minimal parameters count,
                            // depending on the actual primitive
        int primitive_type = AMP_UNKNOWN;
        // Test for a valid symbol at the beginning of a description:
        // it can be: a parameter declaration like $1=$2/4
        // or a digit (macro primitive selection)
        // all other symbols are illegal.
        if( *aText == '$' )  // local parameter declaration, inside the aperture macro
        {
            am.AddLocalParamDefToStack();
            AM_PARAM& param = am.GetLastLocalParamDefFromStack();
            aText = GetNextLine(  aBuff, aBuffSize, aText, gerber_file );

            if( aText == nullptr)   // End of File
                return false;

            param.ReadParamFromAmDef( aText );
            continue;
        }
        else if( !isdigit(*aText)  )     // Ill. symbol
        {
            msg.Printf( wxT( "RS274X: Aperture Macro \"%s\": ill. symbol, line: \"%s\"" ),
                        am.m_AmName, From_UTF8( aBuff ) );
            AddMessageToList( msg );
            primitive_type = AMP_COMMENT;
        }
        else
        {
            primitive_type = ReadInt( aText );
        }

        bool is_comment = false;

        switch( primitive_type )
        {
        case AMP_COMMENT:     // lines starting by 0 are a comment
            paramCount = 0;
            is_comment = true;

            // Skip comment
            GetEndOfBlock( m_LineBuffer, GERBER_BUFZ, aText, m_Current_File );

            break;

        case AMP_CIRCLE:
            paramCount = 4; // minimal count. can have a optional parameter (rotation)
            break;

        case AMP_LINE2:
        case AMP_LINE20:
            paramCount = 7;
            break;

        case AMP_LINE_CENTER:
        case AMP_LINE_LOWER_LEFT:
            paramCount = 6;
            break;

        case AMP_OUTLINE:
            paramCount = 4; // partial count. other parameters are vertices and rotation
                            // Second parameter is vertice (coordinate pairs) count.
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
            msg.Printf( wxT( "RS274X: Aperture Macro \"%s\": Invalid primitive id code %d, line %d: \"%s\"" ),
                        am.m_AmName, primitive_type, m_LineNum, From_UTF8( aBuff ) );
            AddMessageToList( msg );
            return false;
        }

        if( is_comment )
            continue;

        AM_PRIMITIVE prim( m_GerbMetric );
        prim.m_Primitive_id = (AM_PRIMITIVE_ID) primitive_type;
        int ii;

        for( ii = 0; ii < paramCount && *aText && *aText != '*'; ++ii )
        {
            prim.m_Params.push_back( AM_PARAM() );

            AM_PARAM& param = prim.m_Params.back();

            aText = GetNextLine( aBuff, aBuffSize, aText, gerber_file );

            if( aText == nullptr)   // End of File
                return false;

            param.ReadParamFromAmDef( aText );
        }

        if( ii < paramCount )
        {
            // maybe some day we can throw an exception and track a line number
            msg.Printf( wxT( "RS274X: read macro descr type %d: read %d parameters, insufficient "
                             "parameters\n" ),
                        prim.m_Primitive_id, ii );
            AddMessageToList( msg );
        }

        // there are more parameters to read if this is an AMP_OUTLINE
        if( prim.m_Primitive_id == AMP_OUTLINE )
        {
            // so far we have read [0]:exposure, [1]:#points, [2]:X start, [3]: Y start
            // Now read all the points, plus trailing rotation in degrees.

            // m_Params[1] is a count of polygon points, so it must be given
            // in advance, i.e. be immediate.
            wxASSERT( prim.m_Params[1].IsImmediate() );

            paramCount = (int) prim.m_Params[1].GetValueFromMacro( nullptr ) * 2 + 1;

            for( int jj = 0; jj < paramCount && *aText != '*'; ++jj )
            {
                prim.m_Params.push_back( AM_PARAM() );

                AM_PARAM& param = prim.m_Params.back();

                aText = GetNextLine( aBuff, aBuffSize, aText, gerber_file );

                if( aText == nullptr )  // End of File
                    return false;

                param.ReadParamFromAmDef( aText );
            }
        }

        // AMP_CIRCLE can have a optional parameter (rotation)
        if( prim.m_Primitive_id == AMP_CIRCLE && aText && *aText != '*' )
        {
            prim.m_Params.push_back( AM_PARAM() );
            AM_PARAM& param = prim.m_Params.back();
            param.ReadParamFromAmDef( aText );
        }

        // The primitive description is now parsed: push it to the current aperture macro
        am.AddPrimitiveToList( prim );
    }

    m_aperture_macros.insert( std::move( am ) );

    return true;
}

