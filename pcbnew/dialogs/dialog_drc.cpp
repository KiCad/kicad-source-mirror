/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <dialog_drc.h>
#include <board_design_settings.h>
#include <kiface_base.h>
#include <macros.h>
#include <pad.h>
#include <drc/drc_item.h>
#include <drc/drc_report.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <wildcards_and_files_ext.h>
#include <pcb_marker.h>
#include <pgm_base.h>
#include <wx/filedlg.h>
#include <wx/wupdlock.h>
#include <widgets/appearance_controls.h>
#include <widgets/ui_common.h>
#include <widgets/progress_reporter_base.h>
#include <widgets/wx_html_report_box.h>
#include <dialogs/panel_setup_rules_base.h>
#include <dialogs/dialog_text_entry.h>
#include <tools/drc_tool.h>
#include <tools/zone_filler_tool.h>
#include <tools/board_inspection_tool.h>
#include <kiplatform/ui.h>

// wxWidgets spends *far* too long calcuating column widths (most of it, believe it or
// not, in repeatedly creating/destroying a wxDC to do the measurement in).
// Use default column widths instead.
static int DEFAULT_SINGLE_COL_WIDTH = 660;

static BOARD*                g_lastDRCBoard = nullptr;
static bool                  g_lastDRCRun = false;
static bool                  g_lastFootprintTestsRun = false;
static std::vector<wxString> g_lastIgnored;


DIALOG_DRC::DIALOG_DRC( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent ) :
        DIALOG_DRC_BASE( aParent ),
        PROGRESS_REPORTER_BASE( 1 ),
        m_running( false ),
        m_drcRun( false ),
        m_footprintTestsRun( false ),
        m_markersTreeModel( nullptr ),
        m_unconnectedTreeModel( nullptr ),
        m_fpWarningsTreeModel( nullptr ),
        m_severities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING )
{
    SetName( DIALOG_DRC_WINDOW_NAME ); // Set a window name to be able to find it

    m_frame = aEditorFrame;
    m_currentBoard = m_frame->GetBoard();

    m_messages->SetImmediateMode();

    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();
    m_severities = cfg->m_DrcDialog.severities;

    m_markersProvider = std::make_shared<DRC_ITEMS_PROVIDER>( m_currentBoard,
                                                              MARKER_BASE::MARKER_DRC,
                                                              MARKER_BASE::MARKER_DRAWING_SHEET );

    m_ratsnestProvider = std::make_shared<DRC_ITEMS_PROVIDER>( m_currentBoard,
                                                               MARKER_BASE::MARKER_RATSNEST );

    m_fpWarningsProvider = std::make_shared<DRC_ITEMS_PROVIDER>( m_currentBoard,
                                                                 MARKER_BASE::MARKER_PARITY );

    m_markersTreeModel = new RC_TREE_MODEL( m_frame, m_markerDataView );
    m_markerDataView->AssociateModel( m_markersTreeModel );
    m_markersTreeModel->Update( m_markersProvider, m_severities );

    m_unconnectedTreeModel = new RC_TREE_MODEL( m_frame, m_unconnectedDataView );
    m_unconnectedDataView->AssociateModel( m_unconnectedTreeModel );
    m_unconnectedTreeModel->Update( m_ratsnestProvider, m_severities );

    m_fpWarningsTreeModel = new RC_TREE_MODEL( m_frame, m_footprintsDataView );
    m_footprintsDataView->AssociateModel( m_fpWarningsTreeModel );
    m_fpWarningsTreeModel->Update( m_fpWarningsProvider, m_severities );

    m_ignoredList->InsertColumn( 0, wxEmptyString, wxLIST_FORMAT_LEFT, DEFAULT_SINGLE_COL_WIDTH );

    if( m_currentBoard == g_lastDRCBoard )
    {
        m_drcRun = g_lastDRCRun;
        m_footprintTestsRun = g_lastFootprintTestsRun;

        for( const wxString& str : g_lastIgnored )
            m_ignoredList->InsertItem( m_ignoredList->GetItemCount(), str );
    }

    m_Notebook->SetSelection( 0 );

    if( Kiface().IsSingle() )
        m_cbTestFootprints->Hide();

    SetupStandardButtons( { { wxID_OK,     _( "Run DRC" ) },
                            { wxID_CANCEL, _( "Close" )   } } );

    m_markersTitleTemplate     = m_Notebook->GetPageText( 0 );
    m_unconnectedTitleTemplate = m_Notebook->GetPageText( 1 );
    m_footprintsTitleTemplate  = m_Notebook->GetPageText( 2 );
    m_ignoredTitleTemplate     = m_Notebook->GetPageText( 3 );

    m_cbRefillZones->SetValue( cfg->m_DrcDialog.refill_zones );
    m_cbReportAllTrackErrors->SetValue( cfg->m_DrcDialog.test_all_track_errors );

    if( !Kiface().IsSingle() )
        m_cbTestFootprints->SetValue( cfg->m_DrcDialog.test_footprints );

    Layout(); // adding the units above expanded Clearance text, now resize.

    SetFocus();

    syncCheckboxes();

    finishDialogSettings();
}


