/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>

#include "pl_editor_frame.h"
#include "tools/pl_selection_tool.h"

void PL_EDITOR_FRAME::SaveCopyInUndoList()
{
    PICKED_ITEMS_LIST*  lastcmd = new PICKED_ITEMS_LIST();
    DS_PROXY_UNDO_ITEM* copyItem = new DS_PROXY_UNDO_ITEM( this );
    ITEM_PICKER         wrapper( GetScreen(), copyItem, UNDO_REDO::LIBEDIT );

    lastcmd->PushItem( wrapper );
    PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    ClearUndoORRedoList( REDO_LIST );
}


/* Redo the last edit:
 * - Place the current edited layout in undo list
 * - Get previous version of the current edited layput
 */
void PL_EDITOR_FRAME::GetLayoutFromRedoList()
{
    PL_SELECTION_TOOL*  selTool = GetToolManager()->GetTool<PL_SELECTION_TOOL>();

    if ( GetRedoCommandCount() <= 0 )
        return;

    ITEM_PICKER         redoWrapper = PopCommandFromRedoList()->PopItem();
    DS_PROXY_UNDO_ITEM* redoItem = static_cast<DS_PROXY_UNDO_ITEM*>( redoWrapper.GetItem() );
    bool                pageSettingsAndTitleBlock = redoItem->Type() == WS_PROXY_UNDO_ITEM_PLUS_T;

    PICKED_ITEMS_LIST*  undoCmd = new PICKED_ITEMS_LIST();
    DS_PROXY_UNDO_ITEM* undoItem = new DS_PROXY_UNDO_ITEM( pageSettingsAndTitleBlock ? this : nullptr );
    ITEM_PICKER         undoWrapper( GetScreen(), undoItem );

    undoCmd->PushItem( undoWrapper );
    PushCommandToUndoList( undoCmd );

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

    if ( GetUndoCommandCount() <= 0 )
        return;

    ITEM_PICKER         undoWrapper = PopCommandFromUndoList()->PopItem();
    DS_PROXY_UNDO_ITEM* undoItem = static_cast<DS_PROXY_UNDO_ITEM*>( undoWrapper.GetItem() );
    bool                pageSettingsAndTitleBlock = undoItem->Type() == WS_PROXY_UNDO_ITEM_PLUS_T;

    PICKED_ITEMS_LIST*  redoCmd = new PICKED_ITEMS_LIST();
    DS_PROXY_UNDO_ITEM* redoItem = new DS_PROXY_UNDO_ITEM( pageSettingsAndTitleBlock ? this : nullptr );
    ITEM_PICKER         redoWrapper( GetScreen(), redoItem );

    redoCmd->PushItem( redoWrapper );
    PushCommandToRedoList( redoCmd );

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

    if ( GetUndoCommandCount() <= 0 )
        return;

    ITEM_PICKER         undoWrapper = PopCommandFromUndoList()->PopItem();
    DS_PROXY_UNDO_ITEM* undoItem = static_cast<DS_PROXY_UNDO_ITEM*>( undoWrapper.GetItem() );
    bool                pageSettingsAndTitleBlock = undoItem->Type() == WS_PROXY_UNDO_ITEM_PLUS_T;

    selTool->ClearSelection();
    undoItem->Restore( this, GetCanvas()->GetView() );
    selTool->RebuildSelection();

    delete undoItem;

    if( pageSettingsAndTitleBlock )
    {
        GetToolManager()->RunAction( ACTIONS::zoomFitScreen );
        HardRedraw();   // items based off of corners will need re-calculating
    }
    else
        GetCanvas()->Refresh();
}
