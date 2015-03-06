/**
 * @file pl_editor_undo_redo.cpp
 * @brief page layout editor: undo and redo functions
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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
#include <class_drawpanel.h>
#include <macros.h>
#include <worksheet_shape_builder.h>

#include <pl_editor_frame.h>

/* Note: the Undo/redo commands use a "brute" method:
 * the full page layout is converted to a S expression, and saved as string.
 * When a previous version is needed, the old string is parsed,
 * and the description replaces the current desc, just like reading a new file
 *
 * This is not optimal from the memory point of view, but:
 * - the descriptions are never very long (max few thousand of bytes)
 * - this is very easy to code
 */

// A helper class used in undo/redo commad:
class PL_ITEM_LAYOUT: public EDA_ITEM
{
public:
    wxString m_Layout;

public:
    PL_ITEM_LAYOUT() : EDA_ITEM( TYPE_PL_EDITOR_LAYOUT ) {}

    // Required to keep compiler happy on debug builds.
#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const {}
#endif

    /** Get class name
     * @return  string "PL_ITEM_LAYOUT"
     */
    virtual wxString GetClass() const
    {
        return wxT( "PL_ITEM_LAYOUT" );
    }
};

void PL_EDITOR_FRAME::SaveCopyInUndoList()
{
    PL_ITEM_LAYOUT* copyItem = new PL_ITEM_LAYOUT;
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    pglayout.SaveInString( copyItem->m_Layout );

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( copyItem, UR_LIBEDIT );
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/* Redo the last edition:
 * - Place the current edited layout in undo list
 * - Get previous version of the current edited layput
 */
void PL_EDITOR_FRAME::GetLayoutFromRedoList( wxCommandEvent& event )
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    PL_ITEM_LAYOUT* copyItem = new PL_ITEM_LAYOUT;
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    pglayout.SaveInString( copyItem->m_Layout );

    ITEM_PICKER wrapper( copyItem, UR_LIBEDIT );

    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList();

    wrapper = lastcmd->PopItem();
    copyItem = (PL_ITEM_LAYOUT*)wrapper.GetItem();
    pglayout.SetPageLayout( TO_UTF8(copyItem->m_Layout) );
    delete copyItem;

    OnModify();
    RebuildDesignTree();
    m_canvas->Refresh();
}


/* Undo the last edition:
 * - Place the current layout in Redo list
 * - Get previous version of the current edited layout
 */
void PL_EDITOR_FRAME::GetLayoutFromUndoList( wxCommandEvent& event )
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    PL_ITEM_LAYOUT* copyItem = new PL_ITEM_LAYOUT;
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    pglayout.SaveInString( copyItem->m_Layout );

    ITEM_PICKER wrapper( copyItem, UR_LIBEDIT );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList();

    wrapper = lastcmd->PopItem();
    copyItem = (PL_ITEM_LAYOUT*)wrapper.GetItem();
    pglayout.SetPageLayout( TO_UTF8(copyItem->m_Layout) );
    delete copyItem;

    OnModify();
    RebuildDesignTree();
    m_canvas->Refresh();
}

/* Remove the last command in Undo List.
 * Used to clean the uUndo stack after a cancel command
 */
void PL_EDITOR_FRAME::RemoveLastCommandInUndoList()
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = GetScreen()->PopCommandFromUndoList();

    ITEM_PICKER wrapper = lastcmd->PopItem();
    PL_ITEM_LAYOUT* copyItem = (PL_ITEM_LAYOUT*)wrapper.GetItem();
    delete copyItem;
}
