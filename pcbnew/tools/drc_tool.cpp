/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>
#include <tools/zone_filler_tool.h>
#include <tools/drc_tool.h>
#include <kiface_i.h>
#include <dialog_drc.h>
#include <board_commit.h>
#include <widgets/progress_reporter.h>
#include <drc/drc_engine.h>
#include <drc/drc_results_provider.h>
#include <netlist_reader/pcb_netlist.h>
#include <dialogs/panel_setup_rules_base.h>

DRC_TOOL::DRC_TOOL() :
        PCB_TOOL_BASE( "pcbnew.DRCTool" ),
        m_editFrame( nullptr ),
        m_pcb( nullptr ),
        m_drcDialog( nullptr ),
        m_drcRunning( false )
{
}


DRC_TOOL::~DRC_TOOL()
{
}


void DRC_TOOL::Reset( RESET_REASON aReason )
{
    m_editFrame = getEditFrame<PCB_EDIT_FRAME>();

    if( m_pcb != m_editFrame->GetBoard() )
    {
        if( m_drcDialog )
            DestroyDRCDialog();

        m_pcb = m_editFrame->GetBoard();
        m_drcEngine = m_pcb->GetDesignSettings().m_DRCEngine;
    }
}


void DRC_TOOL::ShowDRCDialog( wxWindow* aParent )
{
    bool show_dlg_modal = true;

    // the dialog needs a parent frame. if it is not specified, this is the PCB editor frame
    // specified in DRC_TOOL class.
    if( !aParent )
    {
        // if any parent is specified, the dialog is modal.
        // if this is the default PCB editor frame, it is not modal
        show_dlg_modal = false;
        aParent = m_editFrame;
    }

    Activate();
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( !m_drcDialog )
    {
        m_drcDialog = new DIALOG_DRC( m_editFrame, aParent );
        updatePointers();

        if( show_dlg_modal )
            m_drcDialog->ShowModal();
        else
            m_drcDialog->Show( true );
    }
    else // The dialog is just not visible (because the user has double clicked on an error item)
    {
        updatePointers();
        m_drcDialog->Show( true );
    }
}


int DRC_TOOL::ShowDRCDialog( const TOOL_EVENT& aEvent )
{
    ShowDRCDialog( nullptr );
    return 0;
}


bool DRC_TOOL::IsDRCDialogShown()
{
    if( m_drcDialog )
        return m_drcDialog->IsShown();

    return false;
}


void DRC_TOOL::DestroyDRCDialog()
{
    if( m_drcDialog )
    {
        m_drcDialog->Destroy();
        m_drcDialog = nullptr;
    }
}


void DRC_TOOL::RunTests( PROGRESS_REPORTER* aProgressReporter, bool aTestTracksAgainstZones,
                         bool aRefillZones, bool aReportAllTrackErrors, bool aTestFootprints )
{
    ZONE_FILLER_TOOL* zoneFiller = m_toolMgr->GetTool<ZONE_FILLER_TOOL>();
    BOARD_COMMIT      commit( m_editFrame );
    NETLIST           netlist;
    wxWindowDisabler  disabler( /* disable everything except: */ m_drcDialog );

    // This is not the time to have stale or buggy rules.  Ensure they're up-to-date
    // and that they at least parse.
    try
    {
        m_drcEngine->InitEngine( m_editFrame->Prj().AbsolutePath( "drc-rules" ) );
    }
    catch( PARSE_ERROR& pe )
    {
        // Shouldn't be necessary, but we get into all kinds of wxWidgets window ordering
        // issues (on at least OSX) if we don't.
        DestroyDRCDialog();

        m_editFrame->ShowBoardSetupDialog( _( "Rules" ), pe.What(), ID_RULES_EDITOR,
                                           pe.lineNumber, pe.byteIndex );

        return;
    }

    m_drcRunning = true;

    if( aRefillZones )
    {
        aProgressReporter->AdvancePhase( _( "Refilling all zones..." ) );

        zoneFiller->FillAllZones( m_drcDialog, aProgressReporter );
    }
    else
    {
        aProgressReporter->AdvancePhase( _( "Checking zone fills..." ) );

        zoneFiller->CheckAllZones( m_drcDialog, aProgressReporter );
    }

    m_drcEngine->SetWorksheet( m_editFrame->GetCanvas()->GetWorksheet() );

    if( aTestFootprints && !Kiface().IsSingle() )
    {
        m_editFrame->FetchNetlistFromSchematic( netlist, PCB_EDIT_FRAME::ANNOTATION_DIALOG );

        if( m_drcDialog )
            m_drcDialog->Raise();

        m_drcEngine->SetSchematicNetlist( &netlist );
    }

    m_drcEngine->SetProgressReporter( aProgressReporter );

    m_drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
            {
                if(    aItem->GetErrorCode() == DRCE_MISSING_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_DUPLICATE_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_EXTRA_FOOTPRINT
                    || aItem->GetErrorCode() == DRCE_NET_CONFLICT )
                {
                    m_footprints.push_back( aItem );
                }
                else if( aItem->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
                {
                    m_unconnected.push_back( aItem );
                }
                else
                {
                    MARKER_PCB* marker = new MARKER_PCB( aItem, aPos );
                    commit.Add( marker );
                }
            } );

    m_drcEngine->RunTests( m_editFrame->GetUserUnits(), aTestTracksAgainstZones,
                           aReportAllTrackErrors, aTestFootprints );

    m_drcEngine->SetProgressReporter( nullptr );
    m_drcEngine->ClearViolationHandler();

    if( m_drcDialog )
    {
        m_drcDialog->SetDrcRun();

        if( !netlist.IsEmpty() )
            m_drcDialog->SetFootprintTestsRun();
    }

    commit.Push( _( "DRC" ), false );

    m_drcRunning = false;

    // update the m_drcDialog listboxes
    updatePointers();
}


void DRC_TOOL::updatePointers()
{
    // update my pointers, m_editFrame is the only unchangeable one
    m_pcb = m_editFrame->GetBoard();

    m_editFrame->ResolveDRCExclusions();

    if( m_drcDialog )  // Use diag list boxes only in DRC_TOOL dialog
    {
        m_drcDialog->SetMarkersProvider( new BOARD_DRC_ITEMS_PROVIDER( m_pcb ) );
        m_drcDialog->SetUnconnectedProvider( new RATSNEST_DRC_ITEMS_PROVIDER( m_editFrame,
                                                                              &m_unconnected ) );
        m_drcDialog->SetFootprintsProvider( new VECTOR_DRC_ITEMS_PROVIDER( m_editFrame,
                                                                           &m_footprints ) );
    }
}


void DRC_TOOL::setTransitions()
{
    Go( &DRC_TOOL::ShowDRCDialog,              PCB_ACTIONS::runDRC.MakeEvent() );
}


