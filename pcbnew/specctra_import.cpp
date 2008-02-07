/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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

 
/*  This source is a complement to specctra.cpp and implements the import of
    a specctra session file (*.ses), and import of a specctra design file 
    (*.dsn) file.  The specification for the grammar of the specctra dsn file 
    used to develop this code is given here:
    http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf
        
    Also see the comments at the top of the specctra.cpp file itself.
*/


#include "specctra.h"
#include "common.h"             // IsOK() & EDA_FileSelector()



using namespace DSN;

void WinEDA_PcbFrame::ImportSpecctraDesign( wxCommandEvent& event )
{
    if( !Clear_Pcb( true ) )
        return;
}


void WinEDA_PcbFrame::ImportSpecctraSession( wxCommandEvent& event )
{
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Board Modified: Continue ?" ) ) )
            return;
    }

    wxString sessionExt( wxT( ".ses" ) );
    wxString fileName = GetScreen()->m_FileName;
    wxString mask = wxT( "*" ) + sessionExt;

    ChangeFileNameExt( fileName, sessionExt );
    
    fileName = EDA_FileSelector( _( "Merge Specctra Session file:" ),
                          wxEmptyString,
                          fileName,
                          sessionExt,
                          mask,
                          this,
                          wxFD_OPEN,
                          FALSE );
        
    if( fileName == wxEmptyString )
        return;
    
    SPECCTRA_DB     db;

    try 
    {    
        db.LoadSESSION( fileName );
        db.FromSESSION( m_Pcb );
    }
    catch( IOError ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }
    
    m_SelTrackWidthBox_Changed = TRUE;
    m_SelViaSizeBox_Changed    = TRUE;

    GetScreen()->SetModify();
    m_Pcb->m_Status_Pcb = 0;
    
    Affiche_Message( wxString( _("Session file imported and merged OK.")) );
    
    DrawPanel->Refresh( TRUE );
}



namespace DSN {

// no UI code in this function, throw exception to report problems to the 
// UI handler: void WinEDA_PcbFrame::ImportSpecctraSession( wxCommandEvent& event )

void SPECCTRA_DB::FromSESSION( BOARD* aBoard ) throw( IOError )
{
    wxASSERT( session );

    // delete all the old tracks and vias
    aBoard->m_Track->DeleteStructList();
    aBoard->m_Track = NULL;
    aBoard->m_NbSegmTrack = 0;

	aBoard->DeleteMARKERs();

    
    if( !session->placement )
        ThrowIOError( _("Session file is missing the \"placement\" section") );

    if( !session->route )
        ThrowIOError( _("Session file is missing the \"routes\" section") );

    if( !session->route->library )
        ThrowIOError( _("Session file is missing the \"library_out\" section") );
    
    
    // Walk the PLACEMENT object's components list.
    COMPONENTS& components = session->placement->components;
    for( COMPONENTS::iterator i=components.begin();  i!=components.end();  ++i )
    {
        // reposition and re-orient each component and put on correct side of the board.
    }

    // Walk the NET_OUTs and create tracks and vias anew.    
    NET_OUTS& net_outs = session->route->net_outs;
    for( NET_OUTS::iterator i=net_outs.begin();  i!=net_outs.end();  ++i )
    {
        // create a track or via and position it.
    }
}


} // namespace DSN

