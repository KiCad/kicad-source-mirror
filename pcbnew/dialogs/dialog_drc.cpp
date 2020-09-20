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

#include <bitmaps.h>
#include <confirm.h>
#include <dialog_drc.h>
#include <fctsys.h>
#include <kiface_i.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <wildcards_and_files_ext.h>
#include <class_marker_pcb.h>
#include <wx/wupdlock.h>
#include <widgets/appearance_controls.h>
#include <widgets/ui_common.h>
#include <widgets/progress_reporter.h>
#include <drc/drc_engine.h>
#include <tools/drc_tool.h>
#include <kiplatform/ui.h>

DIALOG_DRC::DIALOG_DRC( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent ) :
        DIALOG_DRC_BASE( aParent ),
        PROGRESS_REPORTER( 1 ),
        m_running( false ),
        m_cancelled( false ),
        m_drcRun( false ),
        m_footprintTestsRun( false ),
        m_markersProvider( nullptr ),
        m_markerTreeModel( nullptr ),
        m_unconnectedItemsProvider( nullptr ),
        m_unconnectedTreeModel( nullptr ),
        m_footprintWarningsProvider( nullptr ),
        m_footprintWarningsTreeModel( nullptr ),
        m_severities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING )
{
    SetName( DIALOG_DRC_WINDOW_NAME ); // Set a window name to be able to find it

    m_brdEditor    = aEditorFrame;
    m_currentBoard = m_brdEditor->GetBoard();

    m_markerTreeModel = new RC_TREE_MODEL( m_brdEditor, m_markerDataView );
    m_markerDataView->AssociateModel( m_markerTreeModel );

    m_unconnectedTreeModel = new RC_TREE_MODEL( m_brdEditor, m_unconnectedDataView );
    m_unconnectedDataView->AssociateModel( m_unconnectedTreeModel );

    m_footprintWarningsTreeModel = new RC_TREE_MODEL( m_brdEditor, m_footprintsDataView );
    m_footprintsDataView->AssociateModel( m_footprintWarningsTreeModel );

    if( Kiface().IsSingle() )
        m_cbTestFootprints->Hide();

    // We use a sdbSizer here to get the order right, which is platform-dependent
    m_sdbSizer1OK->SetLabel( _( "Run DRC" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sizerButtons->Layout();

    m_sdbSizer1OK->SetDefault();

    initValues();
    syncCheckboxes();

    FinishDialogSettings();
}


DIALOG_DRC::~DIALOG_DRC()
{
    m_brdEditor->FocusOnItem( nullptr );

    PCBNEW_SETTINGS* settings = m_brdEditor->GetPcbNewSettings();
    settings->m_DrcDialog.refill_zones          = m_cbRefillZones->GetValue();
    settings->m_DrcDialog.test_track_to_zone    = m_cbReportTracksToZonesErrors->GetValue();
    settings->m_DrcDialog.test_all_track_errors = m_cbReportAllTrackErrors->GetValue();

    if( !Kiface().IsSingle() )
        settings->m_DrcDialog.test_footprints   = m_cbTestFootprints->GetValue();

    settings->m_DrcDialog.severities            = m_severities;

    m_markerTreeModel->DecRef();
}


void DIALOG_DRC::OnActivateDlg( wxActivateEvent& aEvent )
{
    if( m_currentBoard != m_brdEditor->GetBoard() )
    {
        // If m_currentBoard is not the current board, (for instance because a new board
        // was loaded), close the dialog, because many pointers are now invalid in lists
        SetReturnCode( wxID_CANCEL );
        Close();

        DRC_TOOL* drcTool = m_brdEditor->GetToolManager()->GetTool<DRC_TOOL>();
        drcTool->DestroyDRCDialog();

        return;
    }

    m_markerTreeModel->SetProvider( m_markersProvider );
    m_unconnectedTreeModel->SetProvider( m_unconnectedItemsProvider );
    m_footprintWarningsTreeModel->SetProvider( m_footprintWarningsProvider );
    updateDisplayedCounts();
}


void DIALOG_DRC::initValues()
{
    m_markersTitleTemplate     = m_Notebook->GetPageText( 0 );
    m_unconnectedTitleTemplate = m_Notebook->GetPageText( 1 );
    m_footprintsTitleTemplate  = m_Notebook->GetPageText( 2 );

    auto cfg = m_brdEditor->GetPcbNewSettings();

    m_cbRefillZones->SetValue( cfg->m_DrcDialog.refill_zones );
    m_cbReportTracksToZonesErrors->SetValue( cfg->m_DrcDialog.test_track_to_zone );
    m_cbReportAllTrackErrors->SetValue( cfg->m_DrcDialog.test_all_track_errors );

    if( Kiface().IsSingle() )
        m_cbTestFootprints->SetValue( cfg->m_DrcDialog.test_footprints );

    m_severities = cfg->m_DrcDialog.severities;
    m_markerTreeModel->SetSeverities( m_severities );
    m_unconnectedTreeModel->SetSeverities( m_severities );
    m_footprintWarningsTreeModel->SetSeverities( m_severities );

    Layout(); // adding the units above expanded Clearance text, now resize.

    SetFocus();
}


// PROGRESS_REPORTER calls

bool DIALOG_DRC::updateUI()
{
    int cur = std::max( 0, std::min( m_progress.load(), 10000 ) );

    m_gauge->SetValue( cur );
    wxSafeYield( this );

    return !m_cancelled;
}


void DIALOG_DRC::AdvancePhase( const wxString& aMessage )
{
    PROGRESS_REPORTER::AdvancePhase( aMessage );

    m_Messages->AppendText( aMessage + "\n" );
}


void DIALOG_DRC::SetCurrentProgress( double aProgress )
{
    PROGRESS_REPORTER::SetCurrentProgress( aProgress );
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


void DIALOG_DRC::OnRunDRCClick( wxCommandEvent& aEvent )
{
    DRC_TOOL*             drcTool = m_parentFrame->GetToolManager()->GetTool<DRC_TOOL>();
    bool                  testTracksAgainstZones = m_cbReportTracksToZonesErrors->GetValue();
    bool                  refillZones            = m_cbRefillZones->GetValue();
    bool                  reportAllTrackErrors   = m_cbReportAllTrackErrors->GetValue();
    bool                  testFootprints         = m_cbTestFootprints->GetValue();

    m_drcRun = false;
    m_footprintTestsRun = false;
    m_cancelled = false;

    m_brdEditor->RecordDRCExclusions();
    deleteAllMarkers( true );
    m_unconnectedTreeModel->DeleteItems( false, true, true );
    m_footprintWarningsTreeModel->DeleteItems( false, true, true );

    Raise();

    m_runningResultsBook->ChangeSelection( 0 );   // Display the "Tests Running..." tab
    m_Messages->Clear();
    wxYield();                                    // Allow time slice to refresh Messages

    m_running = true;
    m_sdbSizer1Cancel->SetLabel( _( "Cancel" ) );

    drcTool->RunTests( this, testTracksAgainstZones, refillZones, reportAllTrackErrors,
                       testFootprints );

    if( m_cancelled )
        m_Messages->AppendText( _( "-------- DRC cancelled by user.\n\n" ) );
    else
        m_Messages->AppendText( _( "Done.\n\n" ) );

    Raise();
    wxYield();                                    // Allow time slice to refresh Messages

    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_running = false;

    if( !m_cancelled )
    {
        wxMilliSleep( 500 );
        m_runningResultsBook->ChangeSelection( 1 );
        KIPLATFORM::UI::ForceFocus( m_markerDataView );
    }

    refreshBoardEditor();
}


void DIALOG_DRC::SetMarkersProvider( RC_ITEMS_PROVIDER* aProvider )
{
    m_markersProvider = aProvider;
    m_markerTreeModel->SetProvider( m_markersProvider );
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
    BOARD*        board = m_brdEditor->GetBoard();
    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );
    const KIID&   itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM*   item = board->GetItem( itemID );

    if( item )
    {
        PCB_LAYER_ID principalLayer = item->GetLayer();
        std::shared_ptr<RC_ITEM>     rc_item = node->m_RcItem;
        BOARD_ITEM*  a = board->GetItem( rc_item->GetMainItemID() );
        BOARD_ITEM*  b = board->GetItem( rc_item->GetAuxItemID() );
        BOARD_ITEM*  c = board->GetItem( rc_item->GetAuxItem2ID() );
        BOARD_ITEM*  d = board->GetItem( rc_item->GetAuxItem3ID() );

        LSET violationLayers;

        if( a )
            violationLayers &= a->GetLayerSet();

        if( b )
            violationLayers &= b->GetLayerSet();

        if( c )
            violationLayers &= c->GetLayerSet();

        if( d )
            violationLayers &= d->GetLayerSet();

        if( violationLayers.count() )
            principalLayer = violationLayers.Seq().front();
        else
            violationLayers.set( principalLayer );

        WINDOW_THAWER thawer( m_brdEditor );

        m_brdEditor->FocusOnItem( item );
        m_brdEditor->GetCanvas()->Refresh();

        if( ( violationLayers & board->GetVisibleLayers() ) == 0 )
        {
            m_brdEditor->GetAppearancePanel()->SetLayerVisible( item->GetLayer(), true );
            m_brdEditor->GetCanvas()->Refresh();
        }

        if( board->GetVisibleLayers().test( principalLayer ) )
            m_brdEditor->SetActiveLayer( principalLayer );
    }

    aEvent.Skip();
}


void DIALOG_DRC::OnDRCItemDClick( wxDataViewEvent& aEvent )
{
    if( aEvent.GetItem().IsOk() )
    {
        // turn control over to m_brdEditor, hide this DIALOG_DRC window,
        // no destruction so we can preserve listbox cursor
        if( !IsModal() )
            Show( false );
    }

    aEvent.Skip();
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

    menu.AppendSeparator();

    if( bds().m_DRCSeverities[ rcItem->GetErrorCode() ] == RPT_SEVERITY_WARNING )
    {
        msg.Printf( _( "Change severity to Error for all '%s' violations" ),
                    rcItem->GetErrorText(),
                    _( "Violation severities can also be edited in the Board Setup... dialog" ) );
        menu.Append( 3, msg );
    }
    else
    {
        msg.Printf( _( "Change severity to Warning for all '%s' violations" ),
                    rcItem->GetErrorText(),
                    _( "Violation severities can also be edited in the Board Setup... dialog" ) );
        menu.Append( 4, msg );
    }

    msg.Printf( _( "Ignore all '%s' violations" ),
                rcItem->GetErrorText(),
                _( "Violations will not be checked or reported" ) );
    menu.Append( 5, msg );

    menu.AppendSeparator();

    menu.Append( 6, _( "Edit violation severities..." ), _( "Open the Board Setup... dialog" ) );

    bool modified = false;

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case 1:
    {
        MARKER_PCB* marker = dynamic_cast<MARKER_PCB*>( node->m_RcItem->GetParent() );

        if( marker )
        {
            marker->SetExcluded( false );
            m_brdEditor->GetCanvas()->GetView()->Update( marker );

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }
    }
        break;

    case 2:
    {
        MARKER_PCB* marker = dynamic_cast<MARKER_PCB*>( node->m_RcItem->GetParent() );

        if( marker )
        {
            marker->SetExcluded( true );
            m_brdEditor->GetCanvas()->GetView()->Update( marker );

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
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_ERROR;

        for( MARKER_PCB* marker : m_brdEditor->GetBoard()->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_brdEditor->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
        break;

    case 4:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_WARNING;

        for( MARKER_PCB* marker : m_brdEditor->GetBoard()->Markers() )
        {
            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_brdEditor->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
        break;

    case 5:
    {
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_IGNORE;

        std::vector<MARKER_PCB*>& markers = m_brdEditor->GetBoard()->Markers();

        for( unsigned i = 0; i < markers.size(); )
        {
            if( markers[i]->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
            {
                m_brdEditor->GetCanvas()->GetView()->Remove( markers.at( i ) );
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

    case 6:
        m_brdEditor->ShowBoardSetupDialog( _( "Violation Severity" ) );
        break;
    }

    if( modified )
    {
        updateDisplayedCounts();
        refreshBoardEditor();
        m_brdEditor->OnModify();
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

    m_markerTreeModel->SetSeverities( m_severities );
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
        m_Messages->AppendText( wxString::Format( _( "Report file '%s' created\n" ),
                                                  fn.GetFullPath() ) );
    }
    else
    {
        DisplayError( this, wxString::Format( _( "Unable to create report file '%s'" ),
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

    m_brdEditor->FocusOnItem( nullptr );

    SetReturnCode( wxID_CANCEL );

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    DRC_TOOL* drcTool = m_brdEditor->GetToolManager()->GetTool<DRC_TOOL>();
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


void DIALOG_DRC::refreshBoardEditor()
{
    WINDOW_THAWER thawer( m_brdEditor );

    m_brdEditor->GetCanvas()->Refresh();
}


void DIALOG_DRC::deleteAllMarkers( bool aIncludeExclusions )
{
    // Clear current selection list to avoid selection of deleted items
    m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    m_markerTreeModel->DeleteItems( false, aIncludeExclusions, true );
}


bool DIALOG_DRC::writeReport( const wxString& aFullFileName )
{
    FILE* fp = wxFopen( aFullFileName, wxT( "w" ) );

    if( fp == NULL )
        return false;

    std::map<KIID, EDA_ITEM*> itemMap;
    m_brdEditor->GetBoard()->FillItemMap( itemMap );

    int       count;
    EDA_UNITS units = GetUserUnits();

    fprintf( fp, "** Drc report for %s **\n", TO_UTF8( m_brdEditor->GetBoard()->GetFileName() ) );

    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "** Created on %s **\n", TO_UTF8( now.Format( wxT( "%F %T" ) ) ) );

    count = m_markersProvider->GetCount();

    fprintf( fp, "\n** Found %d DRC violations **\n", count );

    for( int i = 0; i < count; ++i )
        fprintf( fp, "%s", TO_UTF8( m_markersProvider->GetItem( i )->ShowReport( units, itemMap ) ) );

    count = m_unconnectedItemsProvider->GetCount();

    fprintf( fp, "\n** Found %d unconnected pads **\n", count );

    for( int i = 0; i < count; ++i )
        fprintf( fp, "%s", TO_UTF8( m_unconnectedItemsProvider->GetItem( i )->ShowReport( units, itemMap ) ) );

    count = m_footprintWarningsProvider->GetCount();

    fprintf( fp, "\n** Found %d Footprint errors **\n", count );

    for( int i = 0; i < count; ++i )
        fprintf( fp, "%s", TO_UTF8( m_footprintWarningsProvider->GetItem( i )->ShowReport( units, itemMap ) ) );


    fprintf( fp, "\n** End of Report **\n" );

    fclose( fp );

    return true;
}


void DIALOG_DRC::OnDeleteOneClick( wxCommandEvent& aEvent )
{
    if( m_Notebook->GetSelection() == 0 )
    {
        // Clear the selection.  It may be the selected DRC marker.
        m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

        m_markerTreeModel->DeleteCurrentItem( true );

        // redraw the pcb
        refreshBoardEditor();
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

    refreshBoardEditor();
    updateDisplayedCounts();
}


void DIALOG_DRC::updateDisplayedCounts()
{
    wxString msg;

    // First the tab headers:
    //

    if( m_drcRun )
    {
        msg.sprintf( m_markersTitleTemplate, m_markerTreeModel->GetDRCItemCount() );
        m_Notebook->SetPageText( 0, msg );

        msg.sprintf( m_unconnectedTitleTemplate, m_unconnectedTreeModel->GetDRCItemCount() );
        m_Notebook->SetPageText( 1, msg );

        if( m_footprintTestsRun )
            msg.sprintf( m_footprintsTitleTemplate, m_footprintWarningsTreeModel->GetDRCItemCount() );
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

    // And now the badges:
    //

    int numErrors = 0;
    int numWarnings = 0;
    int numExcluded = 0;

    if( m_markersProvider )
    {
        numErrors += m_markersProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_markersProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_markersProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    if( m_unconnectedItemsProvider )
    {
        numErrors += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_unconnectedItemsProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    if( m_footprintWarningsProvider )
    {
        numErrors += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_footprintWarningsProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    m_errorsBadge->SetBitmap( MakeBadge( RPT_SEVERITY_ERROR, numErrors, m_errorsBadge ) );
    m_warningsBadge->SetBitmap( MakeBadge( RPT_SEVERITY_WARNING, numWarnings, m_warningsBadge ) );
    m_exclusionsBadge->SetBitmap( MakeBadge( RPT_SEVERITY_EXCLUSION, numExcluded, m_exclusionsBadge ) );
}