DIALOG_DRC::~DIALOG_DRC()
{
    m_frame->FocusOnItem( nullptr );

    g_lastDRCBoard = m_currentBoard;
    g_lastDRCRun = m_drcRun;
    g_lastFootprintTestsRun = m_footprintTestsRun;

    g_lastIgnored.clear();

    for( int ii = 0; ii < m_ignoredList->GetItemCount(); ++ii )
        g_lastIgnored.push_back( m_ignoredList->GetItemText( ii ) );

    PCBNEW_SETTINGS* cfg = nullptr;

    try
    {
        cfg = m_frame->GetPcbNewSettings();
    }
    catch( const std::runtime_error& e )
    {
        wxFAIL_MSG( e.what() );
    }

    if( cfg )
    {
        cfg->m_DrcDialog.refill_zones          = m_cbRefillZones->GetValue();
        cfg->m_DrcDialog.test_all_track_errors = m_cbReportAllTrackErrors->GetValue();

        if( !Kiface().IsSingle() )
            cfg->m_DrcDialog.test_footprints   = m_cbTestFootprints->GetValue();

        cfg->m_DrcDialog.severities            = m_severities;
    }

    m_markersTreeModel->DecRef();
    m_unconnectedTreeModel->DecRef();
    m_fpWarningsTreeModel->DecRef();
}


void DIALOG_DRC::OnActivateDlg( wxActivateEvent& aEvent )
{
    if( m_currentBoard != m_frame->GetBoard() )
    {
        // If m_currentBoard is not the current board, (for instance because a new board
        // was loaded), close the dialog, because many pointers are now invalid in lists
        SetReturnCode( wxID_CANCEL );
        Close();

        DRC_TOOL* drcTool = m_frame->GetToolManager()->GetTool<DRC_TOOL>();
        drcTool->DestroyDRCDialog();
    }
}


// PROGRESS_REPORTER calls

bool DIALOG_DRC::updateUI()
{
    double cur = alg::clamp( 0.0, (double) m_progress.load() / m_maxProgress, 1.0 );

    m_gauge->SetValue( KiROUND( cur * 1000.0 ) );
    Pgm().App().SafeYieldFor( this, wxEVT_CATEGORY_NATIVE_EVENTS );

    return !m_cancelled;
}


void DIALOG_DRC::AdvancePhase( const wxString& aMessage )
{
    PROGRESS_REPORTER_BASE::AdvancePhase( aMessage );
    SetCurrentProgress( 0.0 );

    m_messages->Report( aMessage );
}


// Don't globally define this; different facilities use different definitions of "ALL"
static int RPT_SEVERITY_ALL = RPT_SEVERITY_WARNING | RPT_SEVERITY_ERROR | RPT_SEVERITY_EXCLUSION;


void DIALOG_DRC::syncCheckboxes()
{
    m_showAll->SetValue( m_severities == RPT_SEVERITY_ALL );
    m_showErrors->SetValue( m_severities & RPT_SEVERITY_ERROR );
    m_showWarnings->SetValue( m_severities & RPT_SEVERITY_WARNING );
    m_showExclusions->SetValue( m_severities & RPT_SEVERITY_EXCLUSION );
}


void DIALOG_DRC::OnErrorLinkClicked( wxHtmlLinkEvent& event )
{
    m_frame->ShowBoardSetupDialog( _( "Custom Rules" ) );
}


void DIALOG_DRC::OnRunDRCClick( wxCommandEvent& aEvent )
{
    TOOL_MANAGER*     toolMgr              = m_frame->GetToolManager();
    DRC_TOOL*         drcTool              = toolMgr->GetTool<DRC_TOOL>();
    ZONE_FILLER_TOOL* zoneFillerTool       = toolMgr->GetTool<ZONE_FILLER_TOOL>();
    bool              refillZones          = m_cbRefillZones->GetValue();
    bool              reportAllTrackErrors = m_cbReportAllTrackErrors->GetValue();
    bool              testFootprints       = m_cbTestFootprints->GetValue();

    if( zoneFillerTool->IsBusy() )
    {
        wxBell();
        return;
    }

    // This is not the time to have stale or buggy rules.  Ensure they're up-to-date
    // and that they at least parse.
    try
    {
        drcTool->GetDRCEngine()->InitEngine( m_frame->GetDesignRulesPath() );
    }
    catch( PARSE_ERROR& )
    {
        m_runningResultsBook->ChangeSelection( 0 );   // Display the "Tests Running..." tab
        m_DeleteCurrentMarkerButton->Enable( false );
        m_DeleteAllMarkersButton->Enable( false );
        m_saveReport->Enable( false );

        m_messages->Clear();
        m_messages->Report( _( "DRC incomplete: could not compile custom design rules." )
                            + wxS( "&nbsp;&nbsp;" )
                            + wxS( "<a href='$CUSTOM_RULES'>" ) + _( "Show design rules." ) + wxT( "</a>" ) );
        m_messages->Flush();

        Raise();
        return;
    }

    m_footprintTestsRun = false;
    m_cancelled = false;

    m_frame->GetBoard()->RecordDRCExclusions();
    deleteAllMarkers( true );

    std::vector<std::reference_wrapper<RC_ITEM>> violations = DRC_ITEM::GetItemsWithSeverities();
    m_ignoredList->DeleteAllItems();

    for( std::reference_wrapper<RC_ITEM>& item : violations )
    {
        if( bds().GetSeverity( item.get().GetErrorCode() ) == RPT_SEVERITY_IGNORE )
        {
            m_ignoredList->InsertItem( m_ignoredList->GetItemCount(),
                                       wxT( " • " ) + item.get().GetErrorText() );
        }
    }

    m_ignoredList->SetColumnWidth( 0, m_ignoredList->GetParent()->GetClientSize().x - 20 );

    Raise();

    m_runningResultsBook->ChangeSelection( 0 );   // Display the "Tests Running..." tab
    m_messages->Clear();
    wxYield();                                    // Allow time slice to refresh Messages

    m_running = true;
    m_sdbSizerCancel->SetLabel( _( "Cancel" ) );
    m_sdbSizerOK->Enable( false );
    m_DeleteCurrentMarkerButton->Enable( false );
    m_DeleteAllMarkersButton->Enable( false );
    m_saveReport->Enable( false );

    {
    wxBusyCursor dummy;
    drcTool->RunTests( this, refillZones, reportAllTrackErrors, testFootprints );
    }

    if( m_cancelled )
        m_messages->Report( _( "-------- DRC cancelled by user.<br><br>" ) );
    else
        m_messages->Report( _( "Done.<br><br>" ) );

    Raise();
    wxYield();                                    // Allow time slice to refresh Messages

    m_running = false;
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizerOK->Enable( true );
    m_DeleteCurrentMarkerButton->Enable( true );
    m_DeleteAllMarkersButton->Enable( true );
    m_saveReport->Enable( true );

    if( !m_cancelled )
    {
        m_sdbSizerCancel->SetDefault();
        // wxWidgets has a tendency to keep both buttons highlighted without the following:
        m_sdbSizerOK->Enable( false );

        wxMilliSleep( 500 );
        m_runningResultsBook->ChangeSelection( 1 );
        KIPLATFORM::UI::ForceFocus( m_Notebook );

        // now re-enable m_sdbSizerOK button
        m_sdbSizerOK->Enable( true );
    }

    refreshEditor();
}


