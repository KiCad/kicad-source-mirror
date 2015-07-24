
/**
 * @file pcbnew/dialogs/dialog_netlist.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
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
#include <wxPcbStruct.h>
#include <pcb_netlist.h>
#include <netlist_reader.h>
#include <reporter.h>

#include <pcbnew_config.h>
#include <class_board_design_settings.h>
#include <class_board.h>
#include <class_module.h>
#include <ratsnest_data.h>
#include <wildcards_and_files_ext.h>

#include <dialog_netlist.h>
#include <wx_html_report_panel.h>

#define NETLIST_SILENTMODE_KEY wxT("SilentMode")
#define NETLIST_FILTER_MESSAGES_KEY wxT("NetlistReportFilterMsg")
#define NETLIST_DELETESINGLEPADNETS_KEY wxT("NetlistDeleteSinglePadNets")

void PCB_EDIT_FRAME::InstallNetlistFrame( wxDC* DC )
{
    /* Setup the netlist file name to the last netlist file read,
     * or the board file name if the last filename is empty or last file not existing.
     */
    wxString netlistName = GetLastNetListRead();

    wxFileName fn = netlistName;

    if( !fn.IsOk() || !fn.FileExists() )
    {
        fn = GetBoard()->GetFileName();
        fn.SetExt( NetlistFileExtension );
        netlistName = fn.GetFullPath();
    }

    DIALOG_NETLIST dlg( this, DC, netlistName );

    dlg.ShowModal();

    // Save project settings if needed.
    // Project settings are saved in the corresponding <board name>.pro file
    bool configChanged = !GetLastNetListRead().IsEmpty() && ( netlistName != GetLastNetListRead() );

    if( configChanged && !GetBoard()->GetFileName().IsEmpty()
      && IsOK( NULL, _( "The project configuration has changed.  Do you want to save it?" ) ) )
    {
        wxFileName fn = Prj().AbsolutePath( GetBoard()->GetFileName() );
        fn.SetExt( ProjectFileExtension );

        wxString pro_name = fn.GetFullPath();

        Prj().ConfigSave( Kiface().KifaceSearch(), GROUP_PCB,
                GetProjectFileParameters(), pro_name );
    }
}


DIALOG_NETLIST::DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, wxDC * aDC,
                                const wxString & aNetlistFullFilename )
    : DIALOG_NETLIST_FBP( aParent )
{
    m_parent = aParent;
    m_dc = aDC;
    m_config = Kiface().KifaceSettings();

    m_silentMode = m_config->Read( NETLIST_SILENTMODE_KEY, 0l );
    bool tmp = m_config->Read( NETLIST_DELETESINGLEPADNETS_KEY, 0l );
    m_rbSingleNets->SetSelection( tmp == 0 ? 0 : 1);
    m_NetlistFilenameCtrl->SetValue( aNetlistFullFilename );
    m_checkBoxSilentMode->SetValue( m_silentMode );

    int severities = m_config->Read( NETLIST_FILTER_MESSAGES_KEY, -1l );
    m_MessageWindow->SetVisibleSeverities( severities );

    GetSizer()->SetSizeHints( this );
}

DIALOG_NETLIST::~DIALOG_NETLIST()
{
    m_config->Write( NETLIST_SILENTMODE_KEY, (long) m_silentMode );
    m_config->Write( NETLIST_DELETESINGLEPADNETS_KEY,
                    (long) m_rbSingleNets->GetSelection() );
    m_config->Write( NETLIST_FILTER_MESSAGES_KEY,
                    (long) m_MessageWindow->GetVisibleSeverities() );
}


