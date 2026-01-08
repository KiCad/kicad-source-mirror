/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <reporter.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <connectivity/connectivity_data.h>
#include <wildcards_and_files_ext.h>
#include <widgets/std_bitmap_button.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/board_netlist_updater.h>
#include <project/project_file.h>  // LAST_PATH_TYPE
#include <dialog_import_netlist.h>
#include <widgets/wx_html_report_panel.h>
#include <view/view_controls.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>


void PCB_EDIT_FRAME::InstallNetlistFrame()
{
    wxString netlistName = GetLastPath( LAST_PATH_NETLIST );

    DIALOG_IMPORT_NETLIST dlg( this, netlistName );

    dlg.ShowModal();

    SetLastPath( LAST_PATH_NETLIST, netlistName );
}


DIALOG_IMPORT_NETLIST::DIALOG_IMPORT_NETLIST( PCB_EDIT_FRAME* aParent, wxString& aNetlistFullFilename ) :
        DIALOG_IMPORT_NETLIST_BASE( aParent ),
        m_parent( aParent ),
        m_netlistPath( aNetlistFullFilename ),
        m_initialized( false ),
        m_runDragCommand( false )
{
    m_NetlistFilenameCtrl->SetValue( m_netlistPath );
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_MessageWindow->SetLabel( _("Changes to Be Applied") );
    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    SetupStandardButtons( { { wxID_OK,     _( "Load and Test Netlist" ) },
                            { wxID_CANCEL, _( "Close" ) },
                            { wxID_APPLY,  _( "Update PCB" ) } } );

    finishDialogSettings();

    m_initialized = true;
}

DIALOG_IMPORT_NETLIST::~DIALOG_IMPORT_NETLIST()
{
    if( m_runDragCommand )
    {
        PCB_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
        PCB_SELECTION&      selection = selTool->GetSelection();

        // Set the reference point to (0,0) where the new footprints were spread. This ensures
        // the move tool knows where the items are located, preventing an offset when the "warp
        // cursor to origin of moved object" preference is disabled.
        if( selection.Size() > 0 )
            selection.SetReferencePoint( VECTOR2I( 0, 0 ) );

        KIGFX::VIEW_CONTROLS* controls = m_parent->GetCanvas()->GetViewControls();
        controls->SetCursorPosition( controls->GetMousePosition() );
        m_parent->GetToolManager()->RunAction( PCB_ACTIONS::move );
    }
}


void DIALOG_IMPORT_NETLIST::onBrowseNetlistFiles( wxCommandEvent& event )
{
    wxString dirPath = wxFileName( Prj().GetProjectFullName() ).GetPath();
    wxString filename = m_parent->GetLastPath( LAST_PATH_NETLIST );

    if( !filename.IsEmpty() )
    {
        wxFileName fn = filename;
        dirPath = fn.GetPath();
        filename = fn.GetFullName();
    }

    wxFileDialog FilesDialog( this, _( "Import Netlist" ), dirPath, filename, FILEEXT::NetlistFileWildcard(),
                              wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    m_NetlistFilenameCtrl->SetValue( FilesDialog.GetPath() );
    onFilenameChanged( false );
}



void DIALOG_IMPORT_NETLIST::onImportNetlist( wxCommandEvent& event )
{
    onFilenameChanged( true );
}


void DIALOG_IMPORT_NETLIST::onUpdatePCB( wxCommandEvent& event )
{
    wxFileName fn = m_NetlistFilenameCtrl->GetValue();

    if( !fn.IsOk() )
    {
        wxMessageBox( _( "Please choose a valid netlist file." ) );
        return;
    }

    if( !fn.FileExists() )
    {
        wxMessageBox( _( "The netlist file does not exist." ) );
        return;
    }

    m_MessageWindow->SetLabel( _( "Changes Applied to PCB" ) );
    loadNetlist( false );

    m_sdbSizerCancel->SetDefault();
    m_sdbSizerCancel->SetFocus();
}


void DIALOG_IMPORT_NETLIST::OnFilenameKillFocus( wxFocusEvent& event )
{
    event.Skip();
}


void DIALOG_IMPORT_NETLIST::onFilenameChanged( bool aLoadNetlist )
{
    if( m_initialized )
    {
        wxFileName fn = m_NetlistFilenameCtrl->GetValue();

        if( fn.IsOk() )
        {
            if( fn.FileExists() )
            {
                m_netlistPath = m_NetlistFilenameCtrl->GetValue();

                if( aLoadNetlist )
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


void DIALOG_IMPORT_NETLIST::OnMatchChanged( wxCommandEvent& event )
{
    if( m_initialized )
        loadNetlist( true );
}


void DIALOG_IMPORT_NETLIST::OnOptionChanged( wxCommandEvent& event )
{
    if( m_initialized )
        loadNetlist( true );
}


void DIALOG_IMPORT_NETLIST::loadNetlist( bool aDryRun )
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

    NETLIST netlist;

    netlist.SetFindByTimeStamp( m_matchByTimestamp->GetSelection() == 0 );
    netlist.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );

    if( !m_parent->ReadNetlistFromFile( netlistFileName, netlist, reporter ) )
        return;

    BOARD_NETLIST_UPDATER updater( m_parent, m_parent->GetBoard() );
    updater.SetReporter ( &reporter );
    updater.SetIsDryRun( aDryRun );
    updater.SetLookupByTimestamp( m_matchByTimestamp->GetSelection() == 0 );
    updater.SetDeleteUnusedFootprints( m_cbDeleteExtraFootprints->GetValue());
    updater.SetReplaceFootprints( m_cbUpdateFootprints->GetValue() );
    updater.SetTransferGroups( m_cbTransferGroups->GetValue() );
    updater.SetOverrideLocks( m_cbOverrideLocks->GetValue() );
    updater.SetUpdateFields( true );
    updater.UpdateNetlist( netlist );

    // The creation of the report was made without window update: the full page must be displayed
    m_MessageWindow->Flush( true );

    if( aDryRun )
        return;

    m_parent->OnNetlistChanged( updater, &m_runDragCommand );
}


