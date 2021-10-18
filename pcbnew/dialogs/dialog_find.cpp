/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Marco Mattila <marcom99@gmail.com>
 * Copyright (C) 2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <board.h>
#include <pcb_marker.h>
#include <footprint.h>
#include <pcb_text.h>
#include <fp_text.h>
#include <zone.h>
#include <dialog_find.h>
#include <string_utils.h>
#include <pcb_edit_frame.h>
#include <string>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <wx/fdrepdlg.h>


//Defined as global because these values have to survive the destructor

bool FindOptionCase = false;
bool FindOptionWords = false;
bool FindOptionWildcards = false;
bool FindOptionWrap = true;

bool FindIncludeTexts = true;
bool FindIncludeValues = true;
bool FindIncludeReferences = true;
bool FindIncludeMarkers = true;


DIALOG_FIND::DIALOG_FIND( PCB_BASE_FRAME *aFrame ) :
        DIALOG_FIND_BASE( aFrame, wxID_ANY, _( "Find" ) )
{
    m_frame = aFrame;
    GetSizer()->SetSizeHints( this );

    m_searchCombo->Append( m_frame->GetFindHistoryList() );

    while( m_searchCombo->GetCount() > 10 )
    {
        m_frame->GetFindHistoryList().pop_back();
        m_searchCombo->Delete( 9 );
    }

    if( m_searchCombo->GetCount() )
    {
        m_searchCombo->SetSelection( 0 );
        m_searchCombo->SelectAll();
    }

    m_matchCase->SetValue( FindOptionCase );
    m_matchWords->SetValue( FindOptionWords );
    m_wildcards->SetValue( FindOptionWildcards );
    m_wrap->SetValue( FindOptionWrap );

    m_includeTexts->SetValue( FindIncludeTexts );
    m_includeValues->SetValue( FindIncludeValues );
    m_includeReferences->SetValue( FindIncludeReferences );
    m_includeMarkers->SetValue( FindIncludeMarkers );

    m_status->SetLabel( wxEmptyString);
    m_upToDate = false;

    m_hitList.clear();
    m_it = m_hitList.begin();

    m_findNext->SetDefault();
    SetInitialFocus( m_searchCombo );

    Center();
}


void DIALOG_FIND::onTextEnter( wxCommandEvent& aEvent )
{
    search( true );
}


void DIALOG_FIND::onFindNextClick( wxCommandEvent& aEvent )
{
    search( true );
}


void DIALOG_FIND::onFindPreviousClick( wxCommandEvent& aEvent )
{
    search( false );
}


void DIALOG_FIND::onSearchAgainClick( wxCommandEvent& aEvent )
{
    m_upToDate = false;
    search( true );
}


