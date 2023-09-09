/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "import_proj.h"
#include <kicad_manager_frame.h>
#include <kiway.h>
#include <kiway_player.h>
#include <wildcards_and_files_ext.h>
#include <macros.h>
#include <string_utils.h>
#include <richio.h>

#include <wx/msgdlg.h>


IMPORT_PROJ_HELPER::IMPORT_PROJ_HELPER( KICAD_MANAGER_FRAME*         aFrame,
                                        const std::vector<wxString>& aSchFileExtensions,
                                        const std::vector<wxString>& aPcbFileExtensions ) :
        m_frame( aFrame ),
        m_schExtenstions( aSchFileExtensions ), m_pcbExtenstions( aPcbFileExtensions )
{
}


void IMPORT_PROJ_HELPER::FindEmptyTargetDir()
{
    // Append a new directory with the same name of the project file
    // Keep iterating until we find an empty directory
    wxString newDir = m_TargetProj.GetName();
    int      attempt = 0;

    m_TargetProj.AppendDir( newDir );

    while( m_TargetProj.DirExists() )
    {
        m_TargetProj.RemoveLastDir();
        wxString suffix = wxString::Format( "_%d", ++attempt );
        m_TargetProj.AppendDir( newDir + suffix );
    }
}


void IMPORT_PROJ_HELPER::OutputCopyError( const wxFileName& aSrc, const wxFileName& aFileCopy )
{
    wxString msg;
    msg.Printf( _( "Cannot copy file '%s'\n"
                   "to '%s'\n"
                   "The project cannot be imported." ),
                aSrc.GetFullPath(), aFileCopy.GetFullPath() );

    wxMessageDialog fileCopyErrorDlg( m_frame, msg, _( "Error" ), wxOK_DEFAULT | wxICON_ERROR );
    fileCopyErrorDlg.ShowModal();
}


class SCOPED_FILE_REMOVER
{
    wxString m_file;

public:
    SCOPED_FILE_REMOVER( const wxString& aFile ) : m_file( aFile ) {}
    ~SCOPED_FILE_REMOVER() { wxRemoveFile( m_file ); }
};


void IMPORT_PROJ_HELPER::ImportIndividualFile( KICAD_T aFT, int aImportedFileType )
{
    FRAME_T               frame_type;
    wxString              appImportFile;
    std::vector<wxString> neededExts;

    switch( aFT )
    {
    case SCHEMATIC_T:
        neededExts = m_schExtenstions;
        frame_type = FRAME_SCH;
        break;

    case PCB_T:
        neededExts = m_pcbExtenstions;
        frame_type = FRAME_PCB_EDITOR;
        break;

    default: return;
    }

    std::vector<SCOPED_FILE_REMOVER> copiedFiles;

    for( wxString ext : neededExts )
    {
        if( ext == wxS( "INPUT" ) )
            ext = m_InputFile.GetExt();

        wxFileName candidate = m_InputFile;
        candidate.SetExt( ext );

        if( !candidate.FileExists() )
            continue;

        wxFileName targetFile( m_TargetProj.GetPath(), candidate.GetName(), candidate.GetExt() );

        if( !targetFile.FileExists() )
        {
            bool copied = wxCopyFile( candidate.GetFullPath(), targetFile.GetFullPath(), false );

            if( copied )
            {
                // Will be auto-removed
                copiedFiles.emplace_back( targetFile.GetFullPath() );
            }
        }

        // Pick the first file to pass to application
        if( appImportFile.empty() && targetFile.FileExists() )
            appImportFile = targetFile.GetFullPath();
    }

    if( appImportFile.empty() )
        return;

    KIWAY_PLAYER* frame = m_frame->Kiway().Player( frame_type, true );

    std::string packet = StrPrintf( "%d\n%s", aImportedFileType, TO_UTF8( appImportFile ) );
    frame->Kiway().ExpressMail( frame_type, MAIL_IMPORT_FILE, packet, m_frame );

    if( !frame->IsShown() )
        frame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( frame->IsIconized() )
        frame->Iconize( false );

    frame->Raise();
}


void IMPORT_PROJ_HELPER::ImportFiles( int aImportedSchFileType, int aImportedPcbFileType )
{
    ImportIndividualFile( SCHEMATIC_T, aImportedSchFileType );
    ImportIndividualFile( PCB_T, aImportedPcbFileType );
}