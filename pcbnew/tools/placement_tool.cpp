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
#include "tool/selection.h"
#include "placement_tool.h"
#include "pcb_actions.h"
#include "selection_tool.h"
#include <tool/tool_manager.h>

#include <pcb_edit_frame.h>
#include <class_board.h>
#include <ratsnest_data.h>
#include <board_commit.h>
#include <bitmaps.h>

#include <confirm.h>
#include <menus_helpers.h>

// Placement tool
TOOL_ACTION PCB_ACTIONS::alignTop( "pcbnew.AlignAndDistribute.alignTop",
        AS_GLOBAL, 0,
        _( "Align to Top" ),
        _( "Aligns selected items to the top edge" ), align_items_top_xpm );

TOOL_ACTION PCB_ACTIONS::alignBottom( "pcbnew.AlignAndDistribute.alignBottom",
        AS_GLOBAL, 0,
        _( "Align to Bottom" ),
        _( "Aligns selected items to the bottom edge" ), align_items_bottom_xpm );

TOOL_ACTION PCB_ACTIONS::alignLeft( "pcbnew.AlignAndDistribute.alignLeft",
        AS_GLOBAL, 0,
        _( "Align to Left" ),
        _( "Aligns selected items to the left edge" ), align_items_left_xpm );

TOOL_ACTION PCB_ACTIONS::alignRight( "pcbnew.AlignAndDistribute.alignRight",
        AS_GLOBAL, 0,
        _( "Align to Right" ),
        _( "Aligns selected items to the right edge" ), align_items_right_xpm );

TOOL_ACTION PCB_ACTIONS::alignCenterX( "pcbnew.AlignAndDistribute.alignCenterX",
        AS_GLOBAL, 0,
        _( "Align to Middle" ),
        _( "Aligns selected items to the vertical center" ), align_items_middle_xpm );

TOOL_ACTION PCB_ACTIONS::alignCenterY( "pcbnew.AlignAndDistribute.alignCenterY",
        AS_GLOBAL, 0,
        _( "Align to Center" ),
        _( "Aligns selected items to the horizontal center" ), align_items_center_xpm );

TOOL_ACTION PCB_ACTIONS::distributeHorizontally( "pcbnew.AlignAndDistribute.distributeHorizontally",
        AS_GLOBAL, 0,
        _( "Distribute Horizontally" ),
        _( "Distributes selected items along the horizontal axis" ), distribute_horizontal_xpm );

TOOL_ACTION PCB_ACTIONS::distributeVertically( "pcbnew.AlignAndDistribute.distributeVertically",
        AS_GLOBAL, 0,
        _( "Distribute Vertically" ),
        _( "Distributes selected items along the vertical axis" ), distribute_vertical_xpm );


ALIGN_DISTRIBUTE_TOOL::ALIGN_DISTRIBUTE_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Placement" ), m_selectionTool( NULL ), m_placementMenu( NULL )
{
}

ALIGN_DISTRIBUTE_TOOL::~ALIGN_DISTRIBUTE_TOOL()
{
    delete m_placementMenu;
}


bool ALIGN_DISTRIBUTE_TOOL::Init()
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
    m_placementMenu->SetTitle( _( "Align/Distribute" ) );

    // Add all align/distribute commands
    m_placementMenu->Add( PCB_ACTIONS::alignTop );
    m_placementMenu->Add( PCB_ACTIONS::alignBottom );
    m_placementMenu->Add( PCB_ACTIONS::alignLeft );
    m_placementMenu->Add( PCB_ACTIONS::alignRight );
    m_placementMenu->Add( PCB_ACTIONS::alignCenterX );
    m_placementMenu->Add( PCB_ACTIONS::alignCenterY );
    m_placementMenu->AppendSeparator();
    m_placementMenu->Add( PCB_ACTIONS::distributeHorizontally );
    m_placementMenu->Add( PCB_ACTIONS::distributeVertically );

    m_selectionTool->GetToolMenu().GetMenu().AddMenu( m_placementMenu, false,
            SELECTION_CONDITIONS::MoreThan( 1 ) );

    return true;
}


bool SortLeftmostX( const std::pair<BOARD_ITEM*, EDA_RECT> left, const std::pair<BOARD_ITEM*, EDA_RECT> right)
{
    return ( left.second.GetX() < right.second.GetX() );
}


bool SortRightmostX( const std::pair<BOARD_ITEM*, EDA_RECT> left, const std::pair<BOARD_ITEM*, EDA_RECT> right)
{
    return ( left.second.GetRight() > right.second.GetRight() );
}


bool SortTopmostY( const std::pair<BOARD_ITEM*, EDA_RECT> left, const std::pair<BOARD_ITEM*, EDA_RECT> right)
{
    return ( left.second.GetY() < right.second.GetY() );
}


bool SortCenterX( const std::pair<BOARD_ITEM*, EDA_RECT> left, const std::pair<BOARD_ITEM*, EDA_RECT> right)
{
    return ( left.second.GetCenter().x < right.second.GetCenter().x );
}


bool SortCenterY( const std::pair<BOARD_ITEM*, EDA_RECT> left, const std::pair<BOARD_ITEM*, EDA_RECT> right)
{
    return ( left.second.GetCenter().y < right.second.GetCenter().y );
}


