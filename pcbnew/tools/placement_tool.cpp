/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "pcb_selection_tool.h"

#include <ratsnest/ratsnest_data.h>
#include <tool/tool_manager.h>

#include <pcb_edit_frame.h>
#include <board.h>
#include <board_commit.h>
#include <bitmaps.h>


ALIGN_DISTRIBUTE_TOOL::ALIGN_DISTRIBUTE_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Placement" ),
    m_selectionTool( nullptr ),
    m_placementMenu( nullptr ),
    m_frame( nullptr )
{
}

ALIGN_DISTRIBUTE_TOOL::~ALIGN_DISTRIBUTE_TOOL()
{
    delete m_placementMenu;
}


bool ALIGN_DISTRIBUTE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    // Create a context menu and make it available through selection tool
    m_placementMenu = new ACTION_MENU( true, this );
    m_placementMenu->SetIcon( BITMAPS::align_items );
    m_placementMenu->SetTitle( _( "Align/Distribute" ) );

    // Add all align/distribute commands
    m_placementMenu->Add( PCB_ACTIONS::alignLeft );
    m_placementMenu->Add( PCB_ACTIONS::alignCenterX );
    m_placementMenu->Add( PCB_ACTIONS::alignRight );

    m_placementMenu->AppendSeparator();
    m_placementMenu->Add( PCB_ACTIONS::alignTop );
    m_placementMenu->Add( PCB_ACTIONS::alignCenterY );
    m_placementMenu->Add( PCB_ACTIONS::alignBottom );

    m_placementMenu->AppendSeparator();
    m_placementMenu->Add( PCB_ACTIONS::distributeHorizontally );
    m_placementMenu->Add( PCB_ACTIONS::distributeVertically );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_placementMenu, SELECTION_CONDITIONS::MoreThan( 1 ), 100 );

    return true;
}


template <class T>
std::vector<std::pair<BOARD_ITEM*, BOX2I>> GetBoundingBoxes( const T& aItems )
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> rects;

    for( EDA_ITEM* item : aItems )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( boardItem->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( boardItem );
            rects.emplace_back( std::make_pair( footprint,
                                                footprint->GetBoundingBox( false, false ) ) );
        }
        else
        {
            rects.emplace_back( std::make_pair( boardItem, boardItem->GetBoundingBox() ) );
        }
    }

    return rects;
}


template< typename T >
int ALIGN_DISTRIBUTE_TOOL::selectTarget( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                                         std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aLocked,
                                         T aGetValue )
{
    VECTOR2I curPos = getViewControls()->GetCursorPosition();

    // Prefer locked items to unlocked items.
    // Secondly, prefer items under the cursor to other items.

    if( aLocked.size() >= 1 )
    {
        for( const std::pair<BOARD_ITEM*, BOX2I>& item : aLocked )
        {
            if( item.second.Contains( curPos ) )
                return aGetValue( item );
        }

        return aGetValue( aLocked.front() );
    }

    for( const std::pair<BOARD_ITEM*, BOX2I>& item : aItems )
    {
        if( item.second.Contains( curPos ) )
            return aGetValue( item );
    }

    return aGetValue( aItems.front() );
}


template< typename T >
size_t ALIGN_DISTRIBUTE_TOOL::GetSelections( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItemsToAlign,
                                             std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aLockedItems,
                                             T aCompare )
{
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( item->Type() == PCB_MARKER_T )
                        aCollector.Remove( item );
                }
            } );

    std::vector<BOARD_ITEM*> lockedItems;
    std::vector<BOARD_ITEM*> itemsToAlign;

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        // We do not lock items in the footprint editor
        if( boardItem->IsLocked() && m_frame->IsType( FRAME_PCB_EDITOR ) )
        {
            // Locking a pad but not the footprint means that we align the footprint using
            // the pad position.  So we test for footprint locking here
            if( boardItem->Type() == PCB_PAD_T && !boardItem->GetParent()->IsLocked() )
                itemsToAlign.push_back( boardItem );
            else
                lockedItems.push_back( boardItem );
        }
        else
        {
            itemsToAlign.push_back( boardItem );
        }
    }

    aItemsToAlign = GetBoundingBoxes( itemsToAlign );
    aLockedItems = GetBoundingBoxes( lockedItems );
    std::sort( aItemsToAlign.begin(), aItemsToAlign.end(), aCompare );
    std::sort( aLockedItems.begin(), aLockedItems.end(), aCompare );

    return aItemsToAlign.size();
}


