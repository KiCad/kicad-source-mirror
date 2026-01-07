/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>

#include <kiway.h>
#include <project/project_archiver.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <local_history.h>

#include "kicad_manager_frame.h"


void KICAD_MANAGER_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "KiCad project file" ) );

    if( !filename.IsEmpty() )
        LoadProject( wxFileName( filename ) );
}


void KICAD_MANAGER_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void KICAD_MANAGER_FRAME::UnarchiveFiles()
{
    wxFileName fn = Prj().GetProjectFullName();

    fn.SetExt( FILEEXT::ArchiveFileExtension );

    wxFileDialog zipfiledlg( this, _( "Unzip Project" ), fn.GetPath(), fn.GetFullName(),
                             FILEEXT::ZipFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( zipfiledlg.ShowModal() == wxID_CANCEL )
        return;

    wxString msg = wxString::Format( _( "\nOpen '%s'\n" ), zipfiledlg.GetPath() );
    PrintMsg( msg );

    wxDirDialog dirDlg( this, _( "Target Directory" ), fn.GetPath(), wxDD_DEFAULT_STYLE );

    if( dirDlg.ShowModal() == wxID_CANCEL )
        return;

    wxString unzipDir = dirDlg.GetPath() + wxT( "/" );
    msg.Printf( _( "Unzipping project in '%s'.\n" ), unzipDir );
    PrintMsg( msg );

    if( unzipDir == Prj().GetProjectPath() )
    {
        if( !Kiway().PlayersClose( false ) )
            return;
    }

    STATUSBAR_REPORTER reporter( GetStatusBar(), 1 );

    if( PROJECT_ARCHIVER::Unarchive( zipfiledlg.GetPath(), unzipDir, reporter ) )
    {
        wxArrayString projectFiles;
        wxDir::GetAllFiles( unzipDir, &projectFiles,
                            wxT( "*." ) + wxString::FromUTF8( FILEEXT::ProjectFileExtension ),
                            wxDIR_FILES );

        if( projectFiles.size() == 1 )
            LoadProject( wxFileName( projectFiles[0] ) );
    }
}

void KICAD_MANAGER_FRAME::RestoreLocalHistory()
{
    Kiway().LocalHistory().ShowRestoreDialog( Prj().GetProjectPath(), this );
}
