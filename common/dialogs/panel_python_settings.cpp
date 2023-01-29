/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/panel_python_settings.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <python_manager.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>


PANEL_PYTHON_SETTINGS::PANEL_PYTHON_SETTINGS( wxWindow* aParent ) :
        PANEL_PYTHON_SETTINGS_BASE( aParent )
{
    wxFont helpFont = KIUI::GetInfoFont( this ).Italic();
    m_stPythonStatus->SetFont( helpFont );
}


void PANEL_PYTHON_SETTINGS::ResetPanel()
{
}


bool PANEL_PYTHON_SETTINGS::TransferDataToWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COMMON_SETTINGS*  settings = mgr.GetCommonSettings();

    m_pickerPythonInterpreter->SetFileName( settings->m_Python.interpreter_path );
    validateInterpreter();

    return true;
}


bool PANEL_PYTHON_SETTINGS::TransferDataFromWindow()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    COMMON_SETTINGS*  settings = mgr.GetCommonSettings();

    if( m_interpreterValid )
        settings->m_Python.interpreter_path = m_pickerPythonInterpreter->GetTextCtrlValue();

    return true;
}


void PANEL_PYTHON_SETTINGS::OnPythonInterpreterChanged( wxFileDirPickerEvent& event )
{
    validateInterpreter();
}


void PANEL_PYTHON_SETTINGS::OnBtnDetectAutomaticallyClicked( wxCommandEvent& aEvent )
{
}


void PANEL_PYTHON_SETTINGS::validateInterpreter()
{
    m_interpreterValid = false;

    wxFileName pythonExe( m_pickerPythonInterpreter->GetTextCtrlValue() );

    if( !pythonExe.FileExists() )
    {
        m_stPythonStatus->SetLabel( _( "No valid Python interpreter chosen; external Python "
                                       "plugins will not be available" ) );
        return;
    }

    PYTHON_MANAGER manager( pythonExe.GetFullPath() );

    manager.Execute( wxS( "--version" ),
                     [&]( int aRetCode, const wxString& aStdOut )
                     {
                         wxString msg;

                         if( aRetCode == 0 && aStdOut.Contains( wxS( "Python 3" ) ) )
                         {
                             msg = wxString::Format( _( "Found %s" ), aStdOut );
                             m_interpreterValid = true;
                         }
                         else
                         {
                             msg = _( "Not a valid Python 3 interpreter" );
                         }

                         m_stPythonStatus->SetLabel( msg );
                     } );
}
