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


void ALIGN_DISTRIBUTE_TOOL::filterPadsWithModules( SELECTION &selection )
{
    std::set<BOARD_ITEM*> rejected;
    for( auto i : selection )
    {
        auto item = static_cast<BOARD_ITEM*>( i );
        if( item->Type() == PCB_PAD_T )
        {
            MODULE* mod = static_cast<MODULE*>( item->GetParent() );

            // selection contains both the module and its pads - remove the pads
            if( mod && selection.Contains( mod ) )
                rejected.insert( item );
        }
    }

    for( BOARD_ITEM* item : rejected )
        selection.Remove( item );
}


int ALIGN_DISTRIBUTE_TOOL::checkLockedStatus( const SELECTION &selection ) const
{
    SELECTION moving_items( selection );

    // Remove the anchor from the list
    moving_items.Remove( moving_items.Front() );

    bool containsLocked = false;

    // Check if the selection contains locked items
    for( const auto& item : moving_items )
    {
        switch ( item->Type() )
        {
        case PCB_MODULE_T:
            if( static_cast< MODULE* >( item )->IsLocked() )
                containsLocked = true;
            break;

        case PCB_PAD_T:
        case PCB_MODULE_EDGE_T:
        case PCB_MODULE_TEXT_T:
            if( static_cast< MODULE* >( item->GetParent() )->IsLocked() )
                containsLocked = true;
            break;

        default:    // suppress warnings
            break;
        }
    }

    if( containsLocked )
    {
        if( IsOK( getEditFrame< PCB_EDIT_FRAME >(),
                _( "Selection contains locked items. Do you want to continue?" ) ) )
        {
            return SELECTION_LOCK_OVERRIDE;
        }
        else
            return SELECTION_LOCKED;
    }

    return SELECTION_UNLOCKED;
}


int ALIGN_DISTRIBUTE_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection( SELECTION_EDITABLE );

    if( selection.Size() <= 1 )
        return 0;

    filterPadsWithModules( selection );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortTopmostY );
    if( checkLockedStatus( selection ) == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    // after sorting, the fist item acts as the target for all others
    const int targetTop = itemsToAlign.begin()->second.GetY();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetTop - i.second.GetY();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame->IsType( FRAME_PCB ) )
            item = item->GetParent();

        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to top" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection( SELECTION_EDITABLE );

    if( selection.Size() <= 1 )
        return 0;

    filterPadsWithModules( selection );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortBottommostY );
    if( checkLockedStatus( selection ) == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    // after sorting, the fist item acts as the target for all others
   const int targetBottom = itemsToAlign.begin()->second.GetBottom();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetBottom - i.second.GetBottom();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame->IsType( FRAME_PCB ) )
            item = item->GetParent();

        item->Move( wxPoint( 0, difference ) );
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
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection( SELECTION_EDITABLE );

    if( selection.Size() <= 1 )
        return 0;

    filterPadsWithModules( selection );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortLeftmostX );
    if( checkLockedStatus( selection ) == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    // after sorting, the fist item acts as the target for all others
    const int targetLeft = itemsToAlign.begin()->second.GetX();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetLeft - i.second.GetX();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame->IsType( FRAME_PCB ) )
            item = item->GetParent();

        item->Move( wxPoint( difference, 0 ) );
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
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection( SELECTION_EDITABLE );

    if( selection.Size() <= 1 )
        return 0;

    filterPadsWithModules( selection );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortRightmostX );
    if( checkLockedStatus( selection ) == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    // after sorting, the fist item acts as the target for all others
    const int targetRight = itemsToAlign.begin()->second.GetRight();

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetRight - i.second.GetRight();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame->IsType( FRAME_PCB ) )
            item = item->GetParent();

        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to right" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterX( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection( SELECTION_EDITABLE );

    if( selection.Size() <= 1 )
        return 0;

    filterPadsWithModules( selection );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortCenterX );
    if( checkLockedStatus( selection ) == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    // after sorting use the center x coordinate of the leftmost item as a target
    // for all other items
    const int targetX = itemsToAlign.begin()->second.GetCenter().x;

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetX - i.second.GetCenter().x;
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame->IsType( FRAME_PCB ) )
            item = item->GetParent();

        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to middle" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterY( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection( SELECTION_EDITABLE );

    if( selection.Size() <= 1 )
        return 0;

    filterPadsWithModules( selection );

    auto itemsToAlign = GetBoundingBoxes( selection );
    std::sort( itemsToAlign.begin(), itemsToAlign.end(), SortCenterY );
    if( checkLockedStatus( selection ) == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    // after sorting use the center y coordinate of the top-most item as a target
    // for all other items
    const int targetY = itemsToAlign.begin()->second.GetCenter().y;

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetY - i.second.GetCenter().y;
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame->IsType( FRAME_PCB ) )
            item = item->GetParent();

        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to center" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection(
            SELECTION_EDITABLE | SELECTION_SANITIZE_PADS );

    if( selection.Size() <= 1 )
        return 0;

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortRightmostX );
    const auto lastItem = itemsToDistribute.begin()->first;

    const auto maxRight = itemsToDistribute.begin()->second.GetRight();

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortLeftmostX );
    const auto minX = itemsToDistribute.begin()->second.GetX();
    auto totalGap = maxRight - minX;
    int totalWidth = 0;

    for( auto& i : itemsToDistribute )
    {
        totalWidth += i.second.GetWidth();
    }

    if( totalGap < totalWidth )
    {
        // the width of the items exceeds the gap (overlapping items) -> use center point spacing
        doDistributeCentersHorizontally( itemsToDistribute );
    }
    else
    {
        totalGap -= totalWidth;
        doDistributeGapsHorizontally( itemsToDistribute, lastItem, totalGap );
    }

    commit.Push( _( "Distribute horizontally" ) );

    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGapsHorizontally( ALIGNMENT_RECTS& itemsToDistribute,
                                                          const BOARD_ITEM* lastItem, int totalGap ) const
{
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
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersHorizontally( ALIGNMENT_RECTS &itemsToDistribute ) const
{
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortCenterX );
    const auto totalGap = ( itemsToDistribute.end()-1 )->second.GetCenter().x
                          - itemsToDistribute.begin()->second.GetCenter().x;
    const auto itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    auto targetX = itemsToDistribute.begin()->second.GetCenter().x;

    for( auto& i : itemsToDistribute )
        {
            int difference = targetX - i.second.GetCenter().x;
            i.first->Move( wxPoint( difference, 0 ) );
            targetX += ( itemGap );
        }
}


