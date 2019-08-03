/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_edit_frame.h"
#include "kiface_i.h"
#include "confirm.h"
#include "reporter.h"
#include "class_board.h"
#include "dialog_export_step_base.h"
#include <widgets/text_ctrl_eval.h>
#include <wx_html_report_panel.h>

#define OPTKEY_STEP_ORIGIN_OPT      "STEP_Origin_Opt"
#define OPTKEY_STEP_UORG_UNITS      "STEP_UserOriginUnits"
#define OPTKEY_STEP_UORG_X          "STEP_UserOriginX"
#define OPTKEY_STEP_UORG_Y          "STEP_UserOriginY"
#define OPTKEY_STEP_NOVIRT          "STEP_NoVirtual"


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
    wxConfigBase* m_config;
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
        return DoubleValueFromString( UNSCALED_UNITS, m_STEP_Xorg->GetValue() );
    }

    double GetYOrg()
    {
        return DoubleValueFromString( UNSCALED_UNITS, m_STEP_Yorg->GetValue() );
    }

    STEP_ORG_OPT GetOriginOption();

    bool GetNoVirtOption()
    {
        return m_cbRemoveVirtual->GetValue();
    }

public:
    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aParent, const wxString& aBoardPath );

    ~DIALOG_EXPORT_STEP()
    {
        GetOriginOption(); // Update m_STEP_org_opt member.
        m_config->Write( OPTKEY_STEP_ORIGIN_OPT, (int)m_STEP_org_opt );
        m_config->Write( OPTKEY_STEP_NOVIRT, m_cbRemoveVirtual->GetValue() );
        m_config->Write( OPTKEY_STEP_UORG_UNITS, m_STEP_OrgUnitChoice->GetSelection() );
        m_config->Write( OPTKEY_STEP_UORG_X, m_STEP_Xorg->GetValue() );
        m_config->Write( OPTKEY_STEP_UORG_Y, m_STEP_Yorg->GetValue() );
    }
};


