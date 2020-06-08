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

#include <base_units.h>
#include <bitmaps.h>
#include <collectors.h>
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
#include <rc_item.h>
#include <wx/wupdlock.h>
#include <widgets/ui_common.h>
#include <pcb_layer_widget.h>


DIALOG_DRC::DIALOG_DRC( DRC* aTester, PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent ) :
        DIALOG_DRC_BASE( aParent ),
        m_trackMinWidth( aEditorFrame, m_MinWidthLabel, m_MinWidthCtrl, m_MinWidthUnits, true ),
        m_viaMinSize( aEditorFrame, m_ViaMinLabel, m_ViaMinCtrl, m_ViaMinUnits, true ),
        m_uviaMinSize( aEditorFrame, m_uViaMinLabel, m_uViaMinCtrl, m_uViaMinUnits, true ),
        m_markersProvider( nullptr ),
        m_markerTreeModel( nullptr ),
        m_unconnectedItemsProvider( nullptr ),
        m_unconnectedTreeModel( nullptr ),
        m_footprintWarningsProvider( nullptr ),
        m_footprintWarningsTreeModel( nullptr ),
        m_severities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING )
{
    SetName( DIALOG_DRC_WINDOW_NAME ); // Set a window name to be able to find it

    m_tester       = aTester;
    m_brdEditor    = aEditorFrame;
    m_currentBoard = m_brdEditor->GetBoard();

    m_markerTreeModel = new RC_TREE_MODEL( m_brdEditor, m_markerDataView );
    m_markerDataView->AssociateModel( m_markerTreeModel );

    m_unconnectedTreeModel = new RC_TREE_MODEL( m_brdEditor, m_unconnectedDataView );
    m_unconnectedDataView->AssociateModel( m_unconnectedTreeModel );

    m_footprintWarningsTreeModel = new RC_TREE_MODEL( m_brdEditor, m_footprintsDataView );
    m_footprintsDataView->AssociateModel( m_footprintWarningsTreeModel );

    m_Notebook->SetSelection( 0 );

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
    settings->m_DrcDialog.refill_zones       = m_cbRefillZones->GetValue();
    settings->m_DrcDialog.test_track_to_zone = m_cbReportAllTrackErrors->GetValue();
    settings->m_DrcDialog.test_footprints    = m_cbTestFootprints->GetValue();
    settings->m_DrcDialog.severities         = m_severities;

    m_markerTreeModel->DecRef();
}


void DIALOG_DRC::OnActivateDlg( wxActivateEvent& aEvent )
{
    if( m_currentBoard != m_brdEditor->GetBoard() )
    {
        // If m_currentBoard is not the current parent board,
        // (for instance because a new board was loaded),
        // close the dialog, because many pointers are now invalid
        // in lists
        SetReturnCode( wxID_CANCEL );
        Close();
        m_tester->DestroyDRCDialog( wxID_CANCEL );
        return;
    }

    // updating data which can be modified outside the dialog (DRC parameters, units ...)
    // because the dialog is not modal
    displayDRCValues();

    m_markerTreeModel->SetProvider( m_markersProvider );
    m_unconnectedTreeModel->SetProvider( m_unconnectedItemsProvider );
    m_footprintWarningsTreeModel->SetProvider( m_footprintWarningsProvider );
    updateDisplayedCounts();
}


void DIALOG_DRC::displayDRCValues()
{
    m_trackMinWidth.SetValue( bds().m_TrackMinWidth );
    m_viaMinSize.SetValue( bds().m_ViasMinSize );
    m_uviaMinSize.SetValue( bds().m_MicroViasMinSize );
}


void DIALOG_DRC::initValues()
{
    m_markersTitleTemplate     = m_Notebook->GetPageText( 0 );
    m_unconnectedTitleTemplate = m_Notebook->GetPageText( 1 );
    m_footprintsTitleTemplate  = m_Notebook->GetPageText( 2 );

    displayDRCValues();

    auto cfg = m_brdEditor->GetPcbNewSettings();

    m_cbRefillZones->SetValue( cfg->m_DrcDialog.refill_zones );
    m_cbReportAllTrackErrors->SetValue( cfg->m_DrcDialog.test_track_to_zone );
    m_cbTestFootprints->SetValue( cfg->m_DrcDialog.test_footprints );

    m_severities = cfg->m_DrcDialog.severities;
    m_markerTreeModel->SetSeverities( m_severities );
    m_unconnectedTreeModel->SetSeverities( m_severities );
    m_footprintWarningsTreeModel->SetSeverities( m_severities );

    Layout(); // adding the units above expanded Clearance text, now resize.

    SetFocus();
}


