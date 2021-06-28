/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_global_fp_lib_table_config.h"

#include <confirm.h>
#include <kiface_i.h>

#include "fp_lib_table.h"


DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG::DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG( wxWindow* aParent ) :
    DIALOG_GLOBAL_LIB_TABLE_CONFIG( aParent, _( "footprint" ) )
{
}


DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG::~DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG()
{
}


wxFileName DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG::GetGlobalTableFileName()
{
    return FP_LIB_TABLE::GetGlobalTableFileName();
}


bool DIALOG_GLOBAL_FP_LIB_TABLE_CONFIG::TransferDataFromWindow()
{
    // Create an empty table if requested by the user.
    if( m_emptyRb->GetValue() )
    {
        FP_LIB_TABLE    emptyTable;

        try
        {
            emptyTable.Save( FP_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, wxString::Format( _( "Error occurred writing empty footprint "
                                                     "library table '%s'." ),
                                                  FP_LIB_TABLE::GetGlobalTableFileName() )
                                + wxS( "\n" ) + ioe.What() );
            return false;
        }

        return true;
    }

    wxString fileName = m_filePicker1->GetPath();

    if( fileName.IsEmpty() )
    {
        DisplayError( this, _( "Please select a footprint library table file." ) );
        return false;
    }

    wxFileName fn = fileName;

    // Make sure the footprint library table to copy actually exists.
    if( !fn.FileExists() )
    {
        DisplayError( this, wxString::Format( _( "File '%s' not found." ), fn.GetFullPath() ) );
        return false;
    }

    // Make sure the footprint library table to copy is a valid footprint library table file.
    FP_LIB_TABLE tmpTable;

    try
    {
        tmpTable.Load( fn.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, wxString::Format( _( "'%s' is not a valid footprint library table." ),
                                              fn.GetFullPath() )
                            + wxS( "\n" ) + ioe.What() );
        return false;
    }

    // Create the config path if it doesn't already exist.
    wxFileName fpTableFileName = FP_LIB_TABLE::GetGlobalTableFileName();

    if( !fpTableFileName.DirExists() && !fpTableFileName.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
    {
        DisplayError( this, wxString::Format( _( "Cannot create library table path '%s'." ),
                                              fpTableFileName.GetPath() ) );
        return false;
    }

    // Copy the global footprint library table file to the user config.
    if( !::wxCopyFile( fn.GetFullPath(), fpTableFileName.GetFullPath() ) )
    {
        DisplayError( this, wxString::Format( _( "Cannot copy footprint library table from:\n"
                                                 "%s\n"
                                                 "to:\n"
                                                 "%s." ),
                                              fn.GetFullPath(),
                                              fpTableFileName.GetFullPath() ) );
        return false;
    }

    // Load the successfully copied footprint library table file.  This should not fail
    // since the file was tested above.  Check for failure anyway to keep the compiler
    // from complaining.
    try
    {
        if( !FP_LIB_TABLE::LoadGlobalTable( GFootprintTable ) )
            return false;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, _( "Error loading footprint library table." )
                            + wxS( "\n" ) + ioe.What() );
        return false;
    }

    return true;
}
