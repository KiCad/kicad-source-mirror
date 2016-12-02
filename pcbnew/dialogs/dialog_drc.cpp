/**
 * @file dialog_drc.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2016 KiCad Developers, see change_log.txt for contributors.
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
#include <confirm.h>
#include <wildcards_and_files_ext.h>
#include <pgm_base.h>
#include <dialog_drc.h>
#include <wxPcbStruct.h>
#include <base_units.h>
#include <class_board_design_settings.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>

/* class DIALOG_DRC_CONTROL: a dialog to set DRC parameters (clearance, min cooper size)
 * and run DRC tests
 */

DIALOG_DRC_CONTROL::DIALOG_DRC_CONTROL( DRC* aTester, PCB_EDIT_FRAME* aEditorFrame,
                                        wxWindow* aParent ) :
    DIALOG_DRC_CONTROL_BASE( aParent )
{
    m_tester = aTester;
    m_brdEditor = aEditorFrame;
    m_currentBoard = m_brdEditor->GetBoard();
    m_BrdSettings = m_brdEditor->GetBoard()->GetDesignSettings();

    InitValues();

    FixOSXCancelButtonIssue();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
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
    m_TrackMinWidthUnit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_ViaMinUnit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_MicroViaMinUnit->SetLabel(GetAbbreviatedUnitsLabel( g_UserUnit ) );

    PutValueInLocalUnits( *m_SetTrackMinWidthCtrl, m_BrdSettings.m_TrackMinWidth );
    PutValueInLocalUnits( *m_SetViaMinSizeCtrl, m_BrdSettings.m_ViasMinSize );
    PutValueInLocalUnits( *m_SetMicroViakMinSizeCtrl, m_BrdSettings.m_MicroViasMinSize );
}


void DIALOG_DRC_CONTROL::InitValues()
{
    // Connect events and objects
    m_ClearanceListBox->Connect( ID_CLEARANCE_LIST, wxEVT_LEFT_DCLICK,
                                 wxMouseEventHandler(
                                     DIALOG_DRC_CONTROL::OnLeftDClickClearance ), NULL, this );
    m_ClearanceListBox->Connect( ID_CLEARANCE_LIST, wxEVT_RIGHT_UP,
                                 wxMouseEventHandler(
                                     DIALOG_DRC_CONTROL::OnRightUpClearance ), NULL, this );
    m_UnconnectedListBox->Connect( ID_UNCONNECTED_LIST, wxEVT_LEFT_DCLICK,
                                   wxMouseEventHandler( DIALOG_DRC_CONTROL::
                                                        OnLeftDClickUnconnected ), NULL, this );
    m_UnconnectedListBox->Connect( ID_UNCONNECTED_LIST, wxEVT_RIGHT_UP,
                                   wxMouseEventHandler(
                                       DIALOG_DRC_CONTROL::OnRightUpUnconnected ), NULL, this );

    m_DeleteCurrentMarkerButton->Enable( false );

    DisplayDRCValues();

    // Set the initial "enabled" status of the browse button and the text
    // field for report name
    wxCommandEvent junk;
    OnReportCheckBoxClicked( junk );

    Layout();      // adding the units above expanded Clearance text, now resize.

    SetFocus();
}

/* accept DRC parameters (min clearance value and min sizes
*/
void DIALOG_DRC_CONTROL::SetDrcParmeters( )
{
    m_BrdSettings.m_TrackMinWidth = ValueFromTextCtrl( *m_SetTrackMinWidthCtrl );
    m_BrdSettings.m_ViasMinSize = ValueFromTextCtrl( *m_SetViaMinSizeCtrl );
    m_BrdSettings.m_MicroViasMinSize = ValueFromTextCtrl( *m_SetMicroViakMinSizeCtrl );

    m_brdEditor->GetBoard()->SetDesignSettings( m_BrdSettings );
}


void DIALOG_DRC_CONTROL::SetRptSettings( bool aEnable, const wxString& aFileName )
{
    m_RptFilenameCtrl->Enable( aEnable );
    m_BrowseButton->Enable( aEnable );
    m_CreateRptCtrl->SetValue( aEnable );
    m_RptFilenameCtrl->SetValue( aFileName );
}

void DIALOG_DRC_CONTROL::GetRptSettings( bool* aEnable, wxString& aFileName )
{
    *aEnable = m_CreateRptCtrl->GetValue();
    aFileName = m_RptFilenameCtrl->GetValue();
}

