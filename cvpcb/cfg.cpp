/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jean-pierre.charras
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cfg.cpp
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <id.h>
#include <common.h>
#include <gestfich.h>
#include <param_config.h>
#include <wildcards_and_files_ext.h>
#include <fp_lib_table.h>
#include <confirm.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <class_DisplayFootprintsFrame.h>


#define GROUP wxT("/cvpcb")
#define GROUPLIB wxT("/pcbnew/libraries")
#define GROUPEQU wxT("/cvpcb/libraries")


PARAM_CFG_ARRAY& CVPCB_MAINFRAME::GetProjectFileParameters( void )
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_BASE( GROUPLIB,
                                                       PARAM_COMMAND_ERASE ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),
                                                               &m_ModuleLibNames,
                                                               GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "EquName" ),
                                                               &m_AliasLibNames,
                                                               GROUPEQU ) );
    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "NetIExt" ),
                                                           &m_NetlistFileExtension ) );
    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LibDir" ),
                                                           &m_UserLibraryPath,
                                                           GROUPLIB ) );

    return m_projectFileParams;
}


void CVPCB_MAINFRAME::LoadProjectFile( const wxString& aFileName )
{
    wxFileName fn = aFileName;

    m_ModuleLibNames.Clear();
    m_AliasLibNames.Clear();

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( m_UserLibraryPath );

    wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters(), false );

    if( m_NetlistFileExtension.IsEmpty() )
        m_NetlistFileExtension = wxT( "net" );

    // User library path takes precedent over default library search paths.
    wxGetApp().InsertLibraryPath( m_UserLibraryPath, 1 );

#if defined( USE_FP_LIB_TABLE )
    delete m_footprintLibTable;

    // Attempt to load the project footprint library table if it exists.
    m_footprintLibTable = new FP_LIB_TABLE();

    if( m_DisplayFootprintFrame )
        m_DisplayFootprintFrame->SetFootprintLibTable( m_footprintLibTable );

    wxFileName projectFpLibTableFileName;

    projectFpLibTableFileName = FP_LIB_TABLE::GetProjectFileName( fn );
    FP_LIB_TABLE::SetProjectPathEnvVariable( projectFpLibTableFileName );

    try
    {
        m_footprintLibTable->Load( projectFpLibTableFileName, m_globalFootprintTable );
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
    }
#endif
}


void CVPCB_MAINFRAME::SaveProjectFile( wxCommandEvent& aEvent )
{
    wxFileName fn = m_NetlistFileName;

    fn.SetExt( ProjectFileExtension );

    if( aEvent.GetId() == ID_SAVE_PROJECT_AS || !m_NetlistFileName.IsOk() )
    {
        wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                          wxEmptyString, ProjectFileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();

        if( !fn.HasExt() )
            fn.SetExt( ProjectFileExtension );
    }

    if( !IsWritable( fn ) )
        return;

    wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters() );
}