DIALOG_EXPORT_STEP::DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aParent, const wxString& aBoardPath ) :
    DIALOG_EXPORT_STEP_BASE( aParent )
{
    m_parent = aParent;
    m_boardPath = aBoardPath;
    m_config = Kiface().KifaceSettings();
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

    m_filePickerSTEP->SetPath( path );

    SetFocus();

    m_STEP_org_opt = STEP_ORG_0;;
    int tmp = STEP_ORG_0;

    if( m_config->Read( OPTKEY_STEP_ORIGIN_OPT, &tmp ) )
        m_STEP_org_opt = (STEP_ORG_OPT) tmp;

    switch( m_STEP_org_opt )
    {
        default: break;
        case STEP_ORG_PLOT_AXIS: m_rbDrillAndPlotOrigin->SetValue( true ); break;
        case STEP_ORG_GRID_AXIS: m_rbGridOrigin->SetValue( true ); break;
        case STEP_ORG_USER: m_rbUserDefinedOrigin->SetValue( true ); break;
        case STEP_ORG_BOARD_CENTER: m_rbBoardCenterOrigin->SetValue( true ); break;
    }

    m_config->Read( OPTKEY_STEP_UORG_UNITS, &m_OrgUnits, 0 );
    m_config->Read( OPTKEY_STEP_UORG_X, &m_XOrg, 0.0 );
    m_config->Read( OPTKEY_STEP_UORG_Y, &m_YOrg, 0.0 );
    m_config->Read( OPTKEY_STEP_NOVIRT, &m_noVirtual );
    m_cbRemoveVirtual->SetValue( m_noVirtual );

    m_STEP_OrgUnitChoice->SetSelection( m_OrgUnits );
    wxString tmpStr;
    tmpStr << m_XOrg;
    m_STEP_Xorg->SetValue( tmpStr );
    tmpStr = "";
    tmpStr << m_YOrg;
    m_STEP_Yorg->SetValue( tmpStr );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
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

    if( GetScreen()->IsModify() || brdFile.GetFullPath().empty() )
    {
        if( !doAutoSave() )
        {
            DisplayErrorMessage( this,
                                 _( "STEP export failed!  Please save the PCB and try again" ) );
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

extern bool BuildBoardPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                wxString* aErrorText, unsigned int aTolerance,
                                wxPoint* aErrorLocation = nullptr );

void DIALOG_EXPORT_STEP::onExportButton( wxCommandEvent& aEvent )
{
    m_parent->SetLastPath( LAST_PATH_STEP, m_filePickerSTEP->GetPath() );

    SHAPE_POLY_SET outline;
    wxString msg;

    // Check if the board outline is continuous
    if( !BuildBoardPolygonOutlines( m_parent->GetBoard(), outline, &msg, Millimeter2iu( 0.01 ) ) )
    {
        DisplayErrorMessage( this, _( "Cannot determine the board outline." ), msg );
        return;
    }

    wxFileName fn = m_filePickerSTEP->GetFileName();

    if( fn.FileExists() )
    {
        msg.Printf( _( "File '%s' already exists. Do you want overwrite this file?" ),
                    fn.GetFullPath().GetData() );

        if( wxMessageBox( msg, _( "STEP Export" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
            return;
    }

    DIALOG_EXPORT_STEP::STEP_ORG_OPT orgOpt = GetOriginOption();
    double xOrg = 0.0;
    double yOrg = 0.0;

    wxFileName appK2S( wxStandardPaths::Get().GetExecutablePath() );

#ifdef __WXMAC__
    // On macOS, we have standalone applications inside the main bundle, so we handle that here:
        if( appK2S.GetPath().find( "/Contents/Applications/pcbnew.app/Contents/MacOS" ) != wxNOT_FOUND )
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
            cmdK2S.Append( wxString::Format( " --user-origin=\"%.6f x %.6f\"", xOrg, yOrg ) );
        }
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_BOARD_CENTER:
        {
            EDA_RECT bbox = m_parent->GetBoard()->ComputeBoundingBox( true );
            xOrg = Iu2Millimeter( bbox.GetCenter().x );
            yOrg = Iu2Millimeter( bbox.GetCenter().y );
            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( " --user-origin=\"%.6f x %.6f\"", xOrg, yOrg ) );
        }
            break;
    }

    if( m_tolerance->GetSelection() != 1 )
    {
        LOCALE_IO dummy;
        double tolerance = 0.001 * std::pow<double>( 10.0, m_tolerance->GetSelection() - 1 );
        cmdK2S.Append( wxString::Format( " --min-distance=\"%.4f mm\"", tolerance ) );
    }

    cmdK2S.Append( " -f -o " );
    cmdK2S.Append( wxString::Format("\"%s\"", m_filePickerSTEP->GetPath() ) );  // input file path

    cmdK2S.Append( " " );
    cmdK2S.Append( wxString::Format("\"%s\"", m_boardPath ) );                  // output file path

    int result = 0;
    bool success = false;
    wxArrayString output, errors;
    REPORTER& reporter = m_messagesPanel->Reporter();
    reporter.ReportHead( wxString::Format( _( "Executing '%s'" ), cmdK2S ), REPORTER::RPT_ACTION );

    {
        wxBusyCursor dummy;
        result = wxExecute( cmdK2S, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE );
    }

    // Check the output log for an indication of success,
    // the value returned by wxExecute is not conclusive
    for( auto& l : output )
    {
        if( l.Contains( "Done" ) )
        {
            success = true;
            break;
        }
    }

    for( auto& err : errors )
        reporter.Report( err, REPORTER::RPT_WARNING );

    if( result )    // Any troubles?
    {
        if( !success )
        {
            reporter.ReportTail( _( "Unable to create STEP file.  Check that the board has a "
                                        "valid outline and models." ), REPORTER::RPT_ERROR );
        }
        else
        {
            reporter.ReportTail( _( "STEP file has been created, but there are warnings." ),
                    REPORTER::RPT_INFO );
        }
    }
    else
    {
        reporter.ReportTail( _( "STEP file has been created successfully." ), REPORTER::RPT_INFO );
    }
}
