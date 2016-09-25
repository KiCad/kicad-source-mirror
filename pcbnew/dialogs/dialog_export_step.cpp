/**
 * @file dialog_export_step.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
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
#include "pcbnew.h"
#include "class_board.h"
#include "dialog_export_step_base.h"

#define OPTKEY_STEP_USE_DRILL_ORG   "STEP_UseDrillOrigin"
#define OPTKEY_STEP_USE_AUX_ORG     "STEP_UseAuxOrigin"
#define OPTKEY_STEP_USE_USER_ORG    "STEP_UseUserOrigin"
#define OPTKEY_STEP_UORG_UNITS      "STEP_UserOriginUnits"
#define OPTKEY_STEP_UORG_X          "STEP_UserOriginX"
#define OPTKEY_STEP_UORG_Y          "STEP_UserOriginY"
#define OPTKEY_STEP_NOVIRT          "STEP_NoVirtual"


class DIALOG_EXPORT_STEP: public DIALOG_EXPORT_STEP_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    wxConfigBase* m_config;
    bool   m_useDrillOrg;   // remember last preference for Use Drill Origin
    bool   m_useAuxOrg;     // remember last preference for Use Aux Origin
    bool   m_useUserOrg;    // remember last preference for Use User Origin
    bool   m_noVirtual;     // remember last preference for No Virtual Component
    int    m_OrgUnits;      // remember last units for User Origin
    double m_XOrg;          // remember last User Origin X value
    double m_YOrg;          // remember last User Origin Y value

public:
    DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* parent ) :
            DIALOG_EXPORT_STEP_BASE( parent )
    {
        m_parent = parent;
        m_config = Kiface().KifaceSettings();
        SetFocus();
        m_useDrillOrg = false;
        m_config->Read( OPTKEY_STEP_USE_DRILL_ORG, &m_useDrillOrg );
        m_cbDrillOrigin->SetValue( m_useDrillOrg );
        m_useAuxOrg = false;
        m_config->Read( OPTKEY_STEP_USE_AUX_ORG, &m_useAuxOrg );
        m_cbAuxOrigin->SetValue( m_useAuxOrg );
        m_useUserOrg = false;
        m_config->Read( OPTKEY_STEP_USE_USER_ORG, &m_useUserOrg );
        m_cbUserOrigin->SetValue( m_useUserOrg );
        m_cbUserOrigin->Bind( wxEVT_CHECKBOX, &DIALOG_EXPORT_STEP::OnUserOriginSelect, this );
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

        if( m_useUserOrg )
        {
            m_STEP_OrgUnitChoice->Enable( true );
            m_STEP_Xorg->Enable( true );
            m_STEP_Yorg->Enable( true );
        }
        else
        {
            m_STEP_OrgUnitChoice->Enable( false );
            m_STEP_Xorg->Enable( false );
            m_STEP_Yorg->Enable( false );
        }

        m_sdbSizerOK->SetDefault();

        FixOSXCancelButtonIssue();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    ~DIALOG_EXPORT_STEP()
    {
        m_config->Write( OPTKEY_STEP_USE_DRILL_ORG, m_cbDrillOrigin->GetValue() );
        m_config->Write( OPTKEY_STEP_USE_AUX_ORG, m_cbAuxOrigin->GetValue() );
        m_config->Write( OPTKEY_STEP_USE_USER_ORG, m_cbUserOrigin->GetValue() );
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

    bool GetDrillOrgOption()
    {
        return m_cbDrillOrigin->GetValue();
    }

    bool GetAuxOrgOption()
    {
        return m_cbAuxOrigin->GetValue();
    }

    bool GetUserOrgOption()
    {
        return m_cbUserOrigin->GetValue();
    }

    bool GetNoVirtOption()
    {
        return m_cbRemoveVirtual->GetValue();
    }

    void OnUserOriginSelect( wxCommandEvent& event )
    {
        if( GetUserOrgOption() )
        {
            m_STEP_OrgUnitChoice->Enable( true );
            m_STEP_Xorg->Enable( true );
            m_STEP_Yorg->Enable( true );
        }
        else
        {
            m_STEP_OrgUnitChoice->Enable( false );
            m_STEP_Xorg->Enable( false );
            m_STEP_Yorg->Enable( false );
        }

        event.Skip();
    }

};


void PCB_EDIT_FRAME::OnExportSTEP( wxCommandEvent& event )
{

    wxFileName brdFile = GetBoard()->GetFileName();
    wxString brdName;

    if( GetScreen()->IsModify() || brdFile.GetFullPath().empty() )
    {
        if( !doAutoSave() )
        {
            wxMessageBox( _( "STEP export failed; please save the PCB and try again" ),
                          _( "STEP Export" ) );
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
    bool fileOverwrite = false;
    wxString outputFile;

    while( !fileOverwrite )
    {
        if ( dlg.ShowModal() != wxID_OK )
            return;

        brdFile = dlg.FilePicker()->GetPath();
        brdFile.SetExt( "stp" );
        outputFile = brdFile.GetFullPath();

        if( wxFile::Exists( outputFile ) )
        {
            wxString msg( _( "File: " ) );
            msg.append( outputFile );
            msg.append( "\n" );
            msg.append( _( "File exists, overwrite?" ) );
            int resp = wxMessageBox( msg, _( "STEP Export" ), wxYES_NO | wxCANCEL, this );

            switch( resp )
            {
                case wxCANCEL:
                    return;

                case wxYES:
                    fileOverwrite = true;
                    break;

                default:
                    break;
            }
        }
        else
        {
            fileOverwrite = true;
        }
    }

    outputFile.Prepend( "\"" );
    outputFile.Append( "\"" );
    bool   aUseDrillOrg = dlg.GetDrillOrgOption();
    bool   aUseAuxOrg   = dlg.GetAuxOrgOption();
    bool   aUseUserOrg  = dlg.GetUserOrgOption();
    bool   aNoVirtual = dlg.GetNoVirtOption();
    double aXOrg = 0.0;
    double aYOrg = 0.0;

    if( aUseUserOrg )
    {
        aXOrg = dlg.GetXOrg();
        aYOrg = dlg.GetYOrg();

        if( dlg.GetOrgUnitsChoice() == 1 )
        {
            // selected reference unit is in inches
            aXOrg *= 25.4;
            aYOrg *= 25.4;
        }
    }

    wxFileName appK2S( wxStandardPaths::Get().GetExecutablePath() );
    appK2S.SetName( "kicad2step" );

    wxString cmdK2S = "\"";
    cmdK2S.Append( appK2S.GetFullPath() );
    cmdK2S.Append( "\"" );

    if( aNoVirtual )
        cmdK2S.Append( " --no-virtual" );

    if( aUseDrillOrg )
        cmdK2S.Append( " --drill-origin" );

    if( aUseAuxOrg )
        cmdK2S.Append( " --grid-origin" );

    if( aUseUserOrg )
        cmdK2S.Append( wxString::Format( " --user-origin %.6fx%.6f", aXOrg, aYOrg ) );

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
        wxMessageBox(
            _( "Unable to create STEP file; check that the board has a valid outline and models." ),
            _( "STEP Export" ), wxOK );
    }

    return;
}
