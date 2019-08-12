/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <project.h>
#include <kiface_i.h>
#include <confirm.h>
#include <macros.h>
#include <html_messagebox.h>
#include <pcb_edit_frame.h>
#include <pcb_netlist.h>
#include <reporter.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <tools/drc.h>
#include <tools/pcb_actions.h>
#include <class_board.h>
#include <connectivity/connectivity_data.h>
#include <wildcards_and_files_ext.h>
#include <board_netlist_updater.h>

#include <dialog_netlist.h>
#include <wx_html_report_panel.h>

#define NETLIST_FILTER_MESSAGES_KEY wxT("NetlistReportFilterMsg")
#define NETLIST_UPDATEFOOTPRINTS_KEY wxT("NetlistUpdateFootprints")
#define NETLIST_DELETESHORTINGTRACKS_KEY wxT("NetlistDeleteShortingTracks")
#define NETLIST_DELETEEXTRAFOOTPRINTS_KEY wxT("NetlistDeleteExtraFootprints")
#define NETLIST_DELETESINGLEPADNETS_KEY wxT("NetlistDeleteSinglePadNets")

void PCB_EDIT_FRAME::InstallNetlistFrame()
{
    wxString netlistName = GetLastPath( LAST_PATH_NETLIST );

    DIALOG_NETLIST dlg( this, netlistName );

    dlg.ShowModal();

    SetLastPath( LAST_PATH_NETLIST, netlistName );
}


