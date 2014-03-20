/**
 * @file dialog_drc.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <dialog_drc.h>
#include <wxPcbStruct.h>
#include <base_units.h>
#include <class_board_design_settings.h>


/* class DIALOG_DRC_CONTROL: a dialog to set DRC parameters (clearance, min cooper size)
 * and run DRC tests
 */

DIALOG_DRC_CONTROL::DIALOG_DRC_CONTROL( DRC* aTester, PCB_EDIT_FRAME* parent ) :
    DIALOG_DRC_CONTROL_BASE( parent )
{
    m_tester = aTester;
    m_Parent = parent;
    m_BrdSettings = m_Parent->GetBoard()->GetDesignSettings();

    InitValues();
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    Centre();
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

    AddUnitSymbol( *m_TrackMinWidthTitle );
    AddUnitSymbol( *m_ViaMinTitle );
    AddUnitSymbol( *m_MicroViaMinTitle );

    m_DeleteCurrentMarkerButton->Enable( false );

    /* this looks terrible! does not fit into text field, do it in wxformbuilder instead
    m_SetClearance->SetValue( _("Netclasses values"));
    */

    Layout();      // adding the units above expanded Clearance text, now resize.

    // Set the initial "enabled" status of the browse button and the text
    // field for report name
    wxCommandEvent junk;
    OnReportCheckBoxClicked( junk );

    SetFocus();

    // deselect the existing text, seems SetFocus() wants to emulate
    // Microsoft and select all text, which is not desireable here.
//    m_SetClearance->SetSelection(0,0);
}

