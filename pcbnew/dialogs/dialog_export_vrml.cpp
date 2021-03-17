/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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

#include <board.h>
#include <confirm.h>
#include <kiface_i.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <project/project_file.h>  // LAST_PATH_TYPE


/* the dialog to create VRML files, derived from DIALOG_EXPORT_3DFILE_BASE,
 * created by wxFormBuilder
 */
#include <dialog_export_vrml_base.h> // the wxFormBuilder header file


class DIALOG_EXPORT_3DFILE : public DIALOG_EXPORT_3DFILE_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    int             m_unitsOpt;             // Remember last units option
    bool            m_copy3DFilesOpt;       // Remember last copy model files option
    bool            m_useRelativePathsOpt;  // Remember last use absolute paths option
    bool            m_usePlainPCBOpt;       // Remember last Plain Board option
    int             m_RefUnits;             // Remember last units for Reference Point
    double          m_XRef;                 // Remember last X Reference Point
    double          m_YRef;                 // Remember last Y Reference Point
    int             m_originMode;           // Origin selection option
                                            // (0 = user, 1 = board center)

public:
    DIALOG_EXPORT_3DFILE( PCB_EDIT_FRAME* parent ) :
        DIALOG_EXPORT_3DFILE_BASE( parent ), m_parent( parent )
    {
        m_filePicker->SetFocus();

        PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

        m_unitsOpt            = cfg->m_ExportVrml.units;
        m_copy3DFilesOpt      = cfg->m_ExportVrml.copy_3d_models;
        m_useRelativePathsOpt = cfg->m_ExportVrml.use_relative_paths;
        m_usePlainPCBOpt      = cfg->m_ExportVrml.use_plain_pcb;
        m_RefUnits            = cfg->m_ExportVrml.ref_units;
        m_XRef                = cfg->m_ExportVrml.ref_x;
        m_YRef                = cfg->m_ExportVrml.ref_y;
        m_originMode          = cfg->m_ExportVrml.origin_mode;


        m_rbCoordOrigin->SetSelection( m_originMode );
        m_rbSelectUnits->SetSelection( m_unitsOpt );
        m_cbCopyFiles->SetValue( m_copy3DFilesOpt );
        m_cbUseRelativePaths->SetValue( m_useRelativePathsOpt );
        m_cbPlainPCB->SetValue( m_usePlainPCBOpt );
        m_VRML_RefUnitChoice->SetSelection( m_RefUnits );
        wxString tmpStr;
        tmpStr << m_XRef;
        m_VRML_Xref->SetValue( tmpStr );
        tmpStr = wxT( "" );
        tmpStr << m_YRef;
        m_VRML_Yref->SetValue( tmpStr );
        m_sdbSizerOK->SetDefault();

        // Now all widgets have the size fixed, call FinishDialogSettings
        finishDialogSettings();
    }

    ~DIALOG_EXPORT_3DFILE()
    {
        m_unitsOpt = GetUnits();
        m_copy3DFilesOpt = GetCopyFilesOption();

        PCBNEW_SETTINGS* cfg = m_parent->GetPcbNewSettings();

        cfg->m_ExportVrml.units              = m_unitsOpt;
        cfg->m_ExportVrml.copy_3d_models     = m_copy3DFilesOpt;
        cfg->m_ExportVrml.use_relative_paths = m_useRelativePathsOpt;
        cfg->m_ExportVrml.use_plain_pcb      = m_usePlainPCBOpt;
        cfg->m_ExportVrml.ref_units          = m_VRML_RefUnitChoice->GetSelection();
        cfg->m_ExportVrml.origin_mode        = m_rbCoordOrigin->GetSelection();

        double val = 0.0;
        m_VRML_Xref->GetValue().ToDouble( &val );
        cfg->m_ExportVrml.ref_x = val;

        m_VRML_Yref->GetValue().ToDouble( &val );
        cfg->m_ExportVrml.ref_y = val;
    };

    void SetSubdir( const wxString & aDir )
    {
        m_SubdirNameCtrl->SetValue( aDir );
    }

    wxString GetSubdir3Dshapes()
    {
        return m_SubdirNameCtrl->GetValue();
    }

    wxFilePickerCtrl* FilePicker()
    {
        return m_filePicker;
    }

    int GetRefUnitsChoice()
    {
        return m_VRML_RefUnitChoice->GetSelection();
    }

    int GetOriginChoice()
    {
        return m_rbCoordOrigin->GetSelection();
    }

    double GetXRef()
    {
        return DoubleValueFromString( EDA_UNITS::UNSCALED, m_VRML_Xref->GetValue() );
    }

    double GetYRef()
    {
        return DoubleValueFromString( EDA_UNITS::UNSCALED, m_VRML_Yref->GetValue() );
    }

    int GetUnits()
    {
        return m_unitsOpt = m_rbSelectUnits->GetSelection();
    }

    bool GetCopyFilesOption()
    {
        return m_copy3DFilesOpt = m_cbCopyFiles->GetValue();
    }

    bool GetUseRelativePathsOption()
    {
        return m_useRelativePathsOpt = m_cbUseRelativePaths->GetValue();
    }

    bool GetUsePlainPCBOption()
    {
        return m_usePlainPCBOpt = m_cbPlainPCB->GetValue();
    }

    void OnUpdateUseRelativePath( wxUpdateUIEvent& event ) override
    {
        // Making path relative or absolute has no meaning when VRML files are not copied.
        event.Enable( m_cbCopyFiles->GetValue() );
    }

    bool TransferDataFromWindow() override;
};


