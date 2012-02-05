/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <protos.h>
#include <module_editor_frame.h>


void FOOTPRINT_EDIT_FRAME::SaveCopyInUndoList( BOARD_ITEM*    aItem,
                                               UNDO_REDO_T    aTypeCommand,
                                               const wxPoint& aTransformPoint )
{
    EDA_ITEM*          item;
    MODULE*            CopyItem;
    PICKED_ITEMS_LIST* lastcmd;

    CopyItem = new MODULE( *( (MODULE*) aItem ) );
    CopyItem->SetParent( GetBoard() );

    lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( CopyItem, UR_MODEDIT );
    lastcmd->PushItem( wrapper );

    GetScreen()->PushCommandToUndoList( lastcmd );
    /* Clear current flags (which can be temporary set by a current edit command) */
    for( item = CopyItem->m_Drawings; item != NULL; item = item->Next() )
        item->ClearFlags();

    /* Clear redo list, because after new save there is no redo to do */
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


void FOOTPRINT_EDIT_FRAME::SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList,
                                               UNDO_REDO_T        aTypeCommand,
                                               const wxPoint&     aTransformPoint )
{
    // Currently unused in modedit, because the module itself is saved for each change
    wxMessageBox( wxT( "SaveCopyInUndoList( PICKED_ITEMS_LIST& aItemsList..) not yet in use" ) );
}


void FOOTPRINT_EDIT_FRAME::GetComponentFromRedoList( wxCommandEvent& event )
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
    module = (MODULE *)wrapper.GetItem();
    delete lastcmd;

    if( module )
        GetBoard()->Add( module );

    SetCurItem( NULL );

    OnModify();
    m_canvas->Refresh();
}


void FOOTPRINT_EDIT_FRAME::GetComponentFromUndoList( wxCommandEvent& event )
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
    module = (MODULE *)wrapper.GetItem();
    delete lastcmd;

    if( module )
        GetBoard()->Add( module, ADD_APPEND );


    SetCurItem( NULL );

    OnModify();
    m_canvas->Refresh();
}
