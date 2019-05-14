/**
 * @file dialog_drc.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2016 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <confirm.h>
#include <wildcards_and_files_ext.h>
#include <bitmaps.h>
#include <pgm_base.h>
#include <dialog_drc.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <board_design_settings.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <collectors.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

/* class DIALOG_DRC_CONTROL: a dialog to set DRC parameters (clearance, min cooper size)
 * and run DRC tests
 */

// Keywords for read and write config
#define RefillZonesBeforeDrcKey        wxT( "RefillZonesBeforeDrc" )
#define DrcTrackToZoneTestKey        wxT( "DrcTrackToZoneTest" )


DIALOG_DRC_CONTROL::DIALOG_DRC_CONTROL( DRC* aTester, PCB_EDIT_FRAME* aEditorFrame,
                                        wxWindow* aParent ) :
    DIALOG_DRC_CONTROL_BASE( aParent ),
    m_trackMinWidth( aEditorFrame, m_TrackMinWidthTitle, m_SetTrackMinWidthCtrl,
                     m_TrackMinWidthUnit, true ),
    m_viaMinSize( aEditorFrame, m_ViaMinTitle, m_SetViaMinSizeCtrl, m_ViaMinUnit, true ),
    m_uviaMinSize( aEditorFrame, m_MicroViaMinTitle, m_SetMicroViakMinSizeCtrl,
                   m_MicroViaMinUnit, true )
{
    m_config = Kiface().KifaceSettings();
    m_tester = aTester;
    m_brdEditor = aEditorFrame;
    m_currentBoard = m_brdEditor->GetBoard();
    m_BrdSettings = m_brdEditor->GetBoard()->GetDesignSettings();

    wxFont messagesLabelFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    messagesLabelFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_messagesLabel->SetFont( messagesLabelFont );

    m_BrowseButton->SetBitmap( KiBitmap( folder_xpm ) );

    // We use a sdbSizer here to get the order right, which is platform-dependent
    m_sdbSizer1OK->SetLabel( _( "Run DRC" ) );
    m_sdbSizer1Apply->SetLabel( _( "List Unconnected" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sizerButtons->Layout();

    m_sdbSizer1OK->SetDefault();

    InitValues();

    // Connect events
    m_ClearanceListBox->Connect( ID_CLEARANCE_LIST, wxEVT_LEFT_DCLICK,
                                 wxMouseEventHandler( DIALOG_DRC_CONTROL::OnLeftDClickClearance ),
                                 NULL, this );
    m_ClearanceListBox->Connect( ID_CLEARANCE_LIST, wxEVT_RIGHT_UP,
                                 wxMouseEventHandler( DIALOG_DRC_CONTROL::OnRightUpClearance ),
                                 NULL, this );
    m_UnconnectedListBox->Connect( ID_UNCONNECTED_LIST, wxEVT_LEFT_DCLICK,
                                   wxMouseEventHandler( DIALOG_DRC_CONTROL::OnLeftDClickUnconnected ),
                                   NULL, this );
    m_UnconnectedListBox->Connect( ID_UNCONNECTED_LIST, wxEVT_RIGHT_UP,
                                   wxMouseEventHandler( DIALOG_DRC_CONTROL::OnRightUpUnconnected ),                                    NULL, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_DRC_CONTROL::~DIALOG_DRC_CONTROL()
{
    m_config->Write( RefillZonesBeforeDrcKey, m_cbRefillZones->GetValue() );
    m_config->Write( DrcTrackToZoneTestKey, m_cbReportTracksToZonesErrors->GetValue() );

    // Disconnect events
    m_ClearanceListBox->Disconnect( ID_CLEARANCE_LIST, wxEVT_LEFT_DCLICK,
                                    wxMouseEventHandler( DIALOG_DRC_CONTROL::OnLeftDClickClearance ),
                                    NULL, this );
    m_ClearanceListBox->Disconnect( ID_CLEARANCE_LIST, wxEVT_RIGHT_UP,
                                    wxMouseEventHandler( DIALOG_DRC_CONTROL::OnRightUpClearance ),
                                    NULL, this );
    m_UnconnectedListBox->Disconnect( ID_UNCONNECTED_LIST, wxEVT_LEFT_DCLICK,
                                      wxMouseEventHandler( DIALOG_DRC_CONTROL::OnLeftDClickUnconnected ),
                                      NULL, this );
    m_UnconnectedListBox->Disconnect( ID_UNCONNECTED_LIST, wxEVT_RIGHT_UP,
                                      wxMouseEventHandler( DIALOG_DRC_CONTROL::OnRightUpUnconnected ),
                                      NULL, this );
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
    m_markersTitleTemplate = m_Notebook->GetPageText( 0 );
    m_unconnectedTitleTemplate = m_Notebook->GetPageText( 1 );

    m_DeleteCurrentMarkerButton->Enable( false );

    DisplayDRCValues();

    // read options
    bool value;
    m_config->Read( RefillZonesBeforeDrcKey, &value, false );
    m_cbRefillZones->SetValue( value );
    m_config->Read( DrcTrackToZoneTestKey, &value, false );
    m_cbReportTracksToZonesErrors->SetValue( value );

    Layout();      // adding the units above expanded Clearance text, now resize.

    SetFocus();
}


void DIALOG_DRC_CONTROL::SetDrcParmeters( )
{
    m_BrdSettings.m_TrackMinWidth = m_trackMinWidth.GetValue();
    m_BrdSettings.m_ViasMinSize = m_viaMinSize.GetValue();
    m_BrdSettings.m_MicroViasMinSize = m_uviaMinSize.GetValue();

    m_brdEditor->GetBoard()->SetDesignSettings( m_BrdSettings );
}


void DIALOG_DRC_CONTROL::SetRptSettings( bool aEnable, const wxString& aFileName )
{
    m_RptFilenameCtrl->SetValue( aFileName );
    m_CreateRptCtrl->SetValue( aEnable );
}


void DIALOG_DRC_CONTROL::GetRptSettings( bool* aEnable, wxString& aFileName )
{
    *aEnable = m_CreateRptCtrl->GetValue();
    aFileName = m_RptFilenameCtrl->GetValue();
}


void DIALOG_DRC_CONTROL::OnStartdrcClick( wxCommandEvent& event )
{
    wxString reportName, msg;

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
                           // DRC test for zones enabled/disabled:
                           m_cbReportTracksToZonesErrors->GetValue(),
                           true,        // DRC test for keepout areas enabled
                           m_cbRefillZones->GetValue(),
                           m_cbReportAllTrackErrors->GetValue(),
                           reportName, make_report );

    DelDRCMarkers();

    wxBeginBusyCursor();
    wxWindowDisabler disabler;

    // run all the tests, with no UI at this time.
    m_Messages->Clear();
    wxSafeYield();                             // Allows time slice to refresh the Messages
    m_brdEditor->GetBoard()->m_Status_Pcb = 0; // Force full connectivity and ratsnest calculations
    m_tester->RunTests(m_Messages);
    m_Notebook->ChangeSelection( 0 );          // display the "Problems/Markers" tab

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
    wxString reportName, msg;

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
                           m_cbRefillZones->GetValue(),
                           m_cbReportAllTrackErrors->GetValue(),
                           reportName, make_report );

    DelDRCMarkers();

    wxBeginBusyCursor();

    m_Messages->Clear();
    m_tester->ListUnconnectedPads();

    m_Notebook->ChangeSelection( 1 );       // display the "Unconnected" tab

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

    UpdateDisplayedCounts();

    wxEndBusyCursor();

    /* there is currently nothing visible on the DrawPanel for unconnected pads
     *  RedrawDrawPanel();
     */
}


void DIALOG_DRC_CONTROL::OnButtonBrowseRptFileClick( wxCommandEvent&  )
{
    wxFileName fn = m_brdEditor->GetBoard()->GetFileName();
    fn.SetExt( ReportFileExtension );
    wxString prj_path =  Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Save DRC Report File" ), prj_path, fn.GetFullName(),
                      ReportFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_CreateRptCtrl->SetValue( true );
    m_RptFilenameCtrl->SetValue( dlg.GetPath() );
}


void DIALOG_DRC_CONTROL::OnCancelClick( wxCommandEvent& event )
{
    SetReturnCode( wxID_CANCEL );
    SetDrcParmeters();

    // The dialog can be modal or not modal.
    // Leave the DRC caller destroy (or not) the dialog
    m_tester->DestroyDRCDialog( wxID_CANCEL );
}


void DIALOG_DRC_CONTROL::OnReportCheckBoxClicked( wxCommandEvent& event )
{
    if( m_CreateRptCtrl->IsChecked() )
        m_RptFilenameCtrl->SetFocus();
}


void DIALOG_DRC_CONTROL::OnReportFilenameEdited( wxCommandEvent &event )
{
    m_CreateRptCtrl->SetValue( event.GetString().Length() );
}


void DIALOG_DRC_CONTROL::OnLeftDClickClearance( wxMouseEvent& event )
{
    event.Skip();

    int selection = m_ClearanceListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        if( focusOnItem( m_ClearanceListBox->GetItem( selection ) ) )
        {
            if( !IsModal() )
            {
                // turn control over to m_brdEditor, hide this DIALOG_DRC_CONTROL window,
                // no destruction so we can preserve listbox cursor
                Show( false );

                // We do not want the clarify selection popup when releasing the
                // left button in the main window
                m_brdEditor->SkipNextLeftButtonReleaseEvent();
            }
        }
    }
}


void DIALOG_DRC_CONTROL::OnLeftUpClearance( wxMouseEvent& event )
{
    int selection = m_ClearanceListBox->GetSelection();

    if( selection != wxNOT_FOUND )
        focusOnItem( m_ClearanceListBox->GetItem( selection ) );
}


bool DIALOG_DRC_CONTROL::focusOnItem( const DRC_ITEM* aItem )
{
    if( !aItem )
        return false;

    auto toolmgr = m_brdEditor->GetToolManager();
    auto pos = aItem->GetPointA();
    auto marker = static_cast<MARKER_PCB*>( aItem->GetParent() );

    if( marker )
    {
        pos = marker->GetPos();

        toolmgr->RunAction( PCB_ACTIONS::selectionClear, true );
        toolmgr->RunAction( PCB_ACTIONS::selectItem, true, marker );
    }

    toolmgr->GetView()->SetCenter( pos );
    m_brdEditor->GetCanvas()->Refresh();

    return true;
}


void DIALOG_DRC_CONTROL::OnRightUpUnconnected( wxMouseEvent& event )
{
    // popup menu to go to either of the items listed in the DRC_ITEM.
    // Check if user right-clicked on a different item
    int selection = m_UnconnectedListBox->HitTest( event.GetPosition() );

    if( selection == wxNOT_FOUND )
        selection = m_UnconnectedListBox->GetSelection();
    else
        m_UnconnectedListBox->SetSelection( selection );

    if( selection != wxNOT_FOUND )
        doSelectionMenu( m_UnconnectedListBox->GetItem( selection ) );
}


void DIALOG_DRC_CONTROL::OnRightUpClearance( wxMouseEvent& event )
{
    // popup menu to go to either of the items listed in the DRC_ITEM.
    // Check if user right-clicked on a different item
    int selection = m_ClearanceListBox->HitTest( event.GetPosition() );

    if( selection == wxNOT_FOUND )
        selection = m_ClearanceListBox->GetSelection();
    else
        m_ClearanceListBox->SetSelection( selection );

    if( selection != wxNOT_FOUND )
        doSelectionMenu( m_ClearanceListBox->GetItem( selection ) );
}


void DIALOG_DRC_CONTROL::doSelectionMenu( const DRC_ITEM* aItem )
{
    // popup menu to go to either of the items listed in the DRC_ITEM.

    BOARD_ITEM* first = aItem->GetMainItem( m_brdEditor->GetBoard() );
    BOARD_ITEM* second = nullptr;

    GENERAL_COLLECTOR items;

    items.Append( first );

    if( aItem->HasSecondItem() )
    {
        second = aItem->GetAuxiliaryItem( m_brdEditor->GetBoard() );
        items.Append( second );
    }

    WINDOW_THAWER thawer( m_brdEditor );
    m_brdEditor->GetToolManager()->VetoContextMenuMouseWarp();
    m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionMenu, true, &items );

    // If we got an item, focus on it
    BOARD_ITEM* selection = m_brdEditor->GetCurItem();

    if( selection && ( selection == first || selection == second ) )
        m_brdEditor->GetToolManager()->GetView()->SetCenter( selection->GetPosition() );

    m_brdEditor->GetCanvas()->Refresh();
}


