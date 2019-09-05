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
#include "edit_tool.h"
#include <tool/tool_manager.h>

#include <pcb_edit_frame.h>
#include <class_board.h>
#include <ratsnest_data.h>
#include <board_commit.h>
#include <bitmaps.h>

#include <confirm.h>
#include <menus_helpers.h>


ALIGN_DISTRIBUTE_TOOL::ALIGN_DISTRIBUTE_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Placement" ), m_selectionTool( NULL ), m_placementMenu( NULL ),
    m_frame( NULL )
{
}

ALIGN_DISTRIBUTE_TOOL::~ALIGN_DISTRIBUTE_TOOL()
{
    delete m_placementMenu;
}


bool ALIGN_DISTRIBUTE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    wxASSERT_MSG( m_selectionTool, _( "pcbnew.InteractiveSelection tool is not available" ) );

    m_frame = getEditFrame<PCB_BASE_FRAME>();

    // Create a context menu and make it available through selection tool
    m_placementMenu = new ACTION_MENU( true );
    m_placementMenu->SetTool( this );
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

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_placementMenu, SELECTION_CONDITIONS::MoreThan( 1 ) );

    return true;
}


template <class T>
ALIGNMENT_RECTS GetBoundingBoxes( const T &sel )
{
    ALIGNMENT_RECTS rects;

    for( auto item : sel )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( item->Type() == PCB_MODULE_T )
            rects.emplace_back( std::make_pair( boardItem, static_cast<MODULE*>( item )->GetFootprintRect() ) );
        else
            rects.emplace_back( std::make_pair( boardItem, item->GetBoundingBox() ) );
    }

    return rects;
}


template< typename T >
int ALIGN_DISTRIBUTE_TOOL::selectTarget( ALIGNMENT_RECTS& aItems, ALIGNMENT_RECTS& aLocked, T aGetValue )
{
    wxPoint curPos( getViewControls()->GetCursorPosition().x,  getViewControls()->GetCursorPosition().y );

    // after sorting, the fist item acts as the target for all others
    // unless we have a locked item, in which case we use that for the target
    int target = !aLocked.size() ? aGetValue( aItems.front() ): aGetValue( aLocked.front() );

    // Iterate through both lists to find if we are mouse-over on one of the items.
    for( auto sel = aLocked.begin(); sel != aItems.end(); sel++ )
    {
        // If there are locked items, prefer aligning to them over
        // aligning to the cursor as they do not move
        if( sel == aLocked.end() )
        {
            if( aLocked.size() == 0 )
            {
                sel = aItems.begin();
                continue;
            }

            break;
        }

        // We take the first target that overlaps our cursor.
        // This is deterministic because we assume sorted arrays
        if( sel->second.Contains( curPos ) )
        {
            target = aGetValue( *sel );
            break;
        }
    }

    return target;
}


template< typename T >
size_t ALIGN_DISTRIBUTE_TOOL::GetSelections( ALIGNMENT_RECTS& aItems, ALIGNMENT_RECTS& aLocked, T aCompare )
{

    PCBNEW_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_TRANSIENTS ); } );

    if( selection.Size() <= 1 )
        return 0;

    std::vector<BOARD_ITEM*> lockedItems;
    selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED ); }, &lockedItems );

    aItems = GetBoundingBoxes( selection );
    aLocked = GetBoundingBoxes( lockedItems );
    std::sort( aItems.begin(), aItems.end(), aCompare );
    std::sort( aLocked.begin(), aLocked.end(), aCompare );

    return aItems.size();
}


