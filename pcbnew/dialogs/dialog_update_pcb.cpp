#include <common.h>
#include <wxPcbStruct.h>
#include <pcb_netlist.h>
#include <dialog_update_pcb.h>
#include <wx_html_report_panel.h>
#include <board_netlist_updater.h>
#include <tool/tool_manager.h>
#include <tools/common_actions.h>
#include <class_draw_panel_gal.h>
#include <class_board.h>
#include <ratsnest_data.h>

#include <boost/bind.hpp>

DIALOG_UPDATE_PCB::DIALOG_UPDATE_PCB( PCB_EDIT_FRAME* aParent, NETLIST *aNetlist ) :
    DIALOG_UPDATE_PCB_BASE ( aParent ),
    m_frame (aParent),
    m_netlist (aNetlist)
{
    m_messagePanel->SetLabel( _("Changes to be applied:") );
    m_messagePanel->SetLazyUpdate ( true );
    m_netlist->SortByReference();
    m_btnPerformUpdate->SetFocus();

    m_messagePanel->SetVisibleSeverities( REPORTER::RPT_WARNING | REPORTER::RPT_ERROR | REPORTER::RPT_ACTION );
}

DIALOG_UPDATE_PCB::~DIALOG_UPDATE_PCB()
{

}

void DIALOG_UPDATE_PCB::PerformUpdate( bool aDryRun )
{
    m_messagePanel->Clear();

    REPORTER &reporter = m_messagePanel->Reporter();
    KIGFX::VIEW*    view = m_frame->GetGalCanvas()->GetView();
    TOOL_MANAGER *toolManager = m_frame->GetToolManager();
    BOARD *board = m_frame->GetBoard();

    if( !aDryRun )
    {
        // Remove old modules
        for( MODULE* module = board->m_Modules; module; module = module->Next() )
        {
            module->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, view, _1 ) );
            view->Remove( module );
        }

        // Clear selection, just in case a selected item has to be removed
        toolManager->RunAction( COMMON_ACTIONS::selectionClear, true );
    }

    m_frame->LoadFootprints( *m_netlist, &reporter );

    BOARD_NETLIST_UPDATER updater( m_frame, m_frame->GetBoard() );

    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun);
    updater.SetLookupByTimestamp( true );
    updater.SetDeleteUnusedComponents ( true );
    updater.SetReplaceFootprints( true );
    updater.SetDeleteSinglePadNets ( false );

    updater.UpdateNetlist( *m_netlist );

    m_messagePanel->Flush();

    if( aDryRun )
        return;

    m_frame->OnModify();

    m_frame->SetCurItem( NULL );

    // Reload modules
    for( MODULE* module = board->m_Modules; module; module = module->Next() )
    {
        module->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, view, _1 ) );
        view->Add( module );
        module->ViewUpdate();
    }

    // Rebuild the board connectivity:
    if( m_frame->IsGalCanvasActive() )
        board->GetRatsnest()->ProcessBoard();

    m_frame->Compile_Ratsnest( NULL, true );

    m_frame->SetMsgPanel( board );

}

void DIALOG_UPDATE_PCB::OnMatchChange( wxCommandEvent& event )
{

}

void DIALOG_UPDATE_PCB::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}

void DIALOG_UPDATE_PCB::OnUpdateClick( wxCommandEvent& event )
{
    m_messagePanel->SetLabel( _("Changes applied to the PCB:") );
    PerformUpdate( false );
    m_btnCancel->SetFocus( );
}
