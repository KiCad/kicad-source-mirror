/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "startwizard_provider_schlib.h"
#include "symbol_lib_table.h"
#include <confirm.h>
#include <kiplatform/io.h>

#include <dialogs/panel_global_lib_table_config.h>


class PANEL_GLOBAL_SYM_LIB_TABLE_CONFIG : public PANEL_GLOBAL_LIB_TABLE_CONFIG
{
public:
    PANEL_GLOBAL_SYM_LIB_TABLE_CONFIG(
            wxWindow* aParent, std::shared_ptr<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL> aModel ) :
            PANEL_GLOBAL_LIB_TABLE_CONFIG( aParent, _( "symbol" ), aModel, KIWAY::FACE_SCH )
    {
    }

    virtual ~PANEL_GLOBAL_SYM_LIB_TABLE_CONFIG(){};

    bool TransferDataFromWindow() override;

    virtual wxFileName GetGlobalTableFileName() override
    {
        return SYMBOL_LIB_TABLE::GetGlobalTableFileName();
    }
};

bool PANEL_GLOBAL_SYM_LIB_TABLE_CONFIG::TransferDataFromWindow()
{
    // Create an empty table if requested by the user.
    if( m_emptyRb->GetValue() )
    {
        SYMBOL_LIB_TABLE emptyTable;

        try
        {
            emptyTable.Save( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, wxString::Format( _( "Error creating symbol library table '%s'.\n"
                                                     "%s" ),
                                                  SYMBOL_LIB_TABLE::GetGlobalTableFileName(),
                                                  ioe.What() ) );
            return false;
        }
    }
    else
    {
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
            DisplayError( this, wxString::Format( _( "Error reading symbol library table '%s'.\n"
                                                     "%s" ),
                                                  fn.GetFullPath(), ioe.What() ) );
            return false;
        }

        // Create the config path if it doesn't already exist.
        wxFileName symTableFileName = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

        if( !wxFileName::DirExists( symTableFileName.GetPath() )
            && !wxFileName::Mkdir( symTableFileName.GetPath(), 0x777, wxPATH_MKDIR_FULL ) )
        {
            DisplayError( this, wxString::Format( _( "Cannot create global library table '%s'." ),
                                                  symTableFileName.GetPath() ) );
            return false;
        }

        // Copy the global symbol library table file to the user config.
        if( !::wxCopyFile( fn.GetFullPath(), symTableFileName.GetFullPath() ) )
        {
            DisplayError( this,
                          wxString::Format( _( "Error copying global symbol library table '%s' "
                                               "to '%s'." ),
                                            fn.GetFullPath(), symTableFileName.GetFullPath() ) );
            return false;
        }

        // Ensure the copied file is writable
        KIPLATFORM::IO::MakeWriteable( symTableFileName.GetFullPath() );
    }

    // Load the successfully copied symbol library table file.  This should not fail since the
    // file was tested above.  Check for failure anyway to keep the compiler from complaining.
    try
    {
        if( !SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE::GetGlobalLibTable() ) )
            return false;
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    return true;
}


STARTWIZARD_PROVIDER_SCHLIB::STARTWIZARD_PROVIDER_SCHLIB() :
        STARTWIZARD_PROVIDER( wxT( "Schematic Libraries" ) )
{
}


wxPanel* STARTWIZARD_PROVIDER_SCHLIB::GetWizardPanel( wxWindow* aParent )
{
    m_model = std::make_shared<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL>();
    return new PANEL_GLOBAL_SYM_LIB_TABLE_CONFIG( aParent, m_model );
}


bool STARTWIZARD_PROVIDER_SCHLIB::NeedsUserInput() const
{
    wxFileName fn = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    return !fn.FileExists();
}


void STARTWIZARD_PROVIDER_SCHLIB::Finish()
{

}