void DIALOG_DRC::setDRCParameters()
{
    bds().m_TrackMinWidth    = (int) m_trackMinWidth.GetValue();
    bds().m_ViasMinSize      = (int) m_viaMinSize.GetValue();
    bds().m_MicroViasMinSize = (int) m_uviaMinSize.GetValue();
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
    setDRCParameters();
    m_tester->m_testTracksAgainstZones = m_cbReportTracksToZonesErrors->GetValue();
    m_tester->m_refillZones            = m_cbRefillZones->GetValue();
    m_tester->m_reportAllTrackErrors   = m_cbReportAllTrackErrors->GetValue();
    m_tester->m_testFootprints         = m_cbTestFootprints->GetValue();

    m_brdEditor->RecordDRCExclusions();
    deleteAllMarkers();

    wxBeginBusyCursor();
    wxWindowDisabler disabler;

    // run all the tests, with no UI at this time.
    m_Messages->Clear();
    wxSafeYield(); // Allows time slice to refresh the Messages
    m_tester->RunTests( m_Messages );

    m_Notebook->ChangeSelection( 0 ); // display the "Problems/Markers" tab

    wxEndBusyCursor();

    refreshBoardEditor();

    wxSafeYield();
    Raise();
    m_Notebook->GetPage( m_Notebook->GetSelection() )->SetFocus();
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
    const KIID&   itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM*   item = m_brdEditor->GetBoard()->GetItem( itemID );
    LSET          visibleLayers = m_brdEditor->GetBoard()->GetVisibleLayers();
    WINDOW_THAWER thawer( m_brdEditor );

    if( item && ( item->GetLayerSet() & visibleLayers ) == 0 )
    {
        if( IsOK( this, wxString::Format( _( "Item not currently visible.\nShow the '%s' layer?" ),
                                          item->GetLayerName() ) ) )
        {
            m_brdEditor->GetLayerManager()->SetLayerVisible( item->GetLayer(), true );
        }
    }

    m_brdEditor->FocusOnItem( item );
    m_brdEditor->GetCanvas()->Refresh();

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

    RC_ITEM*  rcItem = node->m_RcItem;
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
        node->m_RcItem->GetParent()->SetExcluded( false );

        // Update view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
        modified = true;
        break;

    case 2:
        node->m_RcItem->GetParent()->SetExcluded( true );

        // Update view
        if( m_severities & RPT_SEVERITY_EXCLUSION )
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
        else
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->DeleteCurrentItem( false );

        modified = true;
        break;

    case 3:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_ERROR;

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
        break;

    case 4:
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_WARNING;

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markersProvider );
        modified = true;
        break;

    case 5:
    {
        bds().m_DRCSeverities[ rcItem->GetErrorCode() ] = RPT_SEVERITY_IGNORE;

        std::vector<MARKER_PCB*>& markers = m_brdEditor->GetBoard()->Markers();
        KIGFX::VIEW*              view = m_parentFrame->GetToolManager()->GetView();

        for( unsigned i = 0; i < markers.size(); )
        {
            if( markers[i]->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
            {
                view->Remove( markers.at( i ) );
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
        m_brdEditor->OnModify();
        m_brdEditor->SyncToolbars();
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


void DIALOG_DRC::OnCancelClick( wxCommandEvent& aEvent )
{
    m_brdEditor->FocusOnItem( nullptr );

    SetReturnCode( wxID_CANCEL );
    setDRCParameters();

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    m_tester->DestroyDRCDialog( wxID_CANCEL );
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


void DIALOG_DRC::deleteAllMarkers()
{
    // Clear current selection list to avoid selection of deleted items
    m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    m_markerTreeModel->DeleteAllItems();
    m_unconnectedTreeModel->DeleteAllItems();
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
    deleteAllMarkers();

    refreshBoardEditor();
    updateDisplayedCounts();
}


void DIALOG_DRC::updateDisplayedCounts()
{
    wxString msg;

    // First the tab headers:
    //

    if( m_tester->m_drcRun )
    {
        msg.sprintf( m_markersTitleTemplate, m_markerTreeModel->GetDRCItemCount() );
        m_Notebook->SetPageText( 0, msg );

        msg.sprintf( m_unconnectedTitleTemplate, m_unconnectedTreeModel->GetDRCItemCount() );
        m_Notebook->SetPageText( 1, msg );

        if( m_tester->m_footprintsTested )
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