int ALIGN_DISTRIBUTE_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToAlign;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.GetTop() < rhs.second.GetTop() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetTop = selectTarget( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& aVal )
            {
                return aVal.second.GetTop();
            } );

    // Move the selected items
    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToAlign )
    {
        BOARD_ITEM* item = i.first;
        int difference = targetTop - i.second.GetTop();

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( 0, difference ) );
    }

    commit.Push( _( "Align to Top" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToAlign;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs)
            {
                return ( lhs.second.GetBottom() > rhs.second.GetBottom() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetBottom = selectTarget( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& aVal )
            {
                return aVal.second.GetBottom();
            } );

    // Move the selected items
    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToAlign )
    {
        int difference = targetBottom - i.second.GetBottom();
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( 0, difference ) );
    }

    commit.Push( _( "Align to Bottom" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignLeft( const TOOL_EVENT& aEvent )
{
    // Because this tool uses bounding boxes and they aren't mirrored even when
    // the view is mirrored, we need to call the other one if mirrored.
    if( getView()->IsMirroredX() )
        return doAlignRight();
    else
        return doAlignLeft();
}


int ALIGN_DISTRIBUTE_TOOL::doAlignLeft()
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToAlign;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.GetLeft() < rhs.second.GetLeft() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetLeft = selectTarget( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& aVal )
            {
                return aVal.second.GetLeft();
            } );

    // Move the selected items
    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToAlign )
    {
        int difference = targetLeft - i.second.GetLeft();
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( difference, 0 ) );
    }

    commit.Push( _( "Align to Left" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignRight( const TOOL_EVENT& aEvent )
{
    // Because this tool uses bounding boxes and they aren't mirrored even when
    // the view is mirrored, we need to call the other one if mirrored.
    if( getView()->IsMirroredX() )
        return doAlignLeft();
    else
        return doAlignRight();
}


int ALIGN_DISTRIBUTE_TOOL::doAlignRight()
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToAlign;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.GetRight() > rhs.second.GetRight() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetRight = selectTarget( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& aVal )
            {
                return aVal.second.GetRight();
            } );

    // Move the selected items
    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToAlign )
    {
        int difference = targetRight - i.second.GetRight();
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( difference, 0 ) );
    }

    commit.Push( _( "Align to Right" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterX( const TOOL_EVENT& aEvent )
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToAlign;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.Centre().x < rhs.second.Centre().x );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetX = selectTarget( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& aVal )
            {
                return aVal.second.Centre().x;
            } );

    // Move the selected items
    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToAlign )
    {
        int difference = targetX - i.second.Centre().x;
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( difference, 0 ) );
    }

    commit.Push( _( "Align to Middle" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterY( const TOOL_EVENT& aEvent )
{
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToAlign;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.Centre().y < rhs.second.Centre().y );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetY = selectTarget( itemsToAlign, locked_items,
            []( const std::pair<BOARD_ITEM*, BOX2I>& aVal )
            {
                return aVal.second.Centre().y;
            } );

    // Move the selected items
    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToAlign )
    {
        int difference = targetY - i.second.Centre().y;
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( 0, difference ) );
    }

    commit.Push( _( "Align to Center" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( item->Type() == PCB_MARKER_T )
                        aCollector.Remove( item );
                }
            },
            m_frame->IsType( FRAME_PCB_EDITOR ) /* prompt user regarding locked items */ );

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT                               commit( m_frame );
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.GetRight() > rhs.second.GetRight() );
            } );

    BOARD_ITEM* lastItem = itemsToDistribute.begin()->first;
    const int   maxRight = itemsToDistribute.begin()->second.GetRight();

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.GetX() < rhs.second.GetX() );
            } );

    const int minX = itemsToDistribute.begin()->second.GetX();
    int       totalGap = maxRight - minX;
    int       totalWidth = 0;

    for( const auto& [ item, rect ] : itemsToDistribute )
        totalWidth += rect.GetWidth();

    if( totalGap < totalWidth )
    {
        // the width of the items exceeds the gap (overlapping items) -> use center point spacing
        doDistributeCentersHorizontally( itemsToDistribute, commit );
    }
    else
    {
        totalGap -= totalWidth;
        doDistributeGapsHorizontally( itemsToDistribute, commit, lastItem, totalGap );
    }

    commit.Push( _( "Distribute Horizontally" ) );
    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGapsHorizontally( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                                                          BOARD_COMMIT& aCommit,
                                                          const BOARD_ITEM* lastItem,
                                                          int totalGap ) const
{
    const int itemGap = totalGap / ( aItems.size() - 1 );
    int       targetX = aItems.begin()->second.GetX();

    for( const std::pair<BOARD_ITEM*, BOX2I>& i : aItems )
    {
        BOARD_ITEM* item = i.first;

        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == item )
            continue;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetX - i.second.GetX();
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( difference, 0 ) );
        targetX += ( i.second.GetWidth() + itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersHorizontally( std::vector<std::pair<BOARD_ITEM*, BOX2I>> &aItems,
                                                             BOARD_COMMIT& aCommit ) const
{
    std::sort( aItems.begin(), aItems.end(),
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.Centre().x < rhs.second.Centre().x );
            } );

    const int totalGap = ( aItems.end()-1 )->second.Centre().x - aItems.begin()->second.Centre().x;
    const int itemGap = totalGap / ( aItems.size() - 1 );
    int       targetX = aItems.begin()->second.Centre().x;

    for( const std::pair<BOARD_ITEM*, BOX2I>& i : aItems )
    {
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetX - i.second.Centre().x;
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( difference, 0 ) );
        targetX += ( itemGap );
    }
}