void DIALOG_DRC_CONTROL::OnStartdrcClick( wxCommandEvent& event )
{
    wxString reportName;

    bool make_report = m_CreateRptCtrl->IsChecked();

    if( make_report )      // Create a rpt file
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

    SetDrcParmeters();
    m_tester->SetSettings( true,        // Pad to pad DRC test enabled
                           true,        // unconnected pads DRC test enabled
                           true,        // DRC test for zones enabled
                           true,        // DRC test for keepout areas enabled
                           reportName, make_report );

    DelDRCMarkers();

    wxBeginBusyCursor();

    // run all the tests, with no UI at this time.
    m_Messages->Clear();
    wxSafeYield();                          // Allows time slice to refresh the m_Messages window
    m_brdEditor->GetBoard()->m_Status_Pcb = 0; // Force full connectivity and ratsnest recalculations
    m_tester->RunTests(m_Messages);
    m_Notebook->ChangeSelection( 0 );       // display the 1at tab "...Markers ..."


    // Generate the report
    if( !reportName.IsEmpty() )
    {
        if( writeReport( reportName ) )
        {
            wxString        msg;
            msg.Printf( _( "Report file \"%s\" created" ), GetChars( reportName ) );

            wxString        caption( _( "Disk File Report Completed" ) );
            wxMessageDialog popupWindow( this, msg, caption );
            popupWindow.ShowModal();
        }
        else
            DisplayError( this, wxString::Format( _( "Unable to create report file '%s' "),
                          GetChars( reportName ) ) );
    }

    wxEndBusyCursor();

    RedrawDrawPanel();
}


void DIALOG_DRC_CONTROL::OnDeleteAllClick( wxCommandEvent& event )
{
    DelDRCMarkers();
    RedrawDrawPanel();
    UpdateDisplayedCounts();
}


void DIALOG_DRC_CONTROL::OnListUnconnectedClick( wxCommandEvent& event )
{
    wxString reportName;

    bool make_report = m_CreateRptCtrl->IsChecked();

    if( make_report )      // Create a file rpt
    {
        reportName = m_RptFilenameCtrl->GetValue();

        if( reportName.IsEmpty() )
        {
            wxCommandEvent junk;
            OnButtonBrowseRptFileClick( junk );
        }

        if( !reportName.IsEmpty() )
            reportName = makeValidFileNameReport();
    }

    SetDrcParmeters();

    m_tester->SetSettings( true,        // Pad to pad DRC test enabled
                           true,        // unconnected pads DRC test enabled
                           true,        // DRC test for zones enabled
                           true,        // DRC test for keepout areas enabled
                           reportName, make_report );

    DelDRCMarkers();

    wxBeginBusyCursor();

    m_Messages->Clear();
    m_tester->ListUnconnectedPads();

    m_Notebook->ChangeSelection( 1 );       // display the 2nd tab "Unconnected..."

    // Generate the report
    if( !reportName.IsEmpty() )
    {
        if( writeReport( reportName ) )
        {
            wxString        msg;
            msg.Printf( _( "Report file \"%s\" created" ), GetChars( reportName ) );
            wxString        caption( _( "Disk File Report Completed" ) );
            wxMessageDialog popupWindow( this, msg, caption );
            popupWindow.ShowModal();
        }
        else
            DisplayError( this, wxString::Format( _( "Unable to create report file '%s' "),
                          GetChars( reportName ) ) );
    }

    UpdateDisplayedCounts();

    wxEndBusyCursor();

    /* there is currently nothing visible on the DrawPanel for unconnected pads
     *  RedrawDrawPanel();
     */
}


void DIALOG_DRC_CONTROL::OnButtonBrowseRptFileClick( wxCommandEvent& event )
{
    wxFileName fn = m_brdEditor->GetBoard()->GetFileName();
    fn.SetExt( ReportFileExtension );
    wxString prj_path =  Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Save DRC Report File" ), prj_path,
                      fn.GetFullName(), ReportFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_RptFilenameCtrl->SetValue( dlg.GetPath() );
}


void DIALOG_DRC_CONTROL::OnOkClick( wxCommandEvent& event )
{
    SetReturnCode( wxID_OK );
    SetDrcParmeters();

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    m_tester->DestroyDRCDialog( wxID_OK );
}


void DIALOG_DRC_CONTROL::OnCancelClick( wxCommandEvent& event )
{
    SetReturnCode( wxID_CANCEL );

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    m_tester->DestroyDRCDialog( wxID_CANCEL );
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
 */

void DIALOG_DRC_CONTROL::OnReportCheckBoxClicked( wxCommandEvent& event )
{
    m_RptFilenameCtrl->Enable( m_CreateRptCtrl->IsChecked() );
    m_BrowseButton->Enable( m_CreateRptCtrl->IsChecked() );
}


void DIALOG_DRC_CONTROL::OnLeftDClickClearance( wxMouseEvent& event )
{
    event.Skip();

    // I am assuming that the double click actually changed the selected item.
    // please verify this.
    int selection = m_ClearanceListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // Find the selected MARKER in the PCB, position cursor there.
        // Then close the dialog.
        const DRC_ITEM* item = m_ClearanceListBox->GetItem( selection );

        if( item )
        {
            m_brdEditor->CursorGoto( item->GetPointA() );
            m_brdEditor->GetGalCanvas()->GetView()->SetCenter( VECTOR2D( item->GetPointA() ) );

            if( !IsModal() )
            {
                // turn control over to m_brdEditor, hide this DIALOG_DRC_CONTROL window,
                // no destruction so we can preserve listbox cursor
                Show( false );

                // We do not want the clarification popup window.
                // when releasing the left button in the main window
                m_brdEditor->SkipNextLeftButtonReleaseEvent();
            }
        }
    }
}