void DIALOG_DRC_CONTROL::OnLeftDClickUnconnected( wxMouseEvent& event )
{
    event.Skip();

    int selection = m_UnconnectedListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        if( focusOnItem( m_UnconnectedListBox->GetItem( selection ) ) )
        {
            if( !IsModal() )
            {
                // turn control over to m_brdEditor, hide this DIALOG_DRC_CONTROL window,
                // no destruction so we can preserve listbox cursor
                Show( false );

                // We do not want the clarify selection popup when releasing the
                // left button in the main window
                m_brdEditor->SkipNextLeftButtonReleaseEvent();
            }
        }
    }
}


void DIALOG_DRC_CONTROL::OnLeftUpUnconnected( wxMouseEvent& event )
{
    int selection = m_UnconnectedListBox->GetSelection();

    if( selection != wxNOT_FOUND )
        focusOnItem( m_UnconnectedListBox->GetItem( selection ) );
}


void DIALOG_DRC_CONTROL::OnChangingMarkerList( wxNotebookEvent& event )
{
    // Shouldn't be necessary, but is on at least OSX
    m_Notebook->ChangeSelection( event.GetSelection() );

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

        // Find the selected DRC_ITEM in the listbox, position cursor there.
        focusOnItem( m_ClearanceListBox->GetItem( selection ) );
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

        // Find the selected DRC_ITEM in the listbox, position cursor there.
        focusOnItem( m_UnconnectedListBox->GetItem( selection ) );
    }

    event.Skip();
}


