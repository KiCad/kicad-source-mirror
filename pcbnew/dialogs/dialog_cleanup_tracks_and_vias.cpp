/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_cleanup_tracks_and_vias.h>
#include <board_commit.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tracks_cleaner.h>
#include <drc/drc_item.h>
#include <tools/zone_filler_tool.h>
#include <reporter.h>

DIALOG_CLEANUP_TRACKS_AND_VIAS::DIALOG_CLEANUP_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParentFrame ) :
        DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( aParentFrame ),
        m_parentFrame( aParentFrame ),
        m_firstRun( true )
{
    auto cfg = m_parentFrame->GetPcbNewSettings();
    m_reporter = new WX_TEXT_CTRL_REPORTER( m_tcReport );

    m_cleanViasOpt->SetValue( cfg->m_Cleanup.cleanup_vias );
    m_mergeSegmOpt->SetValue( cfg->m_Cleanup.merge_segments );
    m_deleteUnconnectedOpt->SetValue( cfg->m_Cleanup.cleanup_unconnected );
    m_cleanShortCircuitOpt->SetValue( cfg->m_Cleanup.cleanup_short_circuits );
    m_deleteTracksInPadsOpt->SetValue( cfg->m_Cleanup.cleanup_tracks_in_pad );
    m_deleteDanglingViasOpt->SetValue( cfg->m_Cleanup.delete_dangling_vias );

    m_changesTreeModel = new RC_TREE_MODEL( m_parentFrame, m_changesDataView );
    m_changesDataView->AssociateModel( m_changesTreeModel );

    setupOKButtonLabel();

    m_sdbSizer->SetSizeHints( this );

    finishDialogSettings();
}


DIALOG_CLEANUP_TRACKS_AND_VIAS::~DIALOG_CLEANUP_TRACKS_AND_VIAS()
{
    auto cfg = m_parentFrame->GetPcbNewSettings();

    cfg->m_Cleanup.cleanup_vias           = m_cleanViasOpt->GetValue();
    cfg->m_Cleanup.merge_segments         = m_mergeSegmOpt->GetValue();
    cfg->m_Cleanup.cleanup_unconnected    = m_deleteUnconnectedOpt->GetValue();
    cfg->m_Cleanup.cleanup_short_circuits = m_cleanShortCircuitOpt->GetValue();
    cfg->m_Cleanup.cleanup_tracks_in_pad  = m_deleteTracksInPadsOpt->GetValue();
    cfg->m_Cleanup.delete_dangling_vias   = m_deleteDanglingViasOpt->GetValue();

    m_changesTreeModel->DecRef();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::setupOKButtonLabel()
{
    if( m_firstRun )
        SetupStandardButtons( { { wxID_OK, _( "Build Changes" ) } } );
    else
        SetupStandardButtons( { { wxID_OK, _( "Update PCB" ) } } );
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnCheckBox( wxCommandEvent& anEvent )
{
    m_firstRun = true;
    setupOKButtonLabel();
    m_tcReport->Clear();
}


bool DIALOG_CLEANUP_TRACKS_AND_VIAS::TransferDataToWindow()
{
    return true;
}


bool DIALOG_CLEANUP_TRACKS_AND_VIAS::TransferDataFromWindow()
{
    bool dryRun = m_firstRun;

    doCleanup( dryRun );

    return !dryRun;
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::doCleanup( bool aDryRun )
{
    m_tcReport->Clear();
    wxSafeYield();      // Timeslice to update UI

    wxBusyCursor busy;
    BOARD_COMMIT commit( m_parentFrame );
    TRACKS_CLEANER cleaner( m_parentFrame->GetBoard(), commit );

    if( !aDryRun )
    {
        // Clear current selection list to avoid selection of deleted items
        m_parentFrame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

        // ... and to keep the treeModel from trying to refresh a deleted item
        m_changesTreeModel->Update( nullptr, RPT_SEVERITY_ACTION );
    }

    m_items.clear();

    if( m_firstRun )
    {
        m_reporter->Report( _( "Checking zones..." ) );
        wxSafeYield();      // Timeslice to update UI
        m_parentFrame->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( this );
        wxSafeYield();      // Timeslice to close zone progress reporter
        m_firstRun = false;
    }

    // Old model has to be refreshed, GAL normally does not keep updating it
    m_reporter->Report( _( "Rebuilding connectivity..." ) );
    wxSafeYield();      // Timeslice to update UI
    m_parentFrame->Compile_Ratsnest( false );

    cleaner.CleanupBoard( aDryRun, &m_items, m_cleanShortCircuitOpt->GetValue(),
                                             m_cleanViasOpt->GetValue(),
                                             m_mergeSegmOpt->GetValue(),
                                             m_deleteUnconnectedOpt->GetValue(),
                                             m_deleteTracksInPadsOpt->GetValue(),
                                             m_deleteDanglingViasOpt->GetValue(),
                                             m_reporter );

    if( aDryRun )
    {
        m_changesTreeModel->Update( std::make_shared<VECTOR_CLEANUP_ITEMS_PROVIDER>( &m_items ),
                                    RPT_SEVERITY_ACTION );
    }
    else if( !commit.Empty() )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        commit.Push( _( "Board cleanup" ) );
        m_parentFrame->GetCanvas()->Refresh( true );
    }

    m_reporter->Report( _( "Done." ) );
    setupOKButtonLabel();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnSelectItem( wxDataViewEvent& aEvent )
{
    const KIID&   itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM*   item = m_parentFrame->GetBoard()->GetItem( itemID );
    WINDOW_THAWER thawer( m_parentFrame );

    m_parentFrame->FocusOnItem( item );
    m_parentFrame->GetCanvas()->Refresh();

    aEvent.Skip();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnLeftDClickItem( wxMouseEvent& event )
{
    event.Skip();

    if( m_changesDataView->GetCurrentItem().IsOk() )
    {
        if( !IsModal() )
            Show( false );
    }
}


