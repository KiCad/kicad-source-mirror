/**************/
/* rs274x.cpp */
/**************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "macros.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"

#define CODE( x, y ) ( ( (x) << 8 ) + (y) )

enum RS274X_PARAMETERS {
    FORMAT_STATEMENT = CODE( 'F', 'S' ),
    AXIS_SELECT   = CODE( 'A', 'S' ),
    MIRROR_IMAGE  = CODE( 'M', 'I' ),
    MODE_OF_UNITS = CODE( 'M', 'O' ),
    INCH   = CODE( 'I', 'N' ),
    MILLIMETER = CODE( 'M', 'M' ),
    OFFSET = CODE( 'O', 'F' ),
    SCALE_FACTOR = CODE( 'S', 'F' ),

    IMAGE_NAME   = CODE( 'I', 'N' ),
    IMAGE_JUSTIFY   = CODE( 'I', 'J' ),
    IMAGE_OFFSET    = CODE( 'I', 'O' ),
    IMAGE_POLARITY  = CODE( 'I', 'P' ),
    IMAGE_ROTATION  = CODE( 'I', 'R' ),
    PLOTTER_FILM    = CODE( 'P', 'M' ),
    INCLUDE_FILE    = CODE( 'I', 'F' ),

    AP_DEFINITION   = CODE( 'A', 'D' ),

    AP_MACRO = CODE( 'A', 'M' ),
    LAYER_NAME      = CODE( 'L', 'N' ),
    LAYER_POLARITY  = CODE( 'L', 'P' ),
    KNOCKOUT = CODE( 'K', 'O' ),
    STEP_AND_REPEAT = CODE( 'S', 'P' ),
    ROTATE = CODE( 'R', 'O' )
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


/**
 * Function ReadInt
 * reads an int from an ASCII character buffer.  If there is a comma after the
 * int, then skip over that.
 * @param text A reference to a character pointer from which bytes are read
 *    and the pointer is advanced for each byte read.
 * @param int - The int read in.
 */
static int ReadInt( char*& text )
{
    int ret = (int) strtol( text, &text, 10 );

    if( *text == ',' || isspace( *text ) )
        ++text;

    return ret;
}


/**
 * Function ReadDouble
 * reads a double from an ASCII character buffer. If there is a comma after
 * the double, then skip over that.
 * @param text A reference to a character pointer from which the ASCII double
 *          is read from and the pointer advanced for each character read.
 * @return double
 */
static double ReadDouble( char*& text )
{
    double ret = strtod( text, &text );

    if( *text == ',' || isspace( *text ) )
        ++text;

    return ret;
}


bool GERBER::ReadRS274XCommand( WinEDA_GerberFrame* frame,
                                char buff[GERBER_BUFZ], char*& text )
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


