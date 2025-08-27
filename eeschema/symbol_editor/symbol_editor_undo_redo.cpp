/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <symbol_edit_frame.h>
#include <lib_symbol_library_manager.h>
#include <widgets/lib_tree.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <tools/symbol_editor_drawing_tools.h>


void SYMBOL_EDIT_FRAME::PushSymbolToUndoList( const wxString& aDescription, LIB_SYMBOL* aSymbolCopy,
                                              UNDO_REDO aUndoType )
{
    if( !aSymbolCopy )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();

    // Clear current flags (which can be temporary set by a current edit command).
    aSymbolCopy->ClearTempFlags();
    aSymbolCopy->ClearEditFlags();
    aSymbolCopy->SetFlags( UR_TRANSIENT );

    ITEM_PICKER wrapper( GetScreen(), aSymbolCopy, aUndoType );
    lastcmd->PushItem( wrapper );
    lastcmd->SetDescription( aDescription );
    PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    ClearUndoORRedoList( REDO_LIST );
}


void SYMBOL_EDIT_FRAME::SaveCopyInUndoList( const wxString& aDescription, LIB_SYMBOL* aSymbol,
                                            UNDO_REDO aUndoType )
{
    if( aSymbol )
        PushSymbolToUndoList( aDescription, new LIB_SYMBOL( *aSymbol ), aUndoType );
}


void SYMBOL_EDIT_FRAME::GetSymbolFromRedoList()
{
    if( GetRedoCommandCount() <= 0 )
        return;

    // Load the last redo entry
    PICKED_ITEMS_LIST* redoCommand = PopCommandFromRedoList();
    ITEM_PICKER        redoWrapper = redoCommand->PopItem();
    wxString           description = redoCommand->GetDescription();

    delete redoCommand;

    LIB_SYMBOL* symbol = (LIB_SYMBOL*) redoWrapper.GetItem();
    UNDO_REDO   undoRedoType = redoWrapper.GetStatus();
    wxCHECK( symbol, /* void */ );
    symbol->ClearFlags( UR_TRANSIENT );

    // Store the current symbol in the undo buffer
    PICKED_ITEMS_LIST* undoCommand = new PICKED_ITEMS_LIST();
    LIB_SYMBOL*        oldSymbol = m_symbol;

    oldSymbol->SetFlags( UR_TRANSIENT );
    ITEM_PICKER undoWrapper( GetScreen(), oldSymbol, undoRedoType );
    undoCommand->SetDescription( description );
    undoCommand->PushItem( undoWrapper );
    PushCommandToUndoList( undoCommand );

    // Do not delete the previous symbol by calling SetCurSymbol( symbol )
    // which calls delete <previous symbol>.
    // <previous symbol> is now put in undo list and is owned by this list
    // Just set the current symbol to the symbol which come from the redo list
    m_symbol = symbol;

    if( undoRedoType == UNDO_REDO::LIB_RENAME )
    {
        wxString lib = GetCurLib();
        m_libMgr->UpdateSymbolAfterRename( symbol, oldSymbol->GetName(), lib );

        // Reselect the renamed symbol
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, symbol->GetName() ) );
    }

    RebuildSymbolUnitAndBodyStyleLists();
    UpdateTitle();

    RebuildView();
    OnModify();
}


void SYMBOL_EDIT_FRAME::GetSymbolFromUndoList()
{
    if( GetUndoCommandCount() <= 0 )
        return;

    // Load the last undo entry
    PICKED_ITEMS_LIST* undoCommand = PopCommandFromUndoList();
    wxString           description = undoCommand->GetDescription();
    ITEM_PICKER        undoWrapper = undoCommand->PopItem();

    delete undoCommand;

    LIB_SYMBOL* symbol = (LIB_SYMBOL*) undoWrapper.GetItem();
    UNDO_REDO   undoRedoType = undoWrapper.GetStatus();
    wxCHECK( symbol, /* void */ );
    symbol->ClearFlags( UR_TRANSIENT );

    // Store the current symbol in the redo buffer
    PICKED_ITEMS_LIST* redoCommand = new PICKED_ITEMS_LIST();
    LIB_SYMBOL*        oldSymbol = m_symbol;

    oldSymbol->SetFlags( UR_TRANSIENT );
    ITEM_PICKER redoWrapper( GetScreen(), oldSymbol, undoRedoType );
    redoCommand->PushItem( redoWrapper );
    redoCommand->SetDescription( description );
    PushCommandToRedoList( redoCommand );

    // Do not delete the previous symbol by calling SetCurSymbol( symbol ),
    // which calls delete <previous symbol>.
    // <previous symbol> is now put in redo list and is owned by this list.
    // Just set the current symbol to the symbol which come from the undo list
    m_symbol = symbol;

    if( undoRedoType == UNDO_REDO::LIB_RENAME )
    {
        wxString lib = GetCurLib();
        m_libMgr->UpdateSymbolAfterRename( symbol, oldSymbol->GetName(), lib );

        // Reselect the renamed symbol
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, symbol->GetName() ) );
    }

    RebuildSymbolUnitAndBodyStyleLists();
    UpdateTitle();

    RebuildView();
    OnModify();
}


