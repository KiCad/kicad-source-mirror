/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_global_lib_table_config.h>

#include <pgm_base.h>
#include <search_stack.h>
#include <systemdirsappend.h>

#include <wx/stdpaths.h>


DIALOG_GLOBAL_LIB_TABLE_CONFIG::DIALOG_GLOBAL_LIB_TABLE_CONFIG( wxWindow* aParent,
                                                                const wxString& aTableName  ) :
    DIALOG_GLOBAL_LIB_TABLE_CONFIG_BASE( aParent ),
    m_defaultFileFound( false )
{
    m_tableName = aTableName;

    wxString tmp;

    tmp.Printf( _( "Configure Global %s Library Table" ), aTableName.Capitalize() );
    SetTitle( tmp );

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
                            wxUpdateUIEventHandler( DIALOG_GLOBAL_LIB_TABLE_CONFIG::onUpdateFilePicker ),
                            nullptr, this );

    wxButton* okButton = (wxButton *) FindWindowById( wxID_OK );

    if( okButton )
        okButton->SetDefault();

    finishDialogSettings();
}


DIALOG_GLOBAL_LIB_TABLE_CONFIG::~DIALOG_GLOBAL_LIB_TABLE_CONFIG()
{
    m_filePicker1->Disconnect( wxEVT_UPDATE_UI,
                               wxUpdateUIEventHandler( DIALOG_GLOBAL_LIB_TABLE_CONFIG::onUpdateFilePicker ),
                               nullptr, this );
}


void DIALOG_GLOBAL_LIB_TABLE_CONFIG::onUpdateFilePicker( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_customRb->GetValue() );
}


void DIALOG_GLOBAL_LIB_TABLE_CONFIG::onUpdateDefaultSelection( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_defaultFileFound );
}


bool DIALOG_GLOBAL_LIB_TABLE_CONFIG::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    wxFileName fn = GetGlobalTableFileName();

    SEARCH_STACK ss;

    SystemDirsAppend( &ss );

    wxString templatePath =
        Pgm().GetLocalEnvVariables().at( wxT( "KICAD6_TEMPLATE_DIR" ) ).GetValue();

    if( !templatePath.IsEmpty() )
        ss.AddPaths( templatePath, 0 );
    else
        templatePath = wxStandardPaths::Get().GetUserConfigDir();

    m_filePicker1->SetInitialDirectory( templatePath );

    // Attempt to find the default global file table from the KiCad template folder.
    wxString fileName = ss.FindValidPath( fn.GetName() );

    m_defaultFileFound = wxFileName::FileExists( fileName );

    if( m_defaultFileFound )
    {
        m_filePicker1->SetPath(fileName );
        m_filePicker1->Enable( false );
    }
    else
    {
        m_customRb->SetValue( true );
    }

    return true;
}