void DIALOG_DRC::UpdateData()
{
    m_markersTreeModel->Update( m_markersProvider, m_severities );
    m_unconnectedTreeModel->Update( m_ratsnestProvider, m_severities );
    m_fpWarningsTreeModel->Update( m_fpWarningsProvider, m_severities );

    updateDisplayedCounts();
}


void DIALOG_DRC::OnDRCItemSelected( wxDataViewEvent& aEvent )
{
    BOARD*        board = m_frame->GetBoard();
    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );

    auto getActiveLayers =
            []( BOARD_ITEM* aItem ) -> LSET
            {
                if( aItem->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( aItem );
                    LSET layers;

                    for( int layer : aItem->GetLayerSet().Seq() )
                    {
                        if( pad->FlashLayer( layer ) )
                            layers.set( layer );
                    }

                    return layers;
                }
                else
                {
                    return aItem->GetLayerSet();
                }
            };

    if( !node )
    {
        // list is being freed; don't do anything with null ptrs

        aEvent.Skip();
        return;
    }

    std::shared_ptr<RC_ITEM> rc_item = node->m_RcItem;

    if( rc_item->GetErrorCode() == DRCE_UNRESOLVED_VARIABLE
            && rc_item->GetParent()->GetMarkerType() == MARKER_BASE::MARKER_DRAWING_SHEET )
    {
        m_frame->FocusOnLocation( node->m_RcItem->GetParent()->GetPos() );

        aEvent.Skip();
        return;
    }

    const KIID& itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM* item = board->GetItem( itemID );

    if( !item || item == DELETED_BOARD_ITEM::GetInstance() )
    {
        // nothing to highlight / focus on

        aEvent.Skip();
        return;
    }

    PCB_LAYER_ID principalLayer;
    LSET         violationLayers;
    BOARD_ITEM*  a = board->GetItem( rc_item->GetMainItemID() );
    BOARD_ITEM*  b = board->GetItem( rc_item->GetAuxItemID() );
    BOARD_ITEM*  c = board->GetItem( rc_item->GetAuxItem2ID() );
    BOARD_ITEM*  d = board->GetItem( rc_item->GetAuxItem3ID() );

    if( rc_item->GetErrorCode() == DRCE_MALFORMED_COURTYARD )
    {
        if( a && ( a->GetFlags() & MALFORMED_B_COURTYARD ) > 0
              && ( a->GetFlags() & MALFORMED_F_COURTYARD ) == 0 )
        {
            principalLayer = B_CrtYd;
        }
        else
        {
            principalLayer = F_CrtYd;
        }
    }
    else if (rc_item->GetErrorCode() == DRCE_INVALID_OUTLINE )
    {
        principalLayer = Edge_Cuts;
    }
    else
    {
        principalLayer = UNDEFINED_LAYER;

        if( a || b || c || d )
            violationLayers = LSET::AllLayersMask();

        // Try to initialize principalLayer to a valid layer.  Note that some markers have
        // a layer set to UNDEFINED_LAYER, so we may need to keep looking.

        for( BOARD_ITEM* it: { a, b, c, d } )
        {
            if( !it )
                continue;

            LSET layersList = getActiveLayers( it );
            violationLayers &= layersList;

            if( principalLayer <= UNDEFINED_LAYER && layersList.count() )
                principalLayer = layersList.Seq().front();
        }
    }

    if( violationLayers.count() )
        principalLayer = violationLayers.Seq().front();
    else if( !(principalLayer <= UNDEFINED_LAYER ) )
        violationLayers.set( principalLayer );

    WINDOW_THAWER thawer( m_frame );

    if( principalLayer > UNDEFINED_LAYER && ( violationLayers & board->GetVisibleLayers() ) == 0 )
        m_frame->GetAppearancePanel()->SetLayerVisible( principalLayer, true );

    if( principalLayer > UNDEFINED_LAYER && board->GetVisibleLayers().test( principalLayer ) )
        m_frame->SetActiveLayer( principalLayer );

    if( rc_item->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
    {
        if( !m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest )
            m_frame->GetToolManager()->RunAction( PCB_ACTIONS::showRatsnest );

        if( item->Type() == PCB_ZONE_T )
        {
            m_frame->FocusOnItem( item, principalLayer );

            m_frame->GetBoard()->GetConnectivity()->RunOnUnconnectedEdges(
                    [&]( CN_EDGE& edge )
                    {
                        // Connectivity was valid when DRC was run, but this is a modeless dialog
                        // so it might not be now.
                        if( !edge.GetSourceNode() || edge.GetSourceNode()->Dirty() )
                            return true;

                        if( !edge.GetTargetNode() || edge.GetTargetNode()->Dirty() )
                            return true;

                        if( edge.GetSourceNode()->Parent() == a
                            && edge.GetTargetNode()->Parent() == b )
                        {
                            VECTOR2I focusPos;

                            if( item == a && item == b )
                            {
                                focusPos = ( node->m_Type == RC_TREE_NODE::MAIN_ITEM )
                                                   ? edge.GetSourcePos()
                                                   : edge.GetTargetPos();
                            }
                            else
                            {
                                focusPos = ( item == edge.GetSourceNode()->Parent() )
                                                   ? edge.GetSourcePos()
                                                   : edge.GetTargetPos();
                            }

                            m_frame->FocusOnLocation( focusPos );
                            m_frame->RefreshCanvas();

                            return false;
                        }

                        return true;
                    } );
        }
        else
        {
            m_frame->FocusOnItem( item, principalLayer );
        }
    }
    else if( rc_item->GetErrorCode() == DRCE_DIFF_PAIR_UNCOUPLED_LENGTH_TOO_LONG )
    {
        BOARD_CONNECTED_ITEM*    track = dynamic_cast<PCB_TRACK*>( item );
        std::vector<BOARD_ITEM*> items;

        if( track )
        {
            int net = track->GetNetCode();

            wxASSERT( net > 0 );    // Without a net how can it be a diff-pair?

            for( const KIID& id : rc_item->GetIDs() )
            {
                auto* candidate = dynamic_cast<BOARD_CONNECTED_ITEM*>( board->GetItem( id ) );

                if( candidate && candidate->GetNetCode() == net )
                    items.push_back( candidate );
            }
        }
        else
        {
            items.push_back( item );
        }

        m_frame->FocusOnItems( items, principalLayer );
    }
    else
    {
        m_frame->FocusOnItem( item, principalLayer );
    }

    aEvent.Skip();
}


