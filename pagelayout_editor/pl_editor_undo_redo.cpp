/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <macros.h>
#include <ws_data_model.h>
#include <ws_draw_item.h>

#include <pl_editor_frame.h>
#include <tool/tool_manager.h>
#include <tools/pl_selection_tool.h>
#include <ws_proxy_undo_item.h>
#include <tool/actions.h>

void PL_EDITOR_FRAME::SaveCopyInUndoList( bool aSavePageSettingsAndTitleBlock )
{
    PICKED_ITEMS_LIST*  lastcmd = new PICKED_ITEMS_LIST();
    WS_PROXY_UNDO_ITEM* copyItem = new WS_PROXY_UNDO_ITEM( this );
    ITEM_PICKER         wrapper( copyItem, UR_LIBEDIT );

    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/* Redo the last edit:
 * - Place the current edited layout in undo list
 * - Get previous version of the current edited layput
 */
void PL_EDITOR_FRAME::GetLayoutFromRedoList()
{
    PL_SELECTION_TOOL*  selTool = GetToolManager()->GetTool<PL_SELECTION_TOOL>();

    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    ITEM_PICKER         redoWrapper = GetScreen()->PopCommandFromRedoList()->PopItem();
    WS_PROXY_UNDO_ITEM* redoItem = static_cast<WS_PROXY_UNDO_ITEM*>( redoWrapper.GetItem() );
    bool                pageSettingsAndTitleBlock = redoItem->Type() == WS_PROXY_UNDO_ITEM_PLUS_T;

    PICKED_ITEMS_LIST*  undoCmd = new PICKED_ITEMS_LIST();

    undoCmd->PushItem( new WS_PROXY_UNDO_ITEM( pageSettingsAndTitleBlock ? this : nullptr ) );
    GetScreen()->PushCommandToUndoList( undoCmd );

    selTool->ClearSelection();
    redoItem->Restore( this, GetCanvas()->GetView() );
    selTool->RebuildSelection();

    delete redoItem;

    if( pageSettingsAndTitleBlock )
        HardRedraw();   // items based off of corners will need re-calculating
    else
        GetCanvas()->Refresh();

    OnModify();
}


/* Undo the last edit:
 * - Place the current layout in Redo list
 * - Get previous version of the current edited layout
 */
void PL_EDITOR_FRAME::GetLayoutFromUndoList()
{
    PL_SELECTION_TOOL*  selTool = GetToolManager()->GetTool<PL_SELECTION_TOOL>();

    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    ITEM_PICKER         undoWrapper = GetScreen()->PopCommandFromUndoList()->PopItem();
    WS_PROXY_UNDO_ITEM* undoItem = static_cast<WS_PROXY_UNDO_ITEM*>( undoWrapper.GetItem() );
    bool                pageSettingsAndTitleBlock = undoItem->Type() == WS_PROXY_UNDO_ITEM_PLUS_T;

    PICKED_ITEMS_LIST*  redoCmd = new PICKED_ITEMS_LIST();

    redoCmd->PushItem( new WS_PROXY_UNDO_ITEM( pageSettingsAndTitleBlock ? this : nullptr ) );
    GetScreen()->PushCommandToRedoList( redoCmd );

    selTool->ClearSelection();
    undoItem->Restore( this, GetCanvas()->GetView() );
    selTool->RebuildSelection();

    delete undoItem;

    if( pageSettingsAndTitleBlock )
        HardRedraw();   // items based off of corners will need re-calculating
    else
        GetCanvas()->Refresh();

    OnModify();
}


/* Remove the last command in Undo List.
 * Used to clean the uUndo stack after a cancel command
 */
void PL_EDITOR_FRAME::RollbackFromUndo()
{
    PL_SELECTION_TOOL*  selTool = GetToolManager()->GetTool<PL_SELECTION_TOOL>();

    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    ITEM_PICKER         undoWrapper = GetScreen()->PopCommandFromUndoList()->PopItem();
    WS_PROXY_UNDO_ITEM* undoItem = static_cast<WS_PROXY_UNDO_ITEM*>( undoWrapper.GetItem() );
    bool                pageSettingsAndTitleBlock = undoItem->Type() == WS_PROXY_UNDO_ITEM_PLUS_T;

    selTool->ClearSelection();
    undoItem->Restore( this, GetCanvas()->GetView() );
    selTool->RebuildSelection();

    delete undoItem;

    if( pageSettingsAndTitleBlock )
    {
        GetToolManager()->RunAction( ACTIONS::zoomFitScreen, true );
        HardRedraw();   // items based off of corners will need re-calculating
    }
    else
        GetCanvas()->Refresh();
}
