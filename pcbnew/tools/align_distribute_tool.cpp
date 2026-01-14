/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "align_distribute_tool.h"
#include "pcb_actions.h"
#include "pcb_selection_tool.h"

#include <ratsnest/ratsnest_data.h>
#include <tool/tool_manager.h>

#include <board.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <geometry/distribute.h>
#include <view/view_controls.h>


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
    m_placementMenu = new CONDITIONAL_MENU( this );
    m_placementMenu->SetIcon( BITMAPS::align_items );
    m_placementMenu->SetUntranslatedTitle( _HKI( "Align/Distribute" ) );

    const auto canAlign = SELECTION_CONDITIONS::MoreThan( 1 );
    const auto canDistribute = SELECTION_CONDITIONS::MoreThan( 2 );

    // Add all align/distribute commands
    m_placementMenu->AddItem( PCB_ACTIONS::alignLeft, canAlign );
    m_placementMenu->AddItem( PCB_ACTIONS::alignCenterX, canAlign );
    m_placementMenu->AddItem( PCB_ACTIONS::alignRight, canAlign );

    m_placementMenu->AddSeparator( canAlign );
    m_placementMenu->AddItem( PCB_ACTIONS::alignTop, canAlign );
    m_placementMenu->AddItem( PCB_ACTIONS::alignCenterY, canAlign );
    m_placementMenu->AddItem( PCB_ACTIONS::alignBottom, canAlign );

    m_placementMenu->AddSeparator( canDistribute );
    m_placementMenu->AddItem( PCB_ACTIONS::distributeHorizontallyCenters, canDistribute );
    m_placementMenu->AddItem( PCB_ACTIONS::distributeHorizontallyGaps, canDistribute );
    m_placementMenu->AddItem( PCB_ACTIONS::distributeVerticallyCenters, canDistribute );
    m_placementMenu->AddItem( PCB_ACTIONS::distributeVerticallyGaps, canDistribute );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_placementMenu, SELECTION_CONDITIONS::MoreThan( 1 ), 100 );

    return true;
}