void DIALOG_NETLIST::OnOpenNetlistClick( wxCommandEvent& event )
{
    wxString lastPath = wxFileName( Prj().GetProjectFullName() ).GetPath();

    wxString lastNetlistRead = m_parent->GetLastNetListRead();

    if( !lastNetlistRead.IsEmpty() && !wxFileName::FileExists( lastNetlistRead ) )
    {
        lastNetlistRead = wxEmptyString;
    }
    else
    {
        wxFileName fn = lastNetlistRead;
        lastPath = fn.GetPath();
        lastNetlistRead = fn.GetFullName();
    }

    wxLogDebug( wxT( "Last net list read path '%s', file name '%s'." ),
                GetChars( lastPath ), GetChars( lastNetlistRead ) );

    wxFileDialog FilesDialog( this, _( "Select Netlist" ), lastPath, lastNetlistRead,
                              NetlistFileWildcard, wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    m_NetlistFilenameCtrl->SetValue( FilesDialog.GetPath() );
}

void DIALOG_NETLIST::OnReadNetlistFileClick( wxCommandEvent& event )
{
    wxString msg;
    wxString netlistFileName = m_NetlistFilenameCtrl->GetValue();

    // Give the user a chance to bail out when making changes from a netlist.
    if( !m_checkDryRun->GetValue() && !m_silentMode
      && !m_parent->GetBoard()->IsEmpty()
      && !IsOK( NULL, _( "The changes made by reading the netlist cannot be undone.  Are you "
                         "sure you want to read the netlist?" ) ) )
        return;

    m_MessageWindow->Clear();
    REPORTER& reporter = m_MessageWindow->Reporter();

    wxBusyCursor busy;

    msg.Printf( _( "Reading netlist file \"%s\".\n" ), GetChars( netlistFileName ) );
    reporter.Report( msg, REPORTER::RPT_INFO );

    if( m_Select_By_Timestamp->GetSelection() == 1 )
        msg = _( "Using time stamps to match components and footprints.\n" );
    else
        msg = _( "Using references to match components and footprints.\n" );

    reporter.Report( msg, REPORTER::RPT_INFO );
    m_MessageWindow->SetLazyUpdate( true ); // use a "lazy" update to speed up the creation of the report
                                            // (The window is not updated for each message)

    m_parent->ReadPcbNetlist( netlistFileName, wxEmptyString, &reporter,
                              m_ChangeExistingFootprintCtrl->GetSelection() == 1,
                              m_DeleteBadTracks->GetSelection() == 1,
                              m_RemoveExtraFootprintsCtrl->GetSelection() == 1,
                              m_Select_By_Timestamp->GetSelection() == 1,
                              m_rbSingleNets->GetSelection() == 1,
                              m_checkDryRun->GetValue() );
    // The creation of the report was made without window update:
    // the full page must be displayed
    m_MessageWindow->Flush();
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    if( m_parent->GetBoard()->m_Modules == NULL )
    {
        DisplayInfoMessage( this, _( "No footprints" ) );
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
            list << CoordinateToString( module->GetPosition().x ),
            list << wxT(", ") << CoordinateToString( module->GetPosition().y ),
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
            list << CoordinateToString( module->GetPosition().x ),
            list << wxT( ", " ) << CoordinateToString( module->GetPosition().y ),
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


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_COMPILE_RATSNEST
 */

void DIALOG_NETLIST::OnCompileRatsnestClick( wxCommandEvent& event )
{
    // Rebuild the board connectivity:
    if( m_parent->IsGalCanvasActive() )
        m_parent->GetBoard()->GetRatsnest()->ProcessBoard();

    m_parent->Compile_Ratsnest( m_dc, true );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_NETLIST::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_NETLIST::OnSaveMessagesToFile( wxCommandEvent& aEvent )
{
    wxFileName fn;

    if( !m_parent->GetLastNetListRead().IsEmpty() )
    {
        fn = m_parent->GetLastNetListRead();
        fn.SetExt( wxT( "txt" ) );
    }
    else
    {
        fn = wxPathOnly( Prj().GetProjectFullName() );
    }

    wxFileDialog dlg( this, _( "Save contents of message window" ), fn.GetPath(), fn.GetName(),
                      TextWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( wxT( "txt" ) );

    wxFile f( fn.GetFullPath(), wxFile::write );

    if( !f.IsOpened() )
    {
        wxString msg;

        msg.Printf( _( "Cannot write message contents to file \"%s\"." ),
                    GetChars( fn.GetFullPath() ) );
        wxMessageBox( msg, _( "File Write Error" ), wxOK | wxICON_ERROR, this );
        return;
    }
}


void DIALOG_NETLIST::OnUpdateUISaveMessagesToFile( wxUpdateUIEvent& aEvent )
{
    //aEvent.Enable( !m_MessageWindow->IsEmpty() );
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

        std::auto_ptr< NETLIST_READER > nlr( netlistReader );
        netlistReader->LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error loading netlist file:\n%s" ), ioe.errorText.GetData() );
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
