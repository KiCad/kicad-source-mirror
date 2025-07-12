/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <pgm_base.h>
#include <bitmaps/bitmaps_list.h>
#include <bitmaps/bitmap_types.h>
#include <dialogs/panel_startwizard_libraries_base.h>
#include <libraries/library_manager.h>
#include <startwizard/startwizard.h>
#include <startwizard/startwizard_provider_libraries.h>
#include <startwizard/startwizard_provider_settings.h>


class PANEL_STARTWIZARD_LIBRARIES : public PANEL_STARTWIZARD_LIBRARIES_BASE
{
public:
    PANEL_STARTWIZARD_LIBRARIES( const std::shared_ptr<STARTWIZARD_PROVIDER_LIBRARIES_MODEL>& aModel,
                                 wxWindow* aParent, STARTWIZARD* aWizard ) :
            PANEL_STARTWIZARD_LIBRARIES_BASE( aParent ),
            m_model( aModel ),
            m_wizard( aWizard )
    {
        m_bmpWarning->SetBitmap( KiBitmapBundle( BITMAPS::dialog_warning ) );
        m_sizerWarning->Layout();
    }

    bool TransferDataFromWindow() override
    {
        if( m_rbDefaultTables->GetValue() )
            m_model->mode = STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS;
        else if( m_rbImport->GetValue() )
            m_model->mode = STARTWIZARD_LIBRARIES_MODE::IMPORT;
        else
            m_model->mode = STARTWIZARD_LIBRARIES_MODE::CREATE_BLANK;

        return true;
    }

    bool TransferDataToWindow() override
    {
        switch( m_model->mode )
        {
        case STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS:  m_rbDefaultTables->SetValue( true ); break;
        case STARTWIZARD_LIBRARIES_MODE::IMPORT:        m_rbImport->SetValue( true );        break;
        case STARTWIZARD_LIBRARIES_MODE::CREATE_BLANK:  m_rbBlankTables->SetValue( true );   break;
        }

        if( auto settings = dynamic_cast<STARTWIZARD_PROVIDER_SETTINGS*>( m_wizard->GetProvider( "settings" ) ) )
        {
            if( !settings->NeedsUserInput() )
            {
                // If the user didn't see the settings screen at all, just hide this option; we
                // only offer import when we have the context of a previous version settings
                // path selected by the user
                m_rbImport->Show( false );
            }
            else if( settings->GetModel().mode == STARTWIZARD_SETTINGS_MODE::USE_DEFAULTS )
            {
                // But if they did see the screen and chose not to import, show the option but
                // disable it as a breadcrumb that they can go back and choose to import settings
                m_rbImport->Disable();

                if( m_rbImport->GetValue() )
                    m_rbDefaultTables->SetValue( true );
            }
            else
            {
                m_rbImport->Enable();
            }
        }

        wxString missingTablesText;
        bool showWarning = false;

        for( const LIBRARY_TABLE_TYPE& type : m_model->missing_tables )
        {
            if( !LIBRARY_MANAGER::IsTableValid( LIBRARY_MANAGER::DefaultGlobalTablePath( type ) ) )
                showWarning = true;

            switch( type )
            {
            case LIBRARY_TABLE_TYPE::SYMBOL:
                missingTablesText.Append( _( "Symbol library table" ) + "\n" );
                break;

            case LIBRARY_TABLE_TYPE::FOOTPRINT:
                missingTablesText.Append( _( "Footprint library table" ) + "\n" );
                break;

            case LIBRARY_TABLE_TYPE::DESIGN_BLOCK:
                missingTablesText.Append( _( "Design Block library table" ) + "\n" );
                break;
            }
        }

        m_stRequiredTables->SetLabel( missingTablesText.BeforeLast( '\n' ) );

        m_sizerWarning->Hide( !showWarning );

        return true;
    }

private:
    std::shared_ptr<STARTWIZARD_PROVIDER_LIBRARIES_MODEL> m_model;
    STARTWIZARD* m_wizard;
};


STARTWIZARD_PROVIDER_LIBRARIES::STARTWIZARD_PROVIDER_LIBRARIES() :
        STARTWIZARD_PROVIDER( wxT( "Libraries" ) )
{
}


bool STARTWIZARD_PROVIDER_LIBRARIES::NeedsUserInput() const
{
    return !LIBRARY_MANAGER::GlobalTablesValid();
}


wxPanel* STARTWIZARD_PROVIDER_LIBRARIES::GetWizardPanel( wxWindow* aParent, STARTWIZARD* aWizard )
{
    m_model = std::make_shared<STARTWIZARD_PROVIDER_LIBRARIES_MODEL>();
    m_model->missing_tables = LIBRARY_MANAGER::InvalidGlobalTables();
    return new PANEL_STARTWIZARD_LIBRARIES( m_model, aParent, aWizard );
}


void STARTWIZARD_PROVIDER_LIBRARIES::Finish()
{
    // TODO(JE) handle importing tables from previous version and sanitizing them

    bool populateTables = m_model->mode == STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS;

    for( LIBRARY_TABLE_TYPE type : LIBRARY_MANAGER::InvalidGlobalTables() )
    {
        LIBRARY_MANAGER::CreateGlobalTable( type, populateTables );
    }
}


void STARTWIZARD_PROVIDER_LIBRARIES::ApplyDefaults()
{
    for( LIBRARY_TABLE_TYPE type : LIBRARY_MANAGER::InvalidGlobalTables() )
    {
        LIBRARY_MANAGER::CreateGlobalTable( type, true );
    }
}

