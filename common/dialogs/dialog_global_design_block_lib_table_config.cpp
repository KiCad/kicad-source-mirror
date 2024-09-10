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

#include "dialog_global_design_block_lib_table_config.h"

#include <confirm.h>
#include <kiface_base.h>
#include <kiway.h>
#include <macros.h>

#include "design_block_lib_table.h"


DIALOG_GLOBAL_DESIGN_BLOCK_LIB_TABLE_CONFIG::DIALOG_GLOBAL_DESIGN_BLOCK_LIB_TABLE_CONFIG(
        wxWindow* aParent ) :
        DIALOG_GLOBAL_LIB_TABLE_CONFIG( aParent, _( "design block" ), KIWAY::FACE_SCH )
{
}


DIALOG_GLOBAL_DESIGN_BLOCK_LIB_TABLE_CONFIG::~DIALOG_GLOBAL_DESIGN_BLOCK_LIB_TABLE_CONFIG()
{
}


wxFileName DIALOG_GLOBAL_DESIGN_BLOCK_LIB_TABLE_CONFIG::GetGlobalTableFileName()
{
    return DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName();
}


bool DIALOG_GLOBAL_DESIGN_BLOCK_LIB_TABLE_CONFIG::TransferDataFromWindow()
{
    // Create an empty table if requested by the user.
    if( m_emptyRb->GetValue() )
    {
        DESIGN_BLOCK_LIB_TABLE emptyTable;

        try
        {
            emptyTable.Save( DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this,
                          wxString::Format( _( "Error creating design block library table '%s'.\n"
                                               "%s" ),
                                            DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName(),
                                            ioe.What() ) );
            return false;
        }
    }
    else
    {
        wxString fileName = m_filePicker1->GetPath();

        if( fileName.IsEmpty() )
        {
            DisplayError( this, _( "Please select a design block library table file." ) );
            return false;
        }

        wxFileName fn = fileName;

        // Make sure the design block library table to copy actually exists.
        if( !fn.FileExists() )
        {
            DisplayError( this, wxString::Format( _( "File '%s' not found." ), fn.GetFullPath() ) );
            return false;
        }

        // Make sure the design block library table to copy is a valid design block library table file.
        DESIGN_BLOCK_LIB_TABLE tmpTable;

        try
        {
            tmpTable.Load( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this,
                          wxString::Format( _( "Error reading design block library table '%s'.\n"
                                               "%s" ),
                                            fn.GetFullPath(), ioe.What() ) );
            return false;
        }

        // Create the config path if it doesn't already exist.
        wxFileName designBlockTableFileName = DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName();

        if( !designBlockTableFileName.DirExists()
            && !designBlockTableFileName.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
        {
            DisplayError( this, wxString::Format( _( "Cannot create global library table '%s'." ),
                                                  designBlockTableFileName.GetPath() ) );
            return false;
        }

        // Copy the global design block library table file to the user config.
        if( !::wxCopyFile( fn.GetFullPath(), designBlockTableFileName.GetFullPath() ) )
        {
            DisplayError(
                    this,
                    wxString::Format( _( "Error copying global design block library table '%s' "
                                         "to '%s'." ),
                                      fn.GetFullPath(), designBlockTableFileName.GetFullPath() ) );
            return false;
        }
    }

    // Load the successfully copied design block library table file.  This should not fail since the
    // file was tested above.  Check for failure anyway to keep the compiler from complaining.
    try
    {
        if( !DESIGN_BLOCK_LIB_TABLE::LoadGlobalTable(
                    DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() ) )
            return false;
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    return true;
}
