/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_global_sym_lib_table_config.h"

#include <confirm.h>
#include <grid_tricks.h>
#include <kiface_i.h>
#include <lib_table_grid.h>
#include <lib_table_lexer.h>
#include <macros.h>

#include <wx/filename.h>

#include "symbol_lib_table.h"


DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG( wxWindow* aParent ) :
        DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE( aParent )
{
    wxFileName fn = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    // Attempt to find the default global file table from the KiCad template folder.
    wxString fileName = Kiface().KifaceSearch().FindValidPath( fn.GetName() );

    m_defaultFileFound = wxFileName::FileExists( fileName );

    m_filePicker_new = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _( "Select a file" ),
            wxFileSelectorDefaultWildcardStr, wxDefaultPosition, wxDefaultSize,
            wxFLP_DEFAULT_STYLE | wxFLP_FILE_MUST_EXIST | wxFLP_OPEN );
    m_filePicker_new->SetFileName( wxFileName( fileName ) );
    m_filePicker_new->Enable( false );
    m_filePicker_new->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::onUpdateFilePicker ), NULL, this );

    bSizer2->Replace( m_filePicker1, m_filePicker_new, true );
    bSizer2->Layout();

    if( !m_defaultFileFound )
        m_customRb->SetValue( true );

    wxButton* okButton = (wxButton *) FindWindowById( wxID_OK );

    if( okButton )
        okButton->SetDefault();

    FinishDialogSettings();
}


DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::~DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG()
{
    m_filePicker_new->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::onUpdateFilePicker ), NULL, this );
}


void DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::onUpdateFilePicker( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_customRb->GetValue() );
}


void DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::onUpdateDefaultSelection( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_defaultFileFound );
}


bool DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::TransferDataFromWindow()
{
    // Create an empty table if requested by the user.
    if( m_emptyRb->GetValue() )
    {
        SYMBOL_LIB_TABLE    emptyTable;

        try
        {
            emptyTable.Save( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this,
                          wxString::Format( _( "Error occurred writing empty symbol library table "
                                               "file.\n\n%s" ),
                                            SYMBOL_LIB_TABLE::GetGlobalTableFileName(),
                                            ioe.What() ) );
            return false;
        }

        return true;
    }

    wxString fileName = m_filePicker_new->GetPath();

    if( fileName.IsEmpty() )
    {
        DisplayError( this, _( "Please select a symbol library table file." ) );
        return false;
    }

    wxFileName fn = fileName;

    // Make sure the symbol library table to copy actually exists.
    if( !fn.FileExists() )
    {
        DisplayError( this,
                      wxString::Format( _( "File \"%s\" not found." ), fn.GetFullPath() ) );
        return false;
    }

    // Make sure the symbol library table to copy is a valid symbol library table file.
    SYMBOL_LIB_TABLE tmpTable;

    try
    {
        tmpTable.Load( fn.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this,
                      wxString::Format( _( "File \"%s\" is not a valid symbol library table "
                                           "file.\n\n%s" ), fn.GetFullPath(), ioe.What() ) );
        return false;
    }

    // Create the config path if it doesn't already exist.
    wxFileName symTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    if( !symTableFileName.DirExists() && !symTableFileName.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
    {
        DisplayError( this,
                      wxString::Format( _( "Cannot create global library table path \"%s\"." ),
                                            symTableFileName.GetPath() ) );
        return false;
    }

    // Copy the global symbol library table file to the user config.
    if( !::wxCopyFile( fn.GetFullPath(), symTableFileName.GetFullPath() ) )
    {
        DisplayError( this,
                      wxString::Format( _( "Cannot copy global symbol library table "
                                           "file:\n\n \"%s\"\n\n:to:\n\n\"%s\"." ),
                                        fn.GetFullPath(), symTableFileName.GetFullPath() ) );
        return false;
    }

    // Load the successfully copied symbol library table file.  This should not fail
    // since the file was tested above.  Check for failure anyway to keep the compiler
    // from complaining.
    try
    {
        if( !SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE::GetGlobalLibTable() ) )
            return false;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this,
                      wxString::Format( _( "Error occurred loading global symbol library table:"
                                           "\n\n%s" ), ioe.What() ) );
        return false;
    }

    return true;
}
