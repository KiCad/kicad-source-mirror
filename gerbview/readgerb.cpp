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


/* Read a gerber file, RS274D or RS274X format.
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
        g_GERBER_List[layer] = new GERBER( this, layer );
    }

    gerber = g_GERBER_List[layer];
    ClearMessageList( );

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