/* accept DRC parameters (min clearance value and min sizes
*/
void DIALOG_DRC_CONTROL::SetDrcParmeters( )
{
     m_BrdSettings.m_TrackMinWidth = ValueFromTextCtrl( *m_SetTrackMinWidthCtrl );
     m_BrdSettings.m_ViasMinSize = ValueFromTextCtrl( *m_SetViaMinSizeCtrl );
     m_BrdSettings.m_MicroViasMinSize = ValueFromTextCtrl( *m_SetMicroViakMinSizeCtrl );

     m_Parent->GetBoard()->SetDesignSettings( m_BrdSettings );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DRC_RUN
 */

void DIALOG_DRC_CONTROL::OnStartdrcClick( wxCommandEvent& event )
{
    wxString reportName;

    if( m_CreateRptCtrl->IsChecked() )      // Create a file rpt
    {
        reportName = m_RptFilenameCtrl->GetValue();

        if( reportName.IsEmpty() )
        {
            wxCommandEvent junk;
            OnButtonBrowseRptFileClick( junk );
        }

        reportName = m_RptFilenameCtrl->GetValue();
    }

    SetDrcParmeters();

    m_tester->SetSettings( true,        // Pad to pad DRC test enabled
                           true,        // unconnected pdas DRC test enabled
                           true,        // DRC test for zones enabled
                           true,        // DRC test for keepout areas enabled
                           reportName, m_CreateRptCtrl->IsChecked() );

    DelDRCMarkers();

    wxBeginBusyCursor();

    // run all the tests, with no UI at this time.
    m_Messages->Clear();
    wxSafeYield();                          // Allows time slice to refresh the m_Messages window
    m_tester->m_pcb->m_Status_Pcb = 0;      // Force full connectivity and ratsnest recalculations
    m_tester->RunTests(m_Messages);

#if wxCHECK_VERSION( 2, 8, 0 )
    m_Notebook->ChangeSelection( 0 );       // display the 1at tab "...Markers ..."
#else
    m_Notebook->SetSelection( 0 );          // display the 1at tab "... Markers..."
#endif


    // Generate the report
    if( !reportName.IsEmpty() )
    {
        FILE* fp = wxFopen( reportName, wxT( "w" ) );
        writeReport( fp );
        fclose( fp );

        wxString        msg;
        msg.Printf( _( "Report file \"%s\" created" ), GetChars( reportName ) );

        wxString        caption( _( "Disk File Report Completed" ) );
        wxMessageDialog popupWindow( this, msg, caption );

        popupWindow.ShowModal();
    }

    wxEndBusyCursor();

    RedrawDrawPanel();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERASE_DRC_MARKERS
 */

void DIALOG_DRC_CONTROL::OnDeleteAllClick( wxCommandEvent& event )
{
    DelDRCMarkers();
    RedrawDrawPanel();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LIST_UNCONNECTED_PADS
 */

void DIALOG_DRC_CONTROL::OnListUnconnectedClick( wxCommandEvent& event )
{
    wxString reportName;

    if( m_CreateRptCtrl->IsChecked() )      // Create a file rpt
    {
        reportName = m_RptFilenameCtrl->GetValue();

        if( reportName.IsEmpty() )
        {
            wxCommandEvent junk;
            OnButtonBrowseRptFileClick( junk );
        }

        reportName = m_RptFilenameCtrl->GetValue();
    }

    SetDrcParmeters();

    m_tester->SetSettings( true,        // Pad to pad DRC test enabled
                           true,        // unconnected pdas DRC test enabled
                           true,        // DRC test for zones enabled
                           true,        // DRC test for keepout areas enabled
                           reportName, m_CreateRptCtrl->IsChecked() );

    DelDRCMarkers();

    wxBeginBusyCursor();

    m_Messages->Clear();
    m_tester->ListUnconnectedPads();

#if wxCHECK_VERSION( 2, 8, 0 )
    m_Notebook->ChangeSelection( 1 );       // display the 2nd tab "Unconnected..."
#else
    m_Notebook->SetSelection( 1 );          // display the 2nd tab "Unconnected..."
#endif

    // Generate the report
    if( !reportName.IsEmpty() )
    {
        FILE* fp = wxFopen( reportName, wxT( "w" ) );
        writeReport( fp );
        fclose( fp );

        wxString        msg;
        msg.Printf( _( "Report file \"%s\" created" ), GetChars( reportName ) );
        wxString        caption( _( "Disk File Report Completed" ) );
        wxMessageDialog popupWindow( this, msg, caption );
        popupWindow.ShowModal();
    }

    wxEndBusyCursor();

    /* there is currently nothing visible on the DrawPanel for unconnected pads
     *  RedrawDrawPanel();
     */
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BROWSE_RPT_FILE
 */

void DIALOG_DRC_CONTROL::OnButtonBrowseRptFileClick( wxCommandEvent& event )
{
    wxFileName fn;
    wxString   wildcard( _( "DRC report files (.rpt)|*.rpt" ) );
    wxString   Ext( wxT( "rpt" ) );

    fn = m_Parent->GetBoard()->GetFileName() + wxT( "-drc" );
    fn.SetExt( Ext );

    wxFileDialog dlg( this, _( "Save DRC Report File" ), wxEmptyString,
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_RptFilenameCtrl->SetValue( dlg.GetPath() );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_DRC_CONTROL::OnOkClick( wxCommandEvent& event )
{
    SetReturnCode( wxID_OK );
    SetDrcParmeters();

    m_tester->DestroyDialog( wxID_OK );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_DRC_CONTROL::OnCancelClick( wxCommandEvent& event )
{
    SetReturnCode( wxID_CANCEL );

    m_tester->DestroyDialog( wxID_CANCEL );
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
 */

void DIALOG_DRC_CONTROL::OnReportCheckBoxClicked( wxCommandEvent& event )
{
    if( m_CreateRptCtrl->IsChecked() )
    {
        m_RptFilenameCtrl->Enable( true );
        m_BrowseButton->Enable( true );
    }
    else
    {
        m_RptFilenameCtrl->Enable( false );
        m_BrowseButton->Enable( false );
    }
}


/*!
 * wxEVT_LEFT_DCLICK event handler for ID_CLEARANCE_LIST
 */

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
            /*
             *  // after the goto, process a button OK command later.
             *  wxCommandEvent  cmd( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK );
             *  ::wxPostEvent( GetEventHandler(), cmd );
             */

            m_Parent->CursorGoto( item->GetPointA() );

            // turn control over to m_Parent, hide this DIALOG_DRC_CONTROL window,
            // no destruction so we can preserve listbox cursor
            Show( false );

            // We do not want the clarification popup window.
            // when releasing the left button in the main window
            m_Parent->SkipNextLeftButtonReleaseEvent();
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
        m_Parent->CursorGoto( pos );
        Show( false );
    }
}


/*!
 * wxEVT_RIGHT_UP event handler for ID_CLEARANCE_LIST
 */

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


/*!
 * wxEVT_RIGHT_UP event handler for ID_CLEARANCE_LIST
 */

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


/*!
 * wxEVT_LEFT_DCLICK event handler for ID_UNCONNECTED_LIST
 */

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
            m_Parent->CursorGoto( item->GetPointA() );

            Show( false );

            // We do not want the clarification popup window.
            // when releasing the left button in the main window
            m_Parent->SkipNextLeftButtonReleaseEvent();
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
            m_Parent->CursorGoto( item->GetPointA(), false );
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
            m_Parent->CursorGoto( item->GetPointA(), false );
    }

    event.Skip();
}


void DIALOG_DRC_CONTROL::RedrawDrawPanel()
{
    m_Parent->GetCanvas()->Refresh();
}


void DIALOG_DRC_CONTROL::DelDRCMarkers()
{
    m_Parent->SetCurItem( NULL );           // clear curr item, because it could be a DRC marker
    m_ClearanceListBox->DeleteAllItems();
    m_UnconnectedListBox->DeleteAllItems();
    m_DeleteCurrentMarkerButton->Enable( false );
}


void DIALOG_DRC_CONTROL::writeReport( FILE* fp )
{
    int count;

    fprintf( fp, "** Drc report for %s **\n",
             TO_UTF8( m_Parent->GetBoard()->GetFileName() ) );

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
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ONE
 */

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
}