int ALIGN_DISTRIBUTE_TOOL::DistributeVertically( const TOOL_EVENT& aEvent )
{
    auto frame = getEditFrame<PCB_BASE_FRAME>();
    SELECTION& selection = m_selectionTool->RequestSelection(
            SELECTION_EDITABLE | SELECTION_SANITIZE_PADS );

    if( selection.Size() <= 1 )
        return 0;

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    BOARD_COMMIT commit( frame );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortBottommostY );
    const auto maxBottom = itemsToDistribute.begin()->second.GetBottom();
    const auto lastItem = itemsToDistribute.begin()->first;

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortTopmostY );
    auto minY = itemsToDistribute.begin()->second.GetY();

    auto totalGap = maxBottom - minY;
    int totalHeight = 0;

    for( auto& i : itemsToDistribute )
    {
        totalHeight += i.second.GetHeight();
    }

    if( totalGap < totalHeight )
    {
        // the width of the items exceeds the gap (overlapping items) -> use center point spacing
        doDistributeCentersVertically( itemsToDistribute );
    }
    else
    {
        totalGap -= totalHeight;
        doDistributeGapsVertically( itemsToDistribute, lastItem, totalGap );
    }

    commit.Push( _( "Distribute vertically" ) );

    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGapsVertically( ALIGNMENT_RECTS& itemsToDistribute,
                                                        const BOARD_ITEM* lastItem, int totalGap ) const
{
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
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersVertically( ALIGNMENT_RECTS& itemsToDistribute ) const
{
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(), SortCenterY );
    const auto totalGap = ( itemsToDistribute.end()-1 )->second.GetCenter().y
                          - itemsToDistribute.begin()->second.GetCenter().y;
    const auto itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    auto targetY = itemsToDistribute.begin()->second.GetCenter().y;

    for( auto& i : itemsToDistribute )
    {
        int difference = targetY - i.second.GetCenter().y;
        i.first->Move( wxPoint( 0, difference ) );
        targetY += ( itemGap );
    }
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
