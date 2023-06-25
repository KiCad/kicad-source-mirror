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

#include "startwizard_provider_fplib.h"
#include <dialogs/panel_global_lib_table_config.h>

#include <confirm.h>
#include <kiface_base.h>
#include <kiway.h>

#include "fp_lib_table.h"

class PANEL_GLOBAL_FP_LIB_TABLE_CONFIG : public PANEL_GLOBAL_LIB_TABLE_CONFIG
{
public:
    PANEL_GLOBAL_FP_LIB_TABLE_CONFIG( wxWindow* aParent, std::shared_ptr<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL> aModel ) :
            PANEL_GLOBAL_LIB_TABLE_CONFIG( aParent, _( "footprint" ), aModel, KIWAY::FACE_PCB ){};

    virtual ~PANEL_GLOBAL_FP_LIB_TABLE_CONFIG(){};

    virtual wxFileName GetGlobalTableFileName() override { return FP_LIB_TABLE::GetGlobalTableFileName(); };

    bool TransferDataFromWindow() override;
};


bool PANEL_GLOBAL_FP_LIB_TABLE_CONFIG::TransferDataFromWindow()
{

    return true;
}


STARTWIZARD_PROVIDER_FPLIB::STARTWIZARD_PROVIDER_FPLIB() :
        STARTWIZARD_PROVIDER( wxT( "Footprint Libraries" ) )
{
}


wxPanel* STARTWIZARD_PROVIDER_FPLIB::GetWizardPanel( wxWindow* aParent )
{
    m_model = std::make_shared<PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL>();
    return new PANEL_GLOBAL_FP_LIB_TABLE_CONFIG( aParent, m_model );
}


bool STARTWIZARD_PROVIDER_FPLIB::NeedsUserInput() const
{
    wxFileName fn = FP_LIB_TABLE::GetGlobalTableFileName();

    return !fn.FileExists();
}


void STARTWIZARD_PROVIDER_FPLIB::Finish()
{
    // Create an empty table if requested by the user.
    if( m_model->m_tableMode == PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL::TABLE_MODE::EMPTY )
    {
        FP_LIB_TABLE emptyTable;

        try
        {
            emptyTable.Save( FP_LIB_TABLE::GetGlobalTableFileName() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( NULL, wxString::Format( _( "Error occurred writing empty footprint "
                                                     "library table '%s'." ),
                                                  FP_LIB_TABLE::GetGlobalTableFileName() )
                                        + wxS( "\n" ) + ioe.What() );
        }
    }

    wxFileName fn = m_model->m_defaultTablePath;

    if( m_model->m_tableMode == PANEL_GLOBAL_LIB_TABLE_CONFIG_MODEL::TABLE_MODE::CUSTOM )
    {
        if( m_model->m_customTablePath.IsEmpty() )
        {
            DisplayError( NULL, _( "Please select a footprint library table file." ) );
        }

        fn = m_model->m_customTablePath;

        // Make sure the footprint library table to copy actually exists.
        if( !fn.FileExists() )
        {
            DisplayError( NULL, wxString::Format( _( "File '%s' not found." ), fn.GetFullPath() ) );
        }

        // Make sure the footprint library table to copy is a valid footprint library table file.
        FP_LIB_TABLE tmpTable;

        try
        {
            tmpTable.Load( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( NULL,
                          wxString::Format( _( "'%s' is not a valid footprint library table." ),
                                            fn.GetFullPath() )
                                  + wxS( "\n" ) + ioe.What() );
        }
    }

    // Create the config path if it doesn't already exist.
    wxFileName fpTableFileName = FP_LIB_TABLE::GetGlobalTableFileName();

    if( !fpTableFileName.DirExists() && !fpTableFileName.Mkdir( 0x777, wxPATH_MKDIR_FULL ) )
    {
        DisplayError( NULL, wxString::Format( _( "Cannot create library table path '%s'." ),
                                              fpTableFileName.GetPath() ) );
    }

    // Copy the global footprint library table file to the user config.
    if( !::wxCopyFile( fn.GetFullPath(), fpTableFileName.GetFullPath() ) )
    {
        DisplayError( NULL, wxString::Format( _( "Cannot copy footprint library table from:\n"
                                                 "%s\n"
                                                 "to:\n"
                                                 "%s." ),
                                              fn.GetFullPath(), fpTableFileName.GetFullPath() ) );
    }

    // Load the successfully copied footprint library table file.  This should not fail
    // since the file was tested above.  Check for failure anyway to keep the compiler
    // from complaining.
    try
    {
        FP_LIB_TABLE::LoadGlobalTable( GFootprintTable );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( NULL,
                      _( "Error loading footprint library table." ) + wxS( "\n" ) + ioe.What() );
    }
}