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
#include "pcb_selection_tool.h"

#include <ratsnest/ratsnest_data.h>
#include <tool/tool_manager.h>

#include <pcb_edit_frame.h>
#include <board.h>
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
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    // Create a context menu and make it available through selection tool
    m_placementMenu = new ACTION_MENU( true );
    m_placementMenu->SetTool( this );
    m_placementMenu->SetIcon( align_items_xpm );
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
    selToolMenu.AddMenu( m_placementMenu, SELECTION_CONDITIONS::MoreThan( 1 ) );

    return true;
}


template <class T>
ALIGNMENT_RECTS GetBoundingBoxes( const T& aItems )
{
    ALIGNMENT_RECTS rects;

    for( EDA_ITEM* item : aItems )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
            rects.emplace_back( std::make_pair( boardItem, footprint->GetFootprintRect() ) );
        }
        else
        {
            rects.emplace_back( std::make_pair( boardItem, item->GetBoundingBox() ) );
        }
    }

    return rects;
}


template< typename T >
int ALIGN_DISTRIBUTE_TOOL::selectTarget( ALIGNMENT_RECTS& aItems, ALIGNMENT_RECTS& aLocked,
                                         T aGetValue )
{
    wxPoint curPos = (wxPoint) getViewControls()->GetCursorPosition();

    // Prefer locked items to unlocked items.
    // Secondly, prefer items under the cursor to other items.

    if( aLocked.size() >= 1 )
    {
        for( const ALIGNMENT_RECT& item : aLocked )
        {
            if( item.second.Contains( curPos ) )
                return aGetValue( item );
        }

        return aGetValue( aLocked.front() );
    }

    for( const ALIGNMENT_RECT& item : aItems )
    {
        if( item.second.Contains( curPos ) )
            return aGetValue( item );
    }

    return aGetValue( aItems.front() );
}


template< typename T >
size_t ALIGN_DISTRIBUTE_TOOL::GetSelections( ALIGNMENT_RECTS& aItemsToAlign,
                                             ALIGNMENT_RECTS& aLockedItems, T aCompare )
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
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        if( boardItem->IsLocked() )
        {
            // Locking a pad but not the footprint means that we align the footprint using
            // the pad position.  So we test for footprint locking here
            if( m_frame->IsType( FRAME_PCB_EDITOR ) && boardItem->Type() == PCB_PAD_T
                    && !boardItem->GetParent()->IsLocked() )
            {
                itemsToAlign.push_back( boardItem );
            }
            else
            {
                lockedItems.push_back( boardItem );
            }
        }
        else
            itemsToAlign.push_back( boardItem );
    }

    aItemsToAlign = GetBoundingBoxes( itemsToAlign );
    aLockedItems = GetBoundingBoxes( lockedItems );
    std::sort( aItemsToAlign.begin(), aItemsToAlign.end(), aCompare );
    std::sort( aLockedItems.begin(), aLockedItems.end(), aCompare );

    return aItemsToAlign.size();
}


int ALIGN_DISTRIBUTE_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetTop() < right.second.GetTop() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetTop = selectTarget( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT& aVal )
            {
                return aVal.second.GetTop();
            } );

    // Move the selected items
    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToAlign )
    {
        int difference = targetTop - i.second.GetTop();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to top" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetBottom() < right.second.GetBottom() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetBottom = selectTarget( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT& aVal )
            {
                return aVal.second.GetBottom();
            } );

    // Move the selected items
    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToAlign )
    {
        int difference = targetBottom - i.second.GetBottom();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
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

    if( !GetSelections( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetLeft() < right.second.GetLeft() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetLeft = selectTarget( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT& aVal )
            {
                return aVal.second.GetLeft();
            } );

    // Move the selected items
    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToAlign )
    {
        int difference = targetLeft - i.second.GetLeft();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
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

    if( !GetSelections( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetRight() < right.second.GetRight() );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetRight = selectTarget( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT& aVal )
            {
                return aVal.second.GetRight();
            } );

    // Move the selected items
    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToAlign )
    {
        int difference = targetRight - i.second.GetRight();
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to right" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterX( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetCenter().x < right.second.GetCenter().x );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetX = selectTarget( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT& aVal )
            {
                return aVal.second.GetCenter().x;
            } );

    // Move the selected items
    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToAlign )
    {
        int difference = targetX - i.second.GetCenter().x;
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( difference, 0 ) );
    }

    commit.Push( _( "Align to middle" ) );

    return 0;
}


