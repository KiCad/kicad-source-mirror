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

#include <dialogs/dialog_database_lib_settings.h>
#include <sch_io/database/sch_io_database.h>
#include <database/database_lib_settings.h>


DIALOG_DATABASE_LIB_SETTINGS::DIALOG_DATABASE_LIB_SETTINGS( wxWindow* aParent,
                                                            SCH_IO_DATABASE* aPlugin ) :
        DIALOG_DATABASE_LIB_SETTINGS_BASE( aParent ),
        m_plugin( aPlugin )
{
    Layout();
    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_DATABASE_LIB_SETTINGS::TransferDataToWindow()
{
    wxCommandEvent dummy;
    DATABASE_LIB_SETTINGS* settings = m_plugin->Settings();

    m_txtConnectionString->SetValue( settings->m_Source.connection_string );

    if( !settings->m_Source.connection_string.empty() )
    {
        m_rbConnectionString->SetValue( true );
        OnConnectionStringSelected( dummy );
    }
    else
    {
        m_rbDSN->SetValue( true );
        OnDSNSelected( dummy );
        m_txtDSN->SetValue( settings->m_Source.dsn );
        m_txtUser->SetValue( settings->m_Source.username );
        m_txtPassword->SetValue( settings->m_Source.password );
    }

    m_spinCacheSize->SetValue( settings->m_Cache.max_size );
    m_spinCacheTimeout->SetValue( settings->m_Cache.max_age );

    if( hasPotentiallyValidConfig() )
        OnBtnTest( dummy );

    return true;
}


bool DIALOG_DATABASE_LIB_SETTINGS::TransferDataFromWindow()
{
    return true;
}


void DIALOG_DATABASE_LIB_SETTINGS::OnDSNSelected( wxCommandEvent& aEvent )
{
    m_txtConnectionString->Disable();
    m_txtDSN->Enable();
    m_txtUser->Enable();
    m_txtPassword->Enable();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnConnectionStringSelected( wxCommandEvent& aEvent )
{
    m_txtConnectionString->Enable();
    m_txtDSN->Disable();
    m_txtUser->Disable();
    m_txtPassword->Disable();
}


void DIALOG_DATABASE_LIB_SETTINGS::OnBtnTest( wxCommandEvent& aEvent )
{
    wxString errorMsg;

    if( m_plugin->TestConnection( &errorMsg ) )
    {
        m_stConnectionTestStatus->SetLabel( _( "Connected to database successfully" ) );
        m_stConnectionTestStatus->SetToolTip( wxEmptyString );

        wxCommandEvent dummy;
        OnBtnReloadConfig( dummy );
    }
    else
    {
        m_stConnectionTestStatus->SetLabel( wxString::Format( _( "Database connection failed: %s" ),
                                                              errorMsg ) );
        m_stConnectionTestStatus->SetToolTip( errorMsg );
    }
}


void DIALOG_DATABASE_LIB_SETTINGS::OnBtnReloadConfig( wxCommandEvent& aEvent )
{
    if( !m_plugin->TestConnection() )
    {
        m_stLibrariesStatus->SetLabel( _( "No connection to database" ) );
        return;
    }

    std::vector<wxString> libs;
    m_plugin->GetSubLibraryNames( libs );

    m_stLibrariesStatus->SetLabel( wxString::Format( _( "Loaded %zu libraries" ), libs.size() ) );
}


bool DIALOG_DATABASE_LIB_SETTINGS::hasPotentiallyValidConfig()
{
    return ( m_rbDSN->GetValue() && !m_txtDSN->IsEmpty() ) || !m_txtConnectionString->IsEmpty();
}
