/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Russell Oliver <roliver8143@gmail.com>
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
 * @file import_project.cpp
 * @brief routines for importing a non-KiCad project
 */


#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/dir.h>

#include <common.h>
#include <confirm.h>
#include <kiway.h>
#include <macros.h>
#include <richio.h>
#include <wildcards_and_files_ext.h>
#include <kiway_player.h>
#include <stdexcept>
#include "pgm_kicad.h"

#include <io_mgr.h>
#include <sch_io_mgr.h>

#include "kicad_manager_frame.h"


void KICAD_MANAGER_FRAME::ImportNonKiCadProject( wxString aWindowTitle, wxString aFilesWildcard,
        wxString aSchFileExtension, wxString aPcbFileExtension, int aSchFileType, int aPcbFileType )
{
    wxString msg;
    wxString default_dir = GetMruPath();
    int      style       = wxFD_OPEN | wxFD_FILE_MUST_EXIST;

    wxFileDialog schdlg( this, aWindowTitle, default_dir, wxEmptyString, aFilesWildcard, style );

    if( schdlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName sch( schdlg.GetPath() );
    sch.SetExt( aSchFileExtension );

    wxFileName pcb( sch );
    pcb.SetExt( aPcbFileExtension );

    wxFileName pro( sch );
    pro.SetExt( ProjectFileExtension );

    wxString protitle = _( "KiCad Project Destination" );

    // Don't use wxFileDialog here.  On GTK builds, the default path is returned unless a
    // file is actually selected.
    wxDirDialog prodlg( this, protitle, pro.GetPath(), wxDD_DEFAULT_STYLE );

    if( prodlg.ShowModal() == wxID_CANCEL )
        return;

    pro.SetPath( prodlg.GetPath() );

    // Check if the project directory is empty
    wxDir directory( pro.GetPath() );

    if( directory.HasFiles() )
    {
        msg = _( "The selected directory is not empty.  We recommend you "
                 "create projects in their own clean directory.\n\nDo you "
                 "want to create a new empty directory for the project?" );

        KIDIALOG dlg( this, msg, _( "Confirmation" ), wxYES_NO | wxICON_WARNING );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( dlg.ShowModal() == wxID_YES )
        {
            // Append a new directory with the same name of the project file
            // Keep iterating until we find an empty directory
            wxString newDir = pro.GetName();
            int      attempt = 0;

            pro.AppendDir( newDir );

            while( pro.DirExists() )
            {
                pro.RemoveLastDir();
                wxString suffix = wxString::Format( "_%d", ++attempt );
                pro.AppendDir( newDir + suffix );
            }

            if( !wxMkdir( pro.GetPath() ) )
            {
                msg = _( "Error creating new directory. Please try a different path. The "
                         "project cannot be imported." );

                wxMessageDialog dirErrorDlg( this, msg, _( "Error" ), wxOK_DEFAULT | wxICON_ERROR );
                dirErrorDlg.ShowModal();
                return;
            }
        }
    }

    std::string packet;

    pro.SetExt( ProjectFileExtension );

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    wxFileName schCopy( pro );
    schCopy.SetExt( aSchFileExtension );

    if( sch.Exists() && !schCopy.SameAs( sch ) )
    {
        if( !wxCopyFile( sch.GetFullPath(), schCopy.GetFullPath(), true ) )
        {
            ///< @todo Should we remove the newly created folder?
            msg.Printf( _( "Cannot copy file '%s'\n"
                           "to '%s'\n"
                           "The project cannot be imported." ),
                        sch.GetFullPath(), schCopy.GetFullPath() );

            wxMessageDialog schCopyErrorDlg( this, msg, _( "Error" ), wxOK_DEFAULT | wxICON_ERROR );
            schCopyErrorDlg.ShowModal();
            return;
        }
    }

    wxFileName pcbCopy( pro );
    pcbCopy.SetExt( aPcbFileExtension );

    if( pcb.Exists() && !pcbCopy.SameAs( pcb ) )
    {
        if( !wxCopyFile( pcb.GetFullPath(), pcbCopy.GetFullPath(), true ) )
        {
            ///< @todo Should we remove copied schematic file and the newly created folder?
            msg.Printf( _( "Cannot copy file '%s'\n"
                           "to '%s'\n"
                           "The project cannot be imported." ),
                        pcb.GetFullPath(), pcbCopy.GetFullPath() );

            wxMessageDialog brdCopyErrorDlg( this, msg, _( "Error" ), wxOK_DEFAULT | wxICON_ERROR );
            brdCopyErrorDlg.ShowModal();
            return;
        }
    }

    // Close the project and make the new one
    CloseProject( true );
    CreateNewProject( pro.GetFullPath(), false /* Don't create stub files */ );
    LoadProject( pro );

    if( schCopy.FileExists() )
    {
        KIWAY_PLAYER* schframe = Kiway().Player( FRAME_SCH, true );

        packet = StrPrintf( "%d\n%s", aSchFileType, TO_UTF8( schCopy.GetFullPath() ) );
        schframe->Kiway().ExpressMail( FRAME_SCH, MAIL_IMPORT_FILE, packet, this );

        if( !schframe->IsShown() )
            schframe->Show( true );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( schframe->IsIconized() )
            schframe->Iconize( false );

        schframe->Raise();

        if( !schCopy.SameAs( sch ) ) // Do not delete the original file!
            wxRemoveFile( schCopy.GetFullPath() );
    }

    if( pcbCopy.FileExists() )
    {
        KIWAY_PLAYER* pcbframe = Kiway().Player( FRAME_PCB_EDITOR, true );

        if( !pcbframe->IsVisible() )
            pcbframe->Show( true );

        packet = StrPrintf( "%d\n%s", aPcbFileType, TO_UTF8( pcbCopy.GetFullPath() ) );
        pcbframe->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_IMPORT_FILE, packet, this );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( pcbframe->IsIconized() )
            pcbframe->Iconize( false );

        pcbframe->Raise();

        if( !pcbCopy.SameAs( pcb ) ) // Do not delete the original file!
            wxRemoveFile( pcbCopy.GetFullPath() );
    }

    ReCreateTreePrj();
    m_active_project = true;
}


void KICAD_MANAGER_FRAME::OnImportCadstarArchiveFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import CADSTAR Archive Project Files" ),
            CadstarArchiveFilesWildcard(), "csa", "cpa", SCH_IO_MGR::SCH_CADSTAR_ARCHIVE,
            IO_MGR::CADSTAR_PCB_ARCHIVE );
}


void KICAD_MANAGER_FRAME::OnImportEagleFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import Eagle Project Files" ), EagleFilesWildcard(),
            LegacySchematicFileExtension, LegacyPcbFileExtension,
            SCH_IO_MGR::SCH_EAGLE, IO_MGR::EAGLE );
}
