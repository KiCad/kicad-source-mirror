/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_layer_box_selector.h>
#include <board_design_settings.h>
#include <board_connected_item.h>
#include <pcb_group.h>
#include <project/net_settings.h>

DIALOG_CLEANUP_TRACKS_AND_VIAS::DIALOG_CLEANUP_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParentFrame ) :
        DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( aParentFrame ),
        m_parentFrame( aParentFrame ),
        m_brd( aParentFrame->GetBoard() ),
        m_firstRun( true )
{
    PCBNEW_SETTINGS* cfg = m_parentFrame->GetPcbNewSettings();
    m_reporter = new WX_TEXT_CTRL_REPORTER( m_tcReport );

    m_cbRefillZones->SetValue( cfg->m_Cleanup.cleanup_refill_zones );
    m_cleanViasOpt->SetValue( cfg->m_Cleanup.cleanup_vias );
    m_mergeSegmOpt->SetValue( cfg->m_Cleanup.merge_segments );
    m_deleteUnconnectedOpt->SetValue( cfg->m_Cleanup.cleanup_unconnected );
    m_cleanShortCircuitOpt->SetValue( cfg->m_Cleanup.cleanup_short_circuits );
    m_deleteTracksInPadsOpt->SetValue( cfg->m_Cleanup.cleanup_tracks_in_pad );
    m_deleteDanglingViasOpt->SetValue( cfg->m_Cleanup.delete_dangling_vias );

    buildFilterLists();

    m_changesTreeModel = new RC_TREE_MODEL( m_parentFrame, m_changesDataView );
    m_changesDataView->AssociateModel( m_changesTreeModel );

    setupOKButtonLabel();

    m_netFilter->Connect( FILTERED_ITEM_SELECTED,
                          wxCommandEventHandler( DIALOG_CLEANUP_TRACKS_AND_VIAS::OnNetFilterSelect ),
                          nullptr, this );

    m_sdbSizer->SetSizeHints( this );

    finishDialogSettings();
}


