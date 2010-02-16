/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "class_drawpanel.h"

#include "common.h"
#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_libentry.h"


/*************************************************************************/
void WinEDA_LibeditFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                              int             unused_flag )
/*************************************************************************/
{
    LIB_COMPONENT*     CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = new LIB_COMPONENT( *( (LIB_COMPONENT*) ItemToCopy ) );

    if( CopyItem == NULL )
        return;

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CopyItem, UR_LIBEDIT);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );
    /* Clear current flags (which can be temporary set by a current edit
     * command) */
    CopyItem->ClearStatus();

    /* Clear redo list, because after new save there is no redo to do */
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/*************************************************************************/
void WinEDA_LibeditFrame::GetComponentFromRedoList(wxCommandEvent& event)
/*************************************************************************/

/* Redo the last edition:
  * - Place the current edited library component in undo list
  * - Get old version of the current edited library component
 */
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(m_component, UR_LIBEDIT);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList( );

    wrapper = lastcmd->PopItem();
    m_component = (LIB_COMPONENT*) wrapper.m_PickedItem;
    if( m_component )
        m_component->SetNext( NULL );
    m_drawItem = NULL;
    UpdateAliasSelectList();
    UpdatePartSelectList();
    if( m_component )
        SetShowDeMorgan( m_component->HasConversion() );
    DisplayLibInfos();
    DisplayCmpDoc();
    GetScreen()->SetModify();
    DrawPanel->Refresh();
}


/************************************************************************/
void WinEDA_LibeditFrame::GetComponentFromUndoList(wxCommandEvent& event)
/************************************************************************/

/** Undo the last edition:
  * - Place the current edited library component in Redo list
  * - Get old version of the current edited library component
 */
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(m_component, UR_LIBEDIT);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList( );

    wrapper = lastcmd->PopItem();
    m_component = (LIB_COMPONENT*) wrapper.m_PickedItem;

    if( m_component )
        m_component->SetNext( NULL );
    m_drawItem = NULL;
    UpdateAliasSelectList();
    UpdatePartSelectList();
    if( m_component )
        SetShowDeMorgan( m_component->HasConversion() );
    DisplayLibInfos();
    DisplayCmpDoc();
    GetScreen()->SetModify();
    DrawPanel->Refresh();
}
