/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include "ki_exception.h"
#include <string_utils.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <richio.h>
#include <view/view.h>

#include <dialogs/html_message_box.h>
#include <macros.h>

#include <wx/msgdlg.h>

/* Read a gerber file, RS274D, RS274X or RS274X2 format.
 */
bool GERBVIEW_FRAME::Read_GERBER_File( const wxString& GERBER_FullFileName )
{
    wxString msg;

    int layer = GetActiveLayer();
    GERBER_FILE_IMAGE_LIST* images = GetImagesList();
    GERBER_FILE_IMAGE* gerber = GetGbrImage( layer );

    if( gerber != nullptr )
    {
        Erase_Current_DrawLayer( false );
    }

    // use an unique ptr while we load to free on exception properly
    std::unique_ptr<GERBER_FILE_IMAGE> gerber_uptr = std::make_unique<GERBER_FILE_IMAGE>( layer );

    // Read the gerber file. The image will be added only if it can be read
    // to avoid broken data.
    bool success = gerber_uptr->LoadGerberFile( GERBER_FullFileName );

    if( !success )
    {
        gerber_uptr.reset();
        msg.Printf( _( "File '%s' not found" ), GERBER_FullFileName );
        ShowInfoBarError( msg );
        return false;
    }

    gerber = gerber_uptr.release();
    wxASSERT( gerber != nullptr );
    images->AddGbrImage( gerber, layer );

    // Display errors list
    if( gerber->GetMessages().size() > 0 )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Errors" ) );
        dlg.ListSet( gerber->GetMessages() );
        dlg.ShowModal();
    }

    /* if the gerber file has items using D codes but missing D codes definitions,
     * it can be a deprecated RS274D file (i.e. without any aperture information),
     * or has missing definitions,
     * warn the user:
     */
    if( gerber->GetItemsCount() && gerber->m_Has_MissingDCode )
    {
        if( !gerber->m_Has_DCode )
            msg = _("Warning: this file has no D-Code definition\n"
                    "Therefore the size of some items is undefined");
        else
            msg = _("Warning: this file has some missing D-Code definitions\n"
                    "Therefore the size of some items is undefined");

        wxMessageBox( msg );
    }

    if( GetCanvas() )
    {
        if( gerber->m_ImageNegative )
        {
            // TODO: find a way to handle negative images
            // (maybe convert geometry into positives?)
        }

        for( GERBER_DRAW_ITEM* item : gerber->GetItems() )
            GetCanvas()->GetView()->Add( (KIGFX::VIEW_ITEM*) item );
    }

    return true;
}


/*
 * Original function derived from gerber_is_rs274x_p() of gerbv 2.7.0.
 * Copyright of the source file readgerb.cpp included below:
 */
/* gEDA - GNU Electronic Design Automation
 * This is a part of gerbv
 *
 *   Copyright (C) 2000-2003 Stefan Petersen (spe@stacken.kth.se)
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */
bool GERBER_FILE_IMAGE::TestFileIsRS274( const wxString& aFullFileName )
{
    char* letter    = nullptr;
    bool  foundADD  = false;
    bool  foundD0   = false;
    bool  foundD2   = false;
    bool  foundM0   = false;
    bool  foundM2   = false;
    bool  foundStar = false;
    bool  foundX    = false;
    bool  foundY    = false;

    try
    {
        FILE_LINE_READER gerberReader( aFullFileName );

        while( gerberReader.ReadLine() )
        {
            // Remove all whitespace from the beginning and end
            char* line = StrPurge( gerberReader.Line() );

            // Skip empty lines
            if( *line == 0 )
                continue;

            // Check that file is not binary (non-printing chars)
            size_t len = strlen( line );

            for( size_t i = 0; i < len; i++ )
            {
                if( !isascii( line[i] ) )
                    return false;
            }

            if( strstr( line, "%ADD" ) )
                foundADD = true;

            if( strstr( line, "D00" ) || strstr( line, "D0" ) )
                foundD0 = true;

            if( strstr( line, "D02" ) || strstr( line, "D2" ) )
                foundD2 = true;

            if( strstr( line, "M00" ) || strstr( line, "M0" ) )
                foundM0 = true;

            if( strstr( line, "M02" ) || strstr( line, "M2" ) )
                foundM2 = true;

            if( strstr( line, "*" ) )
                foundStar = true;

            /* look for X<number> or Y<number> */
            if( ( letter = strstr( line, "X" ) ) != nullptr )
            {
                if( isdigit( letter[1] ) )
                    foundX = true;
            }

            if( ( letter = strstr( line, "Y" ) ) != nullptr )
            {
                if( isdigit( letter[1] ) )
                    foundY = true;
            }
        }
    }
    catch( IO_ERROR& )
    {
        return false;
    }

    // RS-274X
    if( ( foundD0 || foundD2 || foundM0 || foundM2 ) && foundADD && foundStar
        && ( foundX || foundY ) )
    {
        return true;
    }
    // RS-274D. Could be folded into the expression above, but someday
    // we might want to test for them separately.
    else if( ( foundD0 || foundD2 || foundM0 || foundM2 ) && !foundADD && foundStar
             && ( foundX || foundY ) )
    {
        return true;
    }


    return false;
}


