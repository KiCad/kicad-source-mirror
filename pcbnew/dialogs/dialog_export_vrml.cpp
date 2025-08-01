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
/**
 * @file dialog_export_vrml.cpp
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
        m_editFrame( aEditFrame ),
        m_unitsOpt( 1 ),
        m_noUnspecified( false ),
        m_noDNP( false ),
        m_copy3DFilesOpt( false ),
        m_useRelativePathsOpt( false ),
        m_RefUnits( 0 ),
        m_XRef( 0.0 ),
        m_YRef( 0.0 ),
        m_originMode( 0 )
{
    m_filePicker->SetFocus();

    if( PCBNEW_SETTINGS* cfg = m_editFrame->GetPcbNewSettings() )
    {
        m_unitsOpt = cfg->m_ExportVrml.units;
        m_noUnspecified = cfg->m_ExportVrml.no_unspecified;
        m_noDNP = cfg->m_ExportVrml.no_dnp;
        m_copy3DFilesOpt = cfg->m_ExportVrml.copy_3d_models;
        m_useRelativePathsOpt = cfg->m_ExportVrml.use_relative_paths;
        m_RefUnits = cfg->m_ExportVrml.ref_units;
        m_XRef = cfg->m_ExportVrml.ref_x;
        m_YRef = cfg->m_ExportVrml.ref_y;
        m_originMode = cfg->m_ExportVrml.origin_mode;
    }

    m_rbCoordOrigin->SetSelection( m_originMode );
    m_rbSelectUnits->SetSelection( m_unitsOpt );
    m_cbRemoveUnspecified->SetValue( m_noUnspecified );
    m_cbRemoveDNP->SetValue( m_noDNP );
    m_cbCopyFiles->SetValue( m_copy3DFilesOpt );
    m_cbUseRelativePaths->SetValue( m_useRelativePathsOpt );
    m_VRML_RefUnitChoice->SetSelection( m_RefUnits );
    wxString tmpStr;
    tmpStr << m_XRef;
    m_VRML_Xref->SetValue( tmpStr );
    tmpStr = wxT( "" );
    tmpStr << m_YRef;
    m_VRML_Yref->SetValue( tmpStr );

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_VRML::~DIALOG_EXPORT_VRML()
{
    m_unitsOpt = GetUnits();
    m_noUnspecified = GetNoUnspecifiedOption();
    m_noDNP = GetNoDNPOption();
    m_copy3DFilesOpt = GetCopyFilesOption();

    if( PCBNEW_SETTINGS* cfg = m_editFrame->GetPcbNewSettings() )
    {
        cfg->m_ExportVrml.units = m_unitsOpt;
        cfg->m_ExportVrml.no_unspecified = m_noUnspecified;
        cfg->m_ExportVrml.no_dnp = m_noDNP;
        cfg->m_ExportVrml.copy_3d_models = m_copy3DFilesOpt;
        cfg->m_ExportVrml.use_relative_paths = m_useRelativePathsOpt;
        cfg->m_ExportVrml.ref_units = m_VRML_RefUnitChoice->GetSelection();
        cfg->m_ExportVrml.origin_mode = m_rbCoordOrigin->GetSelection();

        double val = 0.0;
        m_VRML_Xref->GetValue().ToDouble( &val );
        cfg->m_ExportVrml.ref_x = val;

        m_VRML_Yref->GetValue().ToDouble( &val );
        cfg->m_ExportVrml.ref_y = val;
    }
}


bool DIALOG_EXPORT_VRML::TransferDataFromWindow()
{
    wxFileName fn = m_filePicker->GetPath();

    if( fn.Exists() )
    {
        if( wxMessageBox( _( "Are you sure you want to overwrite the existing file?" ),
                          _( "Warning" ), wxYES_NO | wxCENTER | wxICON_QUESTION, this ) == wxNO )
            return false;
    }

    return true;
}


double DIALOG_EXPORT_VRML::GetXRef()
{
    return EDA_UNIT_UTILS::UI::DoubleValueFromString( m_VRML_Xref->GetValue() );
}


double DIALOG_EXPORT_VRML::GetYRef()
{
    return EDA_UNIT_UTILS::UI::DoubleValueFromString( m_VRML_Yref->GetValue() );
}


int BOARD_EDITOR_CONTROL::ExportVRML( const TOOL_EVENT& aEvent )
{
    // These variables are static to keep info during the session.
    static wxString subDirFor3Dshapes;

    BOARD* board = m_frame->GetBoard();

    // Build default output file name
    wxString path = m_frame->GetLastPath( LAST_PATH_VRML );

    if( path.IsEmpty() )
    {
        wxFileName brdFile = board->GetFileName();
        brdFile.SetExt( wxT( "wrl" ) );
        path = brdFile.GetFullPath();
    }

    if( subDirFor3Dshapes.IsEmpty() )
        subDirFor3Dshapes = wxT( "shapes3D" );

    // The general VRML scale factor
    // Assuming the VRML default unit is the mm
    // this is the mm to VRML scaling factor for mm, 0.1 inch, and inch
    double scaleList[4] = { 1.0, 0.001, 10.0/25.4, 1.0/25.4 };

    DIALOG_EXPORT_VRML dlg( m_frame );
    dlg.FilePicker()->SetPath( path );
    dlg.SetSubdir( subDirFor3Dshapes );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    double aXRef = dlg.GetXRef();
    double aYRef = dlg.GetYRef();

    if( dlg.GetRefUnitsChoice() == 1 )
    {
        // selected reference unit is in inches
        aXRef *= 25.4;
        aYRef *= 25.4;
    }

    if( dlg.GetOriginChoice() == 1 )
    {
        // Origin = board center:
        BOX2I  bbox = board->ComputeBoundingBox( true );
        aXRef = pcbIUScale.IUTomm( bbox.GetCenter().x );
        aYRef = pcbIUScale.IUTomm( bbox.GetCenter().y );
    }

    double scale = scaleList[dlg.GetUnits()];     // final scale export
    bool includeUnspecified = !dlg.GetNoUnspecifiedOption();
    bool includeDNP = !dlg.GetNoDNPOption();
    bool export3DFiles = dlg.GetCopyFilesOption();
    bool useRelativePaths = dlg.GetUseRelativePathsOption();

    path = dlg.FilePicker()->GetPath();
    m_frame->SetLastPath( LAST_PATH_VRML, path );
    wxFileName modelPath = path;

    wxBusyCursor dummy;

    subDirFor3Dshapes = dlg.GetSubdir3Dshapes();
    modelPath.AppendDir( subDirFor3Dshapes );

    if( export3DFiles && !modelPath.DirExists() )
    {
        if( !modelPath.Mkdir() )
        {
            DisplayErrorMessage( m_frame, wxString::Format( _( "Failed to create folder '%s'." ),
                                                            modelPath.GetPath() ) );
            return 0;
        }
    }

    if( !m_frame->ExportVRML_File( path, scale, includeUnspecified, includeDNP, export3DFiles,
                                   useRelativePaths, modelPath.GetPath(), aXRef, aYRef ) )
    {
        DisplayErrorMessage( m_frame, wxString::Format( _( "Failed to create file '%s'." ),
                                                        path ) );
    }

    return 0;
}