DIALOG_CLEANUP_TRACKS_AND_VIAS::~DIALOG_CLEANUP_TRACKS_AND_VIAS()
{
    PCBNEW_SETTINGS* cfg = nullptr;

    try
    {
        cfg = m_parentFrame->GetPcbNewSettings();
    }
    catch( const std::runtime_error& e )
    {
        wxFAIL_MSG( e.what() );
    }

    if( cfg )
    {
        cfg->m_Cleanup.cleanup_refill_zones   = m_cbRefillZones->GetValue();
        cfg->m_Cleanup.cleanup_vias           = m_cleanViasOpt->GetValue();
        cfg->m_Cleanup.merge_segments         = m_mergeSegmOpt->GetValue();
        cfg->m_Cleanup.cleanup_unconnected    = m_deleteUnconnectedOpt->GetValue();
        cfg->m_Cleanup.cleanup_short_circuits = m_cleanShortCircuitOpt->GetValue();
        cfg->m_Cleanup.cleanup_tracks_in_pad  = m_deleteTracksInPadsOpt->GetValue();
        cfg->m_Cleanup.delete_dangling_vias   = m_deleteDanglingViasOpt->GetValue();
    }

    m_changesTreeModel->DecRef();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::buildFilterLists()
{
    // Populate the net filter list with net names
    m_netFilter->SetNetInfo( &m_brd->GetNetInfo() );

    if( !m_brd->GetHighLightNetCodes().empty() )
        m_netFilter->SetSelectedNetcode( *m_brd->GetHighLightNetCodes().begin() );

    // Populate the netclass filter list with netclass names
    wxArrayString                  netclassNames;
    std::shared_ptr<NET_SETTINGS>& settings = m_brd->GetDesignSettings().m_NetSettings;

    netclassNames.push_back( settings->GetDefaultNetclass()->GetName() );

    for( const auto& [name, netclass] : settings->GetNetclasses() )
        netclassNames.push_back( name );

    m_netclassFilter->Set( netclassNames );
    m_netclassFilter->SetStringSelection( m_brd->GetDesignSettings().GetCurrentNetClassName() );

    // Populate the layer filter list
    m_layerFilter->SetBoardFrame( m_parentFrame );
    m_layerFilter->SetLayersHotkeys( false );
    m_layerFilter->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_layerFilter->Resync();
    m_layerFilter->SetLayerSelection( m_parentFrame->GetActiveLayer() );
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
    m_changesTreeModel->Update( nullptr, RPT_SEVERITY_ACTION );
    m_firstRun = true;
    setupOKButtonLabel();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnNetFilterSelect( wxCommandEvent& aEvent )
{
    OnCheckBox( aEvent );
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnNetclassFilterSelect( wxCommandEvent& aEvent )
{
    OnCheckBox( aEvent );
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnLayerFilterSelect( wxCommandEvent& aEvent )
{
    OnCheckBox( aEvent );
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
    TRACKS_CLEANER cleaner( m_brd, commit );

    struct FILTER_STATE
    {
        bool selectedOnly;
        int netCodeOnly;
        wxString netClassOnly;
        int layerOnly;
    };

    FILTER_STATE filter_state = {
            .selectedOnly = m_selectedItemsFilter->GetValue(),
            .netCodeOnly = m_netFilterOpt->GetValue() ? m_netFilter->GetSelectedNetcode() : -1,
            .netClassOnly = m_netclassFilterOpt->GetValue()
                                ? m_netclassFilter->GetStringSelection()
                                : wxString(),
            .layerOnly = m_layerFilterOpt->GetValue()
                             ? m_layerFilter->GetLayerSelection()
                             : UNDEFINED_LAYER
            };

    cleaner.SetFilter(
            [&, filter_state]( BOARD_CONNECTED_ITEM* aItem ) -> bool
            {
                if( filter_state.selectedOnly )
                {
                    if( !aItem->IsSelected() )
                    {
                        PCB_GROUP* group = aItem->GetParentGroup();

                        while( group && !group->IsSelected() )
                            group = group->GetParentGroup();

                        if( !group )
                            return true;
                    }
                }

                if( filter_state.netCodeOnly >= 0 )
                {
                    if( aItem->GetNetCode() != filter_state.netCodeOnly )
                        return true;
                }

                if( !filter_state.netClassOnly.IsEmpty() )
                {
                    NETCLASS* netclass = aItem->GetEffectiveNetClass();

                    if( !netclass->ContainsNetclassWithName( filter_state.netClassOnly ) )
                        return true;
                }

                if( filter_state.layerOnly != UNDEFINED_LAYER )
                {
                    if( aItem->GetLayer() != filter_state.layerOnly )
                        return true;
                }

                return false;
            } );

    m_outputBook->SetSelection( 1 );

    if( !aDryRun )
    {
        // Clear current selection list to avoid selection of deleted items
        m_parentFrame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear );

        // ... and to keep the treeModel from trying to refresh a deleted item
        m_changesTreeModel->Update( nullptr, RPT_SEVERITY_ACTION );
    }

    m_items.clear();

    if( m_firstRun )
    {
        if( m_cbRefillZones->GetValue() )
        {
            m_reporter->Report( _( "Checking zones..." ) );
            wxSafeYield();      // Timeslice to update UI
            m_parentFrame->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( this );
            wxSafeYield();      // Timeslice to close zone progress reporter
        }

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

    if( m_cbRefillZones->GetValue() == wxCHK_CHECKED && !aDryRun )
    {
        m_reporter->Report( _( "Refilling all zones..." ) );
        wxSafeYield(); // Timeslice to update UI
        m_parentFrame->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->FillAllZones( this );
        wxSafeYield(); // Timeslice to close zone progress reporter
    }

    if( aDryRun )
    {
        m_changesTreeModel->Update( std::make_shared<VECTOR_CLEANUP_ITEMS_PROVIDER>( &m_items ),
                                    RPT_SEVERITY_ACTION );
    }
    else if( !commit.Empty() )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        commit.Push( _( "Board Cleanup" ) );
        m_parentFrame->GetCanvas()->Refresh( true );
    }

    m_outputBook->SetSelection( 0 );
    setupOKButtonLabel();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnSelectItem( wxDataViewEvent& aEvent )
{
    const KIID&   itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM*   item = m_brd->GetItem( itemID );
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


