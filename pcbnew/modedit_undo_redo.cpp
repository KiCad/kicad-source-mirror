/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include "fctsys.h"
#include "class_drawpanel.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"

#include "protos.h"


/**
 * Function SaveCopyInUndoList.
 * Creates a new entry in undo list of commands.
 * add a picker to handle aItemToCopy
 * @param aItem = the board item modified by the command to undo
 * @param aTypeCommand = command type (see enum UndoRedoOpType)
 * @param aTransformPoint = the reference point of the transformation, for commands like move
 */
void WinEDA_ModuleEditFrame::SaveCopyInUndoList( BOARD_ITEM*    aItem,
                                                 UndoRedoOpType aTypeCommand,
                                                 const wxPoint& aTransformPoint )
{
    EDA_ITEM*          item;
    MODULE*            CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = new MODULE( GetBoard() );
    CopyItem->Copy( (MODULE*) aItem );
    CopyItem->SetParent( GetBoard() );

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( CopyItem, UR_MODEDIT );
    lastcmd->PushItem( wrapper );

    GetScreen()->PushCommandToUndoList( lastcmd );
    /* Clear current flags (which can be temporary set by a current edit command) */
    for( item = CopyItem->m_Drawings; item != NULL; item = item->Next() )
        item->m_Flags = 0;

    /* Clear redo list, because after new save there is no redo to do */
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/**
 * Function SaveCopyInUndoList (overloaded).
 * Creates a new entry in undo list of commands.
 * add a list of pickers to handle a list of items
 * @param aItemsList = the list of items modified by the command to undo
 * @param aTypeCommand = command type (see enum UndoRedoOpType)
 * @param aTransformPoint = the reference point of the transformation, for commands like move
 */
void WinEDA_ModuleEditFrame::SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                                UndoRedoOpType aTypeCommand,
                                                const wxPoint& aTransformPoint )
{
    // Currently unused in modedit, because the module itself is saved for each change
    wxMessageBox( wxT( "SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList..) not yet in use" ) );
}


/********************************************************************************/
void WinEDA_ModuleEditFrame::GetComponentFromRedoList( wxCommandEvent& event )
/********************************************************************************/

/* Redo the last edition:
 *  - Place the current edited library component in undo list
 *  - Get old version of the current edited library component
 */
{
    if( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    // Save current module state in undo list
    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    MODULE * module = GetBoard()->m_Modules.PopFront();
    ITEM_PICKER        wrapper( module, UR_MODEDIT );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Retrieve last module state from undo list
    lastcmd = GetScreen()->PopCommandFromRedoList();
    wrapper = lastcmd->PopItem();
    module = (MODULE *)wrapper.m_PickedItem;
    delete lastcmd;

    if( module )
        GetBoard()->Add( module );

    SetCurItem( NULL );

    OnModify();
    DrawPanel->Refresh();
}


/***************************************************************************/
void WinEDA_ModuleEditFrame::GetComponentFromUndoList( wxCommandEvent& event )
/***************************************************************************/

/* Undo the last edition:
 *  - Place the current edited library component in Redo list
 *  - Get old version of the current edited library component
 */
{
    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    // Save current module state in redo list
    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    MODULE * module = GetBoard()->m_Modules.PopFront();
    ITEM_PICKER        wrapper( module, UR_MODEDIT );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToRedoList( lastcmd );

    // Retrieve last module state from undo list
    lastcmd = GetScreen()->PopCommandFromUndoList();
    wrapper = lastcmd->PopItem();
    module = (MODULE *)wrapper.m_PickedItem;
    delete lastcmd;

    if( module )
        GetBoard()->Add( module, ADD_APPEND );


    SetCurItem( NULL );

    OnModify();
    DrawPanel->Refresh();
}
