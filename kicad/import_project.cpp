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

#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>

#include <confirm.h>
#include <wildcards_and_files_ext.h>

#include <io_mgr.h>
#include <sch_io_mgr.h>

#include "kicad_manager_frame.h"
#include <import_proj.h>


void KICAD_MANAGER_FRAME::ImportNonKiCadProject( const wxString& aWindowTitle,
                                                 const wxString& aFilesWildcard,
                                                 const wxString& aSchFileExtension,
                                                 const wxString& aPcbFileExtension,
                                                 int aSchFileType, int aPcbFileType )
{
    wxString msg;
    wxString default_dir = GetMruPath();
    int      style       = wxFD_OPEN | wxFD_FILE_MUST_EXIST;

    wxFileDialog schdlg( this, aWindowTitle, default_dir, wxEmptyString, aFilesWildcard, style );

    if( schdlg.ShowModal() == wxID_CANCEL )
        return;

    // OK, we got a new project to open.  Time to close any existing project before we go on
    // to collect info about where to put the new one, etc.  Otherwise the workflow is kind of
    // disjoint.
    if( !CloseProject( true ) )
        return;

    IMPORT_PROJ_HELPER importProj( this, schdlg.GetPath(), aSchFileExtension, aPcbFileExtension );

    wxString protitle = _( "KiCad Project Destination" );

    // Don't use wxFileDialog here.  On GTK builds, the default path is returned unless a
    // file is actually selected.
    wxDirDialog prodlg( this, protitle, importProj.GetProjPath(), wxDD_DEFAULT_STYLE );

    if( prodlg.ShowModal() == wxID_CANCEL )
        return;

    importProj.SetProjPath( prodlg.GetPath() );

    // Check if the project directory is empty
    wxDir directory( importProj.GetProjPath() );

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
            importProj.CreateEmptyDirForProject();

            if( !wxMkdir( importProj.GetProjPath() ) )
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

    importProj.SetProjAbsolutePath();

    if( !importProj.CopyImportedFiles() )
        return;

    CreateNewProject( importProj.GetProjFullPath(), false /* Don't create stub files */ );
    LoadProject( importProj.GetProj() );

    importProj.AssociateFilesWithProj( aSchFileType, aPcbFileType );

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
