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

    CopyItem = new MODULE( GetBoard() );
    CopyItem->Copy( (MODULE*) ItemToCopy );
    CopyItem->SetParent( GetBoard() );

    GetScreen()->AddItemToUndoList( (EDA_BaseStruct*) CopyItem );
    /* Clear current flags (which can be temporary set by a current edit command) */
    for( item = CopyItem->m_Drawings; item != NULL; item = item->Next() )
        item->m_Flags = 0;

    /* Clear redo list, because after new save there is no redo to do */
    while( GetScreen()->m_RedoList )
    {
        item = GetScreen()->m_RedoList->Next();
        delete GetScreen()->m_RedoList;
        GetScreen()->m_RedoList = item;
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
    if( GetScreen()->m_RedoList == NULL )
        return;

    GetScreen()->AddItemToUndoList( GetBoard()->m_Modules.PopFront() );

    GetBoard()->Add( (MODULE*) GetScreen()->GetItemFromRedoList() );

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
    if( GetScreen()->m_UndoList == NULL )
        return;

    GetScreen()->AddItemToRedoList( GetBoard()->m_Modules.PopFront() );

    MODULE* module = (MODULE*) GetScreen()->GetItemFromUndoList();
    if( module )
        GetBoard()->Add( module, ADD_APPEND );

/*  Add() calls PushBack(), no need for this
    if( GetBoard()->m_Modules )
        GetBoard()->m_Modules->SetNext( NULL );
*/

    GetScreen()->SetModify();
    SetCurItem( NULL );;
    ReCreateHToolbar();
    SetToolbars();
}
