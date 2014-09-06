/**
 * @file prjconfig.cpp
 * Load and save project configuration files (*.pro)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <pgm_kicad.h>
#include <kiway.h>
#include <project.h>
#include <confirm.h>
#include <gestfich.h>
#include <kicad.h>
#include <config_params.h>
#include <project_template.h>
#include <tree_project_frame.h>
#include <wildcards_and_files_ext.h>
#include <vector>
#include <build_version.h>
#include <macros.h>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "dialogs/dialog_template_selector.h"

#define SEP()   wxFileName::GetPathSeparator()

// Not really useful, provided to save/restore params in project config file,
// (Add them in s_KicadManagerParams if any)
// Used also to create new .pro files from the kicad.pro template file
// for new projects
#define     GeneralGroupName            wxT( "/general" )

PARAM_CFG_ARRAY     s_KicadManagerParams;

void KICAD_MANAGER_FRAME::CreateNewProject( const wxString& aPrjFullFileName,
                                            bool aTemplateSelector = false )
{
    wxFileName  newProjectName = aPrjFullFileName;
    wxChar      sep[2] = { SEP(), 0 };  // nul terminated separator wxChar string.

    ClearMsg();

    // If we are creating a project from a template, make sure the template directory is sane
    if( aTemplateSelector )
    {
        DIALOG_TEMPLATE_SELECTOR* ps = new DIALOG_TEMPLATE_SELECTOR( this );

        wxFileName  templatePath;
        wxString    envStr;

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
                templatePath = wxPathOnly( wxStandardPaths::Get().GetExecutablePath() ) +
                sep + wxT( ".." ) + sep + wxT( "share" ) + sep + wxT( "template" ) + sep;
            }
        }

        ps->AddPage( _( "System Templates" ), templatePath );

        // Add a new tab for user templates
        wxFileName userPath = wxStandardPaths::Get().GetDocumentsDir() +
                sep + wxT( "kicad" ) + sep + wxT( "template" ) + sep;

        ps->AddPage( _( "User Templates" ), userPath );

        // Check to see if a custom template location is available and setup a
        // new selection tab if there is.
        envStr.clear();
        wxGetEnv( wxT( "KICAD_PTEMPLATES" ), &envStr );

        if( !envStr.empty() )
        {
            if( !envStr.EndsWith( sep ) )
                envStr += sep;

            wxFileName envPath = envStr;

            ps->AddPage( _( "Portable Templates" ), envPath );
        }

        // Show the project template selector dialog
        int result = ps->ShowModal();

        if( (result != wxID_OK) || (ps->GetWidget() == NULL) )
        {
            if( ps->GetWidget() == NULL )
            {
                wxMessageBox( _( "No project template was selected.  Cannot generate new "
                                 "project." ),
                              _( "Error" ),
                              wxOK | wxICON_ERROR,
                              this );
            }
        }
        else
        {
            // The selected template widget contains the template we're attempting to use to
            // create a project
            if( !ps->GetWidget()->GetTemplate()->CreateProject( newProjectName ) )
            {
                wxMessageBox( _( "Problem whilst creating new project from template!" ),
                              _( "Template Error" ),
                              wxOK | wxICON_ERROR,
                              this );
            }
        }
    }

    // Init project filename
    SetProjectFileName( newProjectName.GetFullPath() );

    // Write settings to project file
    // was: wxGetApp().WriteProjectConfig( aPrjFullFileName, GeneralGroupName, s_KicadManagerParams );
    Prj().ConfigSave( Pgm().SysSearch(), GeneralGroupName, s_KicadManagerParams );
}


void KICAD_MANAGER_FRAME::OnLoadProject( wxCommandEvent& event )
{
    // Any open KIFACE's must be closed if they are not part of the new project.
    // (We never want a KIWAY_PLAYER open on a KIWAY that isn't in the same project.)
    // User is prompted here to close those KIWAY_PLAYERs:
    if( !Kiway.PlayersClose( false ) )
        return;

    // evt_id can be one of:
    //   ID_NEW_PROJECT, ID_NEW_PROJECT_FROM_TEMPLATE, ID_LOAD_PROJECT, and
    //   wxID_ANY from 3 different places.
    int evt_id = event.GetId();

    wxString    title;

    ClearMsg();

    if( evt_id != wxID_ANY )
    {
        int  style;
        bool newProject = ( evt_id == ID_NEW_PROJECT ) ||
                          ( evt_id == ID_NEW_PROJECT_FROM_TEMPLATE );

        if( newProject )
        {
            title = _( "Create New Project" );
            style = wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
        }
        else
        {
            title = _( "Open Existing Project" );
            style = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
        }

        wxString        default_dir = wxGetCwd();
        wxFileDialog    dlg( this, title, default_dir, wxEmptyString,
                              ProjectFileWildcard, style );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        //DBG( printf( "%s: wxFileDialog::GetPath=%s\n", __func__, TO_UTF8( dlg.GetPath() ) );)

        wxFileName pro( dlg.GetPath() );
        pro.SetExt( ProjectFileExtension );     // enforce extension

        if( !pro.IsAbsolute() )
            pro.MakeAbsolute();

        if( newProject )
        {
            // Check if the project directory is empty
            wxDir directory( pro.GetPath() );

            if( directory.HasFiles() )
            {
                wxString msg = _( "The selected directory is not empty.  We recommend you "
                                  "create projects in their own clean directory.\n\nDo you "
                                  "want to create a new empty directory for the project?" );

                if( IsOK( this, msg ) )
                {
                    // Append a new directory with the same name of the project file
                    // and try to create it
                    pro.AppendDir( pro.GetName() );

                    if( !wxMkdir( pro.GetPath() ) )
                        // There was a problem, undo
                        pro.RemoveLastDir();
                }
            }

            if( evt_id == ID_NEW_PROJECT )
            {
                CreateNewProject( pro.GetFullPath() );
            }
            else if( evt_id == ID_NEW_PROJECT_FROM_TEMPLATE )
            {
                // Launch the template selector dialog
                CreateNewProject( pro.GetFullPath(), true );
            }
        }

        SetProjectFileName( pro.GetFullPath() );
    }

    wxString prj_filename = GetProjectFileName();

    wxString nameless_prj = NAMELESS_PROJECT  wxT( ".pro" );

    // Check if project file exists and if it is not noname.pro
    if( !wxFileExists( prj_filename ) && !prj_filename.IsSameAs( nameless_prj ) )
    {
        wxString msg = wxString::Format( _(
                "KiCad project file '%s' not found" ),
                GetChars( prj_filename ) );

        DisplayError( this, msg );
        return;
    }

    Prj().ConfigLoad( Pgm().SysSearch(), GeneralGroupName, s_KicadManagerParams );

    title = wxT( "KiCad " ) + GetBuildVersion() +  wxT( ' ' ) + prj_filename;

    if( !wxFileName( prj_filename ).IsDirWritable() )
        title += _( " [Read Only]" );

    SetTitle( title );

    if( !prj_filename.IsSameAs( nameless_prj ) )
        UpdateFileHistory( prj_filename, &Pgm().GetFileHistory() );

    m_LeftWin->ReCreateTreePrj();

#ifdef KICAD_USE_FILES_WATCHER
    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );

    wxPostEvent( this, cmd );
#endif

    PrintPrjInfo();
}

/* Creates a new project folder, copy a template into this new folder.
 * and open this new projrct as working project
 */
void KICAD_MANAGER_FRAME::OnCreateProjectFromTemplate( wxCommandEvent& event )
{
    wxString    default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxString    title = _("New Project Folder");
    wxDirDialog dlg( this, title, default_dir );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    // Buils the project .pro filename, from the new project folder name
    wxFileName fn;
    fn.AssignDir( dlg.GetPath() );
    fn.SetName( dlg.GetPath().AfterLast( SEP() ) );
    fn.SetExt( wxT( "pro" ) );

    // Launch the template selector dialog, and copy files
    CreateNewProject( fn.GetFullPath(), true );

    // Initialize the project
    event.SetId( wxID_ANY );
    OnLoadProject( event );
}


void KICAD_MANAGER_FRAME::OnSaveProject( wxCommandEvent& event )
{
    if( !wxIsWritable( GetProjectFileName() ) )
        return;

    // was: wxGetApp().WriteProjectConfig( m_ProjectFileName.GetFullPath(),
    //          GeneralGroupName, s_KicadManagerParams );
    Prj().ConfigSave( Pgm().SysSearch(), GeneralGroupName, s_KicadManagerParams );
}