bool GERBER::ExecuteRS274XCommand( int    command,
                                   char   buff[GERBER_BUFZ],
                                   char*& text )
{
    int      code;
    int      xy_seq_len, xy_seq_char;
    bool     ok = TRUE;
    char     line[GERBER_BUFZ];
    wxString msg;
    double   fcoord;
    double   conv_scale = m_GerbMetric ? PCB_INTERNAL_UNIT /
                          25.4 : PCB_INTERNAL_UNIT;

    D( printf( "%22s: Command <%c%c>\n", __func__, (command >> 8) & 0xFF,
               command & 0xFF ); )

    switch( command )
    {
    case FORMAT_STATEMENT:
        xy_seq_len = 2;

        while( *text != '*' )
        {
            switch( *text )
            {
            case ' ':
                text++;
                break;

            case 'L':       // No Leading 0
                m_NoTrailingZeros = FALSE;
                text++;
                break;

            case 'T':       // No trailing 0
                m_NoTrailingZeros = TRUE;
                text++;
                break;

            case 'A':       // Absolute coord
                m_Relative = FALSE;
                text++;
                break;

            case 'I':       // Absolute coord
                m_Relative = TRUE;
                text++;
                break;

            case 'N':       // Sequence code (followed by the number of digits
                            // for the X,Y command
                text++;
                xy_seq_char = *text++;
                if( (xy_seq_char >= '0') && (xy_seq_char <= '9') )
                    xy_seq_len = -'0';
                break;

            case 'X':
            case 'Y':       // Values transmitted :2 (really xy_seq_len : FIX
                            // ME) digits
            {
                code = *(text++);
                char ctmp = *(text++) - '0';
                if( code == 'X' )
                {
                    // number of digits after the decimal point
                    m_FmtScale.x = *text - '0';
                    m_FmtLen.x   = ctmp + m_FmtScale.x;
                }
                else
                {
                    m_FmtScale.y = *text - '0';
                    m_FmtLen.y   = ctmp + m_FmtScale.y;
                }
                text++;
            }
            break;

            case '*':
                break;

            default:
                GetEndOfBlock( buff, text, m_Current_File );
                ok = FALSE;
                break;
            }
        }

        break;

    case AXIS_SELECT:
    case MIRROR_IMAGE:
        ok = FALSE;
        break;

    case MODE_OF_UNITS:
        code = ReadXCommand( text );
        if( code == INCH )
            m_GerbMetric = FALSE;
        else if( code == MILLIMETER )
            m_GerbMetric = TRUE;
        conv_scale = m_GerbMetric ? PCB_INTERNAL_UNIT /
                     25.4 : PCB_INTERNAL_UNIT;
        break;

    case OFFSET:        // command: OFAnnBnn (nn = float number)
        m_Offset.x = m_Offset.y = 0;
        while( *text != '*' )
        {
            switch( *text )
            {
            case 'A':       // A axis offset in current unit (inch or mm)
                text++;
                fcoord     = ReadDouble( text );
                m_Offset.x = wxRound( fcoord * conv_scale );
                break;

            case 'B':       // B axis offset in current unit (inch or mm)
                text++;
                fcoord     = ReadDouble( text );
                m_Offset.y = wxRound( fcoord * conv_scale );
                break;
            }
        }

        break;

    case SCALE_FACTOR:
    case IMAGE_JUSTIFY:
    case IMAGE_ROTATION:
    case IMAGE_OFFSET:
    case PLOTTER_FILM:
    case LAYER_NAME:
    case KNOCKOUT:
    case STEP_AND_REPEAT:
    case ROTATE:
        msg.Printf( _( "Command <%c%c> ignored by Gerbview" ),
                    (command >> 8) & 0xFF, command & 0xFF );
        if( g_DebugLevel > 0 )
            wxMessageBox( msg );
        break;

    case IMAGE_NAME:
        m_Name.Empty();
        while( *text != '*' )
        {
            m_Name.Append( *text++ );
        }

        break;

    case IMAGE_POLARITY:
        if( strnicmp( text, "NEG", 3 ) == 0 )
            m_ImageNegative = true;
        else
            m_ImageNegative = false;
        D( printf( "%22s: IMAGE_POLARITY m_ImageNegative=%s\n", __func__,
                   m_ImageNegative ? "true" : "false" ); )
        break;

    case LAYER_POLARITY:
        if( *text == 'C' )
            m_LayerNegative = true;
        else
            m_LayerNegative = false;
        D( printf( "%22s: LAYER_POLARITY m_LayerNegative=%s\n", __func__,
                   m_LayerNegative ? "true" : "false" ); )
        break;

    case INCLUDE_FILE:
        if( m_FilesPtr >= 10 )
        {
            ok = FALSE;
            DisplayError( NULL, _( "Too many include files!!" ) );
            break;
        }
        strcpy( line, text );
        strtok( line, "*%%\n\r" );
        m_FilesList[m_FilesPtr] = m_Current_File;

        m_Current_File = fopen( line, "rt" );
        if( m_Current_File == 0 )
        {
            wxString msg;
            msg.Printf( wxT( "file <%s> not found." ), line );
            DisplayError( NULL, msg, 10 );
            ok = FALSE;
            m_Current_File = m_FilesList[m_FilesPtr];
            break;
        }
        m_FilesPtr++;
        break;

    case AP_MACRO:
        ok = ReadApertureMacro( buff, text, m_Current_File );
        if( !ok )
            break;
        break;

    case AP_DEFINITION:

        // input example:  %ADD30R,0.081800X0.101500*%
        // at this point, text points to 2nd 'D'

        if( *text++ != 'D' )
        {
            ok = FALSE;
            break;
        }

        m_Has_DCode = TRUE;

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

            dcode->m_Size.x = dcode->m_Size.y =
                wxRound( ReadDouble( text ) * conv_scale );

            switch( stdAperture )
            {
            case 'C':               // Circle
                dcode->m_Shape = APT_CIRCLE;
                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.x = dcode->m_Drill.y =
                        wxRound( ReadDouble( text ) * conv_scale );
                    dcode->m_DrillShape = 1;
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.y =
                        wxRound( ReadDouble( text ) * conv_scale );

                    dcode->m_DrillShape = 2;
                }
                dcode->m_Defined = TRUE;
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
                        wxRound( ReadDouble( text ) * conv_scale );
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'X' )
                {
                    text++;
                    dcode->m_Drill.x = dcode->m_Drill.y =
                        wxRound( ReadDouble( text ) * conv_scale );
                    dcode->m_DrillShape = 1;
                }

                while( *text == ' ' )
                    text++;

                if( *text == 'Y' )
                {
                    text++;
                    dcode->m_Drill.y =
                        wxRound( ReadDouble( text ) * conv_scale );
                    dcode->m_DrillShape = 2;
                }
                dcode->m_Defined = TRUE;
                break;

            case 'P':               // Polygon
                dcode->m_Shape   = APT_POLYGON;
                dcode->m_Defined = TRUE;
                break;
            }
        }
        else    // text[0] starts an aperture macro name
        {
            APERTURE_MACRO am_lookup;

            while( *text && *text != '*' && *text != ',' )
                am_lookup.name.Append( *text++ );

            if( *text && *text == ',' )
            {
                while( *text && *text != '*' )
                {
                    double param = ReadDouble( text );
                    if( *text == 'X' || isspace( *text ) )
                        ++text;

                    dcode->AppendParam( param );
                }
            }

            // lookup the aperture macro here.
            APERTURE_MACRO* pam = FindApertureMacro( am_lookup );
            if( !pam )
            {
                // @todo not found, don't know how to report an error
                D( printf( "aperture macro %s not found\n",
                          CONV_TO_UTF8( am_lookup.name ) ); )
                ok = false;
                break;
            }

            dcode->m_Shape = APT_MACRO;
            dcode->SetMacro( (APERTURE_MACRO*) pam );
        }
        break;

    default:
        ok = FALSE;
        break;
    }

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
                return TRUE;

            if( *text == '%' )
                return TRUE;

            text++;
        }

        if( fgets( buff, GERBER_BUFZ, gerber_file ) == NULL )
            break;

        text = buff;
    }

    return FALSE;
}


