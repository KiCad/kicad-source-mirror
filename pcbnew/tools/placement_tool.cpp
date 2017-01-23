/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
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

#include "placement_tool.h"
#include "common_actions.h"
#include "selection_tool.h"
#include <tool/tool_manager.h>

#include <wxPcbStruct.h>
#include <class_board.h>
#include <ratsnest_data.h>
#include <board_commit.h>

#include <confirm.h>
#include <menus_helpers.h>

PLACEMENT_TOOL::PLACEMENT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Placement" ), m_selectionTool( NULL ), m_placementMenu( NULL )
{
}

PLACEMENT_TOOL::~PLACEMENT_TOOL()
{
    delete m_placementMenu;
}


bool PLACEMENT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    // Create a context menu and make it available through selection tool
    m_placementMenu = new CONTEXT_MENU;
    m_placementMenu->SetIcon( align_items_xpm );
    m_placementMenu->SetTitle( _( "Align/distribute" ) );

    // Add all align/distribute commands
    m_placementMenu->Add( COMMON_ACTIONS::alignTop );
    m_placementMenu->Add( COMMON_ACTIONS::alignBottom );
    m_placementMenu->Add( COMMON_ACTIONS::alignLeft );
    m_placementMenu->Add( COMMON_ACTIONS::alignRight );
    m_placementMenu->AppendSeparator();
    m_placementMenu->Add( COMMON_ACTIONS::distributeHorizontally );
    m_placementMenu->Add( COMMON_ACTIONS::distributeVertically );

    m_selectionTool->GetToolMenu().GetMenu().AddMenu( m_placementMenu, false,
            SELECTION_CONDITIONS::MoreThan( 1 ) );

    return true;
}


int PLACEMENT_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    // Compute the highest point of selection - it will be the edge of alignment
    int top = selection.Front()->GetBoundingBox().GetY();

    for( int i = 1; i < selection.Size(); ++i )
    {
        int currentTop = selection[i]->GetBoundingBox().GetY();

        if( top > currentTop )      // Y decreases when going up
            top = currentTop;
    }

    // Move the selected items
    for( auto item : selection )
    {
        int difference = top - item->GetBoundingBox().GetY();

        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to top" ) );

    return 0;
}


int PLACEMENT_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    // Compute the lowest point of selection - it will be the edge of alignment
    int bottom = selection.Front()->GetBoundingBox().GetBottom();

    for( int i = 1; i < selection.Size(); ++i )
    {
        int currentBottom = selection[i]->GetBoundingBox().GetBottom();

        if( bottom < currentBottom )      // Y increases when going down
            bottom = currentBottom;
    }

    // Move the selected items
    for( auto item : selection )
    {
        int difference = bottom - item->GetBoundingBox().GetBottom();

        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to bottom" ) );

    return 0;
}


int PLACEMENT_TOOL::AlignLeft( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    // Compute the leftmost point of selection - it will be the edge of alignment
    int left = selection.Front()->GetBoundingBox().GetX();

    for( int i = 1; i < selection.Size(); ++i )
    {
        int currentLeft = selection[i]->GetBoundingBox().GetX();

        if( left > currentLeft )      // X decreases when going left
            left = currentLeft;
    }

    // Move the selected items
    for( auto item : selection )
    {
        int difference = left - item->GetBoundingBox().GetX();

        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to left" ) );

    return 0;
}


int PLACEMENT_TOOL::AlignRight( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    // Compute the rightmost point of selection - it will be the edge of alignment
    int right = selection.Front()->GetBoundingBox().GetRight();

    for( int i = 1; i < selection.Size(); ++i )
    {
        int currentRight = selection[i]->GetBoundingBox().GetRight();

        if( right < currentRight )      // X increases when going right
            right = currentRight;
    }

    // Move the selected items
    for( auto item : selection )
    {
        int difference = right - item->GetBoundingBox().GetRight();

        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to right" ) );

    return 0;
}


static bool compareX( const BOARD_ITEM* aA, const BOARD_ITEM* aB )
{
    return aA->GetBoundingBox().Centre().x < aB->GetBoundingBox().Centre().x;
}


static bool compareY( const BOARD_ITEM* aA, const BOARD_ITEM* aB )
{
    return aA->GetBoundingBox().Centre().y < aB->GetBoundingBox().Centre().y;
}


int PLACEMENT_TOOL::DistributeHorizontally( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    // Prepare a list, so the items can be sorted by their X coordinate
    std::vector<BOARD_ITEM*> itemsList;

    for( auto item : selection )
        itemsList.push_back( item );

    // Sort items by X coordinate
    std::sort(itemsList.begin(), itemsList.end(), compareX );

    // Expected X coordinate for the next item (=minX)
    int position = itemsList.front()->GetBoundingBox().Centre().x;

    // X coordinate for the last item
    const int maxX = itemsList.back()->GetBoundingBox().Centre().x;

    // Distance between items
    const int distance = ( maxX - position ) / ( itemsList.size() - 1 );

    for( auto item : itemsList )
    {
        int difference = position - item->GetBoundingBox().Centre().x;

        item->Move( wxPoint( difference, 0 ) );

        position += distance;
    }

    commit.Push( _( "Distribute horizontally" ) );

    return 0;
}


int PLACEMENT_TOOL::DistributeVertically( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    // Prepare a list, so the items can be sorted by their Y coordinate
    std::vector<BOARD_ITEM*> itemsList;

    for( auto item : selection )
        itemsList.push_back( item );

    // Sort items by Y coordinate
    std::sort( itemsList.begin(), itemsList.end(), compareY );

    // Expected Y coordinate for the next item (=minY)
    int position = (*itemsList.begin())->GetBoundingBox().Centre().y;

    // Y coordinate for the last item
    const int maxY = (*itemsList.rbegin())->GetBoundingBox().Centre().y;

    // Distance between items
    const int distance = ( maxY - position ) / ( itemsList.size() - 1 );

    for( auto item : itemsList )
    {
        int difference = position - item->GetBoundingBox().Centre().y;

        item->Move( wxPoint( 0, difference ) );

        position += distance;
    }

    commit.Push( _( "Distribute vertically" ) );

    return 0;
}


void PLACEMENT_TOOL::SetTransitions()
{
    Go( &PLACEMENT_TOOL::AlignTop,    COMMON_ACTIONS::alignTop.MakeEvent() );
    Go( &PLACEMENT_TOOL::AlignBottom, COMMON_ACTIONS::alignBottom.MakeEvent() );
    Go( &PLACEMENT_TOOL::AlignLeft,   COMMON_ACTIONS::alignLeft.MakeEvent() );
    Go( &PLACEMENT_TOOL::AlignRight,  COMMON_ACTIONS::alignRight.MakeEvent() );

    Go( &PLACEMENT_TOOL::DistributeHorizontally,  COMMON_ACTIONS::distributeHorizontally.MakeEvent() );
    Go( &PLACEMENT_TOOL::DistributeVertically,    COMMON_ACTIONS::distributeVertically.MakeEvent() );
}