bool DIALOG_EXPORT_3DFILE::TransferDataFromWindow()
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


void PCB_EDIT_FRAME::OnExportVRML( wxCommandEvent& event )
{
    // These variables are static to keep info during the session.
    static wxString subDirFor3Dshapes;

    // Build default output file name
    wxString path = GetLastPath( LAST_PATH_VRML );

    if( path.IsEmpty() )
    {
        wxFileName brdFile = GetBoard()->GetFileName();
        brdFile.SetExt( "wrl" );
        path = brdFile.GetFullPath();
    }

    if( subDirFor3Dshapes.IsEmpty() )
        subDirFor3Dshapes = wxT( "shapes3D" );

    // The general VRML scale factor
    // Assuming the VRML default unit is the mm
    // this is the mm to VRML scaling factor for mm, 0.1 inch, and inch
    double scaleList[4] = { 1.0, 0.001, 10.0/25.4, 1.0/25.4 };

    DIALOG_EXPORT_3DFILE dlg( this );
    dlg.FilePicker()->SetPath( path );
    dlg.SetSubdir( subDirFor3Dshapes );

    if( dlg.ShowModal() != wxID_OK )
        return;

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
        BOARD* pcb = GetBoard();
        wxPoint center = pcb->GetBoundingBox().GetCenter();
        aXRef = Iu2Millimeter( center.x );
        aYRef = Iu2Millimeter( center.y );
    }

    double scale = scaleList[dlg.GetUnits()];     // final scale export
    bool export3DFiles = dlg.GetCopyFilesOption();
    bool useRelativePaths = dlg.GetUseRelativePathsOption();
    bool usePlainPCB = dlg.GetUsePlainPCBOption();

    path = dlg.FilePicker()->GetPath();
    SetLastPath( LAST_PATH_VRML, path );
    wxFileName modelPath = path;

    wxBusyCursor dummy;

    subDirFor3Dshapes = dlg.GetSubdir3Dshapes();
    modelPath.AppendDir( subDirFor3Dshapes );

    if( export3DFiles && !modelPath.DirExists() )
    {
        if( !modelPath.Mkdir() )
        {
            wxString msg = wxString::Format(
                    _( "Unable to create directory \"%s\"" ), modelPath.GetPath() );
            DisplayErrorMessage( this, msg );
            return;
        }
    }

    if( !ExportVRML_File( path, scale, export3DFiles, useRelativePaths,
                          usePlainPCB, modelPath.GetPath(), aXRef, aYRef ) )
    {
        wxString msg = wxString::Format( _( "Unable to create file \"%s\"" ), path );
        DisplayErrorMessage( this, msg );
        return;
    }
}
