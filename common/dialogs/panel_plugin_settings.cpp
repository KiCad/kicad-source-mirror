/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/panel_plugin_settings.h>
#include <api/api_server.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <python_manager.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>


PANEL_PLUGIN_SETTINGS::PANEL_PLUGIN_SETTINGS( wxWindow* aParent ) :
        PANEL_PLUGIN_SETTINGS_BASE( aParent ),
        m_pythonInterpreterValid( false )
{
    wxFont helpFont = KIUI::GetInfoFont( this ).Italic();
    m_stPythonStatus->SetFont( helpFont );
    m_stApiStatus->SetFont( helpFont );
}


void PANEL_PLUGIN_SETTINGS::ResetPanel()
{
}


bool PANEL_PLUGIN_SETTINGS::TransferDataToWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COMMON_SETTINGS* settings = mgr.GetCommonSettings();

    m_cbEnableApi->SetValue( settings->m_Api.enable_server );
    m_pickerPythonInterpreter->SetFileName( settings->m_Api.python_interpreter );
    validatePythonInterpreter();
    updateApiStatusText();

    return true;
}


bool PANEL_PLUGIN_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COMMON_SETTINGS* settings = mgr.GetCommonSettings();
    wxString interpreter = m_pickerPythonInterpreter->GetTextCtrlValue();

    if( m_pythonInterpreterValid || interpreter.IsEmpty() )
        settings->m_Api.python_interpreter = interpreter;

    settings->m_Api.enable_server = m_cbEnableApi->GetValue();

    return true;
}


void PANEL_PLUGIN_SETTINGS::OnPythonInterpreterChanged( wxFileDirPickerEvent& event )
{
    validatePythonInterpreter();
}


void PANEL_PLUGIN_SETTINGS::OnBtnDetectAutomaticallyClicked( wxCommandEvent& aEvent )
{
    wxString interpreter = PYTHON_MANAGER::FindPythonInterpreter();

    if( !interpreter.IsEmpty() )
    {
        m_pickerPythonInterpreter->SetPath( interpreter );
        validatePythonInterpreter();
    }
}


void PANEL_PLUGIN_SETTINGS::OnEnableApiChecked( wxCommandEvent& aEvent )
{
    validatePythonInterpreter();
    updateApiStatusText();
}


void PANEL_PLUGIN_SETTINGS::updateApiStatusText()
{
#ifdef KICAD_IPC_API
    if( m_cbEnableApi->GetValue() && Pgm().GetApiServer().Running() )
    {
        m_stApiStatus->SetLabel( wxString::Format( _( "Listening at %s" ),
                                                      Pgm().GetApiServer().SocketPath() ) );
    }
    else
    {
        m_stApiStatus->SetLabel( wxEmptyString );
    }
#else
    m_stApiStatus->SetLabel( _( "This installation of KiCad does not have API support enabled." ) );
#endif
}


void PANEL_PLUGIN_SETTINGS::validatePythonInterpreter()
{
    if( !m_cbEnableApi->GetValue() )
    {
        m_stPythonStatus->SetLabel( _( "KiCad API is not enabled; external Python plugins will "
                                       "not be available" ) );
        return;
    }

    m_pythonInterpreterValid = false;

    wxFileName pythonExe( m_pickerPythonInterpreter->GetTextCtrlValue() );

    if( !pythonExe.FileExists() )
    {
        m_stPythonStatus->SetLabel( _( "No valid Python interpreter chosen; external Python "
                                       "plugins will not be available" ) );
        return;
    }

    PYTHON_MANAGER manager( pythonExe.GetFullPath() );

    manager.Execute( { wxS( "--version" ) },
                     [&]( int aRetCode, const wxString& aStdOut, const wxString& aStdErr )
                     {
                         wxString msg;

                         if( aRetCode == 0 && aStdOut.Contains( wxS( "Python 3" ) ) )
                         {
                             msg = wxString::Format( _( "Found %s" ), aStdOut );
                             m_pythonInterpreterValid = true;
                         }
                         else
                         {
                             msg = _( "Not a valid Python 3 interpreter" );
                         }

                         m_stPythonStatus->SetLabel( msg );
                         Layout();
                    },
                    /* aEnv = */ nullptr,
                    /* aSaveOutput = */ true );
}
