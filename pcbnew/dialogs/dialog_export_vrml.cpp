/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
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

#include <base_units.h>
#include <board.h>
#include <confirm.h>
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <tools/board_editor_control.h>
#include <project/project_file.h>  // LAST_PATH_TYPE
#include <wx/msgdlg.h>

#include <dialog_export_vrml.h>


DIALOG_EXPORT_VRML::DIALOG_EXPORT_VRML( PCB_EDIT_FRAME* aEditFrame ) :
        DIALOG_EXPORT_VRML_BASE( aEditFrame ),
        m_xOrigin( aEditFrame, m_xLabel, m_VRML_Xref, m_xUnits ),
        m_yOrigin( aEditFrame, m_yLabel, m_VRML_Yref, m_yUnits )
{
    m_filePicker->SetFocus();

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_EXPORT_VRML::TransferDataFromWindow()
{
    wxFileName fn = m_filePicker->GetPath();

    if( fn.Exists() )
    {
        if( wxMessageBox( _( "Are you sure you want to overwrite the existing file?" ), _( "Warning" ),
                          wxYES_NO | wxCENTER | wxICON_QUESTION, this )
            == wxNO )
        {
            return false;
        }
    }

    return true;
}


int BOARD_EDITOR_CONTROL::ExportVRML( const TOOL_EVENT& aEvent )
{
    BOARD* board = m_frame->GetBoard();

    // Build default output file name
    wxString path = m_frame->GetLastPath( LAST_PATH_VRML );

    if( path.IsEmpty() )
    {
        wxFileName brdFile = board->GetFileName();
        brdFile.SetExt( wxT( "wrl" ) );
        path = brdFile.GetFullPath();
    }

    DIALOG_EXPORT_VRML dlg( m_frame );
    dlg.FilePicker()->SetPath( path );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    double aXRef;
    double aYRef;

    if( dlg.GetSetUserDefinedOrigin() )
    {
        aXRef = dlg.GetXRefMM();
        aYRef = dlg.GetYRefMM();
    }
    else
    {
        // Origin = board center:
        BOX2I  bbox = board->ComputeBoundingBox( true );
        aXRef = pcbIUScale.IUTomm( bbox.GetCenter().x );
        aYRef = pcbIUScale.IUTomm( bbox.GetCenter().y );
    }

    path = dlg.FilePicker()->GetPath();
    m_frame->SetLastPath( LAST_PATH_VRML, path );
    wxFileName modelPath = path;

    wxBusyCursor dummy;

    modelPath.AppendDir( dlg.GetSubdir3Dshapes() );

    if( dlg.GetCopyFilesOption() && !modelPath.DirExists() )
    {
        if( !modelPath.Mkdir() )
        {
            DisplayErrorMessage( m_frame, wxString::Format( _( "Failed to create folder '%s'." ),
                                                            modelPath.GetPath() ) );
            return 0;
        }
    }

    if( !m_frame->ExportVRML_File( path,
                                   dlg.GetScale(),
                                   !dlg.GetNoUnspecifiedOption(),
                                   !dlg.GetNoDNPOption(),
                                   dlg.GetCopyFilesOption(),
                                   dlg.GetUseRelativePathsOption(),
                                   modelPath.GetPath(),
                                   aXRef, aYRef ) )
    {
        DisplayErrorMessage( m_frame, wxString::Format( _( "Failed to create file '%s'." ), path ) );
    }

    return 0;
}