int ALIGN_DISTRIBUTE_TOOL::DistributeVertically( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( item->Type() == PCB_MARKER_T )
                        aCollector.Remove( item );
                }
            },
            m_frame->IsType( FRAME_PCB_EDITOR ) /* prompt user regarding locked items */ );

    if( selection.Size() <= 1 )
        return 0;

    BOARD_COMMIT                               commit( m_frame );
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.GetBottom() > rhs.second.GetBottom() );
            } );

    BOARD_ITEM* lastItem = itemsToDistribute.begin()->first;
    const int   maxBottom = itemsToDistribute.begin()->second.GetBottom();

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            []( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                return ( lhs.second.Centre().y < rhs.second.Centre().y );
            } );

    int minY = itemsToDistribute.begin()->second.GetY();
    int totalGap = maxBottom - minY;
    int totalHeight = 0;

    for( const std::pair<BOARD_ITEM*, BOX2I>& i : itemsToDistribute )
        totalHeight += i.second.GetHeight();

    if( totalGap < totalHeight )
    {
        // the width of the items exceeds the gap (overlapping items) -> use center point spacing
        doDistributeCentersVertically( itemsToDistribute, commit );
    }
    else
    {
        totalGap -= totalHeight;
        doDistributeGapsVertically( itemsToDistribute, commit, lastItem, totalGap );
    }

    commit.Push( _( "Distribute Vertically" ) );
    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGapsVertically( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                                                        BOARD_COMMIT& aCommit,
                                                        const BOARD_ITEM* lastItem,
                                                        int totalGap ) const
{
    const int itemGap = totalGap / ( aItems.size() - 1 );
    int       targetY = aItems.begin()->second.GetY();

    for( std::pair<BOARD_ITEM*, BOX2I>& i : aItems )
    {
        BOARD_ITEM* item = i.first;

        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == item )
            continue;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetY - i.second.GetY();
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( 0, difference ) );
        targetY += ( i.second.GetHeight() + itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersVertically( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                                                           BOARD_COMMIT& aCommit ) const
{
    std::sort( aItems.begin(), aItems.end(),
        [] ( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs)
        {
            return ( lhs.second.Centre().y < rhs.second.Centre().y );
        } );

    const int totalGap = ( aItems.end() - 1 )->second.Centre().y
                         - aItems.begin()->second.Centre().y;
    const int itemGap  = totalGap / ( aItems.size() - 1 );
    int       targetY  = aItems.begin()->second.Centre().y;

    for( const std::pair<BOARD_ITEM*, BOX2I>& i : aItems )
    {
        BOARD_ITEM* item = i.first;

        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetY - i.second.Centre().y;
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( 0, difference ) );
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

    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeHorizontally,
        PCB_ACTIONS::distributeHorizontally.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeVertically,
        PCB_ACTIONS::distributeVertically.MakeEvent() );
}
