/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <general.h>
#include <protos.h>
#include <libeditframe.h>
#include <class_libentry.h>


void LIB_EDIT_FRAME::SaveCopyInUndoList( EDA_ITEM* ItemToCopy, int unused_flag )
{
    LIB_PART*          CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = new LIB_PART( * (LIB_PART*) ItemToCopy );

    // Clear current flags (which can be temporary set by a current edit command).
    CopyItem->ClearStatus();

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( CopyItem, UR_LIBEDIT );
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


void LIB_EDIT_FRAME::GetComponentFromRedoList( wxCommandEvent& event )
{
    if( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();

    LIB_PART* part = GetCurPart();

    ITEM_PICKER wrapper( part, UR_LIBEDIT );

    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList();

    wrapper = lastcmd->PopItem();

    part = (LIB_PART*) wrapper.GetItem();

    // Do not delete the previous part by calling SetCurPart( part )
    // which calls delete <previous part>.
    // <previous part> is now put in undo list and is owned by this list
    // Just set the current part to the part which come from the redo list
    m_my_part = part;

    if( !part )
        return;

    if( !m_aliasName.IsEmpty() && !part->HasAlias( m_aliasName ) )
        m_aliasName = part->GetName();

    m_drawItem = NULL;
    UpdateAliasSelectList();
    UpdatePartSelectList();
    SetShowDeMorgan( part->HasConversion() );
    DisplayLibInfos();
    DisplayCmpDoc();
    OnModify();
    m_canvas->Refresh();
}


void LIB_EDIT_FRAME::GetComponentFromUndoList( wxCommandEvent& event )
{
    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();

    LIB_PART*      part = GetCurPart();

    ITEM_PICKER wrapper( part, UR_LIBEDIT );

    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList();

    wrapper = lastcmd->PopItem();

    part = (LIB_PART*     ) wrapper.GetItem();

    // Do not delete the previous part by calling SetCurPart( part ),
    // which calls delete <previous part>.
    // <previous part> is now put in redo list and is owned by this list.
    // Just set the current part to the part which come from the undo list
    m_my_part = part;

    if( !part )
        return;

    if( !m_aliasName.IsEmpty() && !part->HasAlias( m_aliasName ) )
        m_aliasName = part->GetName();

    m_drawItem = NULL;
    UpdateAliasSelectList();
    UpdatePartSelectList();
    SetShowDeMorgan( part->HasConversion() );
    DisplayLibInfos();
    DisplayCmpDoc();
    OnModify();
    m_canvas->Refresh();
}
