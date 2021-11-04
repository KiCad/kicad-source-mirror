/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/choicdlg.h>
#include <wx/stdpaths.h>
#include <wx/process.h>
#include <wx/string.h>

#include <pgm_base.h>
#include <board.h>
#include <confirm.h>
#include <dialog_export_step_base.h>
#include <footprint.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <math/vector3.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <reporter.h>
#include <widgets/text_ctrl_eval.h>
#include <wildcards_and_files_ext.h>
#include <filename_resolver.h>

#ifdef KICAD_STEP_EXPORT_LIB
#include <kicad2step.h>
#endif

class DIALOG_EXPORT_STEP: public DIALOG_EXPORT_STEP_BASE
{
public:
    enum STEP_ORG_OPT
    {
        STEP_ORG_0,             // absolute coordinates
        STEP_ORG_PLOT_AXIS,     // origin is plot/drill axis origin
        STEP_ORG_GRID_AXIS,     // origin is grid origin
        STEP_ORG_BOARD_CENTER,  // origin is board center
        STEP_ORG_USER,          // origin is entered by user
    };

private:
    PCB_EDIT_FRAME* m_parent;
    // The last preference for STEP Origin:
    STEP_ORG_OPT m_STEP_org_opt;
    bool   m_noVirtual;     // remember last preference for No Virtual Component
    int    m_OrgUnits;      // remember last units for User Origin
    double m_XOrg;          // remember last User Origin X value
    double m_YOrg;          // remember last User Origin Y value
    wxString m_boardPath;   // path to the exported board file

protected:
    void onUpdateUnits( wxUpdateUIEvent& aEvent ) override;
    void onUpdateXPos( wxUpdateUIEvent& aEvent ) override;
    void onUpdateYPos( wxUpdateUIEvent& aEvent ) override;
    void onExportButton( wxCommandEvent& aEvent ) override;

    int GetOrgUnitsChoice() const
    {
        return m_STEP_OrgUnitChoice->GetSelection();
    }

    double GetXOrg() const
    {
        return DoubleValueFromString( EDA_UNITS::UNSCALED, m_STEP_Xorg->GetValue() );
    }

    double GetYOrg()
    {
        return DoubleValueFromString( EDA_UNITS::UNSCALED, m_STEP_Yorg->GetValue() );
    }

    STEP_ORG_OPT GetOriginOption();

    bool GetNoVirtOption()
    {
        return m_cbRemoveVirtual->GetValue();
    }

    bool GetSubstOption()
    {
        return m_cbSubstModels->GetValue();
    }

    bool GetOverwriteFile()
    {
        return m_cbOverwriteFile->GetValue();
    }

public:
    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aParent, const wxString& aBoardPath );
    ~DIALOG_EXPORT_STEP();
};


