/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiway.h>
#include <kiway_player.h>
#include <wildcards_and_files_ext.h>
#include <macros.h>
#include <richio.h>

#include <wx/msgdlg.h>


IMPORT_PROJ_HELPER::IMPORT_PROJ_HELPER( KICAD_MANAGER_FRAME* aFrame,
                                        const wxString& aFile,
                                        const wxString& aSchFileExtension,
                                        const wxString& aPcbFileExtension ) :
        m_frame( aFrame ),
        m_sch( aFile ), m_pcb( m_sch ), m_pro( m_sch )
{
    m_sch.SetExt( aSchFileExtension );
    m_pcb.SetExt( aPcbFileExtension );
    m_pro.SetExt( ProjectFileExtension );
}


const wxFileName& IMPORT_PROJ_HELPER::GetProj()
{
    return m_pro;
}


wxString IMPORT_PROJ_HELPER::GetProjPath()
{
    return m_pro.GetPath();
}


void IMPORT_PROJ_HELPER::SetProjPath( const wxString aPath )
{
    m_pro.SetPath( aPath );
}


wxString IMPORT_PROJ_HELPER::GetProjFullPath()
{
    return m_pro.GetFullPath();
}


wxString IMPORT_PROJ_HELPER::GetProjName()
{
    return m_pro.GetName();
}


void IMPORT_PROJ_HELPER::CreateEmptyDirForProject()
{
    // Append a new directory with the same name of the project file
    // Keep iterating until we find an empty directory
    wxString newDir = m_pro.GetName();
    int      attempt = 0;

    m_pro.AppendDir( newDir );

    while( m_pro.DirExists() )
    {
        m_pro.RemoveLastDir();
        wxString suffix = wxString::Format( "_%d", ++attempt );
        m_pro.AppendDir( newDir + suffix );
    }
}


void IMPORT_PROJ_HELPER::SetProjAbsolutePath()
{
    m_pro.SetExt( ProjectFileExtension );
    if( !m_pro.IsAbsolute() )
        m_pro.MakeAbsolute();
}


bool IMPORT_PROJ_HELPER::CopyImportedFile( KICAD_T aFT, bool displayError )
{
    wxASSERT( m_pro.GetExt() == ProjectFileExtension );

    wxFileName fileCopy( m_pro );

    wxFileName src, dest;
    switch( aFT )
    {
    case SCHEMATIC_T: src = m_sch; break;

    case PCB_T: src = m_pcb; break;

    default: break;
    }

    fileCopy.SetExt( src.GetExt() );

    if( src.Exists() && !fileCopy.SameAs( src ) )
    {
        if( !wxCopyFile( src.GetFullPath(), fileCopy.GetFullPath(), true ) )
        {
            if( displayError )
                OutputCopyError( src, fileCopy );
            return false;
        }
    }

    switch( aFT )
    {
    case SCHEMATIC_T: m_shCopy = fileCopy; break;

    case PCB_T: m_pcbCopy = fileCopy; break;

    default: break;
    }

    return true;
}

bool IMPORT_PROJ_HELPER::CopyImportedFiles( bool displayError )
{
    return CopyImportedFile( SCHEMATIC_T, displayError ) && CopyImportedFile( PCB_T, displayError );
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


void IMPORT_PROJ_HELPER::AssociateFileWithProj( KICAD_T aFT, int aImportedFileType )
{
    wxFileName fileCopy, importedFile;
    FRAME_T    frame_type;
    switch( aFT )
    {
    case SCHEMATIC_T:
        importedFile = m_sch;
        fileCopy = m_shCopy;
        frame_type = FRAME_SCH;
        break;

    case PCB_T:
        importedFile = m_pcb;
        fileCopy = m_pcbCopy;
        frame_type = FRAME_PCB_EDITOR;
        break;

    default: return;
    }

    if( fileCopy.FileExists() )
    {
        KIWAY_PLAYER* frame = m_frame->Kiway().Player( frame_type, true );

        std::string packet =
                StrPrintf( "%d\n%s", aImportedFileType, TO_UTF8( fileCopy.GetFullPath() ) );
        frame->Kiway().ExpressMail( frame_type, MAIL_IMPORT_FILE, packet, m_frame );

        if( !frame->IsShown() )
            frame->Show( true );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( frame->IsIconized() )
            frame->Iconize( false );

        frame->Raise();

        if( !fileCopy.SameAs( importedFile ) ) // Do not delete the original file!
            wxRemoveFile( fileCopy.GetFullPath() );
    }
}


void IMPORT_PROJ_HELPER::AssociateFilesWithProj( int aImportedSchFileType,
                                                 int aImportedPcbFileType )
{
    AssociateFileWithProj( SCHEMATIC_T, aImportedSchFileType );
    AssociateFileWithProj( PCB_T, aImportedPcbFileType );
}