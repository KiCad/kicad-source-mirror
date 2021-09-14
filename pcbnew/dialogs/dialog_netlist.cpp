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

#include <project.h>
#include <kiface_i.h>
#include <confirm.h>
#include <dialogs/html_message_box.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <reporter.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <board.h>
#include <connectivity/connectivity_data.h>
#include <wildcards_and_files_ext.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <project/project_file.h>  // LAST_PATH_TYPE

#include <dialog_netlist.h>

#include <wx_html_report_panel.h>
#include <wx/filedlg.h>


void PCB_EDIT_FRAME::InstallNetlistFrame()
{
    wxString netlistName = GetLastPath( LAST_PATH_NETLIST );

    DIALOG_NETLIST dlg( this, netlistName );

    dlg.ShowModal();

    SetLastPath( LAST_PATH_NETLIST, netlistName );
}

bool DIALOG_NETLIST::m_warnForNoNetPads = false;
bool DIALOG_NETLIST::m_matchByUUID = false;


DIALOG_NETLIST::DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, wxString& aNetlistFullFilename )
    : DIALOG_NETLIST_BASE( aParent ),
      m_parent( aParent ),
      m_netlistPath( aNetlistFullFilename ),
      m_initialized( false ),
      m_runDragCommand( false )
{
    m_NetlistFilenameCtrl->SetValue( m_netlistPath );
    m_browseButton->SetBitmap( KiBitmap( BITMAPS::small_folder ) );

    auto cfg = m_parent->GetPcbNewSettings();

    m_cbUpdateFootprints->SetValue( cfg->m_NetlistDialog.update_footprints );
    m_cbDeleteShortingTracks->SetValue( cfg->m_NetlistDialog.delete_shorting_tracks );
    m_cbDeleteExtraFootprints->SetValue( cfg->m_NetlistDialog.delete_extra_footprints );
    m_cbDeleteSinglePadNets->SetValue( cfg->m_NetlistDialog.delete_single_pad_nets );

    m_cbWarnNoNetPad->SetValue( m_warnForNoNetPads );
    m_matchByTimestamp->SetSelection( m_matchByUUID ? 0 : 1 );

    m_MessageWindow->SetLabel( _("Changes To Be Applied") );
    m_MessageWindow->SetVisibleSeverities( cfg->m_NetlistDialog.report_filter );
    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Update PCB" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_buttonsSizer->Layout();

    m_sdbSizer1OK->SetDefault();
    finishDialogSettings();

    m_initialized = true;
    loadNetlist( true );
}

DIALOG_NETLIST::~DIALOG_NETLIST()
{
    m_warnForNoNetPads = m_cbWarnNoNetPad->GetValue();
    m_matchByUUID = m_matchByTimestamp->GetSelection() == 0;

    auto cfg = m_parent->GetPcbNewSettings();

    cfg->m_NetlistDialog.report_filter           = m_MessageWindow->GetVisibleSeverities();
    cfg->m_NetlistDialog.update_footprints       = m_cbUpdateFootprints->GetValue();
    cfg->m_NetlistDialog.delete_shorting_tracks  = m_cbDeleteShortingTracks->GetValue();
    cfg->m_NetlistDialog.delete_extra_footprints = m_cbDeleteExtraFootprints->GetValue();
    cfg->m_NetlistDialog.delete_single_pad_nets  = m_cbDeleteSinglePadNets->GetValue();

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
    wxFileName fn = m_NetlistFilenameCtrl->GetValue();

    if( !fn.IsOk() )
    {
        wxMessageBox( _( "Please, choose a valid netlist file." ) );
        return;
    }

    if( !fn.FileExists() )
    {
        wxMessageBox( _( "The netlist file does not exist." ) );
        return;
    }

    m_MessageWindow->SetLabel( _( "Changes Applied to PCB" ) );
    loadNetlist( false );

    m_sdbSizer1Cancel->SetDefault();
}


void DIALOG_NETLIST::OnFilenameKillFocus( wxFocusEvent& event )
{
    onFilenameChanged();
    event.Skip();
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
                m_netlistPath = m_NetlistFilenameCtrl->GetValue();
                loadNetlist( true );
            }
            else
            {
                m_MessageWindow->Clear();
                REPORTER& reporter = m_MessageWindow->Reporter();
                reporter.Report( _( "The netlist file does not exist." ), RPT_SEVERITY_ERROR );
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
    msg.Printf( _( "Reading netlist file '%s'.\n" ), netlistFileName  );
    reporter.ReportHead( msg, RPT_SEVERITY_INFO );

    if( m_matchByTimestamp->GetSelection() == 1 )
        msg = _( "Using reference designators to match symbols and footprints.\n" );
    else
        msg = _( "Using tstamps (unique IDs) to match symbols and footprints.\n" );

    reporter.ReportHead( msg, RPT_SEVERITY_INFO );
    m_MessageWindow->SetLazyUpdate( true ); // Use lazy update to speed the creation of the report
                                            // (the window is not updated for each message)
    m_matchByUUID = m_matchByTimestamp->GetSelection() == 0;

    NETLIST netlist;

    netlist.SetFindByTimeStamp( m_matchByUUID );
    netlist.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );

    if( !m_parent->ReadNetlistFromFile( netlistFileName, netlist, reporter ) )
        return;

    BOARD_NETLIST_UPDATER updater( m_parent, m_parent->GetBoard() );
    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun );
    updater.SetLookupByTimestamp( m_matchByUUID );
    updater.SetDeleteUnusedFootprints( m_cbDeleteExtraFootprints->GetValue());
    updater.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );
    updater.SetDeleteSinglePadNets( m_cbDeleteSinglePadNets->GetValue() );
    m_warnForNoNetPads = m_cbWarnNoNetPad->GetValue();
    updater.SetWarnPadNoNetInNetlist( m_warnForNoNetPads );
    updater.UpdateNetlist( netlist );

    // The creation of the report was made without window update: the full page must be displayed
    m_MessageWindow->Flush( true );

    if( aDryRun )
        return;

    m_parent->OnNetlistChanged( updater, &m_runDragCommand );
}


