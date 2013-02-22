/**
 * @file pcbnew/dialogs/dialog_netlist.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <appl_wxstruct.h>
#include <confirm.h>
#include <dialog_helpers.h>
#include <html_messagebox.h>
#include <base_units.h>
#include <wxPcbStruct.h>
#include <pcbcommon.h>

#include <pcbnew_config.h>
#include <class_board_design_settings.h>
#include <class_board.h>
#include <class_module.h>
#include <wildcards_and_files_ext.h>

#include <dialog_netlist.h>


void PCB_EDIT_FRAME::InstallNetlistFrame( wxDC* DC )
{
    /* Setup the netlist file name to the last netlist file read,
     * or the board file name if the last filename is empty or last file not existing.
     */
    wxFileName fn = GetLastNetListRead();
    wxString lastNetlistName = GetLastNetListRead();

    if( !fn.FileExists() )
    {
        fn = GetBoard()->GetFileName();
        fn.SetExt( NetlistFileExtension );
        lastNetlistName = fn.GetFullPath();
    }

    DIALOG_NETLIST dlg( this, DC, lastNetlistName );

    dlg.ShowModal();

    // Save project settings if needed.
    // Project settings are saved in the corresponding <board name>.pro file
    bool configChanged = lastNetlistName != GetLastNetListRead();
    if( dlg.UseCmpFileForFpNames() != GetUseCmpFileForFpNames() )
    {
        SetUseCmpFileForFpNames( dlg.UseCmpFileForFpNames() );
        configChanged = true;
    }

    if( configChanged &&
        !GetBoard()->GetFileName().IsEmpty() &&
        IsOK(NULL, _("Project config has changed. Save it ?") ) )
    {
        wxFileName fn = GetBoard()->GetFileName();
        fn.SetExt( ProjectFileExtension );
        wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP,
                                       GetProjectFileParameters() );
    }
}


DIALOG_NETLIST::DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, wxDC * aDC,
                                const wxString & aNetlistFullFilename )
    : DIALOG_NETLIST_FBP( aParent )
{
    m_parent = aParent;
    m_dc = aDC;
    m_NetlistFilenameCtrl->SetValue( aNetlistFullFilename );
    m_cmpNameSourceOpt->SetSelection( m_parent->GetUseCmpFileForFpNames() ? 1 : 0 );

    GetSizer()->SetSizeHints( this );
}

void DIALOG_NETLIST::OnOpenNetlistClick( wxCommandEvent& event )
{
    wxString lastPath = wxFileName::GetCwd();
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

    wxLogDebug( wxT( "Last net list read path <%s>, file name <%s>." ),
                GetChars( lastPath ), GetChars( lastNetlistRead ) );

    wxFileDialog FilesDialog( this, _( "Select Netlist" ), lastPath, lastNetlistRead,
                              NetlistFileWildcard, wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    m_NetlistFilenameCtrl->SetValue( FilesDialog.GetPath() );
}


void DIALOG_NETLIST::OnReadNetlistFileClick( wxCommandEvent& event )
{
    wxString fullNetfileName = m_NetlistFilenameCtrl->GetValue();
    wxString cmpFilename;
    if( UseCmpFileForFpNames() )
    {
        wxFileName fn = m_NetlistFilenameCtrl->GetValue();
        fn.SetExt( ComponentFileExtension );
        cmpFilename = fn.GetFullPath();
    }

    m_parent->ReadPcbNetlist( fullNetfileName, cmpFilename, m_MessageWindow,
                              m_ChangeExistingFootprintCtrl->GetSelection() == 1 ? true : false,
                              m_DeleteBadTracks->GetSelection() == 1 ? true : false,
                              m_RemoveExtraFootprintsCtrl->GetSelection() == 1 ? true : false,
                              m_Select_By_Timestamp->GetSelection() == 1 ? true : false );
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    if( m_parent->GetBoard()->m_Modules == NULL )
    {
        DisplayInfoMessage( this, _( "No modules" ) );
        return;
    }

    // Lists of duplicates, missing references and not in netlist footprints:
    std::vector <MODULE*> duplicate;
    wxArrayString missing;
    std::vector <MODULE*> notInNetlist;
    wxString netlistFilename = m_NetlistFilenameCtrl->GetValue();

    if( ! m_parent->Test_Duplicate_Missing_And_Extra_Footprints(
                netlistFilename, duplicate, missing, notInNetlist ) )
    {
        wxMessageBox( _("Netlist file not found!") );
        return;
    }

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

            if( module->m_Reference->m_Text.IsEmpty() )
                list << wxT("<br>") << wxT("[noref)");
            else
                list << wxT("<br>") << module->m_Reference->m_Text;

            list << wxT("  (<i>") << module->m_Value->m_Text << wxT("</i>)");
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
        list << wxT("<p><b>") <<  _( "No missing modules." ) << wxT("</b></p>");
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
        list << wxT("<p><b>") << _( "No extra modules." ) << wxT("</b></p>");
    else
    {
        list << wxT("<p><b>") << _( "Not in Netlist:" ) << wxT("</b></p>");

        for( unsigned ii = 0; ii < notInNetlist.size(); ii++ )
        {
            MODULE* module = notInNetlist[ii];

            if( module->m_Reference->m_Text.IsEmpty() )
                list << wxT("<br>") << wxT("[noref)");
            else
                list << wxT("<br>") << module->m_Reference->m_Text ;
            list << wxT(" (<i>") << module->m_Value->m_Text << wxT("</i>)");
            list << wxT(" @ ");
            list << CoordinateToString( module->GetPosition().x ),
            list << wxT(", ") << CoordinateToString( module->GetPosition().y ),
            err_cnt++;

            if( ERR_CNT_MAX < err_cnt )
                break;
        }
    }

    if( ERR_CNT_MAX < err_cnt )
    {
        list << wxT("<p><b>")
             << _( "Too many errors: some are skipped" )
             << wxT("</b></p>");
    }

    HTML_MESSAGE_BOX dlg( this, _( "Check Modules" ) );
    dlg.AddHTML_Text(list);
    dlg.ShowModal();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_COMPILE_RATSNEST
 */

void DIALOG_NETLIST::OnCompileRatsnestClick( wxCommandEvent& event )
{
    m_parent->Compile_Ratsnest( m_dc, true );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_NETLIST::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}
