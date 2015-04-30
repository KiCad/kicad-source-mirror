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

#include "placement_tool.h"
#include "common_actions.h"
#include "selection_tool.h"

#include <wxPcbStruct.h>
#include <class_board.h>
#include <ratsnest_data.h>

#include <confirm.h>
#include <boost/foreach.hpp>

PLACEMENT_TOOL::PLACEMENT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Placement" ), m_selectionTool( NULL )
{
}

PLACEMENT_TOOL::~PLACEMENT_TOOL()
{
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
    CONTEXT_MENU* menu = new CONTEXT_MENU;
    menu->Add( COMMON_ACTIONS::alignTop );
    menu->Add( COMMON_ACTIONS::alignBottom );
    menu->Add( COMMON_ACTIONS::alignLeft );
    menu->Add( COMMON_ACTIONS::alignRight );
    menu->AppendSeparator();
    menu->Add( COMMON_ACTIONS::distributeHorizontally );
    menu->Add( COMMON_ACTIONS::distributeVertically );
    m_selectionTool->GetMenu().AddMenu( menu, _( "Align/distribute" ), false,
                                        SELECTION_CONDITIONS::MoreThan( 1 ) );

    return true;
}


int PLACEMENT_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() > 1 )
    {
        PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
        RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

        // Compute the highest point of selection - it will be the edge of alignment
        int top = selection.Item<BOARD_ITEM>( 0 )->GetBoundingBox().GetY();

        for( int i = 1; i < selection.Size(); ++i )
        {
            int currentTop = selection.Item<BOARD_ITEM>( i )->GetBoundingBox().GetY();

            if( top > currentTop )      // Y decreases when going up
                top = currentTop;
        }

        // Move the selected items
        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );
            int difference = top - item->GetBoundingBox().GetY();

            item->Move( wxPoint( 0, difference ) );
            item->ViewUpdate();
            ratsnest->Update( item );
        }

        getModel<BOARD>()->GetRatsnest()->Recalculate();
    }

    return 0;
}


int PLACEMENT_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() > 1 )
    {
        PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
        RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

        // Compute the lowest point of selection - it will be the edge of alignment
        int bottom = selection.Item<BOARD_ITEM>( 0 )->GetBoundingBox().GetBottom();

        for( int i = 1; i < selection.Size(); ++i )
        {
            int currentBottom = selection.Item<BOARD_ITEM>( i )->GetBoundingBox().GetBottom();

            if( bottom < currentBottom )      // Y increases when going down
                bottom = currentBottom;
        }

        // Move the selected items
        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );
            int difference = bottom - item->GetBoundingBox().GetBottom();

            item->Move( wxPoint( 0, difference ) );
            item->ViewUpdate();
            ratsnest->Update( item );
        }

        getModel<BOARD>()->GetRatsnest()->Recalculate();
    }

    return 0;
}


int PLACEMENT_TOOL::AlignLeft( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() > 1 )
    {
        PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
        RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

        // Compute the leftmost point of selection - it will be the edge of alignment
        int left = selection.Item<BOARD_ITEM>( 0 )->GetBoundingBox().GetX();

        for( int i = 1; i < selection.Size(); ++i )
        {
            int currentLeft = selection.Item<BOARD_ITEM>( i )->GetBoundingBox().GetX();

            if( left > currentLeft )      // X decreases when going left
                left = currentLeft;
        }

        // Move the selected items
        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );
            int difference = left - item->GetBoundingBox().GetX();

            item->Move( wxPoint( difference, 0 ) );
            item->ViewUpdate();
            ratsnest->Update( item );
        }

        getModel<BOARD>()->GetRatsnest()->Recalculate();
    }

    return 0;
}


int PLACEMENT_TOOL::AlignRight( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() > 1 )
    {
        PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
        RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

        // Compute the rightmost point of selection - it will be the edge of alignment
        int right = selection.Item<BOARD_ITEM>( 0 )->GetBoundingBox().GetRight();

        for( int i = 1; i < selection.Size(); ++i )
        {
            int currentRight = selection.Item<BOARD_ITEM>( i )->GetBoundingBox().GetRight();

            if( right < currentRight )      // X increases when going right
                right = currentRight;
        }

        // Move the selected items
        for( int i = 0; i < selection.Size(); ++i )
        {
            BOARD_ITEM* item = selection.Item<BOARD_ITEM>( i );
            int difference = right - item->GetBoundingBox().GetRight();

            item->Move( wxPoint( difference, 0 ) );
            item->ViewUpdate();
            ratsnest->Update( item );
        }

        getModel<BOARD>()->GetRatsnest()->Recalculate();
    }

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

    if( selection.Size() > 1 )
    {
        PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
        RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

        // Prepare a list, so the items can be sorted by their X coordinate
        std::list<BOARD_ITEM*> itemsList;
        for( int i = 0; i < selection.Size(); ++i )
            itemsList.push_back( selection.Item<BOARD_ITEM>( i ) );

        // Sort items by X coordinate
        itemsList.sort( compareX );

        // Expected X coordinate for the next item (=minX)
        int position = (*itemsList.begin())->GetBoundingBox().Centre().x;

        // X coordinate for the last item
        const int maxX = (*itemsList.rbegin())->GetBoundingBox().Centre().x;

        // Distance between items
        const int distance = ( maxX - position ) / ( itemsList.size() - 1 );

        BOOST_FOREACH( BOARD_ITEM* item, itemsList )
        {
            int difference = position - item->GetBoundingBox().Centre().x;

            item->Move( wxPoint( difference, 0 ) );
            item->ViewUpdate();
            ratsnest->Update( item );

            position += distance;
        }

        getModel<BOARD>()->GetRatsnest()->Recalculate();
    }

    return 0;
}


int PLACEMENT_TOOL::DistributeVertically( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Size() > 1 )
    {
        PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
        RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

        editFrame->OnModify();
        editFrame->SaveCopyInUndoList( selection.items, UR_CHANGED );

        // Prepare a list, so the items can be sorted by their Y coordinate
        std::list<BOARD_ITEM*> itemsList;
        for( int i = 0; i < selection.Size(); ++i )
            itemsList.push_back( selection.Item<BOARD_ITEM>( i ) );

        // Sort items by Y coordinate
        itemsList.sort( compareY );

        // Expected Y coordinate for the next item (=minY)
        int position = (*itemsList.begin())->GetBoundingBox().Centre().y;

        // Y coordinate for the last item
        const int maxY = (*itemsList.rbegin())->GetBoundingBox().Centre().y;

        // Distance between items
        const int distance = ( maxY - position ) / ( itemsList.size() - 1 );

        BOOST_FOREACH( BOARD_ITEM* item, itemsList )
        {
            int difference = position - item->GetBoundingBox().Centre().y;

            item->Move( wxPoint( 0, difference ) );
            item->ViewUpdate();
            ratsnest->Update( item );

            position += distance;
        }

        getModel<BOARD>()->GetRatsnest()->Recalculate();
    }

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
