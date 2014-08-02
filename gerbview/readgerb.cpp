/**********************/
/**** readgerb.cpp ****/
/**********************/

#include <fctsys.h>
#include <common.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_GERBER.h>

#include <html_messagebox.h>
#include <macros.h>

/* Read a gerber file, RS274D or RS274X format.
 */
bool GERBVIEW_FRAME::Read_GERBER_File( const wxString& GERBER_FullFileName,
                                           const wxString& D_Code_FullFileName )
{
    int      G_command = 0;        // command number for G commands like G04
    int      D_commande = 0;       // command number for D commands like D02

    char     line[GERBER_BUFZ];

    wxString msg;
    char*    text;
    int layer;         // current layer used in GerbView

    layer = getActiveLayer();

    if( g_GERBER_List[layer] == NULL )
    {
        g_GERBER_List[layer] = new GERBER_IMAGE( this, layer );
    }

    GERBER_IMAGE* gerber = g_GERBER_List[layer];
    ClearMessageList( );

    /* Set the gerber scale: */
    gerber->ResetDefaultValues();

    /* Read the gerber file */
    gerber->m_Current_File = wxFopen( GERBER_FullFileName, wxT( "rt" ) );
    if( gerber->m_Current_File == 0 )
    {
        msg.Printf( _( "File <%s> not found" ), GetChars( GERBER_FullFileName ) );
        DisplayError( this, msg, 10 );
        return false;
    }

    gerber->m_FileName = GERBER_FullFileName;

    wxString path = wxPathOnly( GERBER_FullFileName );
    if( path != wxEmptyString )
        wxSetWorkingDirectory( path );

    SetLocaleTo_C_standard();

    while( true )
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
                G_command = gerber->GCodeNumber( text );
                gerber->Execute_G_Command( text, G_command );
                break;

            case 'D':       /* Line type Dxx : Tool selection (xx > 0) or
                             * command if xx = 0..9 */
                D_commande = gerber->DCodeNumber( text );
                gerber->Execute_DCODE_Command( text, D_commande );
                break;

            case 'X':
            case 'Y':                   /* Move or draw command */
                gerber->m_CurrentPos = gerber->ReadXYCoord( text );
                if( *text == '*' )      // command like X12550Y19250*
                {
                    gerber->Execute_DCODE_Command( text,
                                                   gerber->m_Last_Pen_Command );
                }
                break;

            case 'I':
            case 'J':       /* Auxiliary Move command */
                gerber->m_IJPos = gerber->ReadIJCoord( text );
                if( *text == '*' )      // command like X35142Y15945J504*
                {
                    gerber->Execute_DCODE_Command( text,
                                                   gerber->m_Last_Pen_Command );
                }
                break;

            case '%':
                if( gerber->m_CommandState != ENTER_RS274X_CMD )
                {
                    gerber->m_CommandState = ENTER_RS274X_CMD;
                    gerber->ReadRS274XCommand( line, text );
                }
                else        //Error
                {
                    ReportMessage( wxT("Expected RS274X Command")  );
                    gerber->m_CommandState = CMD_IDLE;
                    text++;
                }
                break;

            default:
                text++;
                msg.Printf( wxT("Unexpected symbol <%c>"), *text );
                ReportMessage( msg );
                break;
            }
        }
    }
    fclose( gerber->m_Current_File );
    SetLocaleTo_Default();

    gerber->m_InUse = true;

    // Display errors list
    if( m_Messages.size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _("Errors") );
        dlg.ListSet(m_Messages);
        dlg.ShowModal();
    }

    /* if the gerber file is only a RS274D file
     * (i.e. without any aperture information), wran the user:
     */
    if( !gerber->m_Has_DCode )
    {
        msg = _("Warning: this file has no D-Code definition\n"
                "It is perhaps an old RS274D file\n"
                "Therefore the size of items is undefined");
        wxMessageBox( msg );
    }

    return true;
}