static bool CheckForLineEnd(  char buff[GERBER_BUFZ], char*& text, FILE* fp  )
{
    while( *text == '\n' || *text == '\r' || !*text )
    {
        if( *text == '\n' || *text == '\r' )
            ++text;

        if( !*text )
        {
            if( fgets( buff, GERBER_BUFZ, fp ) == NULL )
                return false;

            text = buff;
        }
    }

    return true;
}


bool GERBER::ReadApertureMacro( char   buff[GERBER_BUFZ],
                                char*& text,
                                FILE*  gerber_file )
{
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

    if( g_DebugLevel > 0 )
        wxMessageBox( am.name, wxT( "macro name" ) );

    for( ; ; )
    {
        AM_PRIMITIVE prim;

        if( *text == '*' )
            ++text;

        if( !CheckForLineEnd(  buff, text, gerber_file ) )
            return false;

        if( *text == '%' )
            break;      // exit with text still pointing at %

        prim.primitive_id = (AM_PRIMITIVE_ID) ReadInt( text );

        int paramCount;

        switch( prim.primitive_id )
        {
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
            paramCount = 4;
            break;

        case AMP_MOIRE:
            paramCount = 9;
            break;

        case AMP_THERMAL:
            paramCount = 6;
            break;

        default:

            // @todo, there needs to be a way of reporting the line number
            // and character offset.
            D( printf( "Invalid primitive id code %d\n", prim.primitive_id ); )
            return false;
        }

        int i;
        for( i = 0; i < paramCount && *text != '*'; ++i )
        {
            prim.params.push_back( DCODE_PARAM() );

            DCODE_PARAM& param = prim.params.back();

            if( !CheckForLineEnd(  buff, text, gerber_file ) )
                return false;

            if( *text == '$' )
            {
                ++text;
                param.SetIndex( ReadInt( text ) );
            }
            else
                param.SetValue( ReadDouble( text ) );
        }

        if( i < paramCount )  // maybe some day we can throw an exception and
                              // track a line number
            printf( "i=%d, insufficient parameters\n", i );

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
                prim.params.push_back( DCODE_PARAM() );

                DCODE_PARAM& param = prim.params.back();

                if( !CheckForLineEnd(  buff, text, gerber_file ) )
                    return false;

                if( *text == '$' )
                {
                    ++text;
                    param.SetIndex( ReadInt( text ) );
                }
                else
                    param.SetValue( ReadDouble( text ) );
            }
        }

        am.primitives.push_back( prim );
    }

    m_aperture_macros.insert( am );

    return true;
}