BOX2I getBoundingBox( BOARD_ITEM* aItem )
{
    if( aItem->Type() == PCB_FOOTPRINT_T )
        return static_cast<FOOTPRINT*>( aItem )->GetBoundingBox( false );
    else
        return aItem->GetBoundingBox();
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

    bool                  allPads = true;
    BOARD_ITEM_CONTAINER* currentParent = nullptr;
    bool                  differentParents = false;
    bool                  allowFreePads = m_selectionTool->IsFootprintEditor()
                                       || m_frame->GetPcbNewSettings()->m_AllowFreePads;

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        if( item->Type() != PCB_PAD_T )
            allPads = false;

        BOARD_ITEM*           boardItem = static_cast<BOARD_ITEM*>( item );
        BOARD_ITEM_CONTAINER* parent = boardItem->GetParentFootprint();

        if( !parent )
            parent = boardItem->GetBoard();

        if( !currentParent )
            currentParent = parent;
        else if( parent != currentParent )
            differentParents = true;
    }

    auto addToList =
            []( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& list, BOARD_ITEM* item, FOOTPRINT* parentFp )
            {
                BOARD_ITEM* listItem = parentFp ? parentFp : item;

                for( const auto& [candidate, bbox] : list )
                {
                    if( candidate == listItem )
                        return;
                }

                list.emplace_back( std::make_pair( listItem, getBoundingBox( item ) ) );
            };

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() || item->Type() == PCB_TABLECELL_T )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( boardItem->Type() == PCB_PAD_T && ( !allowFreePads || ( allPads && differentParents ) ) )
        {
            FOOTPRINT* parentFp = boardItem->GetParentFootprint();

            if( parentFp && parentFp->IsLocked() )
                addToList( aLockedItems, boardItem, parentFp );
            else
                addToList( aItemsToAlign, boardItem, parentFp );

            continue;
        }

        if( boardItem->IsLocked() )
            addToList( aLockedItems, boardItem, nullptr );
        else
            addToList( aItemsToAlign, boardItem, nullptr );
    }

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
    for( const auto& [item, bbox] : itemsToAlign )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        int difference = targetTop - bbox.GetTop();

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
    for( const auto& [item, bbox] : itemsToAlign )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        int difference = targetBottom - bbox.GetBottom();

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
    for( const auto& [item, bbox] : itemsToAlign )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        int difference = targetLeft - bbox.GetLeft();

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
    for( const auto& [item, bbox] : itemsToAlign )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        int difference = targetRight - bbox.GetRight();

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
    for( const auto& [item, bbox] : itemsToAlign )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        int difference = targetX - bbox.Centre().x;

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
    for( const auto& [item, bbox] : itemsToAlign )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        int difference = targetY - bbox.Centre().y;

        commit.Stage( item, CHT_MODIFY );
        item->Move( VECTOR2I( 0, difference ) );
    }

    commit.Push( _( "Align to Center" ) );
    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::DistributeItems( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );

                // Don't filter for free pads.  We want to allow for distributing other
                // items (such as a via) between two pads.
                // sTool->FilterCollectorForFreePads( aCollector );

                sTool->FilterCollectorForLockedItems( aCollector );
            } );

    // Need at least 3 items to distribute - one at each end and at least on in the middle
    if( selection.Size() < 3 )
        return 0;

    BOARD_COMMIT                               commit( m_frame );
    wxString                                   commitMsg;
    std::vector<std::pair<BOARD_ITEM*, BOX2I>> itemsToDistribute;

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
        itemsToDistribute.emplace_back( std::make_pair( boardItem, getBoundingBox( boardItem ) ) );
    }

    if( aEvent.Matches( PCB_ACTIONS::distributeHorizontallyCenters.MakeEvent() ) )
    {
        doDistributeCenters( true, itemsToDistribute, commit );
        commitMsg = PCB_ACTIONS::distributeHorizontallyCenters.GetFriendlyName();
    }
    else if( aEvent.Matches( PCB_ACTIONS::distributeHorizontallyGaps.MakeEvent() ) )
    {
        doDistributeGaps( true, itemsToDistribute, commit );
        commitMsg = PCB_ACTIONS::distributeHorizontallyGaps.GetFriendlyName();
    }
    else if( aEvent.Matches( PCB_ACTIONS::distributeVerticallyCenters.MakeEvent() ) )
    {
        doDistributeCenters( false, itemsToDistribute, commit );
        commitMsg = PCB_ACTIONS::distributeVerticallyCenters.GetFriendlyName();
    }
    else
    {
        doDistributeGaps( false, itemsToDistribute, commit );
        commitMsg = PCB_ACTIONS::distributeVerticallyGaps.GetFriendlyName();
    }

    commit.Push( commitMsg );
    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGaps( bool                                        aIsXAxis,
                                              std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                                              BOARD_COMMIT& aCommit ) const
{
    // Sort by start position.
    // This is a simple way to get the items in a sensible order but it's not perfect.
    // It will fail if, say, there's a huge items that's bigger than the total span of
    // all the other items, but at that point a gap-equalising algorithm probably isn't
    // well-defined anyway.
    std::sort( aItems.begin(), aItems.end(),
               [&]( const std::pair<BOARD_ITEM*, BOX2I>& a, const std::pair<BOARD_ITEM*, BOX2I>& b )
               {
                   return aIsXAxis ? a.second.GetLeft() < b.second.GetLeft()
                                   : a.second.GetTop() < b.second.GetTop();
               } );

    // Consruct list of item spans in the relevant axis
    std::vector<std::pair<int, int>> itemSpans;
    itemSpans.reserve( aItems.size() );

    for( const auto& [item, box] : aItems )
    {
        const int start = aIsXAxis ? box.GetLeft() : box.GetTop();
        const int end = aIsXAxis ? box.GetRight() : box.GetBottom();
        itemSpans.emplace_back( start, end );
    }

    // Get the deltas needed to distribute the items evenly
    const std::vector<int> deltas = GetDeltasForDistributeByGaps( itemSpans );

    // Apply the deltas to the items
    for( size_t i = 1; i < aItems.size() - 1; ++i )
    {
        const auto& [item, box] = aItems[i];
        const int delta = deltas[i];

        if( delta != 0 )
        {
            const VECTOR2I deltaVec = aIsXAxis ? VECTOR2I( delta, 0 ) : VECTOR2I( 0, delta );

            aCommit.Stage( item, CHT_MODIFY );
            item->Move( deltaVec );
        }
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCenters( bool aIsXAxis,
                                                 std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                                                 BOARD_COMMIT& aCommit ) const
{
    std::sort(
            aItems.begin(), aItems.end(),
            [&]( const std::pair<BOARD_ITEM*, BOX2I>& lhs, const std::pair<BOARD_ITEM*, BOX2I>& rhs )
            {
                const int lhsPos = aIsXAxis ? lhs.second.Centre().x : lhs.second.Centre().y;
                const int rhsPos = aIsXAxis ? rhs.second.Centre().x : rhs.second.Centre().y;
                return lhsPos < rhsPos;
            } );

    std::vector<int> itemCenters;
    itemCenters.reserve( aItems.size() );

    for( const auto& [item, box] : aItems )
    {
        itemCenters.push_back( aIsXAxis ? box.Centre().x : box.Centre().y );
    }

    const std::vector<int> deltas = GetDeltasForDistributeByPoints( itemCenters );

    // Apply the deltas to the items
    for( size_t i = 1; i < aItems.size() - 1; ++i )
    {
        const auto& [item, box] = aItems[i];
        const int delta = deltas[i];

        if ( delta != 0)
        {
            const VECTOR2I deltaVec = aIsXAxis ? VECTOR2I( delta, 0 ) : VECTOR2I( 0, delta );

            aCommit.Stage( item, CHT_MODIFY );
            item->Move( deltaVec );
        }
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

    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeItems,
        PCB_ACTIONS::distributeHorizontallyCenters.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeItems,
        PCB_ACTIONS::distributeHorizontallyGaps.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeItems,
        PCB_ACTIONS::distributeVerticallyCenters.MakeEvent() );
    Go( &ALIGN_DISTRIBUTE_TOOL::DistributeItems,
        PCB_ACTIONS::distributeVerticallyGaps.MakeEvent() );
}
