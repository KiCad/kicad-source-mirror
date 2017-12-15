/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file prjconfig.cpp
 * Load and save project configuration files (*.pro)
 */


#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>

#include <build_version.h>
#include <config_params.h>
#include <confirm.h>
#include <kiway.h>
#include <project.h>
#include <wildcards_and_files_ext.h>

#include "dialogs/dialog_template_selector.h"

#include "kicad.h"
#include "pgm_kicad.h"
#include "tree_project_frame.h"


#define SEP()   wxFileName::GetPathSeparator()

// Not really useful, provided to save/restore params in project config file,
// (Add them in s_KicadManagerParams if any)
// Used also to create new .pro files from the kicad.pro template file
// for new projects
#define     GeneralGroupName            wxT( "/general" )

PARAM_CFG_ARRAY     s_KicadManagerParams;


void KICAD_MANAGER_FRAME::LoadProject( const wxFileName& aProjectFileName )
{
    // The project file should be valid by the time we get here or something has gone wrong.
    if( !aProjectFileName.Exists() )
        return;

    // Any open KIFACE's must be closed if they are not part of the new project.
    // (We never want a KIWAY_PLAYER open on a KIWAY that isn't in the same project.)
    // User is prompted here to close those KIWAY_PLAYERs:
    if( !Kiway.PlayersClose( false ) )
        return;

    SetTitle( wxString( "KiCad " ) + GetBuildVersion() );

    // Save the project file for the currently loaded project.
    if( m_active_project )
        Prj().ConfigLoad( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    m_active_project = true;
    ClearMsg();
    SetProjectFileName( aProjectFileName.GetFullPath() );
    Prj().ConfigLoad( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    wxString title = GetTitle() + " " + aProjectFileName.GetFullPath();

    if( !aProjectFileName.IsDirWritable() )
        title += _( " [Read Only]" );
    else
        SetMruPath( Prj().GetProjectPath() ); // Only set MRU path if we have write access. Why?

    SetTitle( title );

    UpdateFileHistory( aProjectFileName.GetFullPath(), &PgmTop().GetFileHistory() );

    m_LeftWin->ReCreateTreePrj();

    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );

    wxPostEvent( this, cmd );

    PrintPrjInfo();
}


void KICAD_MANAGER_FRAME::CreateNewProject( const wxFileName& aProjectFileName )
{
    wxCHECK_RET( aProjectFileName.DirExists() && aProjectFileName.IsDirWritable(),
                 "Project folder must exist and be writable to create a new project." );

    // Init project filename.  This clears all elements from the project object.
    SetProjectFileName( aProjectFileName.GetFullPath() );

    // Copy kicad.pro file from template folder.
    if( !aProjectFileName.FileExists() )
    {
        wxString srcFileName = sys_search().FindValidPath( "kicad.pro" );

        // Create a minimal project (.pro) file if the template project file could not be copied.
        if( !wxFileName::FileExists( srcFileName )
          || !wxCopyFile( srcFileName, aProjectFileName.GetFullPath() ) )
        {
            Prj().ConfigSave( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );
        }
    }

    // Ensure a "stub" for a schematic root sheet and a board exist.
    // It will avoid messages from the schematic editor or the board editor to create a new file
    // And forces the user to create main files under the right name for the project manager
    wxFileName fn( aProjectFileName.GetFullPath() );
    fn.SetExt( SchematicFileExtension );

    // If a <project>.sch file does not exist, create a "stub" file ( minimal schematic file )
    if( !fn.FileExists() )
    {
        wxFile file( fn.GetFullPath(), wxFile::write );

        if( file.IsOpened() )
            file.Write( wxT( "EESchema Schematic File Version 2\n"
                             "EELAYER 25 0\nEELAYER END\n$EndSCHEMATC\n" ) );

        // wxFile dtor will close the file
    }

    // If a <project>.kicad_pcb or <project>.brd file does not exist,
    // create a .kicad_pcb "stub" file
    fn.SetExt( KiCadPcbFileExtension );
    wxFileName leg_fn( fn );
    leg_fn.SetExt( LegacyPcbFileExtension );

    if( !fn.FileExists() && !leg_fn.FileExists() )
    {
        wxFile file( fn.GetFullPath(), wxFile::write );

        if( file.IsOpened() )
            file.Write( wxT( "(kicad_pcb (version 4) (host kicad \"dummy file\") )\n" ) );

        // wxFile dtor will close the file
    }
}


void KICAD_MANAGER_FRAME::OnLoadProject( wxCommandEvent& event )
{
    wxString     default_dir = GetMruPath();
    wxFileDialog dlg( this, _( "Open Existing Project" ), default_dir, wxEmptyString,
                      ProjectFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName pro( dlg.GetPath() );
    pro.SetExt( ProjectFileExtension );     // enforce extension

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    if( !pro.FileExists() )
        return;

    LoadProject( pro );
}


void KICAD_MANAGER_FRAME::OnNewProject( wxCommandEvent& aEvent )
{
    wxString        default_dir = GetMruPath();
    wxFileDialog    dlg( this, _( "Create New Project" ), default_dir, wxEmptyString,
                         ProjectFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName pro( dlg.GetPath() );
    pro.SetExt( ProjectFileExtension );     // enforce extension

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    // Append a new directory with the same name of the project file.
    pro.AppendDir( pro.GetName() );

    // Check if the project directory is empty if it already exists.
    wxDir directory( pro.GetPath() );

    if( !pro.DirExists() )
    {
        if( !pro.Mkdir() )
        {
            wxString msg;
            msg.Printf( _( "Directory \"%s\" could not be created.\n\n"
                           "Please make sure you have write permissions and try again." ),
                        pro.GetPath() );
            DisplayErrorMessage( this, msg );
            return;
        }
    }
    else if( directory.HasFiles() )
    {
        wxString msg = _( "The selected directory is not empty.  It is recommended that you "
                          "create projects in their own empty directory.\n\nDo you "
                          "want to continue?" );

        if( !IsOK( this, msg ) )
            return;
    }

    CreateNewProject( pro );
    LoadProject( pro );
}


void KICAD_MANAGER_FRAME::OnCreateProjectFromTemplate( wxCommandEvent& event )
{
    wxString    default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxString    title = _( "New Project Folder" );
    wxDirDialog dlg( this, title, default_dir );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    // Builds the project .pro filename, from the new project folder name
    wxFileName fn;
    fn.AssignDir( dlg.GetPath() );
    fn.SetName( dlg.GetPath().AfterLast( SEP() ) );
    fn.SetExt( wxT( "pro" ) );

    wxChar      sep[2] = { SEP(), 0 };  // nul terminated separator wxChar string.

    ClearMsg();

    DIALOG_TEMPLATE_SELECTOR* ps = new DIALOG_TEMPLATE_SELECTOR( this );

    wxFileName  templatePath;
    wxString    envStr;

#ifndef __WXMAC__
    wxGetEnv( wxT( "KICAD" ), &envStr );

    // Add a new tab for system templates
    if( !envStr.empty() )
    {
        // user may or may not have including terminating separator.
        if( !envStr.EndsWith( sep ) )
            envStr += sep;

        templatePath = envStr + wxT( "template" ) + sep;
    }
    else
    {
        // The standard path should be in the share directory for kicad. As
        // it is normal on Windows to only have the share directory and not
        // the kicad sub-directory we fall back to that if the directory
        // doesn't exist
        templatePath = wxPathOnly( wxStandardPaths::Get().GetExecutablePath() ) +
                       sep + wxT( ".." ) + sep + wxT( "share" ) + sep + wxT( "kicad" ) +
                       sep + wxT( "template" ) + sep;

        if( !wxDirExists( templatePath.GetFullPath() ) )
        {
            templatePath = wxPathOnly( wxStandardPaths::Get().GetExecutablePath() ) + sep +
                           wxT( ".." ) + sep + wxT( "share" ) + sep + wxT( "template" ) + sep;
        }
    }
#else
        // Use what is provided in the bundle data dir
    templatePath = GetOSXKicadDataDir() + sep + wxT( "template" );
#endif

    ps->AddTemplatesPage( _( "System Templates" ), templatePath );

    // Add a new tab for user templates
    wxFileName userPath = wxStandardPaths::Get().GetDocumentsDir() + sep + wxT( "kicad" ) +
                          sep + wxT( "template" ) + sep;

    ps->AddTemplatesPage( _( "User Templates" ), userPath );

    // Check to see if a custom template location is available and setup a
    // new selection tab if there is.
    envStr.clear();
    wxGetEnv( wxT( "KICAD_PTEMPLATES" ), &envStr );

    if( !envStr.empty() )
    {
        if( !envStr.EndsWith( sep ) )
            envStr += sep;

        wxFileName envPath = envStr;

        ps->AddTemplatesPage( _( "Portable Templates" ), envPath );
    }

    // Show the project template selector dialog
    if( ps->ShowModal() != wxID_OK )
        return;

    if( ps->GetSelectedTemplate() == NULL )
    {
        wxMessageBox( _( "No project template was selected.  Cannot generate new project." ),
                      _( "Error" ),
                      wxOK | wxICON_ERROR,
                      this );

        return;
    }

    // Make sure the user has write permissions to the base path.
    wxFileName prjPath = fn;

    while( !prjPath.DirExists() )
        prjPath.RemoveLastDir();

    if( !prjPath.IsDirWritable() )
    {
        wxString msg;

        msg.Printf( _( "Cannot write to folder \"%s\"." ), prjPath.GetPath() );
        wxMessageDialog msgDlg( this, msg, _( "Error!" ), wxICON_ERROR | wxOK | wxCENTER );
        msgDlg.SetExtendedMessage( _( "Plese check your access permissions to this folder "
                                      "and try again." ) );
        msgDlg.ShowModal();
        return;
    }

    // Make sure we are not overwriting anything in the destination folder.
    std::vector< wxFileName > destFiles;

    if( ps->GetSelectedTemplate()->GetDestinationFiles( fn, destFiles ) )
    {
        std::vector< wxFileName > overwrittenFiles;

        for( auto file : destFiles )
        {
            if( file.FileExists() )
                overwrittenFiles.push_back( file );
        }

        if( !overwrittenFiles.empty() )
        {
            wxString extendedMsg = _( "Overwriting files:" ) + "\n";

            for( auto file : overwrittenFiles )
            {
                extendedMsg += "\n" + file.GetFullName();
            }

            wxMessageDialog owDlg( this,
                                   _( "Are you sure you want to overwrite files in "
                                      "the destination folder?" ),
                                   _( "Warning!" ),
                                   wxYES_NO | wxICON_QUESTION | wxCENTER );
            owDlg.SetExtendedMessage( extendedMsg );
            owDlg.SetYesNoLabels( _( "Overwrite" ), _( "Do Not Overwrite" ) );

            if( owDlg.ShowModal() == wxID_NO )
                return;
        }
    }

    wxString errorMsg;

    // The selected template widget contains the template we're attempting to use to
    // create a project
    if( !ps->GetSelectedTemplate()->CreateProject( fn, &errorMsg ) )
    {
        wxMessageDialog createDlg( this,
                                   _( "A problem occurred creating new project from template!" ),
                                   _( "Template Error" ),
                                   wxOK | wxICON_ERROR );

        if( !errorMsg.empty() )
            createDlg.SetExtendedMessage( errorMsg );

        createDlg.ShowModal();
        return;
    }

    CreateNewProject( fn.GetFullPath() );
    LoadProject( fn );
}


void KICAD_MANAGER_FRAME::OnSaveProject( wxCommandEvent& event )
{
    if( !wxIsWritable( GetProjectFileName() ) )
        return;

    Prj().ConfigSave( PgmTop().SysSearch(), GeneralGroupName, s_KicadManagerParams );
}


void KICAD_MANAGER_FRAME::OnUpdateRequiresProject( wxUpdateUIEvent& event )
{
    event.Enable( m_active_project );
}