void DIALOG_DRC_CONTROL::OnPopupMenu( wxCommandEvent& event )
{
    int             source = event.GetId();

    const DRC_ITEM* item = 0;
    wxPoint         pos;

    int             selection;

    switch( source )
    {
    case ID_POPUP_UNCONNECTED_A:
        selection = m_UnconnectedListBox->GetSelection();
        item = m_UnconnectedListBox->GetItem( selection );
        pos  = item->GetPointA();
        break;

    case ID_POPUP_UNCONNECTED_B:
        selection = m_UnconnectedListBox->GetSelection();
        item = m_UnconnectedListBox->GetItem( selection );
        pos  = item->GetPointB();
        break;

    case ID_POPUP_MARKERS_A:
        selection = m_ClearanceListBox->GetSelection();
        item = m_ClearanceListBox->GetItem( selection );
        pos  = item->GetPointA();
        break;

    case ID_POPUP_MARKERS_B:
        selection = m_ClearanceListBox->GetSelection();
        item = m_ClearanceListBox->GetItem( selection );
        pos  = item->GetPointB();
        break;
    }

    if( item )
    {
        m_brdEditor->CursorGoto( pos );
        m_brdEditor->GetGalCanvas()->GetView()->SetCenter( VECTOR2D( item->GetPointA() ) );

        if( !IsModal() )
            Show( false );
    }
}


void DIALOG_DRC_CONTROL::OnRightUpUnconnected( wxMouseEvent& event )
{
    event.Skip();

    // popup menu to go to either of the items listed in the DRC_ITEM.

    int selection = m_UnconnectedListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        wxMenu          menu;
        wxMenuItem*     mItem;
        const DRC_ITEM* dItem = m_UnconnectedListBox->GetItem( selection );

        mItem = new wxMenuItem( &menu, ID_POPUP_UNCONNECTED_A, dItem->GetTextA() );
        menu.Append( mItem );

        if( dItem->HasSecondItem() )
        {
            mItem = new wxMenuItem( &menu, ID_POPUP_UNCONNECTED_B, dItem->GetTextB() );
            menu.Append( mItem );
        }

        PopupMenu( &menu );
    }
}


void DIALOG_DRC_CONTROL::OnRightUpClearance( wxMouseEvent& event )
{
    event.Skip();

    // popup menu to go to either of the items listed in the DRC_ITEM.

    int selection = m_ClearanceListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        wxMenu          menu;
        wxMenuItem*     mItem;
        const DRC_ITEM* dItem = m_ClearanceListBox->GetItem( selection );

        mItem = new wxMenuItem( &menu, ID_POPUP_MARKERS_A, dItem->GetTextA() );
        menu.Append( mItem );

        if( dItem->HasSecondItem() )
        {
            mItem = new wxMenuItem( &menu, ID_POPUP_MARKERS_B, dItem->GetTextB() );
            menu.Append( mItem );
        }

        PopupMenu( &menu );
    }
}


void DIALOG_DRC_CONTROL::OnLeftDClickUnconnected( wxMouseEvent& event )
{
    event.Skip();

    // I am assuming that the double click actually changed the selected item.
    // please verify this.
    int selection = m_UnconnectedListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // Find the selected DRC_ITEM in the listbox, position cursor there,
        // at the first of the two pads.
        // Then hide the dialog.
        const DRC_ITEM* item = m_UnconnectedListBox->GetItem( selection );
        if( item )
        {
            m_brdEditor->CursorGoto( item->GetPointA() );
            m_brdEditor->GetGalCanvas()->GetView()->SetCenter( VECTOR2D( item->GetPointA() ) );

            if( !IsModal() )
            {
                Show( false );

                // We do not want the clarification popup window.
                // when releasing the left button in the main window
                m_brdEditor->SkipNextLeftButtonReleaseEvent();
            }
        }
    }
}

/* called when switching from Error list to Unconnected list
 * To avoid mistakes, the current marker is selection is cleared
 */
