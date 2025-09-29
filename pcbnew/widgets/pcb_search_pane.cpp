/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pcb_edit_frame.h>
#include "pcb_search_pane.h"
#include "search_handlers.h"


PCB_SEARCH_PANE::PCB_SEARCH_PANE( PCB_EDIT_FRAME* aFrame ) :
        SEARCH_PANE( aFrame ),
        m_pcbFrame( aFrame )
{
    m_brd = m_pcbFrame->GetBoard();

    if( m_brd != nullptr )
        m_brd->AddListener( this );

    m_pcbFrame->Bind( EDA_EVT_UNITS_CHANGED, &PCB_SEARCH_PANE::onUnitsChanged, this );
    m_pcbFrame->Bind( EDA_EVT_BOARD_CHANGING, &PCB_SEARCH_PANE::onBoardChanging, this );
    m_pcbFrame->Bind( EDA_EVT_BOARD_CHANGED, &PCB_SEARCH_PANE::onBoardChanged, this );

    wxFont infoFont = KIUI::GetDockedPaneFont( this );
    SetFont( infoFont );
    m_notebook->SetFont( infoFont );

    AddSearcher( std::make_shared<FOOTPRINT_SEARCH_HANDLER>( aFrame ) );
    AddSearcher( std::make_shared<ZONE_SEARCH_HANDLER>( aFrame ) );
    AddSearcher( std::make_shared<NETS_SEARCH_HANDLER>( aFrame ) );
    AddSearcher( std::make_shared<RATSNEST_SEARCH_HANDLER>( aFrame ) );
    AddSearcher( std::make_shared<TEXT_SEARCH_HANDLER>( aFrame ) );
    AddSearcher( std::make_shared<GROUP_SEARCH_HANDLER>( aFrame ) );
    AddSearcher( std::make_shared<DRILL_SEARCH_HANDLER>( aFrame ) );
}


PCB_SEARCH_PANE::~PCB_SEARCH_PANE()
{
    m_pcbFrame->Unbind( EDA_EVT_UNITS_CHANGED, &PCB_SEARCH_PANE::onUnitsChanged, this );
    m_pcbFrame->Unbind( EDA_EVT_BOARD_CHANGING, &PCB_SEARCH_PANE::onBoardChanging, this );
    m_pcbFrame->Unbind( EDA_EVT_BOARD_CHANGED, &PCB_SEARCH_PANE::onBoardChanged, this );
}


void PCB_SEARCH_PANE::onUnitsChanged( wxCommandEvent& event )
{
    ClearAllResults();
    RefreshSearch();
    event.Skip();
}


void PCB_SEARCH_PANE::onBoardChanging( wxCommandEvent& event )
{
    ClearAllResults();
    event.Skip();
}


void PCB_SEARCH_PANE::onBoardChanged( wxCommandEvent& event )
{
    m_brd = m_pcbFrame->GetBoard();

    if( m_brd != nullptr )
        m_brd->AddListener( this );

    ClearAllResults();
    RefreshSearch();
    event.Skip();
}


void PCB_SEARCH_PANE::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}


void PCB_SEARCH_PANE::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}


void PCB_SEARCH_PANE::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}


void PCB_SEARCH_PANE::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}


void PCB_SEARCH_PANE::OnBoardNetSettingsChanged( BOARD& aBoard )
{
}


void PCB_SEARCH_PANE::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}


void PCB_SEARCH_PANE::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}


void PCB_SEARCH_PANE::OnBoardHighlightNetChanged( BOARD& aBoard )
{
}


void PCB_SEARCH_PANE::OnBoardRatsnestChanged( BOARD& aBoard )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}

void PCB_SEARCH_PANE::OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                              std::vector<BOARD_ITEM*>& aRemovedItems,
                                              std::vector<BOARD_ITEM*>& aChangedItems )
{
    if( !IsShownOnScreen() )
        return;

    RefreshSearch();
}
