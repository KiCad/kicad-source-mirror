/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dialog_env_var_config.cpp
 */

#include <dialog_env_var_config.h>

#include <confirm.h>

#include <validators.h>
#include <html_messagebox.h>

#include <wx/regex.h>

/** A helper dialog to edit a env var name and/or its value (often a path)
 */
class DIALOG_ENV_VAR_SINGLE : public DIALOG_ENV_VAR_SINGLE_BASE
{
public:
    DIALOG_ENV_VAR_SINGLE( wxWindow* parent, const wxString& aEnvVarName,
                           const wxString& aEnvVarPath );

    /// @return the new environment variable name
    wxString GetEnvVarName() const
    {
        return m_envVarName->GetValue();
    }

    /// @return the new environment variable value
    wxString GetEnvVarValue() const
    {
        return m_envVarPath->GetValue();
    }

    /// disable  the environment variable name (must be called
    /// for predefined environment variable names, not editable
    void SetEnvVarProtected()
    {
        m_envVarName->Enable( false );
    }

protected:
    void OnSelectPath( wxCommandEvent& event ) override;
    void onHelpClick( wxCommandEvent& event ) override;

    // Currently, only upper case variable names are accepted. onVarNameChange
    // changes on the fly any lower case char by the corresponding upper case
	void onVarNameChange( wxCommandEvent& event ) override;

    bool TransferDataFromWindow() override;
};


DIALOG_ENV_VAR_CONFIG::DIALOG_ENV_VAR_CONFIG( wxWindow* aParent, const ENV_VAR_MAP& aEnvVarMap ) :
    DIALOG_ENV_VAR_CONFIG_BASE( aParent )
{
    // Copy environment variables across
    m_envVarMap = aEnvVarMap;
}


bool DIALOG_ENV_VAR_CONFIG::TransferDataToWindow()
{
    wxLogDebug( wxT( "In DIALOG_ENV_VAR_CONFIG::TransferDataToWindow()." ) );

    if( !wxDialog::TransferDataToWindow() )
        return false;

    //TODO
    /*
    // Grab the project path var (not editable)
    wxString prjPath;

    wxGetEnv( PROJECT_VAR_NAME, &prjPath );

    m_kiprjmod->SetLabel( prjPath );
    */

    //TODO - Call SetAlternateRowColour first to prevent assertion error
    //m_pathList->EnableAlternateRowColours( true );

    PopulatePathList();

    // Select the first item in the list
    SelectListIndex( 0 );

    GetSizer()->Layout();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );

    return true;
}


bool DIALOG_ENV_VAR_CONFIG::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
    {
        return false;
    }

    Pgm().SetLocalEnvVariables( m_envVarMap );

    return true;
}


void DIALOG_ENV_VAR_CONFIG::PopulatePathList()
{
    m_pathList->Freeze();

    m_pathList->ClearAll();

    m_pathList->AppendColumn( _( "Name:" ) );
    m_pathList->AppendColumn( _( "Path:" ) );

    int row = 0;

    for( auto it = m_envVarMap.begin(); it != m_envVarMap.end(); ++it )
    {
        long index = m_pathList->InsertItem( row, it->first );

        m_pathList->SetItem( index, 1, it->second.GetValue() );

        if( it->second.GetDefinedExternally() )
        {
            wxColour color = wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

            m_pathList->SetItemBackgroundColour( index, color );
        }

        row++;
    }

    m_pathList->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    m_pathList->SetColumnWidth( 1, wxLIST_AUTOSIZE );

    m_pathList->Update();

    m_pathList->Thaw();
}


bool DIALOG_ENV_VAR_CONFIG::GetPathAtIndex( unsigned int aIndex, wxString& aEnvVar,
                                            wxString& aEnvPath )
{
    if( aIndex > m_envVarMap.size() )
    {
        return false;
    }

    unsigned int idx = 0;

    for( auto it = m_envVarMap.begin(); it != m_envVarMap.end(); ++it )
    {
        if( idx == aIndex )
        {
            aEnvVar = it->first;
            aEnvPath = it->second.GetValue();

            return true;
        }

        idx++;
    }

    return false;
}


void DIALOG_ENV_VAR_CONFIG::OnAddButton( wxCommandEvent& event )
{
    DIALOG_ENV_VAR_SINGLE dlg( nullptr, wxEmptyString, wxEmptyString );

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString newName = dlg.GetEnvVarName();
        wxString newPath = dlg.GetEnvVarValue();

        // Check that the name does not already exist
        if( m_envVarMap.count( newName ) > 0 )
        {
            //TODO - Improve this message, use DisplayErrorMessage instead
            DisplayError( this, _( "Path already exists." ) );
        }
        else
        {
            m_envVarMap[newName] = ENV_VAR_ITEM( newPath );

            // Update path list
            PopulatePathList();
        }
    }
}