// A large buffer to store one line
char GERBER_FILE_IMAGE::m_LineBuffer[GERBER_BUFZ+1];

bool GERBER_FILE_IMAGE::LoadGerberFile( const wxString& aFullFileName )
{
    int      G_command = 0;        // command number for G commands like G04
    int      D_commande = 0;       // command number for D commands like D02
    char*    text;

    ClearMessageList( );
    ResetDefaultValues();

    // Read the gerber file */
    m_Current_File = wxFopen( aFullFileName, wxT( "rt" ) );

    if( m_Current_File == nullptr )
        return false;

    m_FileName = aFullFileName;

    wxString msg;

    while( true )
    {
        if( fgets( m_LineBuffer, GERBER_BUFZ, m_Current_File ) == nullptr )
            break;

        m_LineNum++;
        text = StrPurge( m_LineBuffer );

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
                m_CommandState = END_BLOCK;
                text++;
                break;

            case 'M':       // End file
                m_CommandState = CMD_IDLE;
                while( *text )
                    text++;
                break;

            case 'G':    /* Line type Gxx : command */
                G_command = CodeNumber( text );
                Execute_G_Command( text, G_command );
                break;

            case 'D':       /* Line type Dxx : Tool selection (xx > 0) or
                             * command if xx = 0..9 */
                D_commande = CodeNumber( text );
                Execute_DCODE_Command( text, D_commande );
                break;

            case 'X':
            case 'Y':                   /* Move or draw command */
                m_CurrentPos = ReadXYCoord( text );
                if( *text == '*' )      // command like X12550Y19250*
                {
                    Execute_DCODE_Command( text, m_Last_Pen_Command );
                }
                break;

            case 'I':
            case 'J':       /* Auxiliary Move command */
                m_IJPos = ReadIJCoord( text );

                if( *text == '*' )      // command like X35142Y15945J504*
                {
                    Execute_DCODE_Command( text, m_Last_Pen_Command );
                }
                break;

            case '%':
                if( m_CommandState != ENTER_RS274X_CMD )
                {
                    m_CommandState = ENTER_RS274X_CMD;
                    ReadRS274XCommand( m_LineBuffer, GERBER_BUFZ, text );
                }
                else        //Error
                {
                    AddMessageToList( wxT( "Expected RS274X Command" ) );
                    m_CommandState = CMD_IDLE;
                    text++;
                }
                break;

            default:
                char    charBuf[2] = { *text, 0 };
                wchar_t wcharBuf[2] = { 0 };

                wxWhateverWorksConv().ToWChar( wcharBuf, 1, charBuf, 1 );

                if( wcharBuf[0] < 32 ) // Don't render control characters
                    wcharBuf[0] = '?';

                msg.Printf( wxT( "Unexpected char 0x%2.2X (%c)" ), (unsigned char) *text,
                            wcharBuf[0] );

                AddMessageToList( EscapeHTML( msg ) );
                text++;
                break;
            }
        }
    }

    fclose( m_Current_File );

    m_InUse = true;

    return true;
}
