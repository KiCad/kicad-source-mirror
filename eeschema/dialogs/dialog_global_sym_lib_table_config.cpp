/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Wayne Stambaugh <stambaughw@gmail.com>
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
#include <kiface_i.h>
#include <macros.h>

#include "symbol_lib_table.h"


DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG( wxWindow* aParent ) :
    DIALOG_GLOBAL_LIB_TABLE_CONFIG( aParent, _( "symbol" ) )
{
}


DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::~DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG()
{
}


wxFileName DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG::GetGlobalTableFileName()
{
    return SYMBOL_LIB_TABLE::GetGlobalTableFileName();
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
            DisplayError( this, wxString::Format( _( "Error occurred writing empty symbol library "
                                                     "table.\n\n%s" ),
                                                  SYMBOL_LIB_TABLE::GetGlobalTableFileName(),
                                                  ioe.What() ) );
            return false;
        }

        return true;
    }

    wxString fileName = m_filePicker1->GetPath();

    if( fileName.IsEmpty() )
    {
        DisplayError( this, _( "Please select a symbol library table file." ) );
        return false;
    }

    wxFileName fn = fileName;

    // Make sure the symbol library table to copy actually exists.
    if( !fn.FileExists() )
    {
        DisplayError( this, wxString::Format( _( "File '%s' not found." ), fn.GetFullPath() ) );
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
        DisplayError( this, wxString::Format( _( "'%s' is not a valid symbol library table.\n\n%s" ),
                                              fn.GetFullPath(),
                                              ioe.What() ) );
        return false;
    }

    // Create the config path if it doesn't already exist.
    wxFileName symTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    if( !symTableFileName.DirExists() && !symTableFileName.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
    {
        DisplayError( this, wxString::Format( _( "Cannot create global library table path '%s'." ),
                                              symTableFileName.GetPath() ) );
        return false;
    }

    // Copy the global symbol library table file to the user config.
    if( !::wxCopyFile( fn.GetFullPath(), symTableFileName.GetFullPath() ) )
    {
        DisplayError( this, wxString::Format( _( "Cannot copy global symbol library table file "
                                                 "'%s' to '%s'." ),
                                              fn.GetFullPath(),
                                              symTableFileName.GetFullPath() ) );
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
        DisplayError( this, wxString::Format( _( "Error loading global symbol library table."
                                                 "\n\n%s" ),
                                              ioe.What() ) );
        return false;
    }

    return true;
}