int ALIGN_DISTRIBUTE_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items, []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetTop() < right.second.GetTop() ); } ) )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( m_selectionTool->GetSelection(), CHT_MODIFY );
    auto targetTop = selectTarget( itemsToAlign, locked_items, []( const ALIGNMENT_RECT& aVal )
            { return aVal.second.GetTop(); } );

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetTop - i.second.GetTop();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to top" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items, []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetBottom() < right.second.GetBottom() ); } ) )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( m_selectionTool->GetSelection(), CHT_MODIFY );
    auto targetBottom = selectTarget( itemsToAlign, locked_items, []( const ALIGNMENT_RECT& aVal )
            { return aVal.second.GetBottom(); } );

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetBottom - i.second.GetBottom();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
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
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items, []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetLeft() < right.second.GetLeft() ); } ) )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( m_selectionTool->GetSelection(), CHT_MODIFY );
    auto targetLeft = selectTarget( itemsToAlign, locked_items, []( const ALIGNMENT_RECT& aVal )
            { return aVal.second.GetLeft(); } );

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetLeft - i.second.GetLeft();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
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
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items, []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetRight() < right.second.GetRight() ); } ) )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( m_selectionTool->GetSelection(), CHT_MODIFY );
    auto targetRight = selectTarget( itemsToAlign, locked_items, []( const ALIGNMENT_RECT& aVal )
            { return aVal.second.GetRight(); } );

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetRight - i.second.GetRight();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to right" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterX( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items, []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetCenter().x < right.second.GetCenter().x ); } ) )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( m_selectionTool->GetSelection(), CHT_MODIFY );
    auto targetX = selectTarget( itemsToAlign, locked_items, []( const ALIGNMENT_RECT& aVal )
            { return aVal.second.GetCenter().x; } );

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetX - i.second.GetCenter().x;
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to middle" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterY( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items, []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetCenter().y < right.second.GetCenter().y ); } ) )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( m_selectionTool->GetSelection(), CHT_MODIFY );
    auto targetY = selectTarget( itemsToAlign, locked_items, []( const ALIGNMENT_RECT& aVal )
            { return aVal.second.GetCenter().y; } );

    // Move the selected items
    for( auto& i : itemsToAlign )
    {
        int difference = targetY - i.second.GetCenter().y;
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to center" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally( const TOOL_EVENT& aEvent )
{
    PCBNEW_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED | EXCLUDE_TRANSIENTS ); } );

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetRight() > right.second.GetRight() ); } );
    const auto lastItem = itemsToDistribute.begin()->first;

    const auto maxRight = itemsToDistribute.begin()->second.GetRight();

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetX() < right.second.GetX() ); } );
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
        BOARD_ITEM* item = i.first;

        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == item )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetX - i.second.GetX();
        item->Move( wxPoint( difference, 0 ) );
        targetX += ( i.second.GetWidth() + itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersHorizontally( ALIGNMENT_RECTS &itemsToDistribute ) const
{
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            { return ( left.second.GetCenter().x < right.second.GetCenter().x ); } );
    const auto totalGap = ( itemsToDistribute.end()-1 )->second.GetCenter().x
                          - itemsToDistribute.begin()->second.GetCenter().x;
    const auto itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    auto targetX = itemsToDistribute.begin()->second.GetCenter().x;

    for( auto& i : itemsToDistribute )
    {
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetX - i.second.GetCenter().x;
        item->Move( wxPoint( difference, 0 ) );
        targetX += ( itemGap );
    }
}


int ALIGN_DISTRIBUTE_TOOL::DistributeVertically( const TOOL_EVENT& aEvent )
{
    PCBNEW_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED | EXCLUDE_TRANSIENTS ); } );

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT commit( m_frame );
    commit.StageItems( selection, CHT_MODIFY );

    auto itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
        [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
        { return ( left.second.GetBottom() > right.second.GetBottom() ); } );
    const auto maxBottom = itemsToDistribute.begin()->second.GetBottom();
    const auto lastItem = itemsToDistribute.begin()->first;

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
        [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
        { return ( left.second.GetCenter().y < right.second.GetCenter().y ); } );
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
        BOARD_ITEM* item = i.first;

        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == item )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetY - i.second.GetY();
        i.first->Move( wxPoint( 0, difference ) );
        targetY += ( i.second.GetHeight() + itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersVertically( ALIGNMENT_RECTS& itemsToDistribute ) const
{
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
        [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
        { return ( left.second.GetCenter().y < right.second.GetCenter().y ); } );
    const auto totalGap = ( itemsToDistribute.end()-1 )->second.GetCenter().y
                          - itemsToDistribute.begin()->second.GetCenter().y;
    const auto itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    auto targetY = itemsToDistribute.begin()->second.GetCenter().y;

    for( auto& i : itemsToDistribute )
    {
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetY - i.second.GetCenter().y;
        item->Move( wxPoint( 0, difference ) );
        targetY += ( itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::setTransitions()
{
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignTop,               PCB_ACTIONS::alignTop.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignBottom,            PCB_ACTIONS::alignBottom.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignLeft,              PCB_ACTIONS::alignLeft.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignRight,             PCB_ACTIONS::alignRight.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignCenterX,           PCB_ACTIONS::alignCenterX.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::AlignCenterY,           PCB_ACTIONS::alignCenterY.MakeEvent() );

    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally, PCB_ACTIONS::distributeHorizontally.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeVertically,   PCB_ACTIONS::distributeVertically.MakeEvent() );
}
