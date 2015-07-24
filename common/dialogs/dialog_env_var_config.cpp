/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015 Kicad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialg_env_var_config.cpp
 */

#include <dialog_env_var_config.h>

#include <validators.h>
#include <html_messagebox.h>


DIALOG_ENV_VAR_CONFIG::DIALOG_ENV_VAR_CONFIG( wxWindow* aParent, const ENV_VAR_MAP& aEnvVarMap ) :
    DIALOG_ENV_VAR_CONFIG_BASE( aParent )
{
    m_extDefsChanged = false;
    m_envVarMap = aEnvVarMap;

    m_grid->AppendRows( (int) m_envVarMap.size() );

    for( size_t row = 0;  row < m_envVarMap.size();  row++ )
    {
        wxGridCellTextEditor* editor = new wxGridCellTextEditor;
        ENVIRONMENT_VARIABLE_CHAR_VALIDATOR envVarValidator;
        editor->SetValidator( envVarValidator );
        m_grid->SetCellEditor( (int) row, 0, editor );

        editor = new wxGridCellTextEditor;
        FILE_NAME_WITH_PATH_CHAR_VALIDATOR pathValidator;
        editor->SetValidator( pathValidator );
        m_grid->SetCellEditor( (int) row, 1, editor );
    }

    wxButton* okButton = (wxButton*) FindWindowById( wxID_OK );

    if( okButton )
        SetDefaultItem( okButton );
}


bool DIALOG_ENV_VAR_CONFIG::TransferDataToWindow()
{
    wxLogDebug( wxT( "In DIALOG_ENV_VAR_CONFIG::TransferDataToWindow()." ) );

    if( !wxDialog::TransferDataToWindow() )
        return false;

    long row = 0L;

    for( ENV_VAR_MAP_ITER it = m_envVarMap.begin(); it != m_envVarMap.end(); ++it )
    {
        m_grid->SetCellValue( row, 0, it->first );
        m_grid->SetCellValue( row, 1, it->second.GetValue() );

        // Highlight environment variables that are externally defined.
        if( it->second.GetDefinedExternally() )
        {
            wxGridCellAttr* attr = m_grid->GetOrCreateCellAttr( row, 0 );
            attr->SetBackgroundColour( *wxLIGHT_GREY );
            m_grid->SetRowAttr( row, attr );
        }

        row++;
    }

    m_grid->AutoSizeColumns();
    m_grid->AutoSizeRows();
    GetSizer()->Layout();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    return true;
}


bool DIALOG_ENV_VAR_CONFIG::TransferDataFromWindow()
{
    wxString nums( wxT( "0123456789" ) );

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    int row;
    wxArrayString envVarNames;

    for( row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        wxString caption = _( "Invalid Input" );
        wxString name = m_grid->GetCellValue( row, 0 );
        wxString value = m_grid->GetCellValue( row, 1 );

        // Ignore completely empty rows.
        if( name.IsEmpty() && value.IsEmpty() )
            continue;

        // Check for empty cells.
        if( name.IsEmpty() )
        {
            wxMessageBox( _( "Path configuration name cannot be empty." ), caption,
                          wxOK | wxICON_ERROR, this );
            return false;
        }
        if( value.IsEmpty() )
        {
            wxMessageBox( _( "Path configuration value cannot be empty." ), caption,
                          wxOK | wxICON_ERROR, this );
            return false;
        }

        // First character of environment variable name cannot be a number.
        if( nums.Find( name[0] ) != wxNOT_FOUND )
        {
            wxMessageBox( _( "Path configuration names cannot have a number as the first "
                             "character." ), caption, wxOK | wxICON_ERROR, this );
            return false;
        }

        // Check for duplicate environment variable names.
        if( envVarNames.Index( name ) != wxNOT_FOUND )
        {
            wxMessageBox( _( "Cannot have duplicate configuration names." ), caption,
                          wxOK | wxICON_ERROR, this );
            return false;
        }

        envVarNames.Add( name );
    }

    // Add new entries and update any modified entries..
    for( row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        wxString name = m_grid->GetCellValue( row, 0 );
        wxString value = m_grid->GetCellValue( row, 1 );
        ENV_VAR_MAP_ITER it = m_envVarMap.find( name );

        if( it == m_envVarMap.end() )
        {
            ENV_VAR_ITEM item( value, wxGetEnv( name, NULL ) );

            // Add new envrionment variable.
            m_envVarMap[ name ] = item;
        }
        else if( it->second.GetValue() != value )
        {
            // Environment variable already defined but it's value changed.
            it->second.SetValue( value );

            // Externally defined variable has been changed.
            if( it->second.GetDefinedExternally() )
                m_extDefsChanged = true;
        }
    }

    std::vector< wxString > removeFromMap;

    // Remove deleted entries from the map.
    for( ENV_VAR_MAP_ITER it = m_envVarMap.begin(); it != m_envVarMap.end(); ++it )
    {
        bool found = false;

        for( row = 0; row < m_grid->GetNumberRows(); row++ )
        {
            if( m_grid->GetCellValue( row, 0 ) == it->first )
            {
                found = true;
                break;
            }
        }

        if( !found )
            removeFromMap.push_back( it->first );
    }

    for( size_t i = 0; i < removeFromMap.size(); i++ )
        m_envVarMap.erase( removeFromMap[i] );

    return true;
}


