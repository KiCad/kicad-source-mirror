/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html,
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>

#include <bitmaps.h>
#include <sch_actions.h>
#include <sch_collectors.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_item.h>
#include <sch_selection.h>
#include <sch_selection_tool.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/tool_event.h>
#include <tools/ee_grid_helper.h>
#include <tools/sch_align_tool.h>
#include <view/view_controls.h>


SCH_ALIGN_TOOL::SCH_ALIGN_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.Align" ),
        m_alignMenu( nullptr )
{
}


SCH_ALIGN_TOOL::~SCH_ALIGN_TOOL()
{
    delete m_alignMenu;
}


bool SCH_ALIGN_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    if( !m_alignMenu )
    {
        m_alignMenu = new CONDITIONAL_MENU( this );
        m_alignMenu->SetIcon( BITMAPS::align_items );
        m_alignMenu->SetUntranslatedTitle( _HKI( "Align" ) );

        const auto canAlign = SELECTION_CONDITIONS::MoreThan( 1 );

        m_alignMenu->AddItem( SCH_ACTIONS::alignLeft, canAlign );
        m_alignMenu->AddItem( SCH_ACTIONS::alignCenterX, canAlign );
        m_alignMenu->AddItem( SCH_ACTIONS::alignRight, canAlign );

        m_alignMenu->AddSeparator( canAlign );
        m_alignMenu->AddItem( SCH_ACTIONS::alignTop, canAlign );
        m_alignMenu->AddItem( SCH_ACTIONS::alignCenterY, canAlign );
        m_alignMenu->AddItem( SCH_ACTIONS::alignBottom, canAlign );
    }

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_alignMenu, SELECTION_CONDITIONS::MoreThan( 1 ), 100 );

    setTransitions();

    return true;
}


template< typename T >
int SCH_ALIGN_TOOL::selectTarget( const std::vector<ITEM_BOX>& aItems,
                                  const std::vector<ITEM_BOX>& aLocked, T aGetValue )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    if( !aLocked.empty() )
    {
        for( const ITEM_BOX& item : aLocked )
        {
            if( item.second.Contains( cursorPos ) )
                return aGetValue( item );
        }

        return aGetValue( aLocked.front() );
    }

    for( const ITEM_BOX& item : aItems )
    {
        if( item.second.Contains( cursorPos ) )
            return aGetValue( item );
    }

    return aGetValue( aItems.front() );
}


template< typename T >
size_t SCH_ALIGN_TOOL::GetSelections( std::vector<ITEM_BOX>& aItemsToAlign,
                                      std::vector<ITEM_BOX>& aLockedItems, T aCompare )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::MovableItems );

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsSCH_ITEM() )
            continue;

        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( item );

        if( schItem->GetParent() && schItem->GetParent()->IsSelected() )
            continue;

        BOX2I bbox = schItem->GetBoundingBox();

        if( schItem->IsLocked() )
            aLockedItems.emplace_back( schItem, bbox );
        else
            aItemsToAlign.emplace_back( schItem, bbox );
    }

    std::sort( aItemsToAlign.begin(), aItemsToAlign.end(), aCompare );
    std::sort( aLockedItems.begin(), aLockedItems.end(), aCompare );

    return aItemsToAlign.size();
}


void SCH_ALIGN_TOOL::moveItem( SCH_ITEM* aItem, const VECTOR2I& aDelta, SCH_COMMIT& aCommit )
{
    if( aDelta == VECTOR2I( 0, 0 ) )
        return;

    VECTOR2I delta = adjustDeltaForGrid( aItem, aDelta );

    if( delta == VECTOR2I( 0, 0 ) )
        return;

    aCommit.Modify( aItem, m_frame->GetScreen(), RECURSE_MODE::RECURSE );
    aItem->Move( delta );
    aItem->ClearFlags( IS_MOVING );
    updateItem( aItem, true );
}


VECTOR2I SCH_ALIGN_TOOL::adjustDeltaForGrid( SCH_ITEM* aItem, const VECTOR2I& aDelta )
{
    if( aDelta == VECTOR2I( 0, 0 ) )
        return aDelta;

    EE_GRID_HELPER grid( m_toolMgr );
    GRID_HELPER_GRIDS gridType = grid.GetItemGrid( aItem );

    if( gridType != GRID_CONNECTABLE )
        return aDelta;

    VECTOR2I desiredPos = aItem->GetPosition() + aDelta;
    VECTOR2I snappedPos = grid.AlignGrid( desiredPos, gridType );

    return snappedPos - aItem->GetPosition();
}


void SCH_ALIGN_TOOL::setTransitions()
{
    Go( &SCH_ALIGN_TOOL::AlignTop,      SCH_ACTIONS::alignTop.MakeEvent() );
    Go( &SCH_ALIGN_TOOL::AlignBottom,   SCH_ACTIONS::alignBottom.MakeEvent() );
    Go( &SCH_ALIGN_TOOL::AlignLeft,     SCH_ACTIONS::alignLeft.MakeEvent() );
    Go( &SCH_ALIGN_TOOL::AlignRight,    SCH_ACTIONS::alignRight.MakeEvent() );
    Go( &SCH_ALIGN_TOOL::AlignCenterX,  SCH_ACTIONS::alignCenterX.MakeEvent() );
    Go( &SCH_ALIGN_TOOL::AlignCenterY,  SCH_ACTIONS::alignCenterY.MakeEvent() );
}


