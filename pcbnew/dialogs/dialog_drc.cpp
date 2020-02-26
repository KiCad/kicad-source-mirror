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
#include <drc/drc_tree_model.h>
#include <wx/wupdlock.h>

DIALOG_DRC_CONTROL::DIALOG_DRC_CONTROL( DRC* aTester, PCB_EDIT_FRAME* aEditorFrame,
                                        wxWindow* aParent ) :
        DIALOG_DRC_CONTROL_BASE( aParent ),
        m_trackMinWidth( aEditorFrame, m_MinWidthLabel, m_MinWidthCtrl, m_MinWidthUnits, true ),
        m_viaMinSize( aEditorFrame, m_ViaMinLabel, m_ViaMinCtrl, m_ViaMinUnits, true ),
        m_uviaMinSize( aEditorFrame, m_uViaMinLabel, m_uViaMinCtrl, m_uViaMinUnits, true )
{
    SetName( DIALOG_DRC_WINDOW_NAME ); // Set a window name to be able to find it

    m_tester       = aTester;
    m_brdEditor    = aEditorFrame;
    m_currentBoard = m_brdEditor->GetBoard();
    m_BrdSettings  = m_brdEditor->GetBoard()->GetDesignSettings();

    m_BrowseButton->SetBitmap( KiBitmap( folder_xpm ) );

    m_markerTreeModel = new DRC_TREE_MODEL( m_markerDataView );
    m_markerDataView->AssociateModel( m_markerTreeModel );

    m_unconnectedTreeModel = new DRC_TREE_MODEL( m_unconnectedDataView );
    m_unconnectedDataView->AssociateModel( m_unconnectedTreeModel );

    m_footprintsTreeModel = new DRC_TREE_MODEL( m_footprintsDataView );
    m_footprintsDataView->AssociateModel( m_footprintsTreeModel );

    m_Notebook->SetSelection( 0 );

    // We use a sdbSizer here to get the order right, which is platform-dependent
    m_sdbSizer1OK->SetLabel( _( "Run DRC" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sizerButtons->Layout();

    m_sdbSizer1OK->SetDefault();

    InitValues();

    FinishDialogSettings();
}


DIALOG_DRC_CONTROL::~DIALOG_DRC_CONTROL()
{
    m_brdEditor->FocusOnItem( nullptr );

    PCBNEW_SETTINGS* settings = m_brdEditor->GetSettings();
    settings->m_DrcDialog.refill_zones       = m_cbRefillZones->GetValue();
    settings->m_DrcDialog.test_track_to_zone = m_cbReportAllTrackErrors->GetValue();
    settings->m_DrcDialog.test_footprints    = m_cbTestFootprints->GetValue();

    m_markerTreeModel->DecRef();
}


void DIALOG_DRC_CONTROL::OnActivateDlg( wxActivateEvent& event )
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
    m_BrdSettings = m_brdEditor->GetBoard()->GetDesignSettings();
    DisplayDRCValues();
}


void DIALOG_DRC_CONTROL::DisplayDRCValues()
{
    m_trackMinWidth.SetValue( m_BrdSettings.m_TrackMinWidth );
    m_viaMinSize.SetValue( m_BrdSettings.m_ViasMinSize );
    m_uviaMinSize.SetValue( m_BrdSettings.m_MicroViasMinSize );
}


void DIALOG_DRC_CONTROL::InitValues()
{
    m_markersTitleTemplate     = m_Notebook->GetPageText( 0 );
    m_unconnectedTitleTemplate = m_Notebook->GetPageText( 1 );
    m_footprintsTitleTemplate  = m_Notebook->GetPageText( 2 );

    DisplayDRCValues();

    auto cfg = m_brdEditor->GetSettings();

    m_cbRefillZones->SetValue( cfg->m_DrcDialog.refill_zones );
    m_cbReportAllTrackErrors->SetValue( cfg->m_DrcDialog.test_track_to_zone );
    m_cbTestFootprints->SetValue( cfg->m_DrcDialog.test_footprints );

    Layout(); // adding the units above expanded Clearance text, now resize.

    SetFocus();
}


void DIALOG_DRC_CONTROL::SetDRCParameters()
{
    m_BrdSettings.m_TrackMinWidth    = (int) m_trackMinWidth.GetValue();
    m_BrdSettings.m_ViasMinSize      = (int) m_viaMinSize.GetValue();
    m_BrdSettings.m_MicroViasMinSize = (int) m_uviaMinSize.GetValue();

    m_brdEditor->GetBoard()->SetDesignSettings( m_BrdSettings );
}


void DIALOG_DRC_CONTROL::SetRptSettings( bool aEnable, const wxString& aFileName )
{
    m_RptFilenameCtrl->SetValue( aFileName );
    m_CreateRptCtrl->SetValue( aEnable );
}


void DIALOG_DRC_CONTROL::GetRptSettings( bool* aEnable, wxString& aFileName )
{
    *aEnable  = m_CreateRptCtrl->GetValue();
    aFileName = m_RptFilenameCtrl->GetValue();
}


void DIALOG_DRC_CONTROL::OnRunDRCClick( wxCommandEvent& event )
{
    wxString reportName, msg;

    bool make_report = m_CreateRptCtrl->IsChecked();

    if( make_report ) // Create a rpt file
    {
        reportName = m_RptFilenameCtrl->GetValue();

        if( reportName.IsEmpty() )
        {
            wxCommandEvent dummy;
            OnButtonBrowseRptFileClick( dummy );
        }

        if( !reportName.IsEmpty() )
            reportName = makeValidFileNameReport();
    }

    SetDRCParameters();
    m_tester->m_doZonesTest          = m_cbReportTracksToZonesErrors->GetValue();
    m_tester->m_rptFilename          = reportName;
    m_tester->m_doCreateRptFile      = make_report;
    m_tester->m_refillZones          = m_cbRefillZones->GetValue();
    m_tester->m_reportAllTrackErrors = m_cbReportAllTrackErrors->GetValue();
    m_tester->m_testFootprints       = m_cbTestFootprints->GetValue();

    DelDRCMarkers();

    wxBeginBusyCursor();
    wxWindowDisabler disabler;

    // run all the tests, with no UI at this time.
    m_Messages->Clear();
    wxSafeYield(); // Allows time slice to refresh the Messages
    m_tester->RunTests( m_Messages );
    m_Notebook->ChangeSelection( 0 ); // display the "Problems/Markers" tab

    // Generate the report
    if( !reportName.IsEmpty() )
    {
        if( writeReport( reportName ) )
        {
            msg.Printf( _( "Report file \"%s\" created" ), GetChars( reportName ) );
            wxMessageDialog popupWindow( this, msg, _( "Disk File Report Completed" ) );
            popupWindow.ShowModal();
        }
        else
        {
            msg.Printf( _( "Unable to create report file \"%s\"" ), GetChars( reportName ) );
            DisplayError( this, msg );
        }
    }

    wxEndBusyCursor();

    RefreshBoardEditor();
    SetFocus();
}


void DIALOG_DRC_CONTROL::SetMarkersProvider( DRC_ITEMS_PROVIDER* aProvider )
{
    m_markerTreeModel->SetProvider( aProvider );
}


void DIALOG_DRC_CONTROL::SetUnconnectedProvider(class DRC_ITEMS_PROVIDER * aProvider)
{
    m_unconnectedTreeModel->SetProvider( aProvider );
}


void DIALOG_DRC_CONTROL::SetFootprintsProvider( DRC_ITEMS_PROVIDER* aProvider )
{
    m_footprintsTreeModel->SetProvider( aProvider );
}


void DIALOG_DRC_CONTROL::OnDRCItemSelected( wxDataViewEvent& event )
{
    BOARD_ITEM*   item = DRC_TREE_MODEL::ToBoardItem( m_brdEditor->GetBoard(), event.GetItem() );
    WINDOW_THAWER thawer( m_brdEditor );

    m_brdEditor->FocusOnItem( item );
    m_brdEditor->GetCanvas()->Refresh();

    event.Skip();
}


void DIALOG_DRC_CONTROL::OnDRCItemDClick( wxDataViewEvent& event )
{
    if( event.GetItem().IsOk() )
    {
        // turn control over to m_brdEditor, hide this DIALOG_DRC_CONTROL window,
        // no destruction so we can preserve listbox cursor
        if( !IsModal() )
            Show( false );
    }

    event.Skip();
}


void DIALOG_DRC_CONTROL::OnButtonBrowseRptFileClick( wxCommandEvent& )
{
    wxFileName fn = m_brdEditor->GetBoard()->GetFileName();
    fn.SetExt( ReportFileExtension );
    wxString prj_path = Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Save DRC Report File" ), prj_path, fn.GetFullName(),
                      ReportFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_CreateRptCtrl->SetValue( true );
    m_RptFilenameCtrl->SetValue( dlg.GetPath() );
}


void DIALOG_DRC_CONTROL::OnCancelClick( wxCommandEvent& event )
{
    m_brdEditor->FocusOnItem( nullptr );

    SetReturnCode( wxID_CANCEL );
    SetDRCParameters();

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    m_tester->DestroyDRCDialog( wxID_CANCEL );
}


void DIALOG_DRC_CONTROL::OnReportCheckBoxClicked( wxCommandEvent& event )
{
    if( m_CreateRptCtrl->IsChecked() )
        m_RptFilenameCtrl->SetFocus();
}


void DIALOG_DRC_CONTROL::OnReportFilenameEdited( wxCommandEvent& event )
{
    m_CreateRptCtrl->SetValue( event.GetString().Length() );
}


void DIALOG_DRC_CONTROL::OnChangingNotebookPage( wxNotebookEvent& event )
{
    // Shouldn't be necessary, but is on at least OSX
    if( event.GetSelection() >= 0 )
        m_Notebook->ChangeSelection( (unsigned) event.GetSelection() );

    m_markerDataView->UnselectAll();
    m_unconnectedDataView->UnselectAll();
    m_footprintsDataView->UnselectAll();
}


void DIALOG_DRC_CONTROL::RefreshBoardEditor()
{
    WINDOW_THAWER thawer( m_brdEditor );

    m_brdEditor->GetCanvas()->Refresh();
}


void DIALOG_DRC_CONTROL::DelDRCMarkers()
{
    // Clear current selection list to avoid selection of deleted items
    m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    m_markerTreeModel->DeleteAllItems();
    m_unconnectedTreeModel->DeleteAllItems();
}


const wxString DIALOG_DRC_CONTROL::makeValidFileNameReport()
{
    wxFileName fn = m_RptFilenameCtrl->GetValue();

    if( !fn.HasExt() )
    {
        fn.SetExt( ReportFileExtension );
        m_RptFilenameCtrl->SetValue( fn.GetFullPath() );
    }

    // Ensure it is an absolute filename. if it is given relative
    // it will be made relative to the project
    if( !fn.IsAbsolute() )
    {
        wxString prj_path = Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    return fn.GetFullPath();
}


bool DIALOG_DRC_CONTROL::writeReport( const wxString& aFullFileName )
{
    FILE* fp = wxFopen( aFullFileName, wxT( "w" ) );

    if( fp == NULL )
        return false;

    int       count;
    EDA_UNITS units = GetUserUnits();

    fprintf( fp, "** Drc report for %s **\n", TO_UTF8( m_brdEditor->GetBoard()->GetFileName() ) );

    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "** Created on %s **\n", TO_UTF8( now.Format( wxT( "%F %T" ) ) ) );

    count = m_markerTreeModel->GetDRCItemCount();

    fprintf( fp, "\n** Found %d DRC errors **\n", count );

    for( int i = 0; i < count; ++i )
        fprintf( fp, "%s", TO_UTF8( m_markerTreeModel->GetDRCItem( i )->ShowReport( units ) ) );

    count = m_unconnectedTreeModel->GetDRCItemCount();

    fprintf( fp, "\n** Found %d unconnected pads **\n", count );

    for( int i = 0; i < count; ++i )
        fprintf( fp, "%s", TO_UTF8( m_unconnectedTreeModel->GetDRCItem( i )->ShowReport( units ) ) );

    count = m_footprintsTreeModel->GetDRCItemCount();

    fprintf( fp, "\n** Found %d Footprint errors **\n", count );

    for( int i = 0; i < count; ++i )
        fprintf( fp, "%s", TO_UTF8( m_footprintsTreeModel->GetDRCItem( i )->ShowReport( units ) ) );


    fprintf( fp, "\n** End of Report **\n" );

    fclose( fp );

    return true;
}


void DIALOG_DRC_CONTROL::OnDeleteOneClick( wxCommandEvent& event )
{
    if( m_Notebook->GetSelection() == 0 )
    {
        // Clear the selection.  It may be the selected DRC marker.
        m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

        m_markerTreeModel->DeleteCurrentItem();

        // redraw the pcb
        RefreshBoardEditor();
    }
    else if( m_Notebook->GetSelection() == 1 )
    {
        m_unconnectedTreeModel->DeleteCurrentItem();
    }

    UpdateDisplayedCounts();
}


void DIALOG_DRC_CONTROL::OnDeleteAllClick( wxCommandEvent& event )
{
    DelDRCMarkers();
    RefreshBoardEditor();
    UpdateDisplayedCounts();
}


void DIALOG_DRC_CONTROL::UpdateDisplayedCounts()
{
    wxString msg;

    if( m_tester->m_drcRun )
    {
        msg.sprintf( m_markersTitleTemplate, m_markerTreeModel->GetDRCItemCount() );
        m_Notebook->SetPageText( 0, msg );

        msg.sprintf( m_unconnectedTitleTemplate, (int) m_unconnectedTreeModel->GetDRCItemCount() );
        m_Notebook->SetPageText( 1, msg );

        if( m_tester->m_footprintsTested )
            msg.sprintf( m_footprintsTitleTemplate, (int) m_footprintsTreeModel->GetDRCItemCount() );
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
}