void DIALOG_ENV_VAR_CONFIG::OnAddRow( wxCommandEvent& aEvent )
{
    m_grid->AppendRows();

    int row = m_grid->GetNumberRows() - 1;
    wxGridCellTextEditor* editor = new wxGridCellTextEditor;
    ENVIRONMENT_VARIABLE_CHAR_VALIDATOR envVarValidator;
    editor->SetValidator( envVarValidator );
    m_grid->SetCellEditor( row, 0, editor );
    editor = new wxGridCellTextEditor;
    FILE_NAME_WITH_PATH_CHAR_VALIDATOR pathValidator;
    editor->SetValidator( pathValidator );
    m_grid->SetCellEditor( row, 1, editor );
}


void DIALOG_ENV_VAR_CONFIG::OnDeleteSelectedRows( wxCommandEvent& aEvent )
{
    if( !m_grid->IsSelection() )
        return;

    wxGridUpdateLocker locker( m_grid );

    for( int n = 0; n < m_grid->GetNumberRows(); )
    {
        if( m_grid->IsInSelection( n , 0 ) )
            m_grid->DeleteRows( n, 1 );
        else
            n++;
    }
}


void DIALOG_ENV_VAR_CONFIG::OnHelpRequest( wxCommandEvent& aEvent )
{
    wxString msg = _( "Enter the names and paths for each path.<br><br>"
                      "<i>Grey enteries are names that have been defined externally</i> "
                      "as system or user level environment variables." );
    msg << wxT( "<br><br><b>" );
    msg << _( "To avoid issues, names accept only upper case letters and digits." );
    msg << wxT( "</b><br><br>" );
    msg << _( "<b><i>KIGITHUB</b></i> is often used in Kicad packages to define "
              "the URL of the repository of our official libraries." );
    msg << wxT( "<br>" );
    msg << _( "<b><i>KISYS3DMOD</b></i> is the base path of footprint 3D shapes (.3Dshapes folders)." );
    msg << wxT( "<br>" );
    msg << _( "<b><i>KISYSMOD</b></i> is the base path of local footprint libraries (.pretty folders)." );
    msg << wxT( "<br><br>" );
    msg << _( "A other environment variable is automatically defined by Kicad (cannot be edited):" );
    msg << wxT( "<br>" );
    msg << _( "<b><i>KIPRJMOD</b></i> is the absolute path of the current project" );
    msg << wxT( "<br>" );
    msg << _( "For instance, ${KIPRJMOD}/libs/footprints.pretty is the folder "
              "libs/footprints.pretty located in the current project." );
    msg << wxT( "<br><br>" );
    msg << _( "Auxiliary environment variable name used by Kicad if exists:" );
    msg << wxT( "<br>" );
    msg << _( "<i>KICAD_PTEMPLATES</i> can be defined if you want to create "
              "and use project templates (specific folders containing the template files) "
              "in a given master folder" );
    msg << wxT( "<br>" );
    msg << _( "It is the base path of these project template folders" );

    HTML_MESSAGE_BOX dlg( this, _( "Paths Definition" ), wxDefaultPosition,
                          wxSize( 650, 450 ) );
    dlg.AddHTML_Text( msg );
    dlg.ShowModal();
}
