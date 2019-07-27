/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Marco Mattila <marcom99@gmail.com>
 * Copyright (C) 2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 Kicad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <kicad_string.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <class_board.h>
#include <class_module.h>
#include <class_marker_pcb.h>
#include <class_text_mod.h>
#include <class_pcb_text.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <dialog_find.h>
#include <wx/fdrepdlg.h>


DIALOG_FIND::DIALOG_FIND( PCB_BASE_FRAME* aFrame ) :
        DIALOG_FIND_BASE( aFrame )
{
    m_frame = aFrame;
    m_foundItem = NULL;
    GetSizer()->SetSizeHints( this );

    m_SearchCombo->Append( m_frame->GetFindHistoryList() );

    if( m_SearchCombo->GetCount() )
    {
        m_SearchCombo->SetSelection( 0 );
        m_SearchCombo->SelectAll();
    }

    m_matchCase->SetValue( ( m_frame->GetFindReplaceData().GetFlags() & wxFR_MATCHCASE ) > 0 );
    m_matchWords->SetValue( ( m_frame->GetFindReplaceData().GetFlags() & wxFR_WHOLEWORD ) > 0 );
    m_wildcards->SetValue( ( m_frame->GetFindReplaceData().GetFlags() & FR_MATCH_WILDCARD ) > 0 );

    m_itemCount = m_markerCount = 0;

    SetInitialFocus( m_SearchCombo );

    Center();
}


void DIALOG_FIND::OnTextEnter( wxCommandEvent& aEvent )
{
    onButtonFindItemClick( aEvent );
}


void DIALOG_FIND::onButtonCloseClick( wxCommandEvent& aEvent )
{
    Close( true );
}


void DIALOG_FIND::onButtonFindItemClick( wxCommandEvent& aEvent )
{
    PCB_SCREEN* screen = m_frame->GetScreen();
    int         flags = 0;
    wxString    msg;
    wxString    searchString = m_SearchCombo->GetValue();
    int         index = m_SearchCombo->FindString( searchString, true );

    if( m_matchCase->GetValue() )
        flags |= wxFR_MATCHCASE;

    if( m_matchWords->GetValue() )
        flags |= wxFR_WHOLEWORD;

    if( m_wildcards->GetValue() )
        flags |= FR_MATCH_WILDCARD;

    if( index == wxNOT_FOUND )
    {
        m_SearchCombo->Insert( searchString, 0 );
    }
    else if( index != 0 )
    {
        /* Move the search string to the top of the list if it isn't already there. */
        m_SearchCombo->Delete( index );
        m_SearchCombo->Insert( searchString, 0 );
        m_SearchCombo->SetSelection( 0 );
    }

    wxString last;

    if( !m_frame->GetFindHistoryList().empty() )
        last = m_frame->GetFindHistoryList().back();

    if( !searchString.IsSameAs( last, false ) )
    {
        m_itemCount = 0;
        m_foundItem = NULL;
        m_frame->GetFindHistoryList().push_back( searchString );
    }

    m_frame->GetFindReplaceData().SetFindString( searchString );
    m_frame->GetFindReplaceData().SetFlags( flags );

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->GetCanvas()->GetViewStart( &screen->m_StartVisu.x, &screen->m_StartVisu.y );

    int count = 0;

    for( MODULE* module : m_frame->GetBoard()->Modules() )
    {
        if( module->Reference().Matches( m_frame->GetFindReplaceData(), nullptr )
            || module->Value().Matches( m_frame->GetFindReplaceData(), nullptr ) )
        {
            count++;

            if( count > m_itemCount )
            {
                m_foundItem = module;
                m_itemCount++;
                break;
            }
        }

        for( BOARD_ITEM* item : module->GraphicalItems() )
        {
            TEXTE_MODULE* textItem = dynamic_cast<TEXTE_MODULE*>( item );

            if( textItem && textItem->Matches( m_frame->GetFindReplaceData(), nullptr ) )
            {
                count++;

                if( count > m_itemCount )
                {
                    m_foundItem = module;
                    m_itemCount++;
                    break;
                }
            }
        }
    }

    for( BOARD_ITEM* item : m_frame->GetBoard()->Drawings() )
    {
        TEXTE_PCB* textItem = dynamic_cast<TEXTE_PCB*>( item );

        if( textItem && textItem->Matches( m_frame->GetFindReplaceData(), nullptr ) )
        {
            count++;

            if( count > m_itemCount )
            {
                m_foundItem = textItem;
                m_itemCount++;
                break;
            }
        }
    }

    if( m_foundItem )
    {
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, m_foundItem );
        m_frame->FocusOnLocation( m_foundItem->GetPosition(), true );
        msg.Printf( _( "\"%s\" found" ), GetChars( searchString ) );
        m_frame->SetStatusText( msg );
    }
    else
    {
        m_frame->SetStatusText( wxEmptyString );
        msg.Printf( _( "\"%s\" not found" ), GetChars( searchString ) );
        DisplayError( this, msg, 10 );
        m_itemCount = 0;
    }

    if( m_highlightCallback )
        m_highlightCallback( m_foundItem );
}


void DIALOG_FIND::onButtonFindMarkerClick( wxCommandEvent& aEvent )
{
    PCB_SCREEN* screen = m_frame->GetScreen();
    wxString    msg;

    m_foundItem = nullptr;

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->GetCanvas()->GetViewStart( &screen->m_StartVisu.x, &screen->m_StartVisu.y );

    MARKER_PCB* marker = m_frame->GetBoard()->GetMARKER( m_markerCount++ );

    if( marker )
        m_foundItem = marker;

    if( m_foundItem )
    {
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, m_foundItem );
        m_frame->FocusOnLocation( m_foundItem->GetPosition() );
        msg = _( "Marker found" );
        m_frame->SetStatusText( msg );
    }
    else
    {
        m_frame->SetStatusText( wxEmptyString );
        msg = _( "No marker found" );
        DisplayError( this, msg, 10 );
        m_markerCount = 0;
    }

    if( m_highlightCallback )
        m_highlightCallback( m_foundItem );
}


void DIALOG_FIND::onClose( wxCloseEvent& aEvent )
{
    int flags = 0;

    if( m_matchCase->GetValue() )
        flags |= wxFR_MATCHCASE;

    if( m_matchWords->GetValue() )
        flags |= wxFR_WHOLEWORD;

    if( m_wildcards->GetValue() )
        flags |= FR_MATCH_WILDCARD;

    m_frame->GetFindReplaceData().SetFlags( flags );

    EndModal( 1 );
}


