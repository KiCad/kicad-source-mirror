/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <macros.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <wildcards_and_files_ext.h>
#include <pcb_marker.h>
#include <wx/wupdlock.h>
#include <widgets/appearance_controls.h>
#include <widgets/ui_common.h>
#include <widgets/progress_reporter.h>
#include <dialogs/wx_html_report_box.h>
#include <dialogs/panel_setup_rules_base.h>
#include <tools/drc_tool.h>
#include <tools/board_inspection_tool.h>
#include <kiplatform/ui.h>

DIALOG_DRC::DIALOG_DRC( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent ) :
        DIALOG_DRC_BASE( aParent ),
        PROGRESS_REPORTER( 1 ),
        m_running( false ),
        m_cancelled( false ),
        m_drcRun( false ),
        m_footprintTestsRun( false ),
        m_markersProvider( nullptr ),
        m_markersTreeModel( nullptr ),
        m_unconnectedItemsProvider( nullptr ),
        m_unconnectedTreeModel( nullptr ),
        m_footprintWarningsProvider( nullptr ),
        m_footprintWarningsTreeModel( nullptr ),
        m_severities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING )
{
    SetName( DIALOG_DRC_WINDOW_NAME ); // Set a window name to be able to find it

    m_frame    = aEditorFrame;
    m_currentBoard = m_frame->GetBoard();

    m_messages->SetImmediateMode();

    m_markersTreeModel = new RC_TREE_MODEL( m_frame, m_markerDataView );
    m_markerDataView->AssociateModel( m_markersTreeModel );

    m_unconnectedTreeModel = new RC_TREE_MODEL( m_frame, m_unconnectedDataView );
    m_unconnectedDataView->AssociateModel( m_unconnectedTreeModel );

    m_footprintWarningsTreeModel = new RC_TREE_MODEL( m_frame, m_footprintsDataView );
    m_footprintsDataView->AssociateModel( m_footprintWarningsTreeModel );

    if( Kiface().IsSingle() )
        m_cbTestFootprints->Hide();

    // We use a sdbSizer here to get the order right, which is platform-dependent
    m_sdbSizerOK->SetLabel( _( "Run DRC" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sizerButtons->Layout();

    m_sdbSizerOK->SetDefault();

    initValues();
    syncCheckboxes();

    finishDialogSettings();
}


DIALOG_DRC::~DIALOG_DRC()
{
    m_frame->FocusOnItem( nullptr );

    PCBNEW_SETTINGS* settings = m_frame->GetPcbNewSettings();
    settings->m_DrcDialog.refill_zones          = m_cbRefillZones->GetValue();
    settings->m_DrcDialog.test_all_track_errors = m_cbReportAllTrackErrors->GetValue();

    if( !Kiface().IsSingle() )
        settings->m_DrcDialog.test_footprints   = m_cbTestFootprints->GetValue();

    settings->m_DrcDialog.severities            = m_severities;

    m_markersTreeModel->DecRef();
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


void DIALOG_DRC::initValues()
{
    m_markersTitleTemplate     = m_Notebook->GetPageText( 0 );
    m_unconnectedTitleTemplate = m_Notebook->GetPageText( 1 );
    m_footprintsTitleTemplate  = m_Notebook->GetPageText( 2 );

    auto cfg = m_frame->GetPcbNewSettings();

    m_cbRefillZones->SetValue( cfg->m_DrcDialog.refill_zones );
    m_cbReportAllTrackErrors->SetValue( cfg->m_DrcDialog.test_all_track_errors );


    if( !Kiface().IsSingle() )
        m_cbTestFootprints->SetValue( cfg->m_DrcDialog.test_footprints );

    m_severities = cfg->m_DrcDialog.severities;
    m_markersTreeModel->SetSeverities( m_severities );
    m_unconnectedTreeModel->SetSeverities( m_severities );
    m_footprintWarningsTreeModel->SetSeverities( m_severities );

    Layout(); // adding the units above expanded Clearance text, now resize.

    SetFocus();
}


// PROGRESS_REPORTER calls

bool DIALOG_DRC::updateUI()
{
    double cur = (double) m_progress.load() / m_maxProgress;
    cur = std::max( 0.0, std::min( cur, 1.0 ) );

    m_gauge->SetValue( KiROUND( cur * 1000.0 ) );
    wxSafeYield( this );

    return !m_cancelled;
}


void DIALOG_DRC::AdvancePhase( const wxString& aMessage )
{
    PROGRESS_REPORTER::AdvancePhase( aMessage );
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
    m_frame->ShowBoardSetupDialog( _( "Rules" ) );
}


void DIALOG_DRC::OnRunDRCClick( wxCommandEvent& aEvent )
{
    DRC_TOOL* drcTool = m_frame->GetToolManager()->GetTool<DRC_TOOL>();
    bool      refillZones            = m_cbRefillZones->GetValue();
    bool      reportAllTrackErrors   = m_cbReportAllTrackErrors->GetValue();
    bool      testFootprints         = m_cbTestFootprints->GetValue();

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
        m_messages->Report( _( "DRC incomplete: could not compile custom design rules.  " )
                            + wxT( "<a href='boardsetup'>" )
                            + _( "Show design rules." )
                            + wxT( "</a>" ) );
        m_messages->Flush();

        Raise();
        return;
    }

    m_drcRun = false;
    m_footprintTestsRun = false;
    m_cancelled = false;

    m_frame->RecordDRCExclusions();
    deleteAllMarkers( true );
    m_unconnectedTreeModel->DeleteItems( false, true, true );
    m_footprintWarningsTreeModel->DeleteItems( false, true, true );

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

    drcTool->RunTests( this, refillZones, reportAllTrackErrors, testFootprints );

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
        wxMilliSleep( 500 );
        m_runningResultsBook->ChangeSelection( 1 );
        KIPLATFORM::UI::ForceFocus( m_markerDataView );
    }

    refreshEditor();
}


void DIALOG_DRC::SetMarkersProvider( RC_ITEMS_PROVIDER* aProvider )
{
    m_markersProvider = aProvider;
    m_markersTreeModel->SetProvider( m_markersProvider );
    updateDisplayedCounts();
}


void DIALOG_DRC::SetUnconnectedProvider( class RC_ITEMS_PROVIDER * aProvider )
{
    m_unconnectedItemsProvider = aProvider;
    m_unconnectedTreeModel->SetProvider( m_unconnectedItemsProvider );
    updateDisplayedCounts();
}


void DIALOG_DRC::SetFootprintsProvider( RC_ITEMS_PROVIDER* aProvider )
{
    m_footprintWarningsProvider = aProvider;
    m_footprintWarningsTreeModel->SetProvider( m_footprintWarningsProvider );
    updateDisplayedCounts();
}


void DIALOG_DRC::OnDRCItemSelected( wxDataViewEvent& aEvent )
{
    BOARD*        board = m_frame->GetBoard();
    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );
    const KIID&   itemID = node ? RC_TREE_MODEL::ToUUID( aEvent.GetItem() ) : niluuid;
    BOARD_ITEM*   item = board->GetItem( itemID );

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

    if( node && item )
    {
        PCB_LAYER_ID             principalLayer = item->GetLayer();
        LSET                     violationLayers;
        std::shared_ptr<RC_ITEM> rc_item = node->m_RcItem;

        if( rc_item->GetErrorCode() == DRCE_MALFORMED_COURTYARD )
        {
            BOARD_ITEM* a = board->GetItem( rc_item->GetMainItemID() );

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
            BOARD_ITEM*  a = board->GetItem( rc_item->GetMainItemID() );
            BOARD_ITEM*  b = board->GetItem( rc_item->GetAuxItemID() );
            BOARD_ITEM*  c = board->GetItem( rc_item->GetAuxItem2ID() );
            BOARD_ITEM*  d = board->GetItem( rc_item->GetAuxItem3ID() );

            if( a || b || c || d )
                violationLayers = LSET::AllLayersMask();

            if( a )
                violationLayers &= getActiveLayers( a );

            if( b )
                violationLayers &= getActiveLayers( b );

            if( c )
                violationLayers &= getActiveLayers( c );

            if( d )
                violationLayers &= getActiveLayers( d );
        }

        if( violationLayers.count() )
            principalLayer = violationLayers.Seq().front();
        else
            violationLayers.set( principalLayer );

        WINDOW_THAWER thawer( m_frame );

        m_frame->FocusOnItem( item );
        m_frame->GetCanvas()->Refresh();

        if( ( violationLayers & board->GetVisibleLayers() ) == 0 )
        {
            m_frame->GetAppearancePanel()->SetLayerVisible( principalLayer, true );
            m_frame->GetCanvas()->Refresh();
        }

        if( board->GetVisibleLayers().test( principalLayer ) )
            m_frame->SetActiveLayer( principalLayer );
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

    // Do not skip aVent here: tihs is not useful, and Pcbnew crashes
    // if skipped (at least on Windows)
}


void DIALOG_DRC::OnDRCItemRClick( wxDataViewEvent& aEvent )
{
    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );

    if( !node )
        return;

    std::shared_ptr<RC_ITEM>  rcItem = node->m_RcItem;
    wxString  listName;
    wxMenu    menu;
    wxString  msg;

    switch( bds().m_DRCSeverities[ rcItem->GetErrorCode() ] )
    {
    case RPT_SEVERITY_ERROR:   listName = _( "errors" );      break;
    case RPT_SEVERITY_WARNING: listName = _( "warnings" );    break;
    default:                   listName = _( "appropriate" ); break;
    }

    if( rcItem->GetParent()->IsExcluded() )
    {
        menu.Append( 1, _( "Remove exclusion for this violation" ),
                     wxString::Format( _( "It will be placed back in the %s list" ), listName ) );
    }
    else
    {
        menu.Append( 2, _( "Exclude this violation" ),
                     wxString::Format( _( "It will be excluded from the %s list" ), listName ) );
    }

    if( rcItem->GetErrorCode() == DRCE_CLEARANCE
            || rcItem->GetErrorCode() == DRCE_COPPER_EDGE_CLEARANCE )
    {
        menu.Append( 3, _( "Run clearance resolution tool..." ) );
    }

    menu.AppendSeparator();

    if( bds().m_DRCSeverities[ rcItem->GetErrorCode() ] == RPT_SEVERITY_WARNING )
    {
        msg.Printf( _( "Change severity to Error for all '%s' violations" ),
                    rcItem->GetErrorText(),
                    _( "Violation severities can also be edited in the Board Setup... dialog" ) );
        menu.Append( 4, msg );
    }
    else
    {
        msg.Printf( _( "Change severity to Warning for all '%s' violations" ),
                    rcItem->GetErrorText(),
                    _( "Violation severities can also be edited in the Board Setup... dialog" ) );
        menu.Append( 5, msg );
    }

    msg.Printf( _( "Ignore all '%s' violations" ),
                rcItem->GetErrorText(),
                _( "Violations will not be checked or reported" ) );
    menu.Append( 6, msg );

    menu.AppendSeparator();

    menu.Append( 7, _( "Edit violation severities..." ), _( "Open the Board Setup... dialog" ) );

    bool modified = false;

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case 1:
    {
        PCB_MARKER* marker = dynamic_cast<PCB_MARKER*>( node->m_RcItem->GetParent() );

        if( marker )
        {
            marker->SetExcluded( false );
            m_frame->GetCanvas()->GetView()->Update( marker );

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }
    }
        break;

    case 2:
    {
        PCB_MARKER* marker = dynamic_cast<PCB_MARKER*>( node->m_RcItem->GetParent() );

        if( marker )
        {
            marker->SetExcluded( true );
            m_frame->GetCanvas()->GetView()->Update( marker );

            // Update view
            if( m_severities & RPT_SEVERITY_EXCLUSION )
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            else
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->DeleteCurrentItem( false );

            modified = true;
        }
    }
        break;

    case 3:
    {
        TOOL_MANAGER*          toolMgr = m_frame->GetToolManager();
        BOARD_INSPECTION_TOOL* inspectionTool = toolMgr->GetTool<BOARD_INSPECTION_TOOL>();

        inspectionTool->InspectDRCError( node->m_RcItem );
    }
        break;

    case 4:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_ERROR;

        for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_frame->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
        break;

    case 5:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_WARNING;

        for( PCB_MARKER* marker : m_frame->GetBoard()->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_frame->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
        break;

    case 6:
    {
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_IGNORE;

        std::vector<PCB_MARKER*>& markers = m_frame->GetBoard()->Markers();

        for( unsigned i = 0; i < markers.size(); )
        {
            if( markers[i]->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
            {
                m_frame->GetCanvas()->GetView()->Remove( markers.at( i ) );
                markers.erase( markers.begin() + i );
            }
            else
                ++i;
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
    }
        break;

    case 7:
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

    // Set the provider's severity levels through the TreeModel so that the old tree
    // can be torn down before the severity changes.
    //
    // It's not clear this is required, but we've had a lot of issues with wxDataView
    // being cranky on various platforms.

    m_markersTreeModel->SetSeverities( m_severities );
    m_unconnectedTreeModel->SetSeverities( m_severities );
    m_footprintWarningsTreeModel->SetSeverities( m_severities );

    updateDisplayedCounts();
}


void DIALOG_DRC::OnSaveReport( wxCommandEvent& aEvent )
{
    wxFileName fn( "./DRC." + ReportFileExtension );

    wxFileDialog dlg( this, _( "Save Report to File" ), fn.GetPath(), fn.GetFullName(),
                      ReportFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( ReportFileExtension );

    if( !fn.IsAbsolute() )
    {
        wxString prj_path = Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    if( writeReport( fn.GetFullPath() ) )
    {
        m_messages->Report( wxString::Format( _( "Report file '%s' created<br>" ),
                                              fn.GetFullPath() ) );
    }
    else
    {
        DisplayError( this, wxString::Format( _( "Unable to create report file '%s'<br>" ),
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
    // Shouldn't be necessary, but is on at least OSX
    if( aEvent.GetSelection() >= 0 )
        m_Notebook->ChangeSelection( (unsigned) aEvent.GetSelection() );

    m_markerDataView->UnselectAll();
    m_unconnectedDataView->UnselectAll();
    m_footprintsDataView->UnselectAll();
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
        case 0: m_markersTreeModel->PrevMarker();           break;
        case 1: m_unconnectedTreeModel->PrevMarker();       break;
        case 2: m_footprintWarningsTreeModel->PrevMarker(); break;
        }
    }
}


void DIALOG_DRC::NextMarker()
{
    if( m_Notebook->IsShown() )
    {
        switch( m_Notebook->GetSelection() )
        {
        case 0: m_markersTreeModel->NextMarker();           break;
        case 1: m_unconnectedTreeModel->NextMarker();       break;
        case 2: m_footprintWarningsTreeModel->NextMarker(); break;
        }
    }
}


void DIALOG_DRC::ExcludeMarker()
{
    if( !m_Notebook->IsShown() || m_Notebook->GetSelection() != 0 )
        return;

    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( m_markerDataView->GetCurrentItem() );
    PCB_MARKER*   marker = dynamic_cast<PCB_MARKER*>( node->m_RcItem->GetParent() );

    if( marker && !marker->IsExcluded() )
    {
        marker->SetExcluded( true );
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
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    m_markersTreeModel->DeleteItems( false, aIncludeExclusions, true );
}


bool DIALOG_DRC::writeReport( const wxString& aFullFileName )
{
    FILE* fp = wxFopen( aFullFileName, wxT( "w" ) );

    if( fp == NULL )
        return false;

    std::map<KIID, EDA_ITEM*> itemMap;
    m_frame->GetBoard()->FillItemMap( itemMap );

    EDA_UNITS              units = GetUserUnits();
    BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();
    int                    count;

    fprintf( fp, "** Drc report for %s **\n", TO_UTF8( m_frame->GetBoard()->GetFileName() ) );

    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "** Created on %s **\n", TO_UTF8( now.Format( wxT( "%F %T" ) ) ) );

    count = m_markersProvider->GetCount();

    fprintf( fp, "\n** Found %d DRC violations **\n", count );

    for( int i = 0; i < count; ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_markersProvider->GetItem( i );
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );

        fprintf( fp, "%s", TO_UTF8( item->ShowReport( units, severity, itemMap ) ) );
    }

    count = m_unconnectedItemsProvider->GetCount();

    fprintf( fp, "\n** Found %d unconnected pads **\n", count );

    for( int i = 0; i < count; ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_unconnectedItemsProvider->GetItem( i );
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );

        fprintf( fp, "%s", TO_UTF8( item->ShowReport( units, severity, itemMap ) ) );
    }

    count = m_footprintWarningsProvider->GetCount();

    fprintf( fp, "\n** Found %d Footprint errors **\n", count );

    for( int i = 0; i < count; ++i )
    {
        const std::shared_ptr<RC_ITEM>& item = m_footprintWarningsProvider->GetItem( i );
        SEVERITY severity = bds.GetSeverity( item->GetErrorCode() );

        fprintf( fp, "%s", TO_UTF8( item->ShowReport( units, severity, itemMap ) ) );
    }


    fprintf( fp, "\n** End of Report **\n" );

    fclose( fp );

    return true;
}


void DIALOG_DRC::OnDeleteOneClick( wxCommandEvent& aEvent )
{
    if( m_Notebook->GetSelection() == 0 )
    {
        // Clear the selection.  It may be the selected DRC marker.
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

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
        m_footprintWarningsTreeModel->DeleteCurrentItem( true );
    }

    updateDisplayedCounts();
}


void DIALOG_DRC::OnDeleteAllClick( wxCommandEvent& aEvent )
{
    static bool s_includeExclusions = false;

    int  numExcluded = 0;

    if( m_markersProvider )
        numExcluded += m_markersProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( m_unconnectedItemsProvider )
        numExcluded += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( m_footprintWarningsProvider )
        numExcluded += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( numExcluded > 0 )
    {
        wxRichMessageDialog dlg( this, _( "Do you wish to delete excluded markers as well?" ),
                                 _( "Delete All Markers" ),
                                 wxOK | wxCANCEL | wxCENTER | wxICON_QUESTION );
        dlg.ShowCheckBox( _( "Delete exclusions" ), s_includeExclusions );

        int ret = dlg.ShowModal();

        if( ret == wxID_CANCEL )
            return;
        else
            s_includeExclusions = dlg.IsCheckBoxChecked();
    }

    deleteAllMarkers( s_includeExclusions );

    m_drcRun = false;
    refreshEditor();
    updateDisplayedCounts();
}


void DIALOG_DRC::updateDisplayedCounts()
{
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

    if( m_unconnectedItemsProvider )
    {
        numUnconnected += m_unconnectedItemsProvider->GetCount();
        numErrors += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    if( m_footprintTestsRun && m_footprintWarningsProvider )
    {
        numFootprints += m_footprintWarningsProvider->GetCount();
        numErrors += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    wxString msg;

    // Update tab headers:

    if( m_drcRun )
    {
        msg.sprintf( m_markersTitleTemplate, numMarkers );
        m_Notebook->SetPageText( 0, msg );

        msg.sprintf( m_unconnectedTitleTemplate, numUnconnected );
        m_Notebook->SetPageText( 1, msg );

        if( m_footprintTestsRun )
            msg.sprintf( m_footprintsTitleTemplate, numFootprints );
        else
        {
            msg = m_footprintsTitleTemplate;
            msg.Replace( wxT( "%d" ), _( "not run" ) );
        }
        m_Notebook->SetPageText( 2, msg );
    }
    else
    {
        msg = m_markersTitleTemplate;
        msg.Replace( wxT( "(%d)" ), wxEmptyString );
        m_Notebook->SetPageText( 0, msg );

        msg = m_unconnectedTitleTemplate;
        msg.Replace( wxT( "(%d)" ), wxEmptyString );
        m_Notebook->SetPageText( 1, msg );

        msg = m_footprintsTitleTemplate;
        msg.Replace( wxT( "(%d)" ), wxEmptyString );
        m_Notebook->SetPageText( 2, msg );
    }

    // Update badges:

    if( !m_drcRun && numErrors == 0 )
        numErrors = -1;

    if( !m_drcRun && numWarnings == 0 )
        numWarnings = -1;

    m_errorsBadge->SetMaximumNumber( numErrors );
    m_errorsBadge->UpdateNumber( numErrors, RPT_SEVERITY_ERROR );

    m_warningsBadge->SetMaximumNumber( numWarnings );
    m_warningsBadge->UpdateNumber( numWarnings, RPT_SEVERITY_WARNING );

    m_exclusionsBadge->SetMaximumNumber( numExcluded );
    m_exclusionsBadge->UpdateNumber( numExcluded, RPT_SEVERITY_EXCLUSION );
}
