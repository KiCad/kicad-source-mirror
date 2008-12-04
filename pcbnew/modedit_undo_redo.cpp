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

    CopyItem = new MODULE( m_Pcb );
    CopyItem->Copy( (MODULE*) ItemToCopy );
    CopyItem->SetParent( m_Pcb );

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

    GetScreen()->AddItemToUndoList( m_Pcb->m_Modules.PopFront() );

    m_Pcb->Add( (MODULE*) GetScreen()->GetItemFromRedoList() );

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

    GetScreen()->AddItemToRedoList( m_Pcb->m_Modules.PopFront() );

    m_Pcb->Add( (MODULE*) GetScreen()->GetItemFromUndoList() );

    if( m_Pcb->m_Modules )
        m_Pcb->m_Modules->SetNext( NULL );
    GetScreen()->SetModify();
    SetCurItem( NULL );;
    ReCreateHToolbar();
    SetToolbars();
}