void DIALOG_DRC_CONTROL::OnChangingMarkerList( wxNotebookEvent& event )
{
    m_DeleteCurrentMarkerButton->Enable( false );
    m_ClearanceListBox->SetSelection( -1 );
    m_UnconnectedListBox->SetSelection( -1 );
}

void DIALOG_DRC_CONTROL::OnMarkerSelectionEvent( wxCommandEvent& event )
{
    int selection = event.GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // until a MARKER is selected, this button is not enabled.
        m_DeleteCurrentMarkerButton->Enable( true );

        // Find the selected DRC_ITEM in the listbox, position cursor there,
        // at the first of the two pads.
        const DRC_ITEM* item = m_ClearanceListBox->GetItem( selection );
        if( item )
        {
            m_brdEditor->CursorGoto( item->GetPointA(), false );
            m_brdEditor->GetGalCanvas()->GetView()->SetCenter( VECTOR2D( item->GetPointA() ) );
        }
    }

    event.Skip();
}


void DIALOG_DRC_CONTROL::OnUnconnectedSelectionEvent( wxCommandEvent& event )
{
    int selection = event.GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // until a MARKER is selected, this button is not enabled.
        m_DeleteCurrentMarkerButton->Enable( true );

        // Find the selected DRC_ITEM in the listbox, position cursor there,
        // at the first of the two pads.
        const DRC_ITEM* item = m_UnconnectedListBox->GetItem( selection );
        if( item )
        {
            m_brdEditor->CursorGoto( item->GetPointA(), false );
            m_brdEditor->GetGalCanvas()->GetView()->SetCenter( VECTOR2D( item->GetPointA() ) );
        }
    }

    event.Skip();
}


void DIALOG_DRC_CONTROL::RedrawDrawPanel()
{
    m_brdEditor->GetCanvas()->Refresh();
}


void DIALOG_DRC_CONTROL::DelDRCMarkers()
{
    m_brdEditor->SetCurItem( NULL );           // clear curr item, because it could be a DRC marker
    m_ClearanceListBox->DeleteAllItems();
    m_UnconnectedListBox->DeleteAllItems();
    m_DeleteCurrentMarkerButton->Enable( false );
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
        wxString prj_path =  Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    return fn.GetFullPath();
}


bool DIALOG_DRC_CONTROL::writeReport( const wxString& aFullFileName )
{
    FILE* fp = wxFopen( aFullFileName, wxT( "w" ) );

    if( fp == NULL )
        return false;

    int count;

    fprintf( fp, "** Drc report for %s **\n",
             TO_UTF8( m_brdEditor->GetBoard()->GetFileName() ) );

    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "** Created on %s **\n", TO_UTF8( now.Format( wxT( "%F %T" ) ) ) );

    count = m_ClearanceListBox->GetItemCount();

    fprintf( fp, "\n** Found %d DRC errors **\n", count );

    for( int i = 0;  i<count;  ++i )
        fprintf( fp, "%s", TO_UTF8( m_ClearanceListBox->GetItem( i )->ShowReport()) );

    count = m_UnconnectedListBox->GetItemCount();

    fprintf( fp, "\n** Found %d unconnected pads **\n", count );

    for( int i = 0;  i<count;  ++i )
        fprintf( fp, "%s", TO_UTF8( m_UnconnectedListBox->GetItem( i )->ShowReport() ) );

    fprintf( fp, "\n** End of Report **\n" );

    fclose( fp );

    return true;
}


void DIALOG_DRC_CONTROL::OnDeleteOneClick( wxCommandEvent& event )
{
    int selectedIndex;
    int curTab = m_Notebook->GetSelection();

    if( curTab == 0 )
    {
        selectedIndex = m_ClearanceListBox->GetSelection();

        if( selectedIndex != wxNOT_FOUND )
        {
            m_ClearanceListBox->DeleteItem( selectedIndex );

            // redraw the pcb
            RedrawDrawPanel();
        }
    }
    else if( curTab == 1 )
    {
        selectedIndex = m_UnconnectedListBox->GetSelection();

        if( selectedIndex != wxNOT_FOUND )
        {
            m_UnconnectedListBox->DeleteItem( selectedIndex );

            /* these unconnected DRC_ITEMs are not currently visible on the pcb
             *  RedrawDrawPanel();
             */
        }
    }

    UpdateDisplayedCounts();
}


void DIALOG_DRC_CONTROL::UpdateDisplayedCounts()
{
    int marker_count = m_ClearanceListBox->GetItemCount();
    int unconnected_count = m_UnconnectedListBox->GetItemCount();

    m_MarkerCount->SetLabelText( wxString::Format( "%d", marker_count ) );
    m_UnconnectedCount->SetLabelText( wxString::Format( "%d", unconnected_count ) );
}
