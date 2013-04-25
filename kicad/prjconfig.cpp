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
#include <appl_wxstruct.h>
#include <confirm.h>
#include <gestfich.h>
#include <kicad.h>
#include <param_config.h>
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
static const wxString GeneralGroupName( wxT( "/general" ) );
PARAM_CFG_ARRAY s_KicadManagerParams;


void KICAD_MANAGER_FRAME::CreateNewProject( const wxString aPrjFullFileName,
                                            bool           aTemplateSelector = false )
{
    wxString    filename;
    wxFileName  newProjectName = aPrjFullFileName;
    wxChar      sep[2] = { SEP(), 0 };  // nul terminated separator wxChar string.

    ClearMsg();

    // Init default config filename
    filename = wxGetApp().FindLibraryPath( wxT( "kicad" ) + g_KicadPrjFilenameExtension );

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

            templatePath = envStr + wxT("template") + sep;
        }
        else
        {
            templatePath = wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) +
                sep + wxT( ".." ) + sep + wxT( "share" ) + sep + wxT( "template" ) + sep;
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
    else
    {
        // Check if file kicad.pro exist in template directory
        if( wxFileName::FileExists( filename ) )
        {
            wxCopyFile( filename, aPrjFullFileName );
        }
        else
        {
            DisplayInfoMessage( NULL, _( "Project template file <kicad.pro> not found. " ) );
            return;
        }
    }

    // Init schematic filename
    m_SchematicRootFileName = wxFileName( newProjectName.GetName(),
                                          SchematicFileExtension ).GetFullName();

    // Init pcb board filename
    m_BoardFileName = wxFileName( newProjectName.GetName(), PcbFileExtension ).GetFullName();

    // Init project filename
    m_ProjectFileName = newProjectName;

    // Write settings to project file
    wxGetApp().WriteProjectConfig( aPrjFullFileName,
                                   GeneralGroupName, s_KicadManagerParams );
}


void KICAD_MANAGER_FRAME::OnLoadProject( wxCommandEvent& event )
{
    int style;
    wxString title;
    bool newProject = ( event.GetId() == ID_NEW_PROJECT ) ||
                      ( event.GetId() == ID_NEW_PROJECT_FROM_TEMPLATE );

    ClearMsg();

    if( event.GetId() != wxID_ANY )
    {
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

        wxString default_dir = wxGetCwd();
        wxFileDialog dlg( this, title, default_dir, wxEmptyString,
                          ProjectFileWildcard, style );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        m_ProjectFileName = dlg.GetPath();

        if( newProject )
        {
            m_ProjectFileName.SetExt( ProjectFileExtension );

            // Check if the project directory is empty
            wxDir directory ( m_ProjectFileName.GetPath() );

            if( directory.HasFiles() )
            {
                wxString msg = _( "The selected directory is not empty.  We recommend you "
                                  "create projects in their own clean directory.\n\nDo you "
                                  "want to create a new empty directory for the project?" );

                if( IsOK( this, msg ) )
                {
                    // Append a new directory with the same name of the project file
                    // and try to create it
                    m_ProjectFileName.AppendDir( m_ProjectFileName.GetName() );

                    if( !wxMkdir( m_ProjectFileName.GetPath() ) )
                        // There was a problem, undo
                        m_ProjectFileName.RemoveLastDir();
                }
            }

            if( event.GetId() == ID_NEW_PROJECT )
            {
                CreateNewProject( m_ProjectFileName.GetFullPath() );
            }
            else if( event.GetId() == ID_NEW_PROJECT_FROM_TEMPLATE )
            {
                // Launch the template selector dialog
                CreateNewProject( m_ProjectFileName.GetFullPath(), true );
            }
        }
    }

    wxLogDebug( wxT( "Loading KiCad project file: " ) + m_ProjectFileName.GetFullPath() );

    /* Check if project file exists and if it is not noname.pro */
    wxString filename = m_ProjectFileName.GetFullName();

    wxString nameless_prj = NAMELESS_PROJECT;
    nameless_prj += g_KicadPrjFilenameExtension;

    if( !m_ProjectFileName.FileExists() && !filename.IsSameAs( nameless_prj ) )
    {
        wxString msg;
        msg.Printf( _( "KiCad project file <%s> not found" ),
                    GetChars( m_ProjectFileName.GetFullPath() ) );

        DisplayError( this, msg );
        return;
    }

    wxSetWorkingDirectory( m_ProjectFileName.GetPath() );
    wxGetApp().ReadProjectConfig( m_ProjectFileName.GetFullPath(),
                                  GeneralGroupName, s_KicadManagerParams, false );

    title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
        wxT( " " ) +  m_ProjectFileName.GetFullPath();

    if( !m_ProjectFileName.IsDirWritable() )
        title += _( " [Read Only]" );

    SetTitle( title );
    UpdateFileHistory( m_ProjectFileName.GetFullPath() );
    m_LeftWin->ReCreateTreePrj();

#ifdef KICAD_USE_FILES_WATCHER
    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );
    wxPostEvent( this, cmd);
#endif

    wxString msg;
    msg.Format( _( "Working dir: <%s>\nProject: <%s>\n" ),
                GetChars( m_ProjectFileName.GetPath() ),
                GetChars( m_ProjectFileName.GetFullName() ) );
    PrintMsg( msg );
}


void KICAD_MANAGER_FRAME::OnSaveProject( wxCommandEvent& event )
{
    if( !IsWritable( m_ProjectFileName ) )
        return;

    wxGetApp().WriteProjectConfig( m_ProjectFileName.GetFullPath(),
                                   GeneralGroupName, s_KicadManagerParams );
}