int SCH_ALIGN_TOOL::AlignTop( const TOOL_EVENT& aEvent )
{
    std::vector<ITEM_BOX> itemsToAlign;
    std::vector<ITEM_BOX> lockedItems;

    if( !GetSelections( itemsToAlign, lockedItems,
            []( const ITEM_BOX& lhs, const ITEM_BOX& rhs )
            {
                return lhs.second.GetTop() < rhs.second.GetTop();
            } ) )
    {
        return 0;
    }

    SCH_COMMIT commit( m_toolMgr );

    int targetTop = selectTarget( itemsToAlign, lockedItems,
            []( const ITEM_BOX& item )
            {
                return item.second.GetTop();
            } );

    for( const ITEM_BOX& item : itemsToAlign )
    {
        int difference = targetTop - item.second.GetTop();
        moveItem( item.first, VECTOR2I( 0, difference ), commit );
    }

    commit.Push( _( "Align to Top" ) );
    return 0;
}


int SCH_ALIGN_TOOL::AlignBottom( const TOOL_EVENT& aEvent )
{
    std::vector<ITEM_BOX> itemsToAlign;
    std::vector<ITEM_BOX> lockedItems;

    if( !GetSelections( itemsToAlign, lockedItems,
            []( const ITEM_BOX& lhs, const ITEM_BOX& rhs )
            {
                return lhs.second.GetBottom() > rhs.second.GetBottom();
            } ) )
    {
        return 0;
    }

    SCH_COMMIT commit( m_toolMgr );

    int targetBottom = selectTarget( itemsToAlign, lockedItems,
            []( const ITEM_BOX& item )
            {
                return item.second.GetBottom();
            } );

    for( const ITEM_BOX& item : itemsToAlign )
    {
        int difference = targetBottom - item.second.GetBottom();
        moveItem( item.first, VECTOR2I( 0, difference ), commit );
    }

    commit.Push( _( "Align to Bottom" ) );
    return 0;
}


int SCH_ALIGN_TOOL::AlignLeft( const TOOL_EVENT& aEvent )
{
    std::vector<ITEM_BOX> itemsToAlign;
    std::vector<ITEM_BOX> lockedItems;

    if( !GetSelections( itemsToAlign, lockedItems,
            []( const ITEM_BOX& lhs, const ITEM_BOX& rhs )
            {
                return lhs.second.GetLeft() < rhs.second.GetLeft();
            } ) )
    {
        return 0;
    }

    SCH_COMMIT commit( m_toolMgr );

    int targetLeft = selectTarget( itemsToAlign, lockedItems,
            []( const ITEM_BOX& item )
            {
                return item.second.GetLeft();
            } );

    for( const ITEM_BOX& item : itemsToAlign )
    {
        int difference = targetLeft - item.second.GetLeft();
        moveItem( item.first, VECTOR2I( difference, 0 ), commit );
    }

    commit.Push( _( "Align to Left" ) );
    return 0;
}


int SCH_ALIGN_TOOL::AlignRight( const TOOL_EVENT& aEvent )
{
    std::vector<ITEM_BOX> itemsToAlign;
    std::vector<ITEM_BOX> lockedItems;

    if( !GetSelections( itemsToAlign, lockedItems,
            []( const ITEM_BOX& lhs, const ITEM_BOX& rhs )
            {
                return lhs.second.GetRight() > rhs.second.GetRight();
            } ) )
    {
        return 0;
    }

    SCH_COMMIT commit( m_toolMgr );

    int targetRight = selectTarget( itemsToAlign, lockedItems,
            []( const ITEM_BOX& item )
            {
                return item.second.GetRight();
            } );

    for( const ITEM_BOX& item : itemsToAlign )
    {
        int difference = targetRight - item.second.GetRight();
        moveItem( item.first, VECTOR2I( difference, 0 ), commit );
    }

    commit.Push( _( "Align to Right" ) );
    return 0;
}


int SCH_ALIGN_TOOL::AlignCenterX( const TOOL_EVENT& aEvent )
{
    std::vector<ITEM_BOX> itemsToAlign;
    std::vector<ITEM_BOX> lockedItems;

    if( !GetSelections( itemsToAlign, lockedItems,
            []( const ITEM_BOX& lhs, const ITEM_BOX& rhs )
            {
                return lhs.second.Centre().x < rhs.second.Centre().x;
            } ) )
    {
        return 0;
    }

    SCH_COMMIT commit( m_toolMgr );

    int targetX = selectTarget( itemsToAlign, lockedItems,
            []( const ITEM_BOX& item )
            {
                return item.second.Centre().x;
            } );

    for( const ITEM_BOX& item : itemsToAlign )
    {
        int difference = targetX - item.second.Centre().x;
        moveItem( item.first, VECTOR2I( difference, 0 ), commit );
    }

    commit.Push( _( "Align to Middle" ) );
    return 0;
}


int SCH_ALIGN_TOOL::AlignCenterY( const TOOL_EVENT& aEvent )
{
    std::vector<ITEM_BOX> itemsToAlign;
    std::vector<ITEM_BOX> lockedItems;

    if( !GetSelections( itemsToAlign, lockedItems,
            []( const ITEM_BOX& lhs, const ITEM_BOX& rhs )
            {
                return lhs.second.Centre().y < rhs.second.Centre().y;
            } ) )
    {
        return 0;
    }

    SCH_COMMIT commit( m_toolMgr );

    int targetY = selectTarget( itemsToAlign, lockedItems,
            []( const ITEM_BOX& item )
            {
                return item.second.Centre().y;
            } );

    for( const ITEM_BOX& item : itemsToAlign )
    {
        int difference = targetY - item.second.Centre().y;
        moveItem( item.first, VECTOR2I( 0, difference ), commit );
    }

    commit.Push( _( "Align to Center" ) );
    return 0;
}