DIALOG_EXPORT_STEP::DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aParent, const wxString& aBoardPath ) :
    DIALOG_EXPORT_STEP_BASE( aParent )
{
    m_parent = aParent;
    m_boardPath = aBoardPath;
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizerOK->SetLabel( _( "Export" ) );
    m_sdbSizer->Layout();

    // Build default output file name
    wxString path = m_parent->GetLastPath( LAST_PATH_STEP );

    if( path.IsEmpty() )
    {
        wxFileName brdFile = m_parent->GetBoard()->GetFileName();
        brdFile.SetExt( "step" );
        path = brdFile.GetFullPath();
    }

    // Reset this picker bc wxFormBuilder doesn't allow untranslated strings
    wxSizerItem* sizer_item = bSizerTop->GetItem( 1UL );
    wxWindow* widget = sizer_item->GetWindow();
    bSizerTop->Hide( widget );
    widget->Destroy();

    m_filePickerSTEP = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString,
            _( "Select a STEP export filename" ),
            _( "STEP files" ) + AddFileExtListToFilter( { "STEP", "STP" } ),
            wxDefaultPosition,
            wxSize( -1, -1 ), wxFLP_SAVE | wxFLP_USE_TEXTCTRL );
    bSizerTop->Add( m_filePickerSTEP, 1, wxTOP|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

    m_filePickerSTEP->SetPath( path );

    Layout();
    bSizerSTEPFile->Fit( this );

    SetFocus();

    auto cfg = m_parent->GetPcbNewSettings();

    m_STEP_org_opt = static_cast<STEP_ORG_OPT>( cfg->m_ExportStep.origin_mode );

    switch( m_STEP_org_opt )
    {
        default: break;
        case STEP_ORG_PLOT_AXIS: m_rbDrillAndPlotOrigin->SetValue( true ); break;
        case STEP_ORG_GRID_AXIS: m_rbGridOrigin->SetValue( true ); break;
        case STEP_ORG_USER: m_rbUserDefinedOrigin->SetValue( true ); break;
        case STEP_ORG_BOARD_CENTER: m_rbBoardCenterOrigin->SetValue( true ); break;
    }

    m_OrgUnits  = cfg->m_ExportStep.origin_units;
    m_XOrg      = cfg->m_ExportStep.origin_x;
    m_YOrg      = cfg->m_ExportStep.origin_y;
    m_noVirtual = cfg->m_ExportStep.no_virtual;

    m_cbRemoveVirtual->SetValue( m_noVirtual );
    m_cbSubstModels->SetValue( cfg->m_ExportStep.replace_models );
    m_cbOverwriteFile->SetValue( cfg->m_ExportStep.overwrite_file );

    m_STEP_OrgUnitChoice->SetSelection( m_OrgUnits );
    wxString tmpStr;
    tmpStr << m_XOrg;
    m_STEP_Xorg->SetValue( tmpStr );
    tmpStr = "";
    tmpStr << m_YOrg;
    m_STEP_Yorg->SetValue( tmpStr );

    wxString bad_scales;
    size_t   bad_count = 0;

    for( auto& fp : aParent->GetBoard()->Footprints() )
    {
        for( auto& model : fp->Models() )
        {

            if( model.m_Scale.x != 1.0 ||
                model.m_Scale.y != 1.0 ||
                model.m_Scale.z != 1.0 )
            {
                bad_scales.Append( wxS("\n") );
                bad_scales.Append( model.m_Filename );
                bad_count++;
            }
        }

        if( bad_count >= 5 )
            break;
    }

    if( !bad_scales.empty()
            && !Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning )
    {
        wxString extendedMsg = _( "Non-unity scaled models:" ) + "\n" + bad_scales;

        KIDIALOG msgDlg( m_parent, _( "Scaled models detected.  "
                                      "Model scaling is not reliable for mechanical export." ),
                         _( "Model Scale Warning" ), wxOK | wxICON_WARNING );
        msgDlg.SetExtendedMessage( extendedMsg );
        msgDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        msgDlg.ShowModal();

        if( msgDlg.DoNotShowAgain() )
            Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning = true;
    }
    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_STEP::~DIALOG_EXPORT_STEP()
{
    GetOriginOption(); // Update m_STEP_org_opt member.

    auto cfg = m_parent->GetPcbNewSettings();

    cfg->m_ExportStep.origin_mode = static_cast<int>( m_STEP_org_opt );
    cfg->m_ExportStep.origin_units = m_STEP_OrgUnitChoice->GetSelection();
    cfg->m_ExportStep.replace_models = m_cbSubstModels->GetValue();
    cfg->m_ExportStep.overwrite_file = m_cbOverwriteFile->GetValue();

    double val = 0.0;

    m_STEP_Xorg->GetValue().ToDouble( &val );
    cfg->m_ExportStep.origin_x = val;

    m_STEP_Yorg->GetValue().ToDouble( &val );
    cfg->m_ExportStep.origin_y = val;

    cfg->m_ExportStep.no_virtual = m_cbRemoveVirtual->GetValue();
}


DIALOG_EXPORT_STEP::STEP_ORG_OPT DIALOG_EXPORT_STEP::GetOriginOption()
{
    m_STEP_org_opt = STEP_ORG_0;

    if( m_rbDrillAndPlotOrigin->GetValue() )
        m_STEP_org_opt = STEP_ORG_PLOT_AXIS;
    else if( m_rbGridOrigin->GetValue() )
        m_STEP_org_opt = STEP_ORG_GRID_AXIS;
    else if( m_rbUserDefinedOrigin->GetValue() )
        m_STEP_org_opt = STEP_ORG_USER;
    else if( m_rbBoardCenterOrigin->GetValue() )
        m_STEP_org_opt = STEP_ORG_BOARD_CENTER;

    return m_STEP_org_opt;
}


void PCB_EDIT_FRAME::OnExportSTEP( wxCommandEvent& event )
{
    wxFileName brdFile = GetBoard()->GetFileName();

    if( GetScreen()->IsContentModified() || brdFile.GetFullPath().empty() )
    {
        if( !doAutoSave() )
        {
            DisplayErrorMessage( this, _( "STEP export failed!  "
                                          "Please save the PCB and try again" ) );
            return;
        }

        // Use auto-saved board for export
        brdFile.SetName( GetAutoSaveFilePrefix() + brdFile.GetName() );
    }

    DIALOG_EXPORT_STEP dlg( this, brdFile.GetFullPath() );
    dlg.ShowModal();
}


void DIALOG_EXPORT_STEP::onUpdateUnits( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onUpdateXPos( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onUpdateYPos( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onExportButton( wxCommandEvent& aEvent )
{
    m_parent->SetLastPath( LAST_PATH_STEP, m_filePickerSTEP->GetPath() );

    double tolerance = 0.01;   // default value in mm

    switch( m_tolerance->GetSelection() )
    {
    case 0:         // small
        tolerance = 0.001;
        break;

    default:
    case 1: break;  // Normal

    case 2:         // large
        tolerance = 0.1;
        break;
    }

    SHAPE_POLY_SET outline;
    wxString msg;

    // Check if the board outline is continuous
    // max dist from one endPt to next startPt to build a closed shape:
    int chainingEpsilon = Millimeter2iu( tolerance );
    // Arc to segment approx error (not critical here: we do not use the outline shape):
    int maxError = Millimeter2iu( 0.005 );
    bool success = BuildBoardPolygonOutlines( m_parent->GetBoard(), outline, maxError,
                                              chainingEpsilon, nullptr );
    if( !success )
    {
        DisplayErrorMessage( this, wxString::Format(
                             _( "Board outline is missing or not closed using %.3f mm tolerance.\n"
                                "Run DRC for a full analysis." ), tolerance ) );
        return;
    }

    wxFileName fn = m_filePickerSTEP->GetFileName();

    if( fn.FileExists() && !GetOverwriteFile() )
    {
        msg.Printf( _( "File '%s' already exists. Do you want overwrite this file?" ),
                    fn.GetFullPath() );

        if( wxMessageBox( msg, _( "STEP Export" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
            return;
    }

    FILENAME_RESOLVER* fnResolver = m_parent->Prj().Get3DFilenameResolver();

    fnResolver->WritePathList( wxStandardPaths::Get().GetTempDir(), "ExportPaths.cfg", true );

    DIALOG_EXPORT_STEP::STEP_ORG_OPT orgOpt = GetOriginOption();
    double xOrg = 0.0;
    double yOrg = 0.0;

#ifndef KICAD_STEP_EXPORT_LIB
    wxFileName appK2S( wxStandardPaths::Get().GetExecutablePath() );

#ifdef __WXMAC__
    // On macOS, we have standalone applications inside the main bundle, so we handle that here:
        if( appK2S.GetPath().Find( "/Contents/Applications/pcbnew.app/Contents/MacOS" ) != wxNOT_FOUND )
        {
            appK2S.AppendDir( ".." );
            appK2S.AppendDir( ".." );
            appK2S.AppendDir( ".." );
            appK2S.AppendDir( ".." );
            appK2S.AppendDir( "MacOS" );
        }
#endif

    appK2S.SetName( "kicad2step" );

    wxString cmdK2S = "\"";
    cmdK2S.Append( appK2S.GetFullPath() );
    cmdK2S.Append( "\"" );

    if( GetNoVirtOption() )
        cmdK2S.Append( " --no-virtual" );

    if( GetSubstOption() )
        cmdK2S.Append( " --subst-models" );

    switch( orgOpt )
    {
        case DIALOG_EXPORT_STEP::STEP_ORG_0:
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_PLOT_AXIS:
            cmdK2S.Append( " --drill-origin" );
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_GRID_AXIS:
            cmdK2S.Append( " --grid-origin" );
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_USER:
        {
            xOrg = GetXOrg();
            yOrg = GetYOrg();

            if( GetOrgUnitsChoice() == 1 )
            {
                // selected reference unit is in inches, and STEP units are mm
                xOrg *= 25.4;
                yOrg *= 25.4;
            }

            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( " --user-origin='%.6f x %.6f'", xOrg, yOrg ) );
        }
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_BOARD_CENTER:
        {
            EDA_RECT bbox = m_parent->GetBoard()->ComputeBoundingBox( true );
            xOrg = Iu2Millimeter( bbox.GetCenter().x );
            yOrg = Iu2Millimeter( bbox.GetCenter().y );
            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( " --user-origin='%.6f x %.6f'", xOrg, yOrg ) );
        }
            break;
    }

    {
        LOCALE_IO dummy;
        cmdK2S.Append( wxString::Format( " --min-distance='%.3f mm'", tolerance ) );
    }

    cmdK2S.Append( " -f -o " );
    cmdK2S.Append( wxString::Format("'%s'", m_filePickerSTEP->GetPath() ) );  // input file path

    cmdK2S.Append( " " );
    cmdK2S.Append( wxString::Format("'%s'", m_boardPath ) );                  // output file path

    wxExecute( cmdK2S, wxEXEC_ASYNC  | wxEXEC_SHOW_CONSOLE );

    #else

    KICAD2MCAD_PRMS params;
    params.m_filename = m_boardPath;
    params.m_outputFile = m_filePickerSTEP->GetPath();

    params.m_includeVirtual = !GetNoVirtOption();

    params.m_substModels = GetSubstOption();
    params.m_minDistance = tolerance;
    params.m_overwrite = true;

    switch( orgOpt )
    {
        case DIALOG_EXPORT_STEP::STEP_ORG_0:
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_PLOT_AXIS:
            params.m_useDrillOrigin = true;
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_GRID_AXIS:
            params.m_useGridOrigin = true;
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_USER:
        {
            xOrg = GetXOrg();
            yOrg = GetYOrg();

            if( GetOrgUnitsChoice() == 1 )
            {
                // selected reference unit is in inches, and STEP units are mm
                xOrg *= 25.4;
                yOrg *= 25.4;
            }

            params.m_xOrigin = xOrg;
            params.m_yOrigin = yOrg;
        }
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_BOARD_CENTER:
        {
            EDA_RECT bbox = m_parent->GetBoard()->ComputeBoundingBox( true );
            xOrg = Iu2Millimeter( bbox.GetCenter().x );
            yOrg = Iu2Millimeter( bbox.GetCenter().y );
            params.m_xOrigin = xOrg;
            params.m_yOrigin = yOrg;
        }
            break;
    }

    KICAD2STEP converter( params );
    converter.Run();
    #endif

    aEvent.Skip(); // Close the dialog
}