int ALIGN_DISTRIBUTE_TOOL::AlignCenterY( const TOOL_EVENT& aEvent )
{
    ALIGNMENT_RECTS itemsToAlign;
    ALIGNMENT_RECTS locked_items;

    if( !GetSelections( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetCenter().y < right.second.GetCenter().y );
            } ) )
    {
        return 0;
    }

    BOARD_COMMIT commit( m_frame );

    int targetY = selectTarget( itemsToAlign, locked_items,
            []( const ALIGNMENT_RECT& aVal )
            {
                return aVal.second.GetCenter().y;
            } );

    // Move the selected items
    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToAlign )
    {
        int difference = targetY - i.second.GetCenter().y;
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        commit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( 0, difference ) );
    }

    commit.Push( _( "Align to center" ) );

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

    BOARD_COMMIT    commit( m_frame );
    ALIGNMENT_RECTS itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetRight() > right.second.GetRight() );
            } );

    BOARD_ITEM* lastItem = itemsToDistribute.begin()->first;
    const int   maxRight = itemsToDistribute.begin()->second.GetRight();

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetX() < right.second.GetX() );
            } );

    const int minX = itemsToDistribute.begin()->second.GetX();
    int       totalGap = maxRight - minX;
    int       totalWidth = 0;

    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToDistribute )
        totalWidth += i.second.GetWidth();

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

    commit.Push( _( "Distribute horizontally" ) );

    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGapsHorizontally( ALIGNMENT_RECTS& itemsToDistribute,
                                                          BOARD_COMMIT& aCommit,
                                                          const BOARD_ITEM* lastItem,
                                                          int totalGap ) const
{
    const int itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    int       targetX = itemsToDistribute.begin()->second.GetX();

    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToDistribute )
    {
        BOARD_ITEM* item = i.first;

        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == item )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetX - i.second.GetX();
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( difference, 0 ) );
        targetX += ( i.second.GetWidth() + itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersHorizontally( ALIGNMENT_RECTS &itemsToDistribute,
                                                             BOARD_COMMIT& aCommit ) const
{
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
            [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
            {
                return ( left.second.GetCenter().x < right.second.GetCenter().x );
            } );

    const int totalGap = ( itemsToDistribute.end()-1 )->second.GetCenter().x
                          - itemsToDistribute.begin()->second.GetCenter().x;
    const int itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    int       targetX = itemsToDistribute.begin()->second.GetCenter().x;

    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToDistribute )
    {
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetX - i.second.GetCenter().x;
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( difference, 0 ) );
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

    BOARD_COMMIT    commit( m_frame );
    ALIGNMENT_RECTS itemsToDistribute = GetBoundingBoxes( selection );

    // find the last item by reverse sorting
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
        [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
        {
            return ( left.second.GetBottom() > right.second.GetBottom() );
        } );

    BOARD_ITEM* lastItem = itemsToDistribute.begin()->first;
    const int   maxBottom = itemsToDistribute.begin()->second.GetBottom();

    // sort to get starting order
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
        [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
        {
            return ( left.second.GetCenter().y < right.second.GetCenter().y );
        } );

    int minY = itemsToDistribute.begin()->second.GetY();
    int totalGap = maxBottom - minY;
    int totalHeight = 0;

    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToDistribute )
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

    commit.Push( _( "Distribute vertically" ) );

    return 0;
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeGapsVertically( ALIGNMENT_RECTS& itemsToDistribute,
                                                        BOARD_COMMIT& aCommit,
                                                        const BOARD_ITEM* lastItem,
                                                        int totalGap ) const
{
    const int itemGap = totalGap / ( itemsToDistribute.size() - 1 );
    int       targetY = itemsToDistribute.begin()->second.GetY();

    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToDistribute )
    {
        BOARD_ITEM* item = i.first;

        // cover the corner case where the last item is wider than the previous item and gap
        if( lastItem == item )
            continue;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetY - i.second.GetY();
        aCommit.Stage( item, CHT_MODIFY );
        item->Move( wxPoint( 0, difference ) );
        targetY += ( i.second.GetHeight() + itemGap );
    }
}


void ALIGN_DISTRIBUTE_TOOL::doDistributeCentersVertically( ALIGNMENT_RECTS& itemsToDistribute,
                                                           BOARD_COMMIT& aCommit ) const
{
    std::sort( itemsToDistribute.begin(), itemsToDistribute.end(),
        [] ( const ALIGNMENT_RECT left, const ALIGNMENT_RECT right)
        {
            return ( left.second.GetCenter().y < right.second.GetCenter().y );
        } );

    const int totalGap = ( itemsToDistribute.end()-1 )->second.GetCenter().y
                          - itemsToDistribute.begin()->second.GetCenter().y;
    const int itemGap  = totalGap / ( itemsToDistribute.size() - 1 );
    int       targetY  = itemsToDistribute.begin()->second.GetCenter().y;

    for( std::pair<BOARD_ITEM*, EDA_RECT>& i : itemsToDistribute )
    {
        BOARD_ITEM* item = i.first;

        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && m_frame->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        int difference = targetY - i.second.GetCenter().y;
        aCommit.Stage( item, CHT_MODIFY );
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
