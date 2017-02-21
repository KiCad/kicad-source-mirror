/**
 * @file pcbnew/dialogs/dialog_update_pcb.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <common.h>
#include <wxPcbStruct.h>
#include <pcb_netlist.h>
#include <dialog_update_pcb.h>
#include <wx_html_report_panel.h>
#include <board_netlist_updater.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <class_draw_panel_gal.h>
#include <class_drawpanel.h>
#include <class_board.h>
#include <ratsnest_data.h>

#include <functional>
using namespace std::placeholders;

DIALOG_UPDATE_PCB::DIALOG_UPDATE_PCB( PCB_EDIT_FRAME* aParent, NETLIST *aNetlist ) :
    DIALOG_UPDATE_PCB_BASE ( aParent ),
    m_frame (aParent),
    m_netlist (aNetlist)
{
    m_messagePanel->SetLabel( _("Changes to be applied:") );
    m_messagePanel->SetLazyUpdate( true );
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

    REPORTER& reporter = m_messagePanel->Reporter();
    TOOL_MANAGER* toolManager = m_frame->GetToolManager();
    BOARD* board = m_frame->GetBoard();

    // keep trace of the initial baord area, if we want to place new footprints
    // outside the existinag board
    EDA_RECT bbox = board->ComputeBoundingBox( false );

    if( !aDryRun )
    {

        // Clear selection, just in case a selected item has to be removed
        toolManager->RunAction( PCB_ACTIONS::selectionClear, true );
    }

    m_netlist->SetDeleteExtraFootprints( true );
    m_netlist->SetFindByTimeStamp( m_matchByTimestamp->GetValue() );
    m_netlist->SetReplaceFootprints( true );

    try
    {
        m_frame->LoadFootprints( *m_netlist, &reporter );
    }
    catch( IO_ERROR &error )
    {
        wxString msg;

        reporter.Report( _( "Failed to load one or more footprints. Please add the missing libraries in PCBNew configuration. "
                            "The PCB will not update completely." ), REPORTER::RPT_ERROR );
        reporter.Report( error.What(), REPORTER::RPT_INFO );
    }

    BOARD_NETLIST_UPDATER updater( m_frame, m_frame->GetBoard() );
    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun);
    updater.SetLookupByTimestamp( m_matchByTimestamp->GetValue() );
    updater.SetDeleteUnusedComponents ( true );
    updater.SetReplaceFootprints( true );
    updater.SetDeleteSinglePadNets( false );
    updater.UpdateNetlist( *m_netlist );

    m_messagePanel->Flush();

    if( aDryRun )
        return;

    m_frame->SetCurItem( NULL );
    m_frame->SetMsgPanel( board );

    std::vector<MODULE*> newFootprints = updater.GetAddedComponents();

    // Spread new footprints.
    wxPoint areaPosition = m_frame->GetCrossHairPosition();

    if( !m_frame->IsGalCanvasActive() )
    {
        // In legacy mode place area to the left side of the board.
        // if the board is empty, the bbox position is (0,0)
        areaPosition.x = bbox.GetEnd().x + Millimeter2iu( 10 );
        areaPosition.y = bbox.GetOrigin().y;
    }

    m_frame->SpreadFootprints( &newFootprints, false, false, areaPosition, false );

    if( m_frame->IsGalCanvasActive() )
    {
        // Start move and place the new modules command
        if( !newFootprints.empty() )
        {
            for( MODULE* footprint : newFootprints )
            {
                toolManager->RunAction( PCB_ACTIONS::selectItem, true, footprint );
            }

            toolManager->InvokeTool( "pcbnew.InteractiveEdit" );
        }
    }
    else    // Legacy canvas
        m_frame->GetCanvas()->Refresh();

    m_btnPerformUpdate->Enable( false );
    m_btnPerformUpdate->SetLabel( _( "Update complete" ) );
    m_btnCancel->SetLabel( _( "Close" ) );
    m_btnCancel->SetFocus();
}


void DIALOG_UPDATE_PCB::OnMatchChange( wxCommandEvent& event )
{
    PerformUpdate( true );
}


void DIALOG_UPDATE_PCB::OnUpdateClick( wxCommandEvent& event )
{
    m_messagePanel->SetLabel( _( "Changes applied to the PCB:" ) );
    PerformUpdate( false );
    m_btnCancel->SetFocus();
}
