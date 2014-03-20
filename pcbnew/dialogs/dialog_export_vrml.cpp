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

#define OPTKEY_OUTPUT_UNIT wxT("VrmlExportUnit" )
#define OPTKEY_3DFILES_OPT wxT("VrmlExport3DShapeFilesOpt" )

class DIALOG_EXPORT_3DFILE : public DIALOG_EXPORT_3DFILE_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    wxConfigBase* m_config;
    int m_unitsOpt;          // to remember last option
    int m_3DFilesOpt;        // to remember last option

    void OnCancelClick( wxCommandEvent& event ){ EndModal( wxID_CANCEL ); }
    void OnOkClick( wxCommandEvent& event ){ EndModal( wxID_OK ); }

public:
    DIALOG_EXPORT_3DFILE( PCB_EDIT_FRAME* parent ) :
        DIALOG_EXPORT_3DFILE_BASE( parent )
    {
        m_parent = parent;
        m_config = Kiface().KifaceSettings();
        SetFocus();
        m_config->Read( OPTKEY_OUTPUT_UNIT, &m_unitsOpt );
        m_config->Read( OPTKEY_3DFILES_OPT, &m_3DFilesOpt );
        m_rbSelectUnits->SetSelection(m_unitsOpt);
        m_rb3DFilesOption->SetSelection(m_3DFilesOpt);
        GetSizer()->SetSizeHints( this );
        Centre();
    }
    ~DIALOG_EXPORT_3DFILE()
    {
        m_unitsOpt = GetUnits( );
        m_3DFilesOpt = Get3DFilesOption( );
        m_config->Write( OPTKEY_OUTPUT_UNIT, m_unitsOpt );
        m_config->Write( OPTKEY_3DFILES_OPT, m_3DFilesOpt );
    };

    void SetSubdir( const wxString & aDir )
    {
        m_SubdirNameCtrl->SetValue( aDir);
    }

    wxString GetSubdir( )
    {
        return m_SubdirNameCtrl->GetValue( );
    }

    wxFilePickerCtrl* FilePicker()
    {
        return m_filePicker;
    }

    int GetUnits( )
    {
        return m_unitsOpt = m_rbSelectUnits->GetSelection();
    }

    int Get3DFilesOption( )
    {
        return m_3DFilesOpt = m_rb3DFilesOption->GetSelection();
    }
};


/**
 * Function OnExportVRML
 * will export the current BOARD to a VRML file.
 */
void PCB_EDIT_FRAME::OnExportVRML( wxCommandEvent& event )
{
    wxFileName fn;
    static wxString subDirFor3Dshapes = wxT("shapes3D");

    // The general VRML scale factor
    // Assuming the VRML default unit is the mm
    // this is the mm to VRML scaling factor for inch, mm and meter
    double scaleList[3] = { 1.0/25.4, 1, 0.001 };

    // Build default file name
    wxString ext = wxT( "wrl" );
    fn = GetBoard()->GetFileName();
    fn.SetExt( ext );

    DIALOG_EXPORT_3DFILE dlg( this );
    dlg.FilePicker()->SetPath( fn.GetFullPath() );
    dlg.SetSubdir( subDirFor3Dshapes );

    if( dlg.ShowModal() != wxID_OK )
        return;

    double scale = scaleList[dlg.GetUnits( )];     // final scale export
    bool export3DFiles = dlg.Get3DFilesOption( ) == 0;

    wxBusyCursor dummy;

    wxString fullFilename = dlg.FilePicker()->GetPath();
    subDirFor3Dshapes = dlg.GetSubdir();

    if( export3DFiles && !wxDirExists( subDirFor3Dshapes ) )
        wxMkdir( subDirFor3Dshapes );

    if( ! ExportVRML_File( fullFilename, scale, export3DFiles, subDirFor3Dshapes ) )
    {
        wxString msg = _( "Unable to create " ) + fullFilename;
        wxMessageBox( msg );
        return;
    }
}
