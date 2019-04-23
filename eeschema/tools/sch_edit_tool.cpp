/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_selection_tool.h>
#include <sch_actions.h>
#include <hotkeys.h>
#include <bitmaps.h>
#include <sch_item_struct.h>
#include <sch_view.h>
#include <sch_item_struct.h>
#include <sch_edit_frame.h>


TOOL_ACTION SCH_ACTIONS::editActivate( "eeschema.InteractiveEdit",
        AS_GLOBAL, 0,
        _( "Edit Activate" ), "", move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::move( "eeschema.InteractiveEdit.move",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE_COMPONENT_OR_ITEM ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::duplicate( "eeschema.InteractiveEdit.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE_ITEM ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ), duplicate_xpm );

TOOL_ACTION SCH_ACTIONS::rotate( "eeschema.InteractiveEdit.rotate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ROTATE ),
        _( "Rotate" ), _( "Rotates selected item(s)" ),
        rotate_ccw_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::properties( "eeschema.InteractiveEdit.properties",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT ),
        _( "Properties..." ), _( "Displays item properties dialog" ), config_xpm );

TOOL_ACTION SCH_ACTIONS::remove( "eeschema.InteractiveEdit.remove",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DELETE ),
        _( "Delete" ), _( "Deletes selected item(s)" ), delete_xpm, AF_NONE );


SCH_EDIT_TOOL::SCH_EDIT_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveEdit" ),
        m_view( nullptr ),
        m_controls( nullptr ),
        m_frame( nullptr ),
        m_menu( *this )
{
};


SCH_EDIT_TOOL::~SCH_EDIT_TOOL()
{
}


bool SCH_EDIT_TOOL::Init()
{
    auto activeToolFunctor = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolFunctor, 1 );
    ctxMenu.AddSeparator( activeToolFunctor, 1 );

    return true;
}


void SCH_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    m_controls = getViewControls();
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
}


int SCH_EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL*    selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    std::vector<SCH_ITEM*> lockedItems;

    // get a copy instead of reference (as we're going to clear the selection before removing items)
    SELECTION selectionCopy = selTool->RequestSelection();

    if( selectionCopy.Empty() )
        return 0;

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    for( EDA_ITEM* it : selectionCopy )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( it );
        bool      itemHasConnections = item->IsConnectable();

        m_frame->GetScreen()->SetCurItem( nullptr );
        m_frame->SetRepeatItem( nullptr );
        m_frame->DeleteItem( item );

        if( itemHasConnections )
            m_frame->TestDanglingEnds();
    }

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


void SCH_EDIT_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TOOL::Remove,                SCH_ACTIONS::remove.MakeEvent() );
}
