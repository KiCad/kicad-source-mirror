/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <sch_edit_frame.h>
#include <netlist.h>

class PCB_EDIT_FRAME;

#include "kicad.h"

void KICAD_MANAGER_FRAME::OnImportEagleFiles( wxCommandEvent& event )
{
    // Close other windows.
    if( !Kiway.PlayersClose( false ) )
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
    wxDirDialog prodlg( this, protitle, pro.GetPath(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

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

        if( IsOK( this, msg ) )
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
    wxFileName netlist( pro );
    pro.SetExt( ProjectFileExtension );         // enforce extension
    pcb.SetExt( LegacyPcbFileExtension );       // enforce extension
    netlist.SetExt( NetlistFileExtension );

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    SetProjectFileName( pro.GetFullPath() );
    wxString prj_filename = GetProjectFileName();
    wxString sch_filename = sch.GetFullPath();

    if( sch.FileExists() )
    {
        SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) Kiway.Player( FRAME_SCH, false );

        if( !schframe )
        {
            try
            {
                schframe = (SCH_EDIT_FRAME*) Kiway.Player( FRAME_SCH, true );
            }
            catch( IO_ERROR err )
            {
                wxMessageBox( _( "Eeschema failed to load:\n" ) + err.What(),
                        _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }

        schframe->ImportFile( sch_filename, SCH_IO_MGR::SCH_EAGLE );

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
        PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Kiway.Player( FRAME_PCB, false );

        if( !pcbframe )
        {
            try
            {
                pcbframe = (PCB_EDIT_FRAME*) Kiway.Player( FRAME_PCB, true );
            }
            catch( IO_ERROR err )
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
            pcbframe->ImportFile( pcb.GetFullPath(), IO_MGR::EAGLE );
            pcbframe->Show( true );
        }

        // On Windows, Raise() does not bring the window on screen, when iconized
        if( pcbframe->IsIconized() )
            pcbframe->Iconize( false );

        pcbframe->Raise();

        // Two stage project update:
        // - first, assign valid timestamps to footprints
        // - second, perform schematic annotation and update footprint references
        pcbframe->Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_PCB_UPDATE_REQUEST, "no-annotate;by-reference", this );
        pcbframe->Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_PCB_UPDATE_REQUEST, "quiet-annotate;by-timestamp", this );
    }

    ReCreateTreePrj();
    m_active_project = true;
}
