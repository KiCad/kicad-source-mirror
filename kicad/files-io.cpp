/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file kicad/files-io.cpp
 */

#include <confirm.h>
#include <kiway.h>
#include <pgm_kicad.h>
#include <project/project_archiver.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include "kicad_manager_frame.h"


void KICAD_MANAGER_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxFileName projFileName = GetFileFromHistory( event.GetId(), _( "KiCad project file" ) );
    if( !projFileName.FileExists() )
        return;

    LoadProject( projFileName );
}


void KICAD_MANAGER_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void KICAD_MANAGER_FRAME::OnUnarchiveFiles( wxCommandEvent& event )
{
    wxFileName fn = Prj().GetProjectFullName();

    fn.SetExt( ArchiveFileExtension );

    wxFileDialog zipfiledlg( this, _( "Unzip Project" ), fn.GetPath(),
                             fn.GetFullName(), ZipFileWildcard(),
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( zipfiledlg.ShowModal() == wxID_CANCEL )
        return;

    wxString msg = wxString::Format( _( "\nOpen \"%s\"\n" ), zipfiledlg.GetPath() );
    PrintMsg( msg );

    wxDirDialog dirDlg( this, _( "Target Directory" ), fn.GetPath(), wxDD_DEFAULT_STYLE );

    if( dirDlg.ShowModal() == wxID_CANCEL )
        return;

    wxString unzipDir = dirDlg.GetPath() + wxT( "/" );
    msg.Printf( _( "Unzipping project in \"%s\"\n" ), unzipDir );
    PrintMsg( msg );

    if( unzipDir == Prj().GetProjectPath() )
    {
        if( !Kiway().PlayersClose( false ) )
            return;
    }

    WX_TEXT_CTRL_REPORTER reporter( m_messagesBox );

    PROJECT_ARCHIVER archiver;

    archiver.Unarchive( zipfiledlg.GetPath(), unzipDir, reporter );

    if( unzipDir == Prj().GetProjectPath() )
    {
        wxString prjPath = Prj().GetProjectFullName();

        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

        mgr.UnloadProject( &Prj(), false );
        mgr.LoadProject( prjPath );

        RefreshProjectTree();
    }
}


void KICAD_MANAGER_FRAME::OnArchiveFiles( wxCommandEvent& event )
{
    wxFileName  fileName = GetProjectFileName();

    fileName.SetExt( ArchiveFileExtension );

    wxFileDialog dlg( this, _( "Archive Project Files" ),
                      fileName.GetPath(), fileName.GetFullName(),
                      ZipFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName zipFile = dlg.GetPath();

    wxString currdirname = fileName.GetPathWithSep();
    wxDir dir( currdirname );

    if( !dir.IsOpened() )   // wxWidgets display a error message on issue.
        return;

    WX_TEXT_CTRL_REPORTER reporter( m_messagesBox );

    PROJECT_ARCHIVER archiver;

    archiver.Archive( currdirname, zipFile.GetFullPath(), reporter, true, true );
}
