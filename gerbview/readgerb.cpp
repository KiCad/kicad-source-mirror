/**********************/
/**** readgerb.cpp ****/
/**********************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "protos.h"

/* Format Gerber: NOTES:
 *   Functions history:
 *   Gn =
 *   G01 linear interpolation (right trace)
 *   G02, G20, G21 Circular interpolation, meaning trig <0
 *   G03, G30, G31 Circular interpolation, meaning trigo> 0
 *   G04 review
 *   G06 parabolic interpolation
 *   G07 Cubic Interpolation
 *   G10 linear interpolation (scale x10)
 *   G11 linear interpolation (0.1x range)
 *   G12 linear interpolation (0.01x scale)
 *   G52 plot symbol reference code by Dnn
 *   G53 plot symbol reference by Dnn; symbol rotates from -90 degrees
 *   G54 Selection Tool
 *   G55 Fashion photo exhibition
 *   G56 plot symbol reference code for DNN
 *   G57 displays the symbol link to the console
 *   G58 plot displays the symbol and link to the console
 *   G60 linear interpolation (scale x100)
 *   G70 Units = Inches
 *   G71 Units = Millimeters
 *   G74 circular interpolation removes 360 degree, has returned G01
 *   G75 Active circular interpolation on 360 degree
 *   G90 mode absolute coordinates
 *   G91 Fashion Related Contacts
 *
 *   X, Y coordinates
 *   X and Y are followed by + or - and m + n digits (not separated)
 *   m = integer part
 *   n = part after the comma
 *   Classic formats: m = 2, n = 3 (size 2.3)
 *   m = 3, n = 4 (size 3.4)
 *   eg
 *   G__ X00345Y-06123 * D__
 *
 *   Tools and D_CODES
 *   Tool number (identification of shapes)
 *   1 to 99 (Classical)
 *   1 to 999
 *   D_CODES:
 *
 *   D01 ... D9 = action codes:
 *   D01 = activating light (lower pen) when placement
 *   D02 = light extinction (lift pen) when placement
 *   D03 = Flash
 *   D09 = VAPE Flash
 *   D51 = G54 preceded by -> Select VAPE
 *
 *   D10 ... D255 = Identification Tool (Opening)
 *   Not in order (see table in PCBPLOT.H)
 */


/* Routine to Read a file D Codes.
 * Accepts standard format or ALSPCB
 * A ';' starts a comment.
 *
 * Standard Format:
 * Tool, Horiz, Vert, drill, speed, acc. Type; [dCode (comment)]
 * Ex: 1, 12, 12, 0, 0, 0, 3; D10
 *
 * Format: ALSPCB
 * Ver, Hor, Type, Tool, [Drill]
 * Eg 0012, 0012, L, D10
 *
 * Rank the characters in buf_tmp tabular structures D_CODE.
 * Returns:
 * <0 if error:
 * -1 = File not found
 * -2 = Error reading file
 * Rank D_code max lu (nbr of dCode)
 *
 * Internal Representation:
 *
 * Lines are represented as standard TRACKS
 * The flash is represented as DRAWSEGMENTS
 * - Round or oval: DRAWSEGMENTS
 * - Rectangles DRAWSEGMENTS
 * Reference to the D-CODE is set in the member Getnet()
 */


/* Read a gerber file (RS274D gold RS274X format).
 * Normal size:
 * Imperial
 * Absolute
 * End of block = *
 * CrLf after each command
 * G codes BROKE
 */
