/**
 * @file dialog_export_step.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "wxPcbStruct.h"
#include "kiface_i.h"
#include "confirm.h"

#include "pcbnew.h"
#include "class_board.h"
#include "dialog_export_step_base.h"
#include <widgets/text_ctrl_eval.h>

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

public:
    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* parent );

    ~DIALOG_EXPORT_STEP()
    {
        GetOriginOption(); // Update m_STEP_org_opt member.
        m_config->Write( OPTKEY_STEP_ORIGIN_OPT, (int)m_STEP_org_opt );

        m_config->Write( OPTKEY_STEP_NOVIRT, m_cbRemoveVirtual->GetValue() );

        m_config->Write( OPTKEY_STEP_UORG_UNITS, m_STEP_OrgUnitChoice->GetSelection() );
        m_config->Write( OPTKEY_STEP_UORG_X, m_STEP_Xorg->GetValue() );
        m_config->Write( OPTKEY_STEP_UORG_Y, m_STEP_Yorg->GetValue() );
    }

    wxFilePickerCtrl* FilePicker()
    {
        return m_filePickerSTEP;
    }

    int GetOrgUnitsChoice()
    {
        return m_STEP_OrgUnitChoice->GetSelection();
    }

    double GetXOrg()
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

    bool TransferDataFromWindow() override;
	void onSelectOrigin( wxCommandEvent& event ) override;
};

DIALOG_EXPORT_STEP::DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* parent ) :
        DIALOG_EXPORT_STEP_BASE( parent )
{
    m_parent = parent;
    m_config = Kiface().KifaceSettings();
    SetFocus();

    m_STEP_org_opt = STEP_ORG_0;;
    int tmp = STEP_ORG_0;

    if( m_config->Read( OPTKEY_STEP_ORIGIN_OPT, &tmp ) )
        m_STEP_org_opt = (STEP_ORG_OPT) tmp;

    switch( m_STEP_org_opt )
    {
        default: break;
        case STEP_ORG_PLOT_AXIS: m_cbPlotOrigin->SetValue( true ); break;
        case STEP_ORG_GRID_AXIS: m_cbGridOrigin->SetValue( true ); break;
        case STEP_ORG_USER: m_cbUserOrigin->SetValue( true ); break;
        case STEP_ORG_BOARD_CENTER: m_cbBoardCenter->SetValue( true ); break;
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

    m_STEP_OrgUnitChoice->Enable( m_cbUserOrigin->IsChecked() );
    m_STEP_Xorg->Enable( m_cbUserOrigin->IsChecked() );
    m_STEP_Yorg->Enable( m_cbUserOrigin->IsChecked() );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_EXPORT_STEP::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    wxFileName fn = m_filePickerSTEP->GetFileName();

    if( fn.FileExists() )
    {
        wxString msg;
        msg.Printf( _( "File: %s\n"
                       "already exists. Do you want overwrite this file?" ),
                    fn.GetFullPath().GetData() );

        if( wxMessageBox( msg, _( "STEP Export" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
            return false;
    }

    return true;
}


void DIALOG_EXPORT_STEP::onSelectOrigin( wxCommandEvent& event )
{
    // If a new checkbox was checked: ensure other options are disabled
    wxCheckBox* cbList[]
    {
		m_cbPlotOrigin, m_cbGridOrigin, m_cbUserOrigin, m_cbBoardCenter, NULL
    };

    if( event.IsChecked() )
    {
        for( int ii = 0; cbList[ii]; ii++ )
        {
            if( cbList[ii] != event.GetEventObject() )
                cbList[ii]->SetValue( false );
        }
    }

    // Enable/disable the user origin widgets:
    m_STEP_OrgUnitChoice->Enable( m_cbUserOrigin->IsChecked() );
    m_STEP_Xorg->Enable( m_cbUserOrigin->IsChecked() );
    m_STEP_Yorg->Enable( m_cbUserOrigin->IsChecked() );
}


DIALOG_EXPORT_STEP::STEP_ORG_OPT DIALOG_EXPORT_STEP::GetOriginOption()
{
    m_STEP_org_opt = STEP_ORG_0;

    if( m_cbPlotOrigin->IsChecked() )
        m_STEP_org_opt = STEP_ORG_PLOT_AXIS;
    else if( m_cbGridOrigin->IsChecked() )
        m_STEP_org_opt = STEP_ORG_GRID_AXIS;
    else if( m_cbUserOrigin->IsChecked() )
        m_STEP_org_opt = STEP_ORG_USER;
    else if( m_cbBoardCenter->IsChecked() )
        m_STEP_org_opt = STEP_ORG_BOARD_CENTER;

    return m_STEP_org_opt;
}


void PCB_EDIT_FRAME::OnExportSTEP( wxCommandEvent& event )
{

    wxFileName brdFile = GetBoard()->GetFileName();
    wxString brdName;

    if( GetScreen()->IsModify() || brdFile.GetFullPath().empty() )
    {
        if( !doAutoSave() )
        {
            DisplayErrorMessage( this,
                                 _( "STEP export failed!  Please save the PCB and try again" ) );
            return;
        }

        brdFile = GetBoard()->GetFileName();
        brdName = GetAutoSaveFilePrefix();
        brdName.append( brdFile.GetName() );
        brdFile.SetName( brdName );
    }

    brdName = "\"";
    brdName.Append( brdFile.GetFullPath() );
    brdName.Append( "\"" );

    // Build default output file name
    brdFile = GetBoard()->GetFileName();
    wxString brdExt = brdFile.GetExt();
    brdFile.SetExt( "stp" );

    DIALOG_EXPORT_STEP dlg( this );
    dlg.FilePicker()->SetPath( brdFile.GetFullPath() );

    if ( dlg.ShowModal() != wxID_OK )
        return;

    wxString outputFile = dlg.FilePicker()->GetPath();
    brdFile.SetExt( brdExt );
    outputFile.Prepend( "\"" );
    outputFile.Append( "\"" );

    DIALOG_EXPORT_STEP::STEP_ORG_OPT orgOpt = dlg.GetOriginOption();
    double xOrg = 0.0;
    double yOrg = 0.0;

    wxFileName appK2S( wxStandardPaths::Get().GetExecutablePath() );
    appK2S.SetName( "kicad2step" );

    wxString cmdK2S = "\"";
    cmdK2S.Append( appK2S.GetFullPath() );
    cmdK2S.Append( "\"" );

    if( dlg.GetNoVirtOption() )
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
            xOrg = dlg.GetXOrg();
            yOrg = dlg.GetYOrg();

            if( dlg.GetOrgUnitsChoice() == 1 )
            {
                // selected reference unit is in inches, and STEP units are mm
                xOrg *= 25.4;
                yOrg *= 25.4;
            }

            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( " --user-origin %.6fx%.6f", xOrg, yOrg ) );
        }
            break;

        case DIALOG_EXPORT_STEP::STEP_ORG_BOARD_CENTER:
        {
            EDA_RECT bbox = GetBoard()->ComputeBoundingBox( true );
            xOrg = Iu2Millimeter( bbox.GetCenter().x );
            yOrg = Iu2Millimeter( bbox.GetCenter().y );
            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( " --user-origin %.6fx%.6f", xOrg, yOrg ) );
        }
            break;
    }

    cmdK2S.Append( " -f -o " );
    cmdK2S.Append( outputFile );

    cmdK2S.Append( " " );
    cmdK2S.Append( brdName );

    int result = 0;

    do
    {
        wxBusyCursor dummy;
        result = wxExecute( cmdK2S, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE );
    } while( 0 );

    if( result )
    {
        DisplayErrorMessage( this, _( "Unable to create STEP file.  Check that the board has a "
                                      "valid outline and models." ), cmdK2S );
    }
}
