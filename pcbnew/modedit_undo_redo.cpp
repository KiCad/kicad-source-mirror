/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "id.h"

#include "protos.h"


/**************************************************************************/
void WinEDA_ModuleEditFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                                 int             unused_flag )
/************************************************************************/
{
    EDA_BaseStruct* item;
    MODULE*         CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = new MODULE( GetBoard() );
    CopyItem->Copy( (MODULE*) ItemToCopy );
    CopyItem->SetParent( GetBoard() );

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


/*********************************************************/
void WinEDA_ModuleEditFrame::GetComponentFromRedoList()
/*********************************************************/

/* Redo the last edition:
 *  - Place the current edited library component in undo list
 *  - Get old version of the current edited library component
 */
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( GetBoard()->m_Modules.PopFront() );
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList( );

    wrapper = lastcmd->PopItem();

    GetBoard()->Add( (MODULE*)  wrapper.m_Item );

    SetCurItem( NULL );;
    GetScreen()->SetModify();
    ReCreateHToolbar();
    SetToolbars();
}


/*********************************************************/
void WinEDA_ModuleEditFrame::GetComponentFromUndoList()
/*********************************************************/

/* Undo the last edition:
 *  - Place the current edited library component in Redo list
 *  - Get old version of the current edited library component
 */
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper(GetBoard()->m_Modules.PopFront());
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList( );

    wrapper = lastcmd->PopItem();

    if( wrapper.m_Item )
        GetBoard()->Add( (MODULE*) wrapper.m_Item, ADD_APPEND );


    GetScreen()->SetModify();
    SetCurItem( NULL );;
    ReCreateHToolbar();
    SetToolbars();
}