void DIALOG_DRC::OnDRCItemDClick( wxDataViewEvent& aEvent )
{
    if( aEvent.GetItem().IsOk() )
    {
        // turn control over to m_frame, hide this DIALOG_DRC window,
        // no destruction so we can preserve listbox cursor
        if( !IsModal() )
            Show( false );
    }

    // Do not skip aEvent here: this is not useful, and Pcbnew crashes
    // if skipped (at least on Windows)
}


void DIALOG_DRC::OnDRCItemRClick( wxDataViewEvent& aEvent )
{
    TOOL_MANAGER*          toolMgr = m_frame->GetToolManager();
    BOARD_INSPECTION_TOOL* inspectionTool = toolMgr->GetTool<BOARD_INSPECTION_TOOL>();
    RC_TREE_NODE*          node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );

    if( !node )
        return;

    std::shared_ptr<RC_ITEM>           rcItem = node->m_RcItem;
    DRC_ITEM*                          drcItem = static_cast<DRC_ITEM*>( rcItem.get() );
    std::shared_ptr<CONNECTIVITY_DATA> conn = m_currentBoard->GetConnectivity();
    wxString                           listName;
    wxMenu                             menu;

    switch( bds().m_DRCSeverities[ rcItem->GetErrorCode() ] )
    {
    case RPT_SEVERITY_ERROR:   listName = _( "errors" );      break;
    case RPT_SEVERITY_WARNING: listName = _( "warnings" );    break;
    default:                   listName = _( "appropriate" ); break;
    }

    enum MENU_IDS
    {
        ID_EDIT_EXCLUSION_COMMENT = 4467,
        ID_REMOVE_EXCLUSION,
        ID_REMOVE_EXCLUSION_ALL,
        ID_ADD_EXCLUSION,
        ID_ADD_EXCLUSION_ALL,
        ID_INSPECT_VIOLATION,
        ID_SET_SEVERITY_TO_ERROR,
        ID_SET_SEVERITY_TO_WARNING,
        ID_SET_SEVERITY_TO_IGNORE,
        ID_EDIT_SEVERITIES
    };

    if( rcItem->GetParent()->IsExcluded() )
    {
        menu.Append( ID_EDIT_EXCLUSION_COMMENT,
                     _( "Edit exclusion comment..." ) );

        menu.Append( ID_REMOVE_EXCLUSION,
                     _( "Remove exclusion for this violation" ),
                     wxString::Format( _( "It will be placed back in the %s list" ), listName ) );

        if( drcItem->GetViolatingRule() && !drcItem->GetViolatingRule()->m_Implicit )
        {
            menu.Append( ID_REMOVE_EXCLUSION_ALL,
                         wxString::Format( _( "Remove all exclusions for violations of rule '%s'" ),
                                           drcItem->GetViolatingRule()->m_Name ),
                         wxString::Format( _( "They will be placed back in the %s list" ), listName ) );
        }
    }
    else
    {
        menu.Append( ID_ADD_EXCLUSION,
                     _( "Exclude this violation..." ),
                     wxString::Format( _( "It will be excluded from the %s list" ), listName ) );

        if( drcItem->GetViolatingRule() && !drcItem->GetViolatingRule()->m_Implicit )
        {
            menu.Append( ID_ADD_EXCLUSION_ALL,
                         wxString::Format( _( "Exclude all violations of rule '%s'..." ),
                                           drcItem->GetViolatingRule()->m_Name ),
                         wxString::Format( _( "They will be excluded from the %s list" ), listName ) );
        }
    }

    wxString inspectDRCErrorMenuText = inspectionTool->InspectDRCErrorMenuText( rcItem );

    if( !inspectDRCErrorMenuText.IsEmpty() )
        menu.Append( ID_INSPECT_VIOLATION, inspectDRCErrorMenuText );

    menu.AppendSeparator();

    if( bds().m_DRCSeverities[ rcItem->GetErrorCode() ] == RPT_SEVERITY_WARNING )
    {
        menu.Append( ID_SET_SEVERITY_TO_ERROR,
                     wxString::Format( _( "Change severity to Error for all '%s' violations" ),
                                       rcItem->GetErrorText() ),
                     _( "Violation severities can also be edited in the Board Setup... dialog" ) );
    }
    else
    {
        menu.Append( ID_SET_SEVERITY_TO_WARNING,
                     wxString::Format( _( "Change severity to Warning for all '%s' violations" ),
                                       rcItem->GetErrorText() ),
                     _( "Violation severities can also be edited in the Board Setup... dialog" ) );
    }

    menu.Append( ID_SET_SEVERITY_TO_IGNORE,
                 wxString::Format( _( "Ignore all '%s' violations" ), rcItem->GetErrorText() ),
                 _( "Violations will not be checked or reported" ) );

    menu.AppendSeparator();

    menu.Append( ID_EDIT_SEVERITIES,
                 _( "Edit violation severities..." ),
                 _( "Open the Board Setup... dialog" ) );

    bool modified = false;

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case ID_EDIT_EXCLUSION_COMMENT:
        if( PCB_MARKER* marker = dynamic_cast<PCB_MARKER*>( node->m_RcItem->GetParent() ) )
        {
            WX_TEXT_ENTRY_DIALOG dlg( this, _( "Optional comment:" ), _( "Exclusion Comment" ),
                                      marker->GetComment(), true );

            if( dlg.ShowModal() == wxID_CANCEL )
                break;

            marker->SetExcluded( true, dlg.GetValue() );

            wxString serialized = marker->Serialize();
            m_frame->GetDesignSettings().m_DrcExclusions.insert( serialized );
            m_frame->GetDesignSettings().m_DrcExclusionComments[ serialized ] = dlg.GetValue();

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }

        break;

    case ID_REMOVE_EXCLUSION:
        if( PCB_MARKER* marker = dynamic_cast<PCB_MARKER*>( rcItem->GetParent() ) )
        {
            marker->SetExcluded( false );

            wxString serialized = marker->Serialize();
            m_frame->GetDesignSettings().m_DrcExclusions.erase( serialized );
            m_frame->GetDesignSettings().m_DrcExclusionComments.erase( serialized );

            if( rcItem->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
            {
                m_frame->GetBoard()->UpdateRatsnestExclusions();
                m_frame->GetCanvas()->RedrawRatsnest();
            }
            else
            {
                m_frame->GetCanvas()->GetView()->Update( marker );
            }

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }

        break;

    case ID_ADD_EXCLUSION:
        if( PCB_MARKER* marker = dynamic_cast<PCB_MARKER*>( rcItem->GetParent() ) )
        {
            WX_TEXT_ENTRY_DIALOG dlg( this, _( "Optional comment:" ), _( "Exclusion Comment" ),
                                      wxEmptyString, true );

            if( dlg.ShowModal() == wxID_CANCEL )
                break;

            marker->SetExcluded( true, dlg.GetValue() );

            wxString serialized = marker->Serialize();
            m_frame->GetDesignSettings().m_DrcExclusions.insert( serialized );
            m_frame->GetDesignSettings().m_DrcExclusionComments[ serialized ] = dlg.GetValue();

            if( rcItem->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
            {
                m_frame->GetBoard()->UpdateRatsnestExclusions();
                m_frame->GetCanvas()->RedrawRatsnest();
            }
            else
            {
                m_frame->GetCanvas()->GetView()->Update( marker );
            }

            // Update view
            if( m_severities & RPT_SEVERITY_EXCLUSION )
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            else
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->DeleteCurrentItem( false );

            modified = true;
        }

        break;

    case ID_REMOVE_EXCLUSION_ALL:
        for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
        {
            DRC_ITEM* candidateDrcItem = static_cast<DRC_ITEM*>( marker->GetRCItem().get() );

            if( candidateDrcItem->GetViolatingRule() == drcItem->GetViolatingRule() )
            {
                marker->SetExcluded( false );

                wxString serialized = marker->Serialize();
                m_frame->GetDesignSettings().m_DrcExclusions.erase( serialized );
                m_frame->GetDesignSettings().m_DrcExclusionComments.erase( serialized );
            }
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markersProvider, m_severities );
        modified = true;
        break;

    case ID_ADD_EXCLUSION_ALL:
        for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
        {
            DRC_ITEM* candidateDrcItem = static_cast<DRC_ITEM*>( marker->GetRCItem().get() );

            if( candidateDrcItem->GetViolatingRule() == drcItem->GetViolatingRule() )
            {
                marker->SetExcluded( true );

                wxString serialized = marker->Serialize();
                m_frame->GetDesignSettings().m_DrcExclusions.insert( serialized );
            }
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markersProvider, m_severities );
        modified = true;
        break;

    case ID_INSPECT_VIOLATION:
        inspectionTool->InspectDRCError( node->m_RcItem );
        break;

    case ID_SET_SEVERITY_TO_ERROR:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_ERROR;

        for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_frame->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markersProvider, m_severities );
        modified = true;
        break;

    case ID_SET_SEVERITY_TO_WARNING:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_WARNING;

        for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_frame->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markersProvider, m_severities );
        modified = true;
        break;

    case ID_SET_SEVERITY_TO_IGNORE:
    {
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_IGNORE;

        m_ignoredList->InsertItem( m_ignoredList->GetItemCount(),
                                   wxT( " • " ) + rcItem->GetErrorText() );

        BOARD* board = m_frame->GetBoard();

        std::vector<BOARD_ITEM*> toRemove;

        for( PCB_MARKER* marker : board->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
            {
                m_frame->GetCanvas()->GetView()->Remove( marker );
                toRemove.emplace_back( marker );
            }
        }

        for( BOARD_ITEM* marker : toRemove )
            board->Remove( marker, REMOVE_MODE::BULK );

        board->FinalizeBulkRemove( toRemove );

        if( rcItem->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
            m_frame->GetCanvas()->RedrawRatsnest();

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markersProvider, m_severities );
        modified = true;
        break;
    }

    case ID_EDIT_SEVERITIES:
        m_frame->ShowBoardSetupDialog( _( "Violation Severity" ) );
        break;
    }

    if( modified )
    {
        updateDisplayedCounts();
        refreshEditor();
        m_frame->OnModify();
    }
}