void DIALOG_DRC_CONTROL::RedrawDrawPanel()
{
    WINDOW_THAWER thawer( m_brdEditor );

    m_brdEditor->GetCanvas()->Refresh();
}


void DIALOG_DRC_CONTROL::DelDRCMarkers()
{
    m_brdEditor->SetCurItem( NULL );           // clear curr item, because it could be a DRC marker

    // Clear current selection list to avoid selection of deleted items
    m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

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
    EDA_UNITS_T units = GetUserUnits();

    fprintf( fp, "** Drc report for %s **\n",
             TO_UTF8( m_brdEditor->GetBoard()->GetFileName() ) );

    wxDateTime now = wxDateTime::Now();

    fprintf( fp, "** Created on %s **\n", TO_UTF8( now.Format( wxT( "%F %T" ) ) ) );

    count = m_ClearanceListBox->GetItemCount();

    fprintf( fp, "\n** Found %d DRC errors **\n", count );

    for( int i = 0;  i<count;  ++i )
        fprintf( fp, "%s", TO_UTF8( m_ClearanceListBox->GetItem( i )->ShowReport( units ) ) );

    count = m_UnconnectedListBox->GetItemCount();

    fprintf( fp, "\n** Found %d unconnected pads **\n", count );

    for( int i = 0;  i<count;  ++i )
        fprintf( fp, "%s", TO_UTF8( m_UnconnectedListBox->GetItem( i )->ShowReport( units ) ) );

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
            // Clear the current item.  It may be the selected DRC marker.
            m_brdEditor->SetCurItem( NULL );
            m_brdEditor->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

            size_t newIndex = wxNOT_FOUND;

            if( m_ClearanceListBox->GetItemCount() > 1 )
            {
                newIndex = ( selectedIndex == m_ClearanceListBox->GetItemCount() - 1 ) ?
                           selectedIndex - 1 : selectedIndex;
            }

            m_ClearanceListBox->DeleteItem( selectedIndex );

            if( newIndex != wxNOT_FOUND )
            {
                focusOnItem( m_ClearanceListBox->GetItem( newIndex ) );
                m_ClearanceListBox->SetSelection( newIndex );
            }

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

    m_Notebook->SetPageText( 0, wxString::Format( m_markersTitleTemplate, marker_count ) );
    m_Notebook->SetPageText( 1, wxString::Format( m_unconnectedTitleTemplate, unconnected_count ) );

}
