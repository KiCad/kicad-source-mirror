/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <sch_draw_panel.h>

#include <lib_edit_frame.h>
#include <class_libentry.h>
#include <lib_manager.h>
#include <widgets/lib_tree.h>
#include <symbol_tree_pane.h>


void LIB_EDIT_FRAME::SaveCopyInUndoList( EDA_ITEM* ItemToCopy, UNDO_REDO_T undoType )
{
    LIB_PART*          CopyItem;
    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();

    CopyItem = new LIB_PART( * (LIB_PART*) ItemToCopy );

    // Clear current flags (which can be temporary set by a current edit command).
    CopyItem->ClearStatus();
    CopyItem->SetFlags( CopyItem->GetFlags() | UR_TRANSIENT );

    ITEM_PICKER wrapper( CopyItem, undoType );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


void LIB_EDIT_FRAME::GetComponentFromRedoList( wxCommandEvent& event )
{
    if( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    // Load the last redo entry
    PICKED_ITEMS_LIST* redoCommand = GetScreen()->PopCommandFromRedoList();
    ITEM_PICKER redoWrapper = redoCommand->PopItem();
    delete redoCommand;
    LIB_PART* part = (LIB_PART*) redoWrapper.GetItem();
    wxCHECK( part, /* void */ );
    part->SetFlags( part->GetFlags() & ~UR_TRANSIENT );
    UNDO_REDO_T undoRedoType = redoWrapper.GetStatus();

    // Store the current part in the undo buffer
    PICKED_ITEMS_LIST* undoCommand = new PICKED_ITEMS_LIST();
    LIB_PART* oldPart = GetCurPart();
    oldPart->SetFlags( oldPart->GetFlags() | UR_TRANSIENT );
    ITEM_PICKER undoWrapper( oldPart, undoRedoType );
    undoCommand->PushItem( undoWrapper );
    GetScreen()->PushCommandToUndoList( undoCommand );

    // Do not delete the previous part by calling SetCurPart( part )
    // which calls delete <previous part>.
    // <previous part> is now put in undo list and is owned by this list
    // Just set the current part to the part which come from the redo list
    m_my_part = part;

    if( undoRedoType == UR_LIB_RENAME )
    {
        wxString lib = GetCurLib();
        m_libMgr->UpdatePartAfterRename( part, oldPart->GetName(), lib );

        // Reselect the renamed part
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, part->GetName() ) );
    }

    SetDrawItem( NULL );
    UpdatePartSelectList();
    SetShowDeMorgan( part->HasConversion() );
    updateTitle();
    DisplayCmpDoc();

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}


void LIB_EDIT_FRAME::GetComponentFromUndoList( wxCommandEvent& event )
{
    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    // Load the last undo entry
    PICKED_ITEMS_LIST* undoCommand = GetScreen()->PopCommandFromUndoList();
    ITEM_PICKER undoWrapper = undoCommand->PopItem();
    delete undoCommand;
    LIB_PART* part = (LIB_PART*) undoWrapper.GetItem();
    wxCHECK( part, /* void */ );
    part->SetFlags( part->GetFlags() & ~UR_TRANSIENT );
    UNDO_REDO_T undoRedoType = undoWrapper.GetStatus();

    // Store the current part in the redo buffer
    PICKED_ITEMS_LIST* redoCommand = new PICKED_ITEMS_LIST();
    LIB_PART* oldPart = GetCurPart();
    oldPart->SetFlags( oldPart->GetFlags() | UR_TRANSIENT );
    ITEM_PICKER redoWrapper( oldPart, undoRedoType );
    redoCommand->PushItem( redoWrapper );
    GetScreen()->PushCommandToRedoList( redoCommand );

    // Do not delete the previous part by calling SetCurPart( part ),
    // which calls delete <previous part>.
    // <previous part> is now put in redo list and is owned by this list.
    // Just set the current part to the part which come from the undo list
    m_my_part = part;

    if( undoRedoType == UR_LIB_RENAME )
    {
        wxString lib = GetCurLib();
        m_libMgr->UpdatePartAfterRename( part, oldPart->GetName(), lib );

        // Reselect the renamed part
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, part->GetName() ) );
    }

    SetDrawItem( NULL );
    UpdatePartSelectList();
    SetShowDeMorgan( part->HasConversion() );
    updateTitle();
    DisplayCmpDoc();

    RebuildView();
    GetCanvas()->Refresh();
    OnModify();
}
