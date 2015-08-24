/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/regex.h>


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

        wxLogDebug( wxT( "Row %d, name: %s, value %s." ), row,
                    GetChars( name ), GetChars( value ) );

        // Name cannot be empty.
        if( name.IsEmpty() )
        {
            wxMessageBox( _( "Environment variable name cannot be empty." ),
                          caption, wxOK | wxICON_ERROR, this );
            m_grid->GoToCell( row, 0 );
            m_grid->SetGridCursor( row, 0 );
            return false;
        }

        // Value cannot be empty.
        if( value.IsEmpty() )
        {
            wxMessageBox( _( "Environment variable value cannot be empty." ), caption,
                          wxOK | wxICON_ERROR, this );
            m_grid->GoToCell( row, 1 );
            m_grid->SetGridCursor( row, 1 );
            m_grid->SetFocus();
            return false;
        }

        // First character of the environment variable name cannot be a digit (0-9).
        if( name.Left( 1 ).IsNumber() )
        {
            wxMessageBox( _( "The first character of an environment variable name cannot be "
                             "a digit (0-9)." ), caption, wxOK | wxICON_ERROR, this );
            m_grid->GoToCell( row, 0 );
            m_grid->SetGridCursor( row, 0 );
            m_grid->SelectBlock( row, 0, row, 0 );
            m_grid->SetFocus();
            return false;
        }

        // Check for duplicate environment variable names.
        if( envVarNames.Index( name ) != wxNOT_FOUND )
        {
            wxMessageBox( _( "Cannot have duplicate environment variable names." ), caption,
                          wxOK | wxICON_ERROR, this );
            m_grid->GoToCell( row, 0 );
            m_grid->SetGridCursor( row, 0 );
            m_grid->SelectRow( row );
            m_grid->SetFocus();
            return false;
        }

        envVarNames.Add( name );
    }

    // Add new entries and update any modified entries.
    for( row = 0; row < m_grid->GetNumberRows(); row++ )
    {
        wxString name = m_grid->GetCellValue( row, 0 );
        wxString value = m_grid->GetCellValue( row, 1 );
        ENV_VAR_MAP_ITER it = m_envVarMap.find( name );

        if( it == m_envVarMap.end() )
        {
            ENV_VAR_ITEM item( value, wxGetEnv( name, NULL ) );

            // Add new environment variable.
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
    ENVIRONMENT_VARIABLE_CHAR_VALIDATOR envVarNameValidator;
    editor->SetValidator( envVarNameValidator );
    m_grid->SetCellEditor( row, 0, editor );

    editor = new wxGridCellTextEditor;
    FILE_NAME_WITH_PATH_CHAR_VALIDATOR pathValidator;
    editor->SetValidator( pathValidator );
    m_grid->SetCellEditor( row, 1, editor );
    m_grid->GoToCell( row, 0 );
    m_grid->SetGridCursor( row, 0 );
    m_grid->SetFocus();
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
    wxString msg = _( "Enter the name and path for each environment variable.  Grey entries "
                      "are names that have been defined externally at the system or user "
                      "level.  Environment variables defined at the system or user level "
                      "take precedence over the ones defined in this table.  This means the "
                      "values in this table are ignored." );
    msg << wxT( "<br><br><b>" );
    msg << _( "To ensure environment variable names are valid on all platforms, the name field "
              "will only accept upper case letters, digits, and the underscore characters." );
    msg << wxT( "</b><br><br>" );
    msg << _( "<b>KIGITHUB</b> is used by KiCad to define the URL of the repository "
              "of the official KiCad libraries." );
    msg << wxT( "<br><br>" );
    msg << _( "<b>KISYS3DMOD</b> is the base path of system footprint 3D "
              "shapes (.3Dshapes folders)." );
    msg << wxT( "<br><br>" );
    msg << _( "<b>KISYSMOD</b> is the base path of locally installed system "
              "footprint libraries (.pretty folders)." );
    msg << wxT( "<br><br>" );
    msg << _( "<b>KIPRJMOD</b> is internally defined by KiCad (cannot be edited) and is set "
              "to the absolute path of the currently loaded project file.  This environment "
              "variable can be used to define files and paths relative to the currently loaded "
              "project.  For instance, ${KIPRJMOD}/libs/footprints.pretty can be defined as a "
              "folder containing a project specific footprint library named footprints.pretty." );
    msg << wxT( "<br><br>" );
    msg << _( "<b>KICAD_PTEMPLATES</b> is optional and can be defined if you want to "
              "create your own project templates folder." );

    HTML_MESSAGE_BOX dlg( GetParent(), _( "Environment Variable Help" ) );
    dlg.AddHTML_Text( msg );
    dlg.ShowModal();
}