void DIALOG_FIND::search( bool aDirection )
{
    PCB_SCREEN* screen = m_frame->GetScreen();
    int         flags;
    int         index;
    wxString    msg;
    wxString    searchString;
    bool        endIsReached = false;
    bool        isFirstSearch = false;

    // Add/move the search string to the top of the list if it isn't already there
    searchString = m_searchCombo->GetValue();
    index = m_searchCombo->FindString( searchString, true );

    if( index == wxNOT_FOUND )
    {
        m_searchCombo->Insert( searchString, 0 );
        m_searchCombo->SetSelection( 0 );
        m_upToDate = false;
        m_frame->GetFindHistoryList().Insert( searchString, 0 );

        if( m_searchCombo->GetCount() > 10 )
        {
            m_frame->GetFindHistoryList().pop_back();
            m_searchCombo->Delete( 10 );
        }
    }
    else if( index != 0 )
    {
        m_searchCombo->Delete( index );
        m_searchCombo->Insert( searchString, 0 );
        m_searchCombo->SetSelection( 0 );
        m_upToDate = false;

        if( m_frame->GetFindHistoryList().Index( searchString ) )
            m_frame->GetFindHistoryList().Remove( searchString );

        m_frame->GetFindHistoryList().Insert( searchString, 0 );
    }

    // Update search flags
    flags = 0;

    if( FindOptionCase != m_matchCase->GetValue() )
    {
        FindOptionCase = m_matchCase->GetValue();
        m_upToDate = false;
    }

    if( FindOptionWords != m_matchWords->GetValue() )
    {
        FindOptionWords = m_matchWords->GetValue();
        m_upToDate = false;
    }

    if( FindOptionWildcards != m_wildcards->GetValue() )
    {
        FindOptionWildcards = m_wildcards->GetValue();
        m_upToDate = false;
    }

    FindOptionWrap = m_wrap->GetValue();

    if( FindIncludeTexts != m_includeTexts->GetValue() )
    {
        FindIncludeTexts = m_includeTexts->GetValue();
        m_upToDate = false;
    }

    if( FindIncludeValues != m_includeValues->GetValue() )
    {
        FindIncludeValues = m_includeValues->GetValue();
        m_upToDate = false;
    }

    if( FindIncludeReferences != m_includeReferences->GetValue() )
    {
        FindIncludeReferences = m_includeReferences->GetValue();
        m_upToDate = false;
    }

    if( FindIncludeMarkers != m_includeMarkers->GetValue() )
    {
        FindIncludeMarkers = m_includeMarkers->GetValue();
        m_upToDate = false;
    }

    if( FindOptionCase )
        flags |= wxFR_MATCHCASE;

    if( FindOptionWords )
        flags |= wxFR_WHOLEWORD;

    if( FindOptionWildcards )
        flags |= FR_MATCH_WILDCARD;

    // Search parameters
    m_frame->GetFindReplaceData().SetFindString( searchString );
    m_frame->GetFindReplaceData().SetFlags( flags );

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->GetCanvas()->GetViewStart( &screen->m_StartVisu.x, &screen->m_StartVisu.y );

    // Refresh the list of results
    if( !m_upToDate )
    {
        m_status->SetLabel( _( "Searching..." ) );
        m_hitList.clear();

        if( FindIncludeTexts || FindIncludeValues || FindIncludeReferences )
        {
            for( FOOTPRINT* fp : m_frame->GetBoard()->Footprints() )
            {
                if( ( fp->Reference().Matches( m_frame->GetFindReplaceData(), nullptr )
                      && FindIncludeReferences )
                        || ( fp->Value().Matches( m_frame->GetFindReplaceData(), nullptr )
                             && FindIncludeValues ) )
                {
                    m_hitList.push_back( fp );
                }

                if( m_includeTexts->GetValue() )
                {
                    for( BOARD_ITEM* item : fp->GraphicalItems() )
                    {
                        FP_TEXT* textItem = dynamic_cast<FP_TEXT*>( item );

                        if( textItem && textItem->Matches( m_frame->GetFindReplaceData(),
                                                           nullptr ) )
                        {
                            m_hitList.push_back( fp );
                        }
                    }
                }
            }

            if( FindIncludeTexts )
            {
                for( BOARD_ITEM* item : m_frame->GetBoard()->Drawings() )
                {
                    PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item );

                    if( textItem && textItem->Matches( m_frame->GetFindReplaceData(), nullptr ) )
                    {
                        m_hitList.push_back( textItem );
                    }
                }

                for( BOARD_ITEM* item : m_frame->GetBoard()->Zones() )
                {
                    ZONE* zoneItem = dynamic_cast<ZONE*>( item );

                    if( zoneItem && zoneItem->Matches( m_frame->GetFindReplaceData(), nullptr ) )
                    {
                        m_hitList.push_back( zoneItem );
                    }
                }
            }
        }

        if( FindIncludeMarkers )
        {
            for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
            {
                if( marker->Matches( m_frame->GetFindReplaceData(), nullptr ) )
                    m_hitList.push_back( marker );
            }
        }

        m_upToDate = true;
        isFirstSearch = true;

        if( aDirection )
            m_it = m_hitList.begin();
        else
            m_it = m_hitList.end();
    }

    // Do we want a sorting algorithm ? If so, implement it here.

    // Get the item to display
    if( m_hitList.empty() )
    {
        m_frame->SetStatusText( wxEmptyString );
    }
    else
    {
        if( aDirection )
        {
            if( m_it != m_hitList.end() && !isFirstSearch )
                m_it++;

            if( m_it == m_hitList.end() )
            {
                if( m_wrap->GetValue() )
                {
                    m_it = m_hitList.begin();
                }
                else
                {
                    endIsReached = true;
                    m_it--; // point to the last REAL result
                }
            }
        }
        else
        {
            if( m_it == m_hitList.begin() )
            {
                if( m_wrap->GetValue() )
                    m_it = m_hitList.end();
                else
                    endIsReached = true;
            }

            if( m_it != m_hitList.begin() )
                m_it--;
        }
    }

    // Display the item
    if( m_hitList.empty() )
    {
        m_frame->SetStatusText( wxEmptyString );
        msg.Printf( _( "'%s' not found" ), searchString );
        m_frame->ShowInfoBarMsg( msg );

        m_status->SetLabel( msg );
    }
    else if( endIsReached )
    {
        m_frame->SetStatusText( wxEmptyString );
        m_frame->ShowInfoBarMsg( _( "No more items to show" ) );

        m_status->SetLabel( _( "No hits" ) );
    }
    else
    {
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, *m_it );
        m_frame->FocusOnLocation( ( *m_it )->GetPosition() );

        msg.Printf( _( "'%s' found" ), searchString );
        m_frame->SetStatusText( msg );

        msg.Printf( _( "Hit(s): %ld / %lu" ),
                    std::distance( m_hitList.begin(), m_it ) + 1,
                    m_hitList.size() );
        m_status->SetLabel( msg );
    }

    if( m_highlightCallback )
        m_highlightCallback( GetItem() );
}


void DIALOG_FIND::OnCloseButtonClick( wxCommandEvent& aEvent )
{
    wxCloseEvent tmp;

    OnClose( tmp );

    aEvent.Skip();
}


void DIALOG_FIND::OnClose( wxCloseEvent& aEvent )
{
    FindOptionCase = m_matchCase->GetValue();
    FindOptionWords = m_matchWords->GetValue();
    FindOptionWildcards = m_wildcards->GetValue();
    FindOptionWrap = m_wrap->GetValue();

    FindIncludeTexts = m_includeTexts->GetValue();
    FindIncludeValues = m_includeValues->GetValue();
    FindIncludeMarkers = m_includeMarkers->GetValue();
    FindIncludeReferences = m_includeReferences->GetValue();

    aEvent.Skip();
}