void DIALOG_DRC::OnEditViolationSeverities( wxHyperlinkEvent& aEvent )
{
    m_frame->ShowBoardSetupDialog( _( "Violation Severity" ) );
}


void DIALOG_DRC::OnSeverity( wxCommandEvent& aEvent )
{
    int flag = 0;

    if( aEvent.GetEventObject() == m_showAll )
        flag = RPT_SEVERITY_ALL;
    else if( aEvent.GetEventObject() == m_showErrors )
        flag = RPT_SEVERITY_ERROR;
    else if( aEvent.GetEventObject() == m_showWarnings )
        flag = RPT_SEVERITY_WARNING;
    else if( aEvent.GetEventObject() == m_showExclusions )
        flag = RPT_SEVERITY_EXCLUSION;

    if( aEvent.IsChecked() )
        m_severities |= flag;
    else if( aEvent.GetEventObject() == m_showAll )
        m_severities = RPT_SEVERITY_ERROR;
    else
        m_severities &= ~flag;

    syncCheckboxes();
    UpdateData();
}


void DIALOG_DRC::OnSaveReport( wxCommandEvent& aEvent )
{
    wxFileName fn( "DRC." + FILEEXT::ReportFileExtension );

    wxFileDialog dlg( this, _( "Save Report File" ), Prj().GetProjectPath(), fn.GetFullName(),
                      FILEEXT::ReportFileWildcard() + wxS( "|" ) + FILEEXT::JsonFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( FILEEXT::ReportFileExtension );

    if( !fn.IsAbsolute() )
    {
        wxString prj_path = Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    DRC_REPORT reportWriter( m_frame->GetBoard(), GetUserUnits(), m_markersProvider,
                             m_ratsnestProvider, m_fpWarningsProvider );

    bool success = false;
    if( fn.GetExt() == FILEEXT::JsonFileExtension )
        success = reportWriter.WriteJsonReport( fn.GetFullPath() );
    else
        success = reportWriter.WriteTextReport( fn.GetFullPath() );

    if( success )
    {
        m_messages->Report( wxString::Format( _( "Report file '%s' created<br>" ),
                                              fn.GetFullPath() ) );
    }
    else
    {
        DisplayError( this, wxString::Format( _( "Failed to create file '%s'." ),
                                              fn.GetFullPath() ) );
    }
}


void DIALOG_DRC::OnClose( wxCloseEvent& aEvent )
{
    if( m_running )
        aEvent.Veto();

    wxCommandEvent dummy;

    OnCancelClick( dummy );
}


void DIALOG_DRC::OnCancelClick( wxCommandEvent& aEvent )
{
    if( m_running )
    {
        m_cancelled = true;
        return;
    }

    m_frame->FocusOnItem( nullptr );

    SetReturnCode( wxID_CANCEL );

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    DRC_TOOL* drcTool = m_frame->GetToolManager()->GetTool<DRC_TOOL>();
    drcTool->DestroyDRCDialog();
}


void DIALOG_DRC::OnChangingNotebookPage( wxNotebookEvent& aEvent )
{
    m_markerDataView->UnselectAll();
    m_unconnectedDataView->UnselectAll();
    m_footprintsDataView->UnselectAll();

    aEvent.Skip();
}


void DIALOG_DRC::refreshEditor()
{
    WINDOW_THAWER thawer( m_frame );

    m_frame->GetCanvas()->Refresh();
}


void DIALOG_DRC::PrevMarker()
{
    if( m_Notebook->IsShown() )
    {
        switch( m_Notebook->GetSelection() )
        {
        case 0: m_markersTreeModel->PrevMarker();      break;
        case 1: m_unconnectedTreeModel->PrevMarker();  break;
        case 2: m_fpWarningsTreeModel->PrevMarker();   break;
        case 3:                                        break;
        }
    }
}


void DIALOG_DRC::NextMarker()
{
    if( m_Notebook->IsShown() )
    {
        switch( m_Notebook->GetSelection() )
        {
        case 0: m_markersTreeModel->NextMarker();      break;
        case 1: m_unconnectedTreeModel->NextMarker();  break;
        case 2: m_fpWarningsTreeModel->NextMarker();   break;
        case 3:                                        break;
        }
    }
}


void DIALOG_DRC::SelectMarker( const PCB_MARKER* aMarker )
{
    if( m_Notebook->IsShown() )
    {
        m_Notebook->SetSelection( 0 );
        m_markersTreeModel->SelectMarker( aMarker );

        CallAfter(
                [=]
                {
                    m_markersTreeModel->CenterMarker( aMarker );
                } );
    }
}


void DIALOG_DRC::ExcludeMarker()
{
    if( !m_Notebook->IsShown() || m_Notebook->GetSelection() != 0 )
        return;

    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( m_markerDataView->GetCurrentItem() );
    PCB_MARKER*   marker = dynamic_cast<PCB_MARKER*>( node->m_RcItem->GetParent() );

    if( marker && marker->GetSeverity() != RPT_SEVERITY_EXCLUSION )
    {
        marker->SetExcluded( true );
        m_frame->GetDesignSettings().m_DrcExclusions.insert( marker->Serialize() );
        m_frame->GetCanvas()->GetView()->Update( marker );

        // Update view
        if( m_severities & RPT_SEVERITY_EXCLUSION )
            m_markersTreeModel->ValueChanged( node );
        else
            m_markersTreeModel->DeleteCurrentItem( false );

        updateDisplayedCounts();
        refreshEditor();
        m_frame->OnModify();
    }
}


void DIALOG_DRC::deleteAllMarkers( bool aIncludeExclusions )
{
    // Clear current selection list to avoid selection of deleted items
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear );

    m_markersTreeModel->DeleteItems( false, aIncludeExclusions, false );
    m_unconnectedTreeModel->DeleteItems( false, aIncludeExclusions, false );
    m_fpWarningsTreeModel->DeleteItems( false, aIncludeExclusions, false );

    m_frame->GetBoard()->DeleteMARKERs( true, aIncludeExclusions );
}


void DIALOG_DRC::OnDeleteOneClick( wxCommandEvent& aEvent )
{
    if( m_Notebook->GetSelection() == 0 )
    {
        // Clear the selection.  It may be the selected DRC marker.
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear );

        m_markersTreeModel->DeleteCurrentItem( true );

        // redraw the pcb
        refreshEditor();
    }
    else if( m_Notebook->GetSelection() == 1 )
    {
        m_unconnectedTreeModel->DeleteCurrentItem( true );
    }
    else if( m_Notebook->GetSelection() == 2 )
    {
        m_fpWarningsTreeModel->DeleteCurrentItem( true );
    }

    updateDisplayedCounts();
}


