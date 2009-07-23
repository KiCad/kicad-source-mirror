/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "id.h"

#include "protos.h"


/*************************************************************************/
void WinEDA_LibeditFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                              int             unused_flag )
/*************************************************************************/
{
    EDA_BaseStruct*         item;
    EDA_LibComponentStruct* CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = CopyLibEntryStruct( this, (EDA_LibComponentStruct*) ItemToCopy );

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CopyItem);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );
    /* Clear current flags (which can be temporary set by a current edit command) */
    for( item = CopyItem->m_Drawings; item != NULL; item = item->Next() )
        item->m_Flags = 0;

    /* Clear redo list, because after new save there is no redo to do */
    while( (lastcmd = GetScreen()->PopCommandFromRedoList( ) ) != NULL )
    {
        while ( 1 )
        {
            wrapper = lastcmd->PopItem();
            if ( wrapper.m_Item == NULL )
                break;      // All items are removed
            delete wrapper.m_Item;
        }
        delete lastcmd;
    }
}


/******************************************************/
bool WinEDA_LibeditFrame::GetComponentFromRedoList()
/******************************************************/

/* Redo the last edition:
  * - Place the current edited library component in undo list
  * - Get old version of the current edited library component
 *  @return FALSE if nothing done, else true
 */
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return false;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CurrentLibEntry);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList( );

    wrapper = lastcmd->PopItem();
    CurrentLibEntry = (EDA_LibComponentStruct*) wrapper.m_Item;
    if( CurrentLibEntry )
        CurrentLibEntry->SetNext( NULL );
    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();

    return true;
}


/******************************************************/
bool WinEDA_LibeditFrame::GetComponentFromUndoList()
/******************************************************/

/* Undo the last edition:
  * - Place the current edited library component in Redo list
  * - Get old version of the current edited library component
 *  @return FALSE if nothing done, else true
 */
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return false;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(CurrentLibEntry);
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList( );

    wrapper = lastcmd->PopItem();
    CurrentLibEntry = (EDA_LibComponentStruct*) wrapper.m_Item;

    if( CurrentLibEntry )
        CurrentLibEntry->SetNext( NULL );
    CurrentDrawItem = NULL;
    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();

    return true;
}
