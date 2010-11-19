/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "class_drawpanel.h"

#include "common.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"


void LIB_EDIT_FRAME::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy, int unused_flag )
{
    LIB_COMPONENT*     CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = new LIB_COMPONENT( *( (LIB_COMPONENT*) ItemToCopy ) );

    if( CopyItem == NULL )
        return;

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( CopyItem, UR_LIBEDIT );
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Clear current flags (which can be temporary set by a current edit command).
    CopyItem->ClearStatus();

    // Clear redo list, because after new save there is no redo to do.
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/* Redo the last edition:
 * - Place the current edited library component in undo list
 * - Get old version of the current edited library component
 */
void LIB_EDIT_FRAME::GetComponentFromRedoList( wxCommandEvent& event )
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( m_component, UR_LIBEDIT );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList();

    wrapper = lastcmd->PopItem();
    m_component = (LIB_COMPONENT*) wrapper.m_PickedItem;

    if( m_component == NULL )
        return;

    if( !m_aliasName.IsEmpty() && !m_component->HasAlias( m_aliasName ) )
        m_aliasName = m_component->GetName();

    m_drawItem = NULL;
    UpdateAliasSelectList();
    UpdatePartSelectList();
    SetShowDeMorgan( m_component->HasConversion() );
    DisplayLibInfos();
    DisplayCmpDoc();
    OnModify( );
    DrawPanel->Refresh();
}


/** Undo the last edition:
 * - Place the current edited library component in Redo list
 * - Get old version of the current edited library component
 */
void LIB_EDIT_FRAME::GetComponentFromUndoList( wxCommandEvent& event )
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( m_component, UR_LIBEDIT );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList();

    wrapper = lastcmd->PopItem();
    m_component = (LIB_COMPONENT*) wrapper.m_PickedItem;

    if( m_component == NULL )
        return;

    if( !m_aliasName.IsEmpty() && !m_component->HasAlias( m_aliasName ) )
        m_aliasName = m_component->GetName();

    m_drawItem = NULL;
    UpdateAliasSelectList();
    UpdatePartSelectList();
    SetShowDeMorgan( m_component->HasConversion() );
    DisplayLibInfos();
    DisplayCmpDoc();
    OnModify();
    DrawPanel->Refresh();
}