void DIALOG_DRC::OnDeleteAllClick( wxCommandEvent& aEvent )
{
    static bool s_includeExclusions = false;

    int  numExcluded = 0;

    if( m_markersProvider )
        numExcluded += m_markersProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( m_ratsnestProvider )
        numExcluded += m_ratsnestProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( m_fpWarningsProvider )
        numExcluded += m_fpWarningsProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( numExcluded > 0 )
    {
        wxMessageDialog dlg( this, _( "Delete exclusions too?" ), _( "Delete All Markers" ),
                             wxYES_NO | wxCANCEL | wxCENTER | wxICON_QUESTION );
        dlg.SetYesNoLabels( _( "Errors and Warnings Only" ),
                            _( "Errors, Warnings and Exclusions" ) );

        int ret = dlg.ShowModal();

        if( ret == wxID_CANCEL )
            return;
        else if( ret == wxID_NO )
            s_includeExclusions = true;
    }

    deleteAllMarkers( s_includeExclusions );

    refreshEditor();
    updateDisplayedCounts();
}


void DIALOG_DRC::updateDisplayedCounts()
{
    BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();
    DRC_TOOL*              drcTool = m_frame->GetToolManager()->GetTool<DRC_TOOL>();
    DRC_ENGINE*            drcEngine = drcTool->GetDRCEngine().get();

    // Collect counts:

    int numMarkers = 0;
    int numUnconnected = 0;
    int numFootprints = 0;

    int numErrors = 0;
    int numWarnings = 0;
    int numExcluded = 0;

    if( m_markersProvider )
    {
        numMarkers += m_markersProvider->GetCount();
        numErrors += m_markersProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_markersProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_markersProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    if( m_ratsnestProvider )
    {
        numUnconnected += m_ratsnestProvider->GetCount();
        numErrors += m_ratsnestProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_ratsnestProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_ratsnestProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    if( m_footprintTestsRun && m_fpWarningsProvider )
    {
        numFootprints += m_fpWarningsProvider->GetCount();
        numErrors += m_fpWarningsProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_fpWarningsProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_fpWarningsProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    bool showErrors = m_showErrors->GetValue();
    bool showWarnings = m_showWarnings->GetValue();
    bool errorsOverflowed = false;
    bool warningsOverflowed = false;
    bool markersOverflowed = false;
    bool unconnectedOverflowed = false;
    bool footprintsOverflowed = false;

    for( int ii = DRCE_FIRST; ii < DRCE_LAST; ++ii )
    {
        if( drcEngine->IsErrorLimitExceeded( ii ) )
        {
            if( bds.GetSeverity( ii ) == RPT_SEVERITY_ERROR )
                errorsOverflowed = true;
            else if( bds.GetSeverity( ii ) == RPT_SEVERITY_WARNING )
                warningsOverflowed = true;

            if( ii == DRCE_UNCONNECTED_ITEMS )
            {
                if( showWarnings && bds.GetSeverity( ii ) == RPT_SEVERITY_WARNING )
                    unconnectedOverflowed = true;
                else if( showErrors && bds.GetSeverity( ii ) == RPT_SEVERITY_ERROR )
                    unconnectedOverflowed = true;
            }
            else if(    ii == DRCE_MISSING_FOOTPRINT
                     || ii == DRCE_DUPLICATE_FOOTPRINT
                     || ii == DRCE_EXTRA_FOOTPRINT
                     || ii == DRCE_NET_CONFLICT
                     || ii == DRCE_SCHEMATIC_PARITY_ISSUES )
            {
                if( showWarnings && bds.GetSeverity( ii ) == RPT_SEVERITY_WARNING )
                    footprintsOverflowed = true;
                else if( showErrors && bds.GetSeverity( ii ) == RPT_SEVERITY_ERROR )
                    footprintsOverflowed = true;
            }
            else
            {
                if( showWarnings && bds.GetSeverity( ii ) == RPT_SEVERITY_WARNING )
                    markersOverflowed = true;
                else if( showErrors && bds.GetSeverity( ii ) == RPT_SEVERITY_ERROR )
                    markersOverflowed = true;
            }
        }
    }

    wxString msg;
    wxString num;

    // Update tab headers:

    if( m_drcRun )
    {
        num.Printf( markersOverflowed ? wxT( "%d+" ) : wxT( "%d" ), numMarkers );
        msg.Printf( m_markersTitleTemplate, num );
    }
    else
    {
        msg = m_markersTitleTemplate;
        msg.Replace( wxT( "(%s)" ), wxEmptyString );
    }

    m_Notebook->SetPageText( 0, msg );

    if( m_drcRun )
    {
        num.Printf( unconnectedOverflowed ? wxT( "%d+" ) : wxT( "%d" ), numUnconnected );
        msg.sprintf( m_unconnectedTitleTemplate, num );
    }
    else
    {
        msg = m_unconnectedTitleTemplate;
        msg.Replace( wxT( "(%s)" ), wxEmptyString );
    }

    m_Notebook->SetPageText( 1, msg );

    if( m_footprintTestsRun )
    {
        num.Printf( footprintsOverflowed ? wxT( "%d+" ) : wxT( "%d" ), numFootprints );
        msg.sprintf( m_footprintsTitleTemplate, num );
    }
    else if( m_drcRun )
    {
        msg = m_footprintsTitleTemplate;
        msg.Replace( wxT( "%s" ), _( "not run" ) );
    }
    else
    {
        msg = m_footprintsTitleTemplate;
        msg.Replace( wxT( "(%s)" ), wxEmptyString );
    }

    m_Notebook->SetPageText( 2, msg );

    if( m_drcRun )
    {
        num.Printf( wxT( "%d" ), m_ignoredList->GetItemCount() );
        msg.sprintf( m_ignoredTitleTemplate, num );
    }
    else
    {
        msg = m_ignoredTitleTemplate;
        msg.Replace( wxT( "(%s)" ), wxEmptyString );
    }

    m_Notebook->SetPageText( 3, msg );

    // Update badges:

    if( !m_drcRun && numErrors == 0 )
        numErrors = -1;

    if( !m_drcRun && numWarnings == 0 )
        numWarnings = -1;

    m_errorsBadge->SetMaximumNumber( numErrors );
    m_errorsBadge->UpdateNumber( errorsOverflowed ? numErrors + 1 : numErrors,
                                 RPT_SEVERITY_ERROR );

    m_warningsBadge->SetMaximumNumber( numWarnings );
    m_warningsBadge->UpdateNumber( warningsOverflowed ? numWarnings + 1 : numWarnings,
                                   RPT_SEVERITY_WARNING );

    m_exclusionsBadge->SetMaximumNumber( numExcluded );
    m_exclusionsBadge->UpdateNumber( numExcluded, RPT_SEVERITY_EXCLUSION );
}
