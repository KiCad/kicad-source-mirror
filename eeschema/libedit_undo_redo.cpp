/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "class_drawpanel.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
//#include "id.h"

#include "protos.h"


/*************************************************************************/
void WinEDA_LibeditFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                              int             unused_flag )
/*************************************************************************/
{
    EDA_BaseStruct*         item;
    EDA_LibComponentStruct* CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = CopyLibEntryStruct( (EDA_LibComponentStruct*) ItemToCopy );

    if( CopyItem == NULL )
        return;

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CopyItem, UR_LIBEDIT);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );
    /* Clear current flags (which can be temporary set by a current edit command) */
    for( item = CopyItem->m_Drawings; item != NULL; item = item->Next() )
        item->m_Flags = 0;

    /* Clear redo list, because after new save there is no redo to do */
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/******************************************************/
void WinEDA_LibeditFrame::GetComponentFromRedoList(wxCommandEvent& event)
/******************************************************/

/* Redo the last edition:
  * - Place the current edited library component in undo list
  * - Get old version of the current edited library component
 *  @return FALSE if nothing done, else true
 */
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CurrentLibEntry, UR_LIBEDIT);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList( );

    wrapper = lastcmd->PopItem();
    CurrentLibEntry = (EDA_LibComponentStruct*) wrapper.m_PickedItem;
    if( CurrentLibEntry )
        CurrentLibEntry->SetNext( NULL );
    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    DrawPanel->Refresh();
}


/******************************************************/
void WinEDA_LibeditFrame::GetComponentFromUndoList(wxCommandEvent& event)
/******************************************************/

/* Undo the last edition:
  * - Place the current edited library component in Redo list
  * - Get old version of the current edited library component
 *  @return FALSE if nothing done, else true
 */
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CurrentLibEntry, UR_LIBEDIT);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList( );

    wrapper = lastcmd->PopItem();
    CurrentLibEntry = (EDA_LibComponentStruct*) wrapper.m_PickedItem;

    if( CurrentLibEntry )
        CurrentLibEntry->SetNext( NULL );
    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    DrawPanel->Refresh();
}
