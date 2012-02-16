/**
 * @file prjconfig.cpp
 * Load and save project configuration files (*.pro)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
#include <prjconfig.h>
#include <kicad.h>
#include <tree_project_frame.h>
#include <wildcards_and_files_ext.h>

#include <build_version.h>

static const wxString GeneralGroupName( wxT( "/general" ) );

/* KiCad project file entry names. */
static const wxString SchematicRootNameEntry( wxT( "RootSch" ) );
static const wxString BoardFileNameEntry( wxT( "BoardNm" ) );


void KICAD_MANAGER_FRAME::CreateNewProject( const wxString PrjFullFileName )
{
    wxString   filename;
    wxFileName newProjectName = PrjFullFileName;

    ClearMsg();

    /* Init default config filename */
    filename = wxGetApp().FindLibraryPath( wxT( "kicad" ) + g_KicadPrjFilenameExtension );

    /* Check if file kicad.pro exist in template directory */
    if( wxFileName::FileExists( filename ) )
    {
        wxCopyFile( filename, PrjFullFileName );
    }
    else
    {
        DisplayInfoMessage( NULL, _( "Project template file <kicad.pro> not found. " ) );
        return;
    }

    /* Init schematic filename */
    m_SchematicRootFileName = wxFileName( newProjectName.GetName(),
                                          SchematicFileExtension ).GetFullName();

    /* Init pcb board filename */
    m_BoardFileName = wxFileName( newProjectName.GetName(), PcbFileExtension ).GetFullName();

    /* Init project filename */
    m_ProjectFileName = newProjectName;

    /* Write settings to project file */
    wxGetApp().WriteProjectConfig( PrjFullFileName, GeneralGroupName, NULL );
}


void KICAD_MANAGER_FRAME::OnLoadProject( wxCommandEvent& event )
{
    int style;
    wxString title;

    ClearMsg();

    if( event.GetId() != wxID_ANY )
    {
        if( event.GetId() == ID_NEW_PROJECT )
        {
            title = _( "Create New Project" );
            style = wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
        }
        else
        {
            title = _( "Open Existing Project" );
            style = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
        }

        wxFileDialog dlg( this, title, wxGetCwd(), wxEmptyString, ProjectFileWildcard, style );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        m_ProjectFileName = dlg.GetPath();

        if( event.GetId() == ID_NEW_PROJECT )
        {
            // Ensure project filename extension is .pro
            wxString fullname = m_ProjectFileName.GetFullPath();

            if ( !fullname.EndsWith( g_KicadPrjFilenameExtension ) )
            {
                fullname += g_KicadPrjFilenameExtension;
                m_ProjectFileName.SetFullName( fullname );
            }

            CreateNewProject( m_ProjectFileName.GetFullPath() );
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