bool WinEDA_GerberFrame::Read_GERBER_File( const wxString& GERBER_FullFileName,
                                           const wxString& D_Code_FullFileName )
{
    int      G_commande = 0,
             D_commande = 0;            /* command number for G or D commands
                                         * (like G04 or D02) */

    char     line[GERBER_BUFZ];

    wxString msg;
    char*    text;
    int      layer;         /* current layer used in gerbview */
    GERBER*  gerber;
    int      error = 0;

    layer = GetScreen()->m_Active_Layer;

    if( g_GERBER_List[layer] == NULL )
    {
        g_GERBER_List[layer] = new GERBER( layer );
    }

    gerber = g_GERBER_List[layer];

    /* Set the gerber scale: */
    gerber->ResetDefaultValues();

    /* Read the gerber file */
    gerber->m_Current_File = wxFopen( GERBER_FullFileName, wxT( "rt" ) );
    if( gerber->m_Current_File == 0 )
    {
        msg = _( "File " ) + GERBER_FullFileName + _( " not found" );
        DisplayError( this, msg, 10 );
        return FALSE;
    }

    gerber->m_FileName = GERBER_FullFileName;

    wxString path = wxPathOnly( GERBER_FullFileName );
    if( path != wxEmptyString )
        wxSetWorkingDirectory( path );

    wxBusyCursor show_wait;
    SetLocaleTo_C_standard();

    while( TRUE )
    {
        if( fgets( line, sizeof(line), gerber->m_Current_File ) == NULL )
        {
            if( gerber->m_FilesPtr == 0 )
                break;

            fclose( gerber->m_Current_File );

            gerber->m_FilesPtr--;
            gerber->m_Current_File =
                gerber->m_FilesList[gerber->m_FilesPtr];

            continue;
        }

        text = StrPurge( line );

        while( text && *text )
        {
            switch( *text )
            {
            case ' ':
            case '\r':
            case '\n':
                text++;
                break;

            case '*':       // End command
                gerber->m_CommandState = END_BLOCK;
                text++;
                break;

            case 'M':       // End file
                gerber->m_CommandState = CMD_IDLE;
                while( *text )
                    text++;

                break;

            case 'G':    /* Line type Gxx : command */
                G_commande = gerber->ReturnGCodeNumber( text );
                gerber->Execute_G_Command( text, G_commande );
                break;

            case 'D':       /* Line type Dxx : Tool selection (xx > 0) or
                             * command if xx = 0..9 */
                D_commande = gerber->ReturnDCodeNumber( text );
                gerber->Execute_DCODE_Command( this, text, D_commande );
                break;

            case 'X':
            case 'Y':                   /* Move or draw command */
                 gerber->m_CurrentPos = gerber->ReadXYCoord( text );
                if( *text == '*' )      // command like X12550Y19250*
                {
                    gerber->Execute_DCODE_Command( this, text,
                                                   gerber->m_Last_Pen_Command );
                }
                break;

            case 'I':
            case 'J':       /* Auxiliary Move command */
                 gerber->m_IJPos = gerber->ReadIJCoord( text );
                break;

            case '%':
                if( gerber->m_CommandState != ENTER_RS274X_CMD )
                {
                    gerber->m_CommandState = ENTER_RS274X_CMD;

                    if( !gerber->ReadRS274XCommand( this, line, text ) )
                    {
                        error++;
                    }
                }
                else        //Error
                {
                    error++;
                    gerber->m_CommandState = CMD_IDLE;
                    text++;
                }
                break;

            default:
                text++;
                error++;
                break;
            }
        }
    }

    if( error )
    {
        msg.Printf( _( "%d errors while reading Gerber file [%s]" ),
                   error, GetChars(GERBER_FullFileName) );
        DisplayError( this, msg );
    }
    fclose( gerber->m_Current_File );

    SetLocaleTo_Default();

    /* Init DCodes list and perhaps read a DCODES file,
     * if the gerber file is only a RS274D file (without any aperture
     * information)
     */
    if( !gerber->m_Has_DCode )
    {
        wxFileName fn;

        if( D_Code_FullFileName.IsEmpty() )
        {
            wxString wildcard;

            fn = GERBER_FullFileName;
            fn.SetExt( g_PenFilenameExt );
            wildcard.Printf( _( "Gerber DCODE files (%s)|*.%s" ),
                             GetChars( g_PenFilenameExt ),
                             GetChars( g_PenFilenameExt ) );
            wildcard += AllFilesWildcard;

            wxFileDialog dlg( this, _( "Load GERBER DCODE File" ),
                              wxEmptyString, fn.GetFullName(), wildcard,
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST );
        }
        else
            fn = D_Code_FullFileName;

        if( fn.IsOk() )
        {
            Read_D_Code_File( fn.GetFullPath() );
            CopyDCodesSizeToItems();
        }
        else
            return false;
    }

    return true;
}
