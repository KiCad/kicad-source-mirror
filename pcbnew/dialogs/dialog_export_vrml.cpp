/**
 * @file dialog_export_vrml.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2013  Lorenzo Mercantonio
 * Copyright (C) 2013 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
#include <fctsys.h>
#include <wxPcbStruct.h>
#include <kiface_i.h>
#include <pcbnew.h>
#include <class_board.h>


/* the dialog to create VRML files, derived from DIALOG_EXPORT_3DFILE_BASE,
 * created by wxFormBuilder
 */
#include <dialog_export_vrml_base.h> // the wxFormBuilder header file

#define OPTKEY_OUTPUT_UNIT wxT( "VrmlExportUnit" )
#define OPTKEY_3DFILES_OPT wxT( "VrmlExportCopyFiles" )
#define OPTKEY_USE_ABS_PATHS wxT( "VrmlUseRelativePaths" )


class DIALOG_EXPORT_3DFILE : public DIALOG_EXPORT_3DFILE_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    wxConfigBase*   m_config;
    int             m_unitsOpt;          // Remember last units option
    bool            m_copy3DFilesOpt;    // Remember last copy model files option
    bool            m_useRelativePathsOpt;    // Remember last use absolut paths option

public:
    DIALOG_EXPORT_3DFILE( PCB_EDIT_FRAME* parent ) :
        DIALOG_EXPORT_3DFILE_BASE( parent )
    {
        m_parent = parent;
        m_config = Kiface().KifaceSettings();
        m_filePicker->SetFocus();
        m_config->Read( OPTKEY_OUTPUT_UNIT, &m_unitsOpt );
        m_config->Read( OPTKEY_3DFILES_OPT, &m_copy3DFilesOpt );
        m_config->Read( OPTKEY_USE_ABS_PATHS, &m_useRelativePathsOpt );
        m_rbSelectUnits->SetSelection( m_unitsOpt );
        m_cbCopyFiles->SetValue( m_copy3DFilesOpt );
        m_cbUseAbsolutePaths->SetValue( m_useRelativePathsOpt );
        wxButton* okButton = (wxButton*) FindWindowByLabel( wxT( "OK" ) );

        if( okButton )
            SetDefaultItem( okButton );

        GetSizer()->SetSizeHints( this );
        Centre();

        Connect( ID_USE_ABS_PATH, wxEVT_UPDATE_UI,
                 wxUpdateUIEventHandler( DIALOG_EXPORT_3DFILE::OnUpdateUseAbsolutPath ) );
    }

    ~DIALOG_EXPORT_3DFILE()
    {
        m_unitsOpt = GetUnits();
        m_copy3DFilesOpt = GetCopyFilesOption();
        m_config->Write( OPTKEY_OUTPUT_UNIT, m_unitsOpt );
        m_config->Write( OPTKEY_3DFILES_OPT, m_copy3DFilesOpt );
        m_config->Write( OPTKEY_USE_ABS_PATHS, m_useRelativePathsOpt );
    };

    void SetSubdir( const wxString & aDir )
    {
        m_SubdirNameCtrl->SetValue( aDir );
    }

    wxString GetSubdir()
    {
        return m_SubdirNameCtrl->GetValue();
    }

    wxFilePickerCtrl* FilePicker()
    {
        return m_filePicker;
    }

    int GetUnits()
    {
        return m_unitsOpt = m_rbSelectUnits->GetSelection();
    }

    bool GetCopyFilesOption()
    {
        return m_copy3DFilesOpt = m_cbCopyFiles->GetValue();
    }

    bool GetUseAbsolutePathsOption()
    {
        return m_useRelativePathsOpt = m_cbUseAbsolutePaths->GetValue();
    }

    void OnUpdateUseAbsolutPath( wxUpdateUIEvent& event )
    {
        // Making path relative or absolute has no meaning when VRML files are not copied.
        event.Enable( m_cbCopyFiles->GetValue() );
    }
};


void PCB_EDIT_FRAME::OnExportVRML( wxCommandEvent& event )
{
    wxFileName fn;
    wxString   projectPath;

    if( !wxGetEnv( wxT( "KIPRJMOD" ), &projectPath ) )
        projectPath = wxFileName::GetCwd();

    static wxString subDirFor3Dshapes;

    if( subDirFor3Dshapes.IsEmpty() )
    {
        subDirFor3Dshapes = wxT( "shapes3D" );
    }

    // The general VRML scale factor
    // Assuming the VRML default unit is the mm
    // this is the mm to VRML scaling factor for inch, mm and meter
    double scaleList[3] = { 1.0/25.4, 1, 0.001 };

    // Build default file name
    fn = GetBoard()->GetFileName();
    fn.SetExt( wxT( "wrl" ) );

    DIALOG_EXPORT_3DFILE dlg( this );
    dlg.FilePicker()->SetPath( fn.GetFullPath() );
    dlg.SetSubdir( subDirFor3Dshapes );

    if( dlg.ShowModal() != wxID_OK )
        return;

    double scale = scaleList[dlg.GetUnits()];     // final scale export
    bool export3DFiles = dlg.GetCopyFilesOption();
    bool useRelativePaths = dlg.GetUseAbsolutePathsOption();
    wxString fullFilename = dlg.FilePicker()->GetPath();
    wxFileName modelPath = fullFilename;
    wxBusyCursor dummy;

    modelPath.AppendDir( dlg.GetSubdir() );
    subDirFor3Dshapes = dlg.GetSubdir();

    wxLogDebug( wxT( "Exporting enabled=%d to %s." ),
                export3DFiles, GetChars( subDirFor3Dshapes ) );

    if( export3DFiles && !modelPath.DirExists() )
    {
        modelPath.Mkdir();
    }

    if( !ExportVRML_File( fullFilename, scale, export3DFiles, useRelativePaths,
                          modelPath.GetPath() ) )
    {
        wxString msg = _( "Unable to create " ) + fullFilename;
        wxMessageBox( msg );
        return;
    }
}
