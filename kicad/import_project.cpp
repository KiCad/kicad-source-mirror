/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kidialog.h>
#include <wildcards_and_files_ext.h>

#include <sch_io/sch_io_mgr.h>
#include <pcb_io/pcb_io_mgr.h>

#include "kicad_manager_frame.h"
#include <import_proj.h>


void KICAD_MANAGER_FRAME::ImportNonKiCadProject( const wxString& aWindowTitle,
                                                 const wxString& aFilesWildcard,
                                                 const std::vector<std::string>& aSchFileExtensions,
                                                 const std::vector<std::string>& aPcbFileExtensions,
                                                 int aSchFileType, int aPcbFileType )
{
    wxString msg;
    wxString default_dir = GetMruPath();
    int      style = wxFD_OPEN | wxFD_FILE_MUST_EXIST;

    wxFileDialog inputdlg( this, aWindowTitle, default_dir, wxEmptyString, aFilesWildcard, style );

    if( inputdlg.ShowModal() == wxID_CANCEL )
        return;

    // OK, we got a new project to open.  Time to close any existing project before we go on
    // to collect info about where to put the new one, etc.  Otherwise the workflow is kind of
    // disjoint.
    if( !CloseProject( true ) )
        return;

    std::vector<wxString> schFileExts( aSchFileExtensions.begin(), aSchFileExtensions.end() );
    std::vector<wxString> pcbFileExts( aPcbFileExtensions.begin(), aPcbFileExtensions.end() );

    IMPORT_PROJ_HELPER importProj( this, schFileExts, pcbFileExts );
    importProj.m_InputFile = inputdlg.GetPath();

    // Don't use wxFileDialog here.  On GTK builds, the default path is returned unless a
    // file is actually selected.
    wxDirDialog prodlg( this, _( "KiCad Project Destination" ), importProj.m_InputFile.GetPath(),
                        wxDD_DEFAULT_STYLE );

    if( prodlg.ShowModal() == wxID_CANCEL )
        return;

    wxString targetDir = prodlg.GetPath();

    importProj.m_TargetProj.SetPath( targetDir );
    importProj.m_TargetProj.SetName( importProj.m_InputFile.GetName() );
    importProj.m_TargetProj.SetExt( FILEEXT::ProjectFileExtension );
    importProj.m_TargetProj.MakeAbsolute();

    // Check if the project directory exists and is empty
    if( !importProj.m_TargetProj.DirExists() )
    {
        if( !importProj.m_TargetProj.Mkdir() )
        {
            msg.Printf( _( "Folder '%s' could not be created.\n\n"
                           "Make sure you have write permissions and try again." ),
                        importProj.m_TargetProj.GetPath() );
            DisplayErrorMessage( this, msg );
            return;
        }
    }
    else
    {
        wxDir targetDirTest( targetDir );
        if( targetDirTest.IsOpened() && targetDirTest.HasFiles() )
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
                importProj.FindEmptyTargetDir();

                if( !wxMkdir( importProj.m_TargetProj.GetPath() ) )
                {
                    msg = _( "Error creating new directory. Please try a different path. The "
                             "project cannot be imported." );

                    wxMessageDialog dirErrorDlg( this, msg, _( "Error" ),
                                                 wxOK_DEFAULT | wxICON_ERROR );
                    dirErrorDlg.ShowModal();
                    return;
                }
            }
        }

        targetDirTest.Close();
    }

    CreateNewProject( importProj.m_TargetProj.GetFullPath(), false /* Don't create stub files */ );
    LoadProject( importProj.m_TargetProj );

    importProj.ImportFiles( aSchFileType, aPcbFileType );

    ReCreateTreePrj();
    m_active_project = true;
}


void KICAD_MANAGER_FRAME::OnImportAltiumProjectFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import Altium Project Files" ),
                           FILEEXT::AltiumProjectFilesWildcard(), { "SchDoc" }, { "PcbDoc" },
                           SCH_IO_MGR::SCH_ALTIUM, PCB_IO_MGR::ALTIUM_DESIGNER );
}


void KICAD_MANAGER_FRAME::OnImportCadstarArchiveFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import CADSTAR Archive Project Files" ),
                           FILEEXT::CadstarArchiveFilesWildcard(), { "csa" }, { "cpa" },
                           SCH_IO_MGR::SCH_CADSTAR_ARCHIVE, PCB_IO_MGR::CADSTAR_PCB_ARCHIVE );
}


void KICAD_MANAGER_FRAME::OnImportEagleFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import Eagle Project Files" ), FILEEXT::EagleFilesWildcard(), { "sch" },
                           { "brd" }, SCH_IO_MGR::SCH_EAGLE, PCB_IO_MGR::EAGLE );
}


void KICAD_MANAGER_FRAME::OnImportEasyEdaFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import EasyEDA Std Backup" ), FILEEXT::EasyEdaArchiveWildcard(), { "INPUT" },
                           { "INPUT" }, SCH_IO_MGR::SCH_EASYEDA, PCB_IO_MGR::EASYEDA );
}


void KICAD_MANAGER_FRAME::OnImportEasyEdaProFiles( wxCommandEvent& event )
{
    ImportNonKiCadProject( _( "Import EasyEDA Pro Project" ), FILEEXT::EasyEdaProFileWildcard(), { "INPUT" },
                           { "INPUT" }, SCH_IO_MGR::SCH_EASYEDAPRO, PCB_IO_MGR::EASYEDAPRO );
}