DIALOG_NETLIST::DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, const wxString & aNetlistFullFilename )
    : DIALOG_NETLIST_BASE( aParent ),
      m_parent( aParent ),
      m_initialized( false ),
      m_runDragCommand( false )
{
    m_config = Kiface().KifaceSettings();

    m_NetlistFilenameCtrl->SetValue( aNetlistFullFilename );
    m_browseButton->SetBitmap( KiBitmap( folder_xpm ) );

    m_cbUpdateFootprints->SetValue( m_config->Read( NETLIST_UPDATEFOOTPRINTS_KEY, 0l ) );
    m_cbDeleteShortingTracks->SetValue( m_config->Read( NETLIST_DELETESHORTINGTRACKS_KEY, 0l ) );
    m_cbDeleteExtraFootprints->SetValue( m_config->Read( NETLIST_DELETEEXTRAFOOTPRINTS_KEY, 0l ) );
    m_cbDeleteSinglePadNets->SetValue( m_config->Read( NETLIST_DELETESINGLEPADNETS_KEY, 0l ) );

    m_MessageWindow->SetLabel( _("Changes To Be Applied") );
    m_MessageWindow->SetVisibleSeverities( m_config->Read( NETLIST_FILTER_MESSAGES_KEY, -1l ) );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Update PCB" ) );
    m_sdbSizer1Apply->SetLabel( _( "Rebuild Ratsnest" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_buttonsSizer->Layout();

    m_sdbSizer1OK->SetDefault();
    FinishDialogSettings();

    m_initialized = true;
    loadNetlist( true );
}

DIALOG_NETLIST::~DIALOG_NETLIST()
{
    m_config->Write( NETLIST_UPDATEFOOTPRINTS_KEY, m_cbUpdateFootprints->GetValue() );
    m_config->Write( NETLIST_DELETESHORTINGTRACKS_KEY, m_cbDeleteShortingTracks->GetValue() );
    m_config->Write( NETLIST_DELETEEXTRAFOOTPRINTS_KEY, m_cbDeleteExtraFootprints->GetValue() );
    m_config->Write( NETLIST_DELETESINGLEPADNETS_KEY, m_cbDeleteSinglePadNets->GetValue() );
    m_config->Write( NETLIST_FILTER_MESSAGES_KEY, (long) m_MessageWindow->GetVisibleSeverities() );

    if( m_runDragCommand )
    {
        KIGFX::VIEW_CONTROLS* controls = m_parent->GetCanvas()->GetViewControls();
        controls->SetCursorPosition( controls->GetMousePosition() );
        m_parent->GetToolManager()->RunAction( PCB_ACTIONS::move, true );
    }
}


void DIALOG_NETLIST::OnOpenNetlistClick( wxCommandEvent& event )
{
    wxString dirPath = wxFileName( Prj().GetProjectFullName() ).GetPath();

    wxString filename = m_parent->GetLastPath( LAST_PATH_NETLIST );

    if( !filename.IsEmpty() )
    {
        wxFileName fn = filename;
        dirPath = fn.GetPath();
        filename = fn.GetFullName();
    }

    wxFileDialog FilesDialog( this, _( "Select Netlist" ), dirPath, filename,
                              NetlistFileWildcard(), wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    m_NetlistFilenameCtrl->SetValue( FilesDialog.GetPath() );
    onFilenameChanged();
}

void DIALOG_NETLIST::OnUpdatePCB( wxCommandEvent& event )
{
    BOARD*     pcb = m_parent->GetBoard();
    wxFileName fn = m_NetlistFilenameCtrl->GetValue();

    if( !fn.IsOk() )
    {
        wxMessageBox( _("Please, choose a valid netlist file.") );
        return;
    }

    if( !fn.FileExists() )
    {
        wxMessageBox( _("The netlist file does not exist.") );
        return;
    }

    // Give the user a chance to bail out when making changes from a netlist.
    if( pcb->IsEmpty() || IsOK( this, _( "The changes made cannot be undone.  "
                                         "Are you sure you want to update the PCB?" ) ) )
    {
        m_MessageWindow->SetLabel( _( "Changes Applied To PCB" ) );
        loadNetlist( false );

        m_sdbSizer1Cancel->SetDefault();
    }
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    if( m_parent->GetBoard()->GetFirstModule() == nullptr )
    {
        DisplayInfoMessage( this, _( "No footprints." ) );
        return;
    }

    wxString        netlistFilename = m_NetlistFilenameCtrl->GetValue();
    NETLIST         netlist;
    wxBusyCursor    dummy;         // Shows an hourglass while calculating.

    if( !m_parent->ReadNetlistFromFile( netlistFilename, netlist, NULL_REPORTER::GetInstance() ) )
        return;

    HTML_MESSAGE_BOX dlg( this, _( "Check footprints" ) );
    DRC_LIST drcItems;

    DRC::TestFootprints( netlist, m_parent->GetBoard(), GetUserUnits(), drcItems );

    for( DRC_ITEM* item : drcItems )
        dlg.AddHTML_Text( item->ShowHtml( GetUserUnits() ) );

    dlg.ShowModal();
}


void DIALOG_NETLIST::OnFilenameKillFocus( wxFocusEvent& event )
{
    onFilenameChanged();
}


void DIALOG_NETLIST::onFilenameChanged()
{
    if( m_initialized )
    {
        wxFileName fn = m_NetlistFilenameCtrl->GetValue();
        if( fn.IsOk() )
        {
            if( fn.FileExists() )
            {
                loadNetlist( true );
            }
            else
            {
                m_MessageWindow->Clear();
                REPORTER& reporter = m_MessageWindow->Reporter();
                reporter.Report( _("The netlist file does not exist."), REPORTER::RPT_ERROR );
            }
        }
    }
}


void DIALOG_NETLIST::OnMatchChanged( wxCommandEvent& event )
{
    if( m_initialized )
        loadNetlist( true );
}


void DIALOG_NETLIST::OnOptionChanged( wxCommandEvent& event )
{
    if( m_initialized )
        loadNetlist( true );
}


void DIALOG_NETLIST::OnCompileRatsnestClick( wxCommandEvent& event )
{
    // Rebuild the board connectivity:
    auto board = m_parent->GetBoard();
	board->GetConnectivity()->RecalculateRatsnest();
}


void DIALOG_NETLIST::OnUpdateUIValidNetlistFile( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( !m_NetlistFilenameCtrl->GetValue().IsEmpty() );
}


void DIALOG_NETLIST::loadNetlist( bool aDryRun )
{
    wxString netlistFileName = m_NetlistFilenameCtrl->GetValue();
    wxFileName fn = netlistFileName;

    if( !fn.IsOk() || !fn.FileExists() )
        return;

    m_MessageWindow->Clear();
    REPORTER& reporter = m_MessageWindow->Reporter();

    wxBusyCursor busy;

    wxString msg;
    msg.Printf( _( "Reading netlist file \"%s\".\n" ), GetChars( netlistFileName ) );
    reporter.ReportHead( msg, REPORTER::RPT_INFO );

    if( m_matchByTimestamp->GetSelection() == 0 )
        msg = _( "Using references to match components and footprints.\n" );
    else
        msg = _( "Using tstamp fields to match components and footprints.\n" );

    reporter.ReportHead( msg, REPORTER::RPT_INFO );
    m_MessageWindow->SetLazyUpdate( true ); // Use lazy update to speed the creation of the report
                                            // (the window is not updated for each message)
    NETLIST netlist;

    netlist.SetDeleteExtraFootprints( m_cbDeleteExtraFootprints->GetValue() );
    netlist.SetFindByTimeStamp( m_matchByTimestamp->GetSelection() == 1 );
    netlist.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );

    if( !m_parent->ReadNetlistFromFile( netlistFileName, netlist, reporter ) )
        return;

    BOARD_NETLIST_UPDATER updater( m_parent, m_parent->GetBoard() );
    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun );
    updater.SetLookupByTimestamp( m_matchByTimestamp->GetSelection() == 1 );
    updater.SetDeleteUnusedComponents ( m_cbDeleteExtraFootprints->GetValue() );
    updater.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );
    updater.SetDeleteSinglePadNets( m_cbDeleteSinglePadNets->GetValue() );
    updater.UpdateNetlist( netlist );

    // The creation of the report was made without window update: the full page must be displayed
    m_MessageWindow->Flush( true );

    if( aDryRun )
        return;

    m_parent->OnNetlistChanged( updater, &m_runDragCommand );
}