void DIALOG_ENV_VAR_CONFIG::OnEditButton( wxCommandEvent& event )
{
    EditSelectedEntry();
}


void DIALOG_ENV_VAR_CONFIG::EditSelectedEntry()
{
    wxString envName;
    wxString envPath;

    if( GetPathAtIndex( m_pathIndex, envName, envPath ) )
    {
        auto dlg = new DIALOG_ENV_VAR_SINGLE( nullptr, envName, envPath );

        if( IsEnvVarImmutable( envName ) )
        {
            dlg->SetEnvVarProtected();
        }

        if( dlg->ShowModal() == wxID_OK )
        {
            wxString newName = dlg->GetEnvVarName();
            wxString newPath = dlg->GetEnvVarValue();

            // If the path name has not been changed
            if( envName.Cmp( newName ) == 0 )
            {
                m_envVarMap[envName].SetValue( newPath );

                if( m_envVarMap[envName].GetDefinedExternally() )
                {
                    m_extDefsChanged = true;
                }
            }
            // Path-name needs to be updated
            else
            {
                if( IsEnvVarImmutable( envName ) )
                {
                    DisplayErrorMessage( this,
                                         wxString::Format( _( "Environment variable \"%s\" cannot "
                                                              "be renamed." ),
                                                           envName.ToStdString() ),
                                         _( "The selected environment variable name "
                                            "is required for KiCad functionality and "
                                            "can not be renamed." ) );

                    return;
                }

                auto envVar = m_envVarMap[envName];

                m_envVarMap.erase( envName );

                envVar.SetValue( newPath );
                envVar.SetDefinedExternally( false );
                m_envVarMap[newName] = envVar;
            }

            // Update the path list
            PopulatePathList();
        }

        dlg->Destroy();
    }
}


void DIALOG_ENV_VAR_CONFIG::OnHelpButton( wxCommandEvent& event )
{
    wxString msg = _( "Enter the name and value for each environment variable.  Grey entries "
                      "are names that have been defined externally at the system or user "
                      "level.  Environment variables defined at the system or user level "
                      "take precedence over the ones defined in this table.  This means the "
                      "values in this table are ignored." );
    msg << wxT( "<br><br><b>" );
    msg << _( "To ensure environment variable names are valid on all platforms, the name field "
              "will only accept upper case letters, digits, and the underscore characters." );
    msg << wxT( "</b><br><br>" );
    msg << _( "<b>KICAD_SYMBOL_DIR</b> is the base path of the locally installed symbol "
              "libraries." );
    msg << wxT( "<br><br>" );
    msg << _( "<b>KIGITHUB</b> is used by KiCad to define the URL of the repository "
              "of the official KiCad footprint libraries.  This is only required if the "
              "Github plugin is used to access footprint libraries" );
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
    msg << _( "<b>KICAD_TEMPLATE_DIR</b> is required and is the path containing the project "
              "templates installed with KiCad." );
    msg << wxT( "<br><br>" );
    msg << _( "<b>KICAD_USER_TEMPLATE_DIR</b> is required and is the path containing any user "
              "specific project templates." );

    HTML_MESSAGE_BOX dlg( GetParent(), _( "Environment Variable Help" ) );
    dlg.SetDialogSizeInDU( 400, 350 );

    dlg.AddHTML_Text( msg );
    dlg.ShowModal();
}


bool DIALOG_ENV_VAR_CONFIG::IsEnvVarImmutable( const wxString aEnvVar )
{
    /*
     * TODO - Instead of defining these values here,
     * extract them from elsewhere in the program
     * (where they are originally defined)
     */

    static const wxString immutable[] = {
            "KIGITHUB",
            "KISYS3DMOD",
            "KISYSMOD",
            "KIPRJMOD",
            "KICAD_SYMBOL_DIR",
            "KICAD_TEMPLATE_DIR",
            "KICAD_USER_TEMPLATE_DIR"
    };

    for( unsigned int ii=0; ii<6; ii++ )
    {
        if( aEnvVar.Cmp( immutable[ii] ) == 0 )
        {
            return true;
        }
    }

    return false;
}


