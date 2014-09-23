/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

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
