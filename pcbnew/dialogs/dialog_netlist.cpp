
/**
 * @file pcbnew/dialogs/dialog_netlist.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 KiCad Developers, see change_log.txt for contributors.
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
#include <dialog_helpers.h>
#include <html_messagebox.h>
#include <base_units.h>
#include <pcb_edit_frame.h>
#include <pcb_netlist.h>
#include <netlist_reader.h>
#include <reporter.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <board_design_settings.h>
#include <class_board.h>
#include <class_module.h>
#include <connectivity/connectivity_data.h>
#include <wildcards_and_files_ext.h>

#include <dialog_netlist.h>
#include <wx_html_report_panel.h>

#define NETLIST_FILTER_MESSAGES_KEY wxT("NetlistReportFilterMsg")
#define NETLIST_UPDATEFOOTPRINTS_KEY wxT("NetlistUpdateFootprints")
#define NETLIST_DELETESHORTINGTRACKS_KEY wxT("NetlistDeleteShortingTracks")
#define NETLIST_DELETEEXTRAFOOTPRINTS_KEY wxT("NetlistDeleteExtraFootprints")
#define NETLIST_DELETESINGLEPADNETS_KEY wxT("NetlistDeleteSinglePadNets")

void PCB_EDIT_FRAME::InstallNetlistFrame( wxDC* DC )
{
    wxString netlistName = GetLastNetListRead();

    DIALOG_NETLIST dlg( this, netlistName );

    dlg.ShowModal();

    // Save project settings if needed.
    // Project settings are saved in the corresponding <board name>.pro file
    bool configChanged = !GetLastNetListRead().IsEmpty() && ( netlistName != GetLastNetListRead() );

    if( configChanged && !GetBoard()->GetFileName().IsEmpty() )
    {
        wxFileName fn = Prj().AbsolutePath( GetBoard()->GetFileName() );
        fn.SetExt( ProjectFileExtension );
        wxString path = fn.GetFullPath();
        Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_PCB, GetProjectFileParameters(), path );
    }
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
        m_parent->GetToolManager()->InvokeTool( "pcbnew.InteractiveEdit" );
}


void DIALOG_NETLIST::OnOpenNetlistClick( wxCommandEvent& event )
{
    wxString dirPath = wxFileName( Prj().GetProjectFullName() ).GetPath();

    wxString filename = m_parent->GetLastNetListRead();

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
        wxMessageBox( _("Please, choose a valid netlist file.") );
        return;
    }

    if( !fn.FileExists() )
    {
        wxMessageBox( _("The netlist file does not exist.") );
        return;
    }

    // Give the user a chance to bail out when making changes from a netlist.
    if( m_parent->GetBoard()->IsEmpty()
        || IsOK( this, _( "The changes made cannot be undone.  Are you sure you want to update the PCB?" ) ) )
    {
        m_MessageWindow->SetLabel( _( "Changes Applied To PCB" ) );
        loadNetlist( false );

        m_sdbSizer1Cancel->SetDefault();
    }
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    if( m_parent->GetBoard()->m_Modules == nullptr )
    {
        DisplayInfoMessage( this, _( "No footprints." ) );
        return;
    }

    // Lists of duplicates, missing references and not in netlist footprints:
    std::vector <MODULE*> duplicate;
    wxArrayString missing;
    std::vector <MODULE*> notInNetlist;
    wxString netlistFilename = m_NetlistFilenameCtrl->GetValue();

    if( !verifyFootprints( netlistFilename, wxEmptyString, duplicate, missing, notInNetlist ) )
        return;

    #define ERR_CNT_MAX 100 // Max number of errors to output in dialog
                            // to avoid a too long message list

    wxString list;          // The messages to display

    m_parent->SetLastNetListRead( netlistFilename );

    int err_cnt = 0;

    // Search for duplicate footprints.
    if( duplicate.size() == 0 )
        list << wxT("<p><b>") << _( "No duplicate." ) << wxT("</b></p>");
    else
    {
        list << wxT("<p><b>") << _( "Duplicates:" ) << wxT("</b></p>");

        for( unsigned ii = 0; ii < duplicate.size(); ii++ )
        {
            MODULE* module = duplicate[ii];

            if( module->GetReference().IsEmpty() )
                list << wxT("<br>") << wxT("[noref)");
            else
                list << wxT("<br>") << module->GetReference();

            list << wxT("  (<i>") << module->GetValue() << wxT("</i>)");
            list << wxT(" @ ");
            list << MessageTextFromValue( m_units, module->GetPosition().x ),
            list << wxT(", ") << MessageTextFromValue( m_units, module->GetPosition().y ),
            err_cnt++;

            if( ERR_CNT_MAX < err_cnt )
                break;
        }
    }

    // Search for missing modules on board.
    if( missing.size() == 0 )
        list << wxT("<p><b>") <<  _( "No missing footprints." ) << wxT("</b></p>");
    else
    {
        list << wxT("<p><b>") << _( "Missing:" ) << wxT("</b></p>");

        for( unsigned ii = 0; ii < missing.size(); ii += 2 )
        {
            list << wxT("<br>") << missing[ii];
            list << wxT("  (<i>") << missing[ii+1] << wxT("</i>)");
            err_cnt++;

            if( ERR_CNT_MAX < err_cnt )
                break;
        }
    }


    // Search for modules found on board but not in net list.
    if( notInNetlist.size() == 0 )
        list << wxT( "<p><b>" ) << _( "No extra footprints." ) << wxT( "</b></p>" );
    else
    {
        list << wxT( "<p><b>" ) << _( "Not in Netlist:" ) << wxT( "</b></p>" );

        for( unsigned ii = 0; ii < notInNetlist.size(); ii++ )
        {
            MODULE* module = notInNetlist[ii];

            if( module->GetReference().IsEmpty() )
                list << wxT( "<br>" ) << wxT( "[noref)" );
            else
                list << wxT( "<br>" ) << module->GetReference() ;

            list << wxT( " (<i>" ) << module->GetValue() << wxT( "</i>)" );
            list << wxT( " @ " );
            list << MessageTextFromValue( m_units, module->GetPosition().x ),
            list << wxT( ", " ) << MessageTextFromValue( m_units, module->GetPosition().y ),
            err_cnt++;

            if( ERR_CNT_MAX < err_cnt )
                break;
        }
    }

    if( ERR_CNT_MAX < err_cnt )
    {
        list << wxT( "<p><b>" )
             << _( "Too many errors: some are skipped" )
             << wxT( "</b></p>" );
    }

    HTML_MESSAGE_BOX dlg( this, _( "Check footprints" ) );
    dlg.AddHTML_Text( list );
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


bool DIALOG_NETLIST::verifyFootprints( const wxString&         aNetlistFilename,
                                       const wxString &        aCmpFilename,
                                       std::vector< MODULE* >& aDuplicates,
                                       wxArrayString&          aMissing,
                                       std::vector< MODULE* >& aNotInNetlist )
{
    wxString        msg;
    MODULE*         module;
    MODULE*         nextModule;
    NETLIST         netlist;
    wxBusyCursor    dummy;           // Shows an hourglass while calculating.
    NETLIST_READER* netlistReader;
    COMPONENT*      component;

    try
    {
        netlistReader = NETLIST_READER::GetNetlistReader( &netlist, aNetlistFilename,
                                                          aCmpFilename );

        if( netlistReader == NULL )
        {
            msg.Printf( _( "Cannot open netlist file \"%s\"." ), GetChars( aNetlistFilename ) );
            wxMessageBox( msg, _( "Netlist Load Error." ), wxOK | wxICON_ERROR );
            return false;
        }

        std::unique_ptr< NETLIST_READER > nlr( netlistReader );
        netlistReader->LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error loading netlist file:\n%s" ), ioe.What().GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    BOARD* pcb = m_parent->GetBoard();

    // Search for duplicate footprints.
    module = pcb->m_Modules;

    for( ; module != NULL; module = module->Next() )
    {
        nextModule = module->Next();

        for( ; nextModule != NULL; nextModule = nextModule->Next() )
        {
            if( module->GetReference().CmpNoCase( nextModule->GetReference() ) == 0 )
            {
                aDuplicates.push_back( module );
                break;
            }
        }
    }

    // Search for component footprints in the netlist but not on the board.
    for( unsigned ii = 0; ii < netlist.GetCount(); ii++ )
    {
        component = netlist.GetComponent( ii );

        module = pcb->FindModuleByReference( component->GetReference() );

        if( module == NULL )
        {
            aMissing.Add( component->GetReference() );
            aMissing.Add( component->GetValue() );
        }
    }

    // Search for component footprints found on board but not in netlist.
    module = pcb->m_Modules;

    for( ; module != NULL; module = module->Next() )
    {

        component = netlist.GetComponentByReference( module->GetReference() );

        if( component == NULL )
            aNotInNetlist.push_back( module );
    }

    return true;
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
        msg = _( "Using time stamps to match components and footprints.\n" );
    else
        msg = _( "Using references to match components and footprints.\n" );

    reporter.ReportHead( msg, REPORTER::RPT_INFO );
    m_MessageWindow->SetLazyUpdate( true ); // Use lazy update to speed the creation of the report
                                            // (the window is not updated for each message)

    m_parent->ReadPcbNetlist( netlistFileName, wxEmptyString, reporter,
                              m_cbUpdateFootprints->GetValue(),
                              m_cbDeleteShortingTracks->GetValue(),
                              m_cbDeleteExtraFootprints->GetValue(),
                              m_matchByTimestamp->GetSelection() == 0,
                              m_cbDeleteSinglePadNets->GetValue(),
                              aDryRun, &m_runDragCommand );

    // The creation of the report was made without window update: the full page must be displayed
    m_MessageWindow->Flush( true );
}


