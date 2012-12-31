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
#include <prjconfig.h>
#include <project_template.h>
#include <tree_project_frame.h>
#include <wildcards_and_files_ext.h>
#include <vector>
#include <build_version.h>

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "dialogs/dialog_template_selector.h"

#define SEP()   wxFileName::GetPathSeparator()

static const wxString GeneralGroupName( wxT( "/general" ) );

/* KiCad project file entry names. */
static const wxString SchematicRootNameEntry( wxT( "RootSch" ) );
static const wxString BoardFileNameEntry( wxT( "BoardNm" ) );


void KICAD_MANAGER_FRAME::CreateNewProject( const wxString aPrjFullFileName, bool aTemplateSelector = false )
{
    wxString   filename;
    wxFileName newProjectName = aPrjFullFileName;

    ClearMsg();

    // Init default config filename
    filename = wxGetApp().FindLibraryPath( wxT( "kicad" ) + g_KicadPrjFilenameExtension );

    // If we are creating a project from a template, make sure the template directory is sane
    if( aTemplateSelector )
    {
        DIALOG_TEMPLATE_SELECTOR* ps = new DIALOG_TEMPLATE_SELECTOR( this );

        // Add a new tab for system templates
        wxFileName templatePath = wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) +
                SEP() + wxT( ".." ) + SEP() + wxT( "share" ) + SEP() + wxT( "template" ) + SEP();

        ps->AddPage( _( "System Templates" ), templatePath );

        // Add a new tab for user templates
        wxFileName userPath = wxStandardPaths::Get().GetDocumentsDir() +
                SEP() + wxT( "kicad" ) + SEP() + wxT( "template" ) + SEP();

        ps->AddPage( _( "User Templates" ), userPath );

        // Check to see if a custom template location is available and setup a new selection tab
        // if there is
        wxString envStr;
        wxGetEnv( wxT("KICAD_PTEMPLATES"), &envStr );
        wxFileName envPath = envStr;

        if( envStr != wxEmptyString )
        {
            wxFileName envPath = envStr;
            ps->AddPage( _("Portable Templates"), envPath );
        }

        // Show the project template selector dialog
        int result = ps->ShowModal();

        if( result != wxID_OK )
        {
            wxMessageBox( _( "Did not generate new project from template" ),
                          _( "Cancelled new project from template" ),
                          wxOK | wxICON_EXCLAMATION,
                          this );
        }
        else
        {
            // The selected template widget contains the template we're attempting to use to
            // create a project
            if( !ps->GetWidget()->GetTemplate()->CreateProject( newProjectName ) )
            {
                wxMessageBox( _( "Problem whilst creating new project from template!" ),
                              _( "Could not generate new project" ),
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
    wxGetApp().WriteProjectConfig( aPrjFullFileName, GeneralGroupName, NULL );
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
            if ( !m_ProjectFileName.GetFullPath().EndsWith( g_KicadPrjFilenameExtension ) )
            {
                m_ProjectFileName.SetFullName( m_ProjectFileName.GetFullPath() +
                                               g_KicadPrjFilenameExtension );
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
        DisplayError( this, _( "KiCad project file <" ) +
                      m_ProjectFileName.GetFullPath() + _( "> not found" ) );
        return;
    }

    wxSetWorkingDirectory( m_ProjectFileName.GetPath() );
    wxGetApp().ReadProjectConfig( m_ProjectFileName.GetFullPath(),
                                  GeneralGroupName, NULL, false );

    title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
        wxT( " " ) +  m_ProjectFileName.GetFullPath();

    if( !m_ProjectFileName.IsDirWritable() )
        title += _( " [Read Only]" );

    SetTitle( title );
    UpdateFileHistory( m_ProjectFileName.GetFullPath() );
#if wxCHECK_VERSION( 2, 9, 2  )
    m_LeftWin->FileWatcherReset();
#endif
    m_LeftWin->ReCreateTreePrj();

    PrintMsg( _( "Working dir: " ) + m_ProjectFileName.GetPath() +
              _( "\nProject: " ) + m_ProjectFileName.GetFullName() +
              wxT( "\n" ) );
}


void KICAD_MANAGER_FRAME::OnSaveProject( wxCommandEvent& event )
{
    if( !IsWritable( m_ProjectFileName ) )
        return;

    wxGetApp().WriteProjectConfig( m_ProjectFileName.GetFullPath(), GeneralGroupName, NULL );
}
