/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief routines for importing an eagle project
 */


#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

#include <common.h>
#include <confirm.h>
#include <hotkeys_basic.h>
#include <kiway.h>
#include <richio.h>
#include <wildcards_and_files_ext.h>
#include <systemdirsappend.h>
#include <kiway_player.h>
#include <stdexcept>
#include "pgm_kicad.h"

#include <io_mgr.h>
#include <sch_io_mgr.h>

#include "kicad_manager_frame.h"

void KICAD_MANAGER_FRAME::OnImportEagleFiles( wxCommandEvent& event )
{
    // Close other windows.
    if( !Kiway().PlayersClose( false ) )
        return;


    wxString title = _( "Import Eagle Project Files" );
    int style = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
    wxString default_dir = GetMruPath();

    ClearMsg();

    wxFileDialog schdlg( this, title, default_dir, wxEmptyString,
                         EagleFilesWildcard(), style );

    if( schdlg.ShowModal() == wxID_CANCEL )
        return;


    wxFileName sch( schdlg.GetPath() );

    sch.SetExt( SchematicFileExtension );

    wxFileName pro = sch;

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
        wxString msg = _( "The selected directory is not empty.  We recommend you "
                          "create projects in their own clean directory.\n\nDo you "
                          "want to create a new empty directory for the project?" );

        KIDIALOG dlg( this, msg, _( "Confirmation" ), wxYES_NO | wxICON_WARNING );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( dlg.ShowModal() == wxID_YES )
        {
            // Append a new directory with the same name of the project file
            // and try to create it
            pro.AppendDir( pro.GetName() );

            if( !wxMkdir( pro.GetPath() ) )
                // There was a problem, undo
                pro.RemoveLastDir();
        }
    }

    wxFileName pcb( sch );
    pro.SetExt( ProjectFileExtension );         // enforce extension
    pcb.SetExt( LegacyPcbFileExtension );       // enforce extension

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    SetProjectFileName( pro.GetFullPath() );
    wxString prj_filename = GetProjectFileName();

    if( sch.FileExists() )
    {
        KIWAY_PLAYER* schframe = Kiway().Player( FRAME_SCH, false );

        if( !schframe )
        {
            try     // SCH frame was not available, try to start it
            {
                schframe = Kiway().Player( FRAME_SCH, true );
            }
            catch( const IO_ERROR& err )
            {
                wxMessageBox( _( "Eeschema failed to load:\n" ) + err.What(),
                        _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }

        std::string packet = StrPrintf( "%d\n%s", SCH_IO_MGR::SCH_EAGLE,
                                                  TO_UTF8( sch.GetFullPath() ) );
        schframe->Kiway().ExpressMail( FRAME_SCH, MAIL_IMPORT_FILE, packet, this );

        if( !schframe->IsShown() )      // the frame exists, (created by the dialog field editor)
                                        // but no project loaded.
        {
            schframe->Show( true );
        }

        if( schframe->IsIconized() )
            schframe->Iconize( false );

        schframe->Raise();
    }


    if( pcb.FileExists() )
    {
        KIWAY_PLAYER* pcbframe = Kiway().Player( FRAME_PCB_EDITOR, false );

        if( !pcbframe )
        {
            try     // PCB frame was not available, try to start it
            {
                pcbframe = Kiway().Player( FRAME_PCB_EDITOR, true );
            }
            catch( const IO_ERROR& err )
            {
                wxMessageBox( _( "Pcbnew failed to load:\n" ) + err.What(), _( "KiCad Error" ),
                        wxOK | wxICON_ERROR, this );
                return;
            }
        }

        // a pcb frame can be already existing, but not yet used.
        // this is the case when running the footprint editor, or the footprint viewer first
        // if the frame is not visible, the board is not yet loaded
        if( !pcbframe->IsVisible() )
        {
            pcbframe->Show( true );
        }

        std::string packet = StrPrintf( "%d\n%s", IO_MGR::EAGLE,
                                                  TO_UTF8( pcb.GetFullPath() ) );
        pcbframe->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_IMPORT_FILE, packet, this );

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( pcbframe->IsIconized() )
            pcbframe->Iconize( false );

        pcbframe->Raise();
    }

    ReCreateTreePrj();
    m_active_project = true;
}