void DIALOG_ENV_VAR_CONFIG::OnRemoveButton( wxCommandEvent& event )
{
    wxString envName;
    wxString envPath;

    if( GetPathAtIndex( m_pathIndex, envName, envPath ) )
    {
        if( IsEnvVarImmutable( envName ) )
        {
            return;
        }

        m_envVarMap.erase( envName );

        PopulatePathList();
    }
}


void DIALOG_ENV_VAR_CONFIG::SelectListIndex( unsigned int aIndex )
{
    if( aIndex >= m_envVarMap.size() )
    {
        aIndex = 0;
    }

    m_pathIndex = aIndex;

    wxString envName;
    wxString envPath;

    if( GetPathAtIndex( m_pathIndex, envName, envPath ) )
    {
        // Disable the 'delete' button if the path cannot be deleted
        m_deletePathButton->Enable( !IsEnvVarImmutable( envName ) );
    }
}

void DIALOG_ENV_VAR_CONFIG::OnPathSelected( wxListEvent& event )
{
    SelectListIndex( event.GetIndex() );
}


void DIALOG_ENV_VAR_CONFIG::OnPathActivated( wxListEvent& event )
{
    SelectListIndex( event.GetIndex() );

    EditSelectedEntry();
}


///////////////////////////
// DIALOG_ENV_VAR_SINGLE //
///////////////////////////

DIALOG_ENV_VAR_SINGLE::DIALOG_ENV_VAR_SINGLE( wxWindow* parent,
                                              const wxString& aEnvVarName,
                                              const wxString& aEnvVarPath ) :
    DIALOG_ENV_VAR_SINGLE_BASE( parent )
{
    m_envVarName->SetValue( aEnvVarName );
    m_envVarPath->SetValue( aEnvVarPath );
    m_envVarName->SetValidator( ENVIRONMENT_VARIABLE_CHAR_VALIDATOR() );
}


void DIALOG_ENV_VAR_SINGLE::OnSelectPath( wxCommandEvent& event )
{
    wxString title = _( "Select Path for Environment Variable" );
    wxString path;  // Currently the first opened path is not initialized

    wxDirDialog dlg( nullptr, title, path, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dlg.ShowModal() == wxID_OK )
         m_envVarPath->SetValue( dlg.GetPath() );
}


void DIALOG_ENV_VAR_SINGLE::onVarNameChange( wxCommandEvent& event )
{
    wxString upper_var = m_envVarName->GetValue().Upper();

    if( upper_var != m_envVarName->GetValue() )
    {
        int pos = m_envVarName->GetInsertionPoint();
        m_envVarName->ChangeValue( upper_var );
        m_envVarName->SetInsertionPoint( pos );
    }

}


bool DIALOG_ENV_VAR_SINGLE::TransferDataFromWindow()
{
    // The user pressed the OK button, test data validity
    wxString name = m_envVarName->GetValue();
    wxString path = m_envVarPath->GetValue();

    // Neither name nor path can be empty
    if( name.IsEmpty() )
    {
        DisplayError( this, _( "Environment variable name cannot be empty." ) );
        //  Veto:
        return false;
    }

    if( path.IsEmpty() )
    {
        DisplayError( this, _( "Environment variable value cannot be empty." ) );
        //  Veto:
        return false;
    }

    // Name cannot start with a number
    if( name.Left( 1 ).IsNumber() )
    {
        DisplayError( this, _( "Environment variable names cannot start with a digit (0-9)." ) );
        //  Veto:
        return false;
    }

    // No errors detected
    return true;
}


void DIALOG_ENV_VAR_SINGLE::onHelpClick( wxCommandEvent& event )
{
    wxString msg = _( "An environment variable is used for string substitutions.<br>"
                      "Environment variables are primarily used for paths to make KiCad portable "
                      "between platforms.<br><br>"
                      "If an environment variable is defined as <b>MYLIBPATH</b> with a "
                      "value <b>e:/kicad_libs</b>, then a library name "
                      "<b>${MYLIBPATH}/mylib.lib</b> gets expanded to "
                      "<b>e:/kicad_libs/mylib.lib</b>"
                      "<br><br>"
                      "<b>Note:</b><br>"
                      "Only characters <b>ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_</b> are "
                      "allowed in environment variable names and the environment variable name "
                      "cannot start with a digit (0-9)."
                    );

    HTML_MESSAGE_BOX dlg( GetParent(), _( "Environment Variable Help" ) );
    dlg.SetDialogSizeInDU( 400, 350 );

    dlg.AddHTML_Text( msg );
    dlg.ShowModal();
}