bool SortBottommostY( const std::pair<BOARD_ITEM*, EDA_RECT> left, const std::pair<BOARD_ITEM*, EDA_RECT> right)
{
    return ( left.second.GetBottom() > right.second.GetBottom() );
}


ALIGNMENT_RECTS GetBoundingBoxes( const SELECTION &sel )
{
    const SELECTION& selection = sel;

    ALIGNMENT_RECTS rects;

    for( auto item : selection )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( item->Type() == PCB_MODULE_T )
        {
            rects.emplace_back( std::make_pair( boardItem, static_cast<MODULE*>( item )->GetFootprintRect() ) );
        }
        else
        {
            rects.emplace_back( std::make_pair( boardItem, item->GetBoundingBox() ) );
        }
    }
    return rects;
}


int ALIGN_DISTRIBUTE_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortTopmostY );

    // after sorting, the fist item acts as the target for all others
    const int targetTop = itemsToAlign.begin()->second.GetY();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetTop - i.second.GetY();
        i.first->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to top" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortBottommostY );

    // after sorting, the fist item acts as the target for all others
   const int targetBottom = itemsToAlign.begin()->second.GetBottom();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetBottom - i.second.GetBottom();
        i.first->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to bottom" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignLeft( const TOOL_EVENT& aEvent )
{
    // Because this tool uses bounding boxes and they aren't mirrored even when
    // the view is mirrored, we need to call the other one if mirrored.
    if( getView()->IsMirroredX() )
    {
        return doAlignRight();
    }
    else
    {
        return doAlignLeft();
    }
}


int ALIGN_DISTRIBUTE_TOOL::doAlignLeft()
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortLeftmostX );

    // after sorting, the fist item acts as the target for all others
    const int targetLeft = itemsToAlign.begin()->second.GetX();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetLeft - i.second.GetX();
        i.first->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to left" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignRight( const TOOL_EVENT& aEvent )
{
    // Because this tool uses bounding boxes and they aren't mirrored even when
    // the view is mirrored, we need to call the other one if mirrored.
    if( getView()->IsMirroredX() )
    {
        return doAlignLeft();
    }
    else
    {
        return doAlignRight();
    }
}


int ALIGN_DISTRIBUTE_TOOL::doAlignRight()
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortRightmostX );

    // after sorting, the fist item acts as the target for all others
    const int targetRight = itemsToAlign.begin()->second.GetRight();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetRight - i.second.GetRight();
        i.first->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to right" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterX( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortCenterX );

    // after sorting use the x coordinate of the middle item as a target for all other items
    const int targetX = itemsToAlign.at( itemsToAlign.size() / 2 ).second.GetCenter().x;

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetX - i.second.GetCenter().x;
        i.first->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to middle" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterY( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortCenterY );

    // after sorting use the y coordinate of the middle item as a target for all other items
    const int targetY = itemsToAlign.at( itemsToAlign.size() / 2 ).second.GetCenter().y;

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetY - i.second.GetCenter().y;
        i.first->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to center" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortRightmostX );
    const auto maxRight = itemsToDistribute.begin()->second.GetRight();
    const auto lastItem = itemsToDistribute.begin()->first;

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortLeftmostX );

    auto totalGap = maxRight - itemsToDistribute.begin()->second.GetX();

    for( auto& i : itemsToDistribute )
    {
        totalGap -= i.second.GetWidth();
    }

    const auto itemGap = totalGap / ( itemsToDistribute.size() - 1 );

    auto targetX = itemsToDistribute.begin()->second.GetX();

    for( auto& i : itemsToDistribute )
    {
        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == i.first )
            continue;

        int difference = targetX - i.second.GetX();
        i.first->Move( wxPoint( difference, 0 ) );
        targetX += ( i.second.GetWidth() + itemGap );
    }

    commit.Push( _( "Distribute horizontally" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::DistributeVertically( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( getEditFrame<PCB_BASE_FRAME>() );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortBottommostY );
    const auto maxBottom = itemsToDistribute.begin()->second.GetBottom();
    const auto lastItem = itemsToDistribute.begin()->first;

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortTopmostY );

    // determine the distance between the bottommost and topmost Y coordinates
    auto totalGap = maxBottom - itemsToDistribute.begin()->second.GetY();

    for( auto& i : itemsToDistribute )
    {
        totalGap -= i.second.GetHeight();
    }

    const auto itemGap = totalGap / ( itemsToDistribute.size() - 1 );

    auto targetY = itemsToDistribute.begin()->second.GetY();

    for( auto& i : itemsToDistribute )
    {
        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == i.first )
            continue;

        int difference = targetY - i.second.GetY();
        i.first->Move( wxPoint( 0, difference ) );
        targetY += ( i.second.GetHeight() + itemGap );
    }

    commit.Push( _( "Distribute vertically" ) );

    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::setTransitions()
{
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignTop,    PCB_ACTIONS::alignTop.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignBottom, PCB_ACTIONS::alignBottom.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignLeft,   PCB_ACTIONS::alignLeft.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignRight,  PCB_ACTIONS::alignRight.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignCenterX,  PCB_ACTIONS::alignCenterX.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignCenterY,  PCB_ACTIONS::alignCenterY.MakeEvent() );

    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally,  PCB_ACTIONS::distributeHorizontally.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeVertically,    PCB_ACTIONS::distributeVertically.MakeEvent() );
}
