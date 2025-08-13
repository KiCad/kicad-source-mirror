/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers
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

#include <dialogs/dialog_generate_database_connection.h>

#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/msgdlg.h>

#include <database/database_connection.h>
#include <vector>

DIALOG_GENERATE_DATABASE_CONNECTION::DIALOG_GENERATE_DATABASE_CONNECTION( wxWindow* aParent ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Generate Database Connection" ), wxDefaultPosition,
                     wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    m_dsnChoice = new wxChoice( this, wxID_ANY );
    std::vector<std::string> dsns;
    DATABASE_CONNECTION::ListDataSources( dsns );

    for( const std::string& d : dsns )
        m_dsnChoice->Append( d );

    m_dsnChoice->Append( _( "Custom" ) );

    topSizer->Add( new wxStaticText( this, wxID_ANY, _( "Data Source Name" ) ), 0, wxALL, 5 );
    topSizer->Add( m_dsnChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    wxFlexGridSizer* grid = new wxFlexGridSizer( 2, 2, 5, 5 );
    grid->AddGrowableCol( 1, 1 );

    grid->Add( new wxStaticText( this, wxID_ANY, _( "Username" ) ), 0, wxALIGN_CENTER_VERTICAL );
    m_userCtrl = new wxTextCtrl( this, wxID_ANY );
    grid->Add( m_userCtrl, 1, wxEXPAND );

    grid->Add( new wxStaticText( this, wxID_ANY, _( "Password" ) ), 0, wxALIGN_CENTER_VERTICAL );
    m_passCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                 wxTE_PASSWORD );
    grid->Add( m_passCtrl, 1, wxEXPAND );

    grid->Add( new wxStaticText( this, wxID_ANY, _( "Timeout" ) ), 0, wxALIGN_CENTER_VERTICAL );
    m_timeoutCtrl = new wxSpinCtrl( this, wxID_ANY );
    m_timeoutCtrl->SetRange( 0, 999 );
    m_timeoutCtrl->SetValue( DATABASE_CONNECTION::DEFAULT_TIMEOUT );
    grid->Add( m_timeoutCtrl, 0, wxEXPAND );

    grid->Add( new wxStaticText( this, wxID_ANY, _( "Connection String" ) ), 0,
               wxALIGN_CENTER_VERTICAL );
    m_connStrCtrl = new wxTextCtrl( this, wxID_ANY );
    grid->Add( m_connStrCtrl, 1, wxEXPAND );

    topSizer->Add( grid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    m_testButton = new wxButton( this, wxID_ANY, _( "Test Connection" ) );
    topSizer->Add( m_testButton, 0, wxLEFT | wxBOTTOM, 5 );

    topSizer->Add( new wxStaticText( this, wxID_ANY, _( "Tables" ) ), 0, wxLEFT | wxRIGHT, 5 );
    m_tableChoice = new wxChoice( this, wxID_ANY );
    m_tableChoice->Enable( false );
    topSizer->Add( m_tableChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    SetSizerAndFit( topSizer );

    m_dsnChoice->Bind( wxEVT_CHOICE, &DIALOG_GENERATE_DATABASE_CONNECTION::OnDSNChanged, this );
    m_testButton->Bind( wxEVT_BUTTON, &DIALOG_GENERATE_DATABASE_CONNECTION::OnTest, this );

    UpdateControls();
    SetupStandardButtons();
}

DATABASE_SOURCE DIALOG_GENERATE_DATABASE_CONNECTION::GetSource() const
{
    DATABASE_SOURCE src;
    src.type = DATABASE_SOURCE_TYPE::ODBC;

    int sel = m_dsnChoice->GetSelection();
    if( sel != wxNOT_FOUND && sel < (int) m_dsnChoice->GetCount() - 1 )
    {
        src.dsn = m_dsnChoice->GetString( sel ).ToStdString();
        src.username = m_userCtrl->GetValue().ToStdString();
        src.password = m_passCtrl->GetValue().ToStdString();
        src.timeout = m_timeoutCtrl->GetValue();
    }
    else
    {
        src.connection_string = m_connStrCtrl->GetValue().ToStdString();
        src.timeout = DATABASE_CONNECTION::DEFAULT_TIMEOUT;
    }

    return src;
}

void DIALOG_GENERATE_DATABASE_CONNECTION::OnDSNChanged( wxCommandEvent& aEvent )
{
    UpdateControls();
}

void DIALOG_GENERATE_DATABASE_CONNECTION::UpdateControls()
{
    bool custom = m_dsnChoice->GetSelection() == (int) m_dsnChoice->GetCount() - 1;

    m_userCtrl->Enable( !custom );
    m_passCtrl->Enable( !custom );
    m_timeoutCtrl->Enable( !custom );
    m_connStrCtrl->Enable( custom );
}

void DIALOG_GENERATE_DATABASE_CONNECTION::OnTest( wxCommandEvent& aEvent )
{
    m_tableChoice->Clear();

    std::unique_ptr<DATABASE_CONNECTION> conn;

    if( m_dsnChoice->GetSelection() != (int) m_dsnChoice->GetCount() - 1 )
    {
        wxString dsn = m_dsnChoice->GetStringSelection();
        wxString user = m_userCtrl->GetValue();
        wxString pass = m_passCtrl->GetValue();
        int timeout = m_timeoutCtrl->GetValue();

        conn = std::make_unique<DATABASE_CONNECTION>( dsn.ToStdString(), user.ToStdString(), pass.ToStdString(),
                                                      timeout, false );
    }
    else
    {
        conn = std::make_unique<DATABASE_CONNECTION>( m_connStrCtrl->GetValue().ToStdString(),
                                                      DATABASE_CONNECTION::DEFAULT_TIMEOUT, false );
    }

    if( !conn->Connect() )
    {
        wxMessageBox( _( "Unable to connect to database" ), _( "Database Error" ), wxOK | wxICON_ERROR,
                      this );
        return;
    }

    std::vector<std::string> tables;

    if( conn->GetTables( tables ) )
    {
        for( const std::string& t : tables )
            m_tableChoice->Append( t );

        if( !tables.empty() )
            m_tableChoice->SetSelection( 0 );

        m_tableChoice->Enable( true );
    }
}
