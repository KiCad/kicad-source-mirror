/********************************************/
/*  library editor: undo and redo functions */
/********************************************/

#include <boost/bind.hpp>
#include <fctsys.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <tool/tool_manager.h>
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
    for( item = CopyItem->GraphicalItems(); item != NULL; item = item->Next() )
        item->ClearFlags();

    for( D_PAD* pad = CopyItem->Pads();  pad;  pad = pad->Next() )
        pad->ClearFlags();

    CopyItem->Reference().ClearFlags();
    CopyItem->Value().ClearFlags();

    /* Clear redo list, because after new save there is no redo to do */
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


void FOOTPRINT_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                               UNDO_REDO_T        aTypeCommand,
                                               const wxPoint&     aTransformPoint )
{
    assert( aItemsList.GetPickedItem( 0 )->GetParent()->Type() == PCB_MODULE_T );
    MODULE* owner = static_cast<MODULE*>( aItemsList.GetPickedItem( 0 )->GetParent() );

#ifndef NDEBUG
    // All items should have the same parent (MODULE) to make undo/redo entry valid
    for( unsigned int i = 0; i < aItemsList.GetCount(); ++i )
        assert( aItemsList.GetPickedItem( i )->GetParent() == owner );
#endif /* not NDEBUG */

    SaveCopyInUndoList( owner, aTypeCommand, aTransformPoint );
}


void FOOTPRINT_EDIT_FRAME::RestoreCopyFromRedoList( wxCommandEvent& aEvent )
{
    if( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    // Inform tools that undo command was issued
    TOOL_EVENT event( TC_MESSAGE, TA_UNDO_REDO, AS_GLOBAL );
    m_toolManager->ProcessEvent( event );

    // Save current module state in undo list
    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    MODULE* module = GetBoard()->m_Modules.PopFront();
    ITEM_PICKER wrapper( module, UR_MODEDIT );
    KIGFX::VIEW* view = GetGalCanvas()->GetView();
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    view->Remove( module );
    module->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, view, _1 ) );

    // Retrieve last module state from undo list
    lastcmd = GetScreen()->PopCommandFromRedoList();
    wrapper = lastcmd->PopItem();
    module = (MODULE*) wrapper.GetItem();
    delete lastcmd;

    if( module )
    {
        GetBoard()->Add( module );
        GetGalCanvas()->GetView()->Add( module );
        module->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, view, _1 ) );
        module->ViewUpdate();
    }

    SetCurItem( NULL );

    OnModify();
    m_canvas->Refresh();
}


void FOOTPRINT_EDIT_FRAME::RestoreCopyFromUndoList( wxCommandEvent& aEvent )
{
    if( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    // Inform tools that undo command was issued
    TOOL_EVENT event( TC_MESSAGE, TA_UNDO_REDO, AS_GLOBAL );
    m_toolManager->ProcessEvent( event );

    // Save current module state in redo list
    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    MODULE* module = GetBoard()->m_Modules.PopFront();
    ITEM_PICKER wrapper( module, UR_MODEDIT );
    KIGFX::VIEW* view = GetGalCanvas()->GetView();
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToRedoList( lastcmd );

    view->Remove( module );
    module->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, view, _1 ) );

    // Retrieve last module state from undo list
    lastcmd = GetScreen()->PopCommandFromUndoList();
    wrapper = lastcmd->PopItem();
    module = (MODULE*) wrapper.GetItem();
    delete lastcmd;

    if( module )
    {
        GetBoard()->Add( module, ADD_APPEND );
        view->Add( module );
        module->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, view, _1 ) );
        module->ViewUpdate();
    }

    SetCurItem( NULL );

    OnModify();
    m_canvas->Refresh();
}
