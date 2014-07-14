/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcb_editor_control.h"
#include "common_actions.h"

#include "selection_tool.h"

#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_zone.h>
#include <class_draw_panel_gal.h>


class ZONE_CONTEXT_MENU : public CONTEXT_MENU
{
public:
    ZONE_CONTEXT_MENU()
    {
        Add( COMMON_ACTIONS::zoneFill );
        Add( COMMON_ACTIONS::zoneFillAll );
        Add( COMMON_ACTIONS::zoneUnfill );
    }
};


PCB_EDITOR_CONTROL::PCB_EDITOR_CONTROL() :
    TOOL_INTERACTIVE( "pcbnew.EditorControl" )
{
}


void PCB_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


bool PCB_EDITOR_CONTROL::Init()
{
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( selTool )
    {
        selTool->AddSubMenu( new ZONE_CONTEXT_MENU, wxT( "Zones" ),
                             SELECTION_CONDITIONS::OnlyType( PCB_ZONE_AREA_T ) );
    }

    setTransitions();

    return true;
}


// Track & via size control
int PCB_EDITOR_CONTROL::TrackWidthInc( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int widthIndex = board->GetDesignSettings().GetTrackWidthIndex() + 1;

    if( widthIndex >= (int) board->GetDesignSettings().m_TrackWidthList.size() )
        widthIndex = board->GetDesignSettings().m_TrackWidthList.size() - 1;

    board->GetDesignSettings().SetTrackWidthIndex( widthIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    wxUpdateUIEvent dummy;
    m_frame->OnUpdateSelectTrackWidth( dummy );
    setTransitions();

    m_toolMgr->RunAction( COMMON_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::TrackWidthDec( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int widthIndex = board->GetDesignSettings().GetTrackWidthIndex() - 1;

    if( widthIndex < 0 )
        widthIndex = 0;

    board->GetDesignSettings().SetTrackWidthIndex( widthIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    wxUpdateUIEvent dummy;
    m_frame->OnUpdateSelectTrackWidth( dummy );
    setTransitions();

    m_toolMgr->RunAction( COMMON_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::ViaSizeInc( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int sizeIndex = board->GetDesignSettings().GetViaSizeIndex() + 1;

    if( sizeIndex >= (int) board->GetDesignSettings().m_ViasDimensionsList.size() )
        sizeIndex = board->GetDesignSettings().m_ViasDimensionsList.size() - 1;

    board->GetDesignSettings().SetViaSizeIndex( sizeIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    wxUpdateUIEvent dummy;
    m_frame->OnUpdateSelectViaSize( dummy );
    setTransitions();

    m_toolMgr->RunAction( COMMON_ACTIONS::trackViaSizeChanged );

    return 0;
}


int PCB_EDITOR_CONTROL::ViaSizeDec( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();
    int sizeIndex = board->GetDesignSettings().GetViaSizeIndex() - 1;

    if( sizeIndex < 0 )
        sizeIndex = 0;

    board->GetDesignSettings().SetViaSizeIndex( sizeIndex );
    board->GetDesignSettings().UseCustomTrackViaSize( false );

    wxUpdateUIEvent dummy;
    m_frame->OnUpdateSelectViaSize( dummy );
    setTransitions();

    m_toolMgr->RunAction( COMMON_ACTIONS::trackViaSizeChanged );

    return 0;
}


// Zone actions
int PCB_EDITOR_CONTROL::ZoneFill( TOOL_EVENT& aEvent )
{
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    for( int i = 0; i < selection.Size(); ++i )
    {
        assert( selection.Item<BOARD_ITEM>( i )->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = selection.Item<ZONE_CONTAINER>( i );
        m_frame->Fill_Zone( zone );
        zone->SetIsFilled( true );
        zone->ViewUpdate();
    }

    setTransitions();

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneFillAll( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>();

    for( int i = 0; i < board->GetAreaCount(); ++i )
    {
        ZONE_CONTAINER* zone = board->GetArea( i );
        m_frame->Fill_Zone( zone );
        zone->SetIsFilled( true );
        zone->ViewUpdate();
    }

    setTransitions();

    return 0;
}


int PCB_EDITOR_CONTROL::ZoneUnfill( TOOL_EVENT& aEvent )
{
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    for( int i = 0; i < selection.Size(); ++i )
    {
        assert( selection.Item<BOARD_ITEM>( i )->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = selection.Item<ZONE_CONTAINER>( i );
        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
        zone->ViewUpdate();
    }

    setTransitions();

    return 0;
}


void PCB_EDITOR_CONTROL::setTransitions()
{
    // Track & via size control
    Go( &PCB_EDITOR_CONTROL::TrackWidthInc,      COMMON_ACTIONS::trackWidthInc.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::TrackWidthDec,      COMMON_ACTIONS::trackWidthDec.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ViaSizeInc,         COMMON_ACTIONS::viaSizeInc.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ViaSizeDec,         COMMON_ACTIONS::viaSizeDec.MakeEvent() );

    // Zone actions
    Go( &PCB_EDITOR_CONTROL::ZoneFill,           COMMON_ACTIONS::zoneFill.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneFillAll,        COMMON_ACTIONS::zoneFillAll.MakeEvent() );
    Go( &PCB_EDITOR_CONTROL::ZoneUnfill,         COMMON_ACTIONS::zoneUnfill.MakeEvent() );
}
