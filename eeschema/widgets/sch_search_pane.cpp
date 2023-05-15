/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include "sch_search_pane.h"
#include "search_handlers.h"


SCH_SEARCH_PANE::SCH_SEARCH_PANE( SCH_EDIT_FRAME* aFrame ) :
        SEARCH_PANE( aFrame ), m_schFrame( aFrame )
{
    m_sch = &(m_schFrame->Schematic());

    if( m_sch != nullptr )
        m_sch->AddListener( this );

    m_schFrame->Connect( EDA_EVT_UNITS_CHANGED,
                         wxCommandEventHandler( SCH_SEARCH_PANE::onUnitsChanged ), nullptr, this );

    m_schFrame->Connect( EDA_EVT_SCHEMATIC_CHANGED,
                         wxCommandEventHandler( SCH_SEARCH_PANE::onSchChanged ), nullptr, this );

    wxFont infoFont = KIUI::GetDockedPaneFont( this );
    SetFont( infoFont );
    m_notebook->SetFont( infoFont );

    AddSearcher( new SYMBOL_SEARCH_HANDLER( aFrame ) );
    AddSearcher( new TEXT_SEARCH_HANDLER( aFrame ) );
    AddSearcher( new LABEL_SEARCH_HANDLER( aFrame ) );
}


SCH_SEARCH_PANE::~SCH_SEARCH_PANE()
{
    m_schFrame->Disconnect( EDA_EVT_UNITS_CHANGED,
                            wxCommandEventHandler( SCH_SEARCH_PANE::onUnitsChanged ), nullptr,
                            this );
    m_schFrame->Disconnect( EDA_EVT_SCHEMATIC_CHANGED,
                            wxCommandEventHandler( SCH_SEARCH_PANE::onSchChanged ), nullptr,
                            this );
}


void SCH_SEARCH_PANE::onUnitsChanged( wxCommandEvent& event )
{
    ClearAllResults();
    RefreshSearch();

    event.Skip();
}


void SCH_SEARCH_PANE::onSchChanged( wxCommandEvent& event )
{
    ClearAllResults();
    RefreshSearch();

    event.Skip();
}


void SCH_SEARCH_PANE::OnSchItemsAdded( SCHEMATIC& aBoard, std::vector<SCH_ITEM*>& aBoardItems )
{
    if( !IsShown() )
        return;

    RefreshSearch();
}


void SCH_SEARCH_PANE::OnSchItemsRemoved( SCHEMATIC& aBoard, std::vector<SCH_ITEM*>& aBoardItems )
{
    if( !IsShown() )
        return;

    RefreshSearch();
}


void SCH_SEARCH_PANE::OnSchItemsChanged( SCHEMATIC& aBoard, std::vector<SCH_ITEM*>& aBoardItems )
{
    if( !IsShown() )
        return;

    RefreshSearch();
}