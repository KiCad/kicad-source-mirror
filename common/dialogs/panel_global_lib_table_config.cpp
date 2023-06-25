/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <dialogs/panel_global_lib_table_config.h>

#include <env_vars.h>
#include <pgm_base.h>
#include <search_stack.h>
#include <systemdirsappend.h>
#include <confirm.h>

#include <kiplatform/environment.h>


PANEL_GLOBAL_LIB_TABLE_CONFIG::PANEL_GLOBAL_LIB_TABLE_CONFIG( wxWindow*           aParent,
                                                              const wxString&     aTableName,
                                                              std::shared_ptr<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL> aModel,
                                                              const KIWAY::FACE_T aFaceType ) :
    PANEL_GLOBAL_LIB_TABLE_CONFIG_BASE( aParent ),
    m_tableName( aTableName ),
    m_defaultFileFound( false ),
    m_faceType( aFaceType ),
    m_model( aModel )
{
    wxString tmp;

    tmp.Printf( _( "Configure Global %s Library Table" ), aTableName.Capitalize() );

    tmp.Printf( _( "KiCad has been run for the first time using the new %s library table for\n"
                   "accessing libraries.  In order for KiCad to access %s libraries,\n"
                   "you must configure your global %s library table.  Please select from one\n"
                   "of the options below.  If you are not sure which option to select, please\n"
                   "use the default selection." ), aTableName, aTableName, aTableName );
    m_staticText1->SetLabel( tmp );

    tmp.Printf( _( "Copy default global %s library table (recommended)"), aTableName );
    m_defaultRb->SetLabel( tmp );
    tmp.Printf( _( "Select this option if you not sure about configuring the global %s library "
                   "table" ), aTableName );
    m_defaultRb->SetToolTip( tmp );

	tmp.Printf( _( "Copy custom global %s library table" ), aTableName );
    m_customRb->SetLabel( tmp );
	tmp.Printf( _( "Select this option to copy a %s library table file other than the default" ),
                aTableName );
    m_customRb->SetToolTip( tmp );

	tmp.Printf( _( "Create an empty global %s library table" ), aTableName );
    m_emptyRb->SetLabel( tmp );
	tmp.Printf( _( "Select this option to define %s libraries in project specific library tables" ),
                aTableName );
    m_emptyRb->SetToolTip( tmp );

    tmp.Printf( _( "Select global %s library table file:" ), aTableName );
    m_staticText2->SetLabel( tmp );

    m_filePicker1->Connect( wxEVT_UPDATE_UI,
            wxUpdateUIEventHandler( PANEL_GLOBAL_LIB_TABLE_CONFIG::onUpdateFilePicker ),
                            nullptr, this );


}


PANEL_GLOBAL_LIB_TABLE_CONFIG::~PANEL_GLOBAL_LIB_TABLE_CONFIG()
{
    m_filePicker1->Disconnect( wxEVT_UPDATE_UI,
                               wxUpdateUIEventHandler( PANEL_GLOBAL_LIB_TABLE_CONFIG::onUpdateFilePicker ),
                               nullptr, this );
}


void PANEL_GLOBAL_LIB_TABLE_CONFIG::onUpdateFilePicker( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_customRb->GetValue() );
}


void PANEL_GLOBAL_LIB_TABLE_CONFIG::onUpdateDefaultSelection( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_defaultFileFound );
}


bool PANEL_GLOBAL_LIB_TABLE_CONFIG::TransferDataFromWindow()
{
    if( m_emptyRb->GetValue() )
    {
        m_model->m_tableMode = PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL::TABLE_MODE::EMPTY;
    }
    else if (m_defaultRb->GetValue())
    {
        m_model->m_tableMode = PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL::TABLE_MODE::DEFAULT;
    }
    else if( m_customRb->GetValue() )
    {
        m_model->m_tableMode = PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL::TABLE_MODE::CUSTOM;
        m_model->m_customTablePath = m_filePicker1->GetPath();
    }

    return true;
}


bool PANEL_GLOBAL_LIB_TABLE_CONFIG::TransferDataToWindow()
{
    if( !wxPanel::TransferDataToWindow() )
        return false;

    wxFileName fn = GetGlobalTableFileName();

    SEARCH_STACK ss;

    GlobalPathsAppend( &ss, m_faceType );

    wxString templatePath;
    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();

    if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( envVars,
                                                                      wxT( "TEMPLATE_DIR" ) ) )
    {
        templatePath = *v;
    }

    if( !templatePath.IsEmpty() )
        ss.AddPaths( templatePath, 0 );
    else
        templatePath = KIPLATFORM::ENV::GetUserConfigPath();

    m_filePicker1->SetInitialDirectory( templatePath );

    // Attempt to find the default global file table from the KiCad template folder.
    wxString fileName = ss.FindValidPath( fn.GetName() );

    m_defaultFileFound = wxFileName::FileExists( fileName );

    if( m_defaultFileFound )
    {
        m_model->m_defaultTablePath = fileName;
        m_filePicker1->SetPath( fileName );
        m_filePicker1->Enable( false );
    }
    else
    {
        m_customRb->SetValue( true );
    }

    return true;
}
