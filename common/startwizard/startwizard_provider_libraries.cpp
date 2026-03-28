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
#include <paths.h>
#include <startwizard/startwizard.h>
#include <startwizard/startwizard_provider_libraries.h>
#include <startwizard/startwizard_provider_settings.h>
#include <trace_helpers.h>
#include <regex>
#include <set>
#include <wx/log.h>


class PANEL_STARTWIZARD_LIBRARIES : public PANEL_STARTWIZARD_LIBRARIES_BASE
{
public:
    PANEL_STARTWIZARD_LIBRARIES( const std::shared_ptr<STARTWIZARD_PROVIDER_LIBRARIES_MODEL>& aModel,
                                 wxWindow* aParent, STARTWIZARD* aWizard ) :
            PANEL_STARTWIZARD_LIBRARIES_BASE( aParent ),
            m_model( aModel ),
            m_wizard( aWizard )
    {
        wxWindowBase::SetMaxSize( { FromDIP( 640 ), -1 } );
        m_bmpWarning->SetBitmap( KiBitmapBundle( BITMAPS::dialog_warning ) );
        m_sizerWarning->Layout();

        Bind( wxEVT_RADIOBUTTON, &PANEL_STARTWIZARD_LIBRARIES::OnModeChanged, this,
              m_rbDefaultTables->GetId() );
        Bind( wxEVT_RADIOBUTTON, &PANEL_STARTWIZARD_LIBRARIES::OnModeChanged, this,
              m_rbImport->GetId() );
        Bind( wxEVT_RADIOBUTTON, &PANEL_STARTWIZARD_LIBRARIES::OnModeChanged, this,
              m_rbBlankTables->GetId() );

        InitTableListMsg();
    }

    bool TransferDataFromWindow() override
    {
        if( m_rbDefaultTables->GetValue() )
            m_model->mode = STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS;
        else if( m_rbImport->GetValue() )
            m_model->mode = STARTWIZARD_LIBRARIES_MODE::IMPORT;
        else
            m_model->mode = STARTWIZARD_LIBRARIES_MODE::CREATE_BLANK;

        m_model->mode_initialized = true;

        m_model->migrate_built_in_libraries = m_cbMigrateBuiltInLibraries->GetValue();

        return true;
    }

    bool TransferDataToWindow() override
    {
        if( !m_model->mode_initialized )
        {
            // If the user is importing settings from a previous version, let's default to taking their
            // library settings as well, since we can clean up old stock paths now.
            if( auto settings = dynamic_cast<STARTWIZARD_PROVIDER_SETTINGS*>( m_wizard->GetProvider( "settings" ) ) )
            {
                if( settings->NeedsUserInput() )
                    m_model->mode = STARTWIZARD_LIBRARIES_MODE::IMPORT;
                else
                    m_model->mode = STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS;
            }

            m_model->mode_initialized = true;
        }

        switch( m_model->mode )
        {
        case STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS:  m_rbDefaultTables->SetValue( true ); break;
        case STARTWIZARD_LIBRARIES_MODE::IMPORT:        m_rbImport->SetValue( true );        break;
        case STARTWIZARD_LIBRARIES_MODE::CREATE_BLANK:  m_rbBlankTables->SetValue( true );   break;
        }

        m_cbMigrateBuiltInLibraries->SetValue( m_model->migrate_built_in_libraries );

        if( auto settings = dynamic_cast<STARTWIZARD_PROVIDER_SETTINGS*>( m_wizard->GetProvider( "settings" ) ) )
        {
            if( !settings->NeedsUserInput() )
            {
                // If the user didn't see the settings screen at all, just hide this option; we
                // only offer import when we have the context of a previous version settings
                // path selected by the user
                m_rbImport->Show( false );
                m_cbMigrateBuiltInLibraries->Show( false );
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

        UpdateMigrateCheckboxState();

        m_bmpWarning->Show( m_showWarning );
        m_stWarning->Show( m_showWarning );

        return true;
    }

    void InitTableListMsg()
    {
        wxString missingTablesText;
        m_showWarning = false;

        for( const LIBRARY_TABLE_TYPE& type : m_model->missing_tables )
        {
            if( !LIBRARY_MANAGER::IsTableValid( LIBRARY_MANAGER::StockTablePath( type ) ) )
                m_showWarning = true;

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

            case LIBRARY_TABLE_TYPE::UNINITIALIZED:
                break;
            }
        }

        m_stRequiredTables->SetLabel( missingTablesText.BeforeLast( '\n' ) );
    }

    void OnSize( wxSizeEvent& aEvt ) override
    {
        aEvt.Skip();
        m_stIntro->Wrap( GetClientSize().x - FromDIP( 20 ) );
        m_stWarning->Wrap( GetClientSize().x - m_bmpWarning->GetSize().x - FromDIP( 28 ) );
    }

private:
    void OnModeChanged( wxCommandEvent& aEvt )
    {
        aEvt.Skip();
        UpdateMigrateCheckboxState();
    }

    void UpdateMigrateCheckboxState()
    {
        m_cbMigrateBuiltInLibraries->Enable( m_rbImport->IsShown()
                                             && m_rbImport->IsEnabled()
                                             && m_rbImport->GetValue() );
    }

    std::shared_ptr<STARTWIZARD_PROVIDER_LIBRARIES_MODEL> m_model;
    STARTWIZARD* m_wizard;
    bool m_showWarning;
};


STARTWIZARD_PROVIDER_LIBRARIES::STARTWIZARD_PROVIDER_LIBRARIES() :
        STARTWIZARD_PROVIDER( _( "Libraries" ) )
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
    bool populateTables = m_model->mode == STARTWIZARD_LIBRARIES_MODE::USE_DEFAULTS;

    if( m_model->mode == STARTWIZARD_LIBRARIES_MODE::IMPORT && m_model->migrate_built_in_libraries )
    {
        for( LIBRARY_TABLE_TYPE type : { LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_TYPE::FOOTPRINT } )
        {
            wxString tablePath = LIBRARY_MANAGER::DefaultGlobalTablePath( type );
            wxString stockPath = LIBRARY_MANAGER::StockTablePath( type );

            if( !LIBRARY_MANAGER::IsTableValid( tablePath ) )
                continue;

            wxFileName tableFile( tablePath );
            LIBRARY_TABLE table( tableFile, LIBRARY_TABLE_SCOPE::GLOBAL );

            if( !table.IsOk() )
                continue;

            const std::regex builtInPattern( type == LIBRARY_TABLE_TYPE::SYMBOL
                    ? R"(^\$\{KICAD\d+_SYMBOL_DIR\})"
                    : R"(^\$\{KICAD\d+_FOOTPRINT_DIR\})" );

            bool insertStock = true;
            std::set<wxString> toRemove;

            for( const LIBRARY_TABLE_ROW& row : table.Rows() )
            {
                if( std::regex_search( row.URI().ToStdString(), builtInPattern ) )
                {
                    toRemove.insert( row.URI() );
                    wxLogTrace( traceLibraries, wxT( "StartWizard libraries migration: removing old stock row '%s'" ), row.URI() );
                }
                else if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME && row.URI() == stockPath )
                {
                    insertStock = false;
                    wxLogTrace( traceLibraries, wxT( "StartWizard libraries migration: migrated table already has valid stock setup" ) );
                }
            }

            auto toErase = std::ranges::remove_if( table.Rows(),
                    [&]( const LIBRARY_TABLE_ROW& aRow )
                    {
                        return toRemove.contains( aRow.URI() );
                    } );

            table.Rows().erase( toErase.begin(), toErase.end() );

            if( insertStock )
            {
                LIBRARY_TABLE_ROW chained = table.MakeRow();
                chained.SetType( LIBRARY_TABLE_ROW::TABLE_TYPE_NAME );
                chained.SetNickname( wxT( "KiCad" ) );
                chained.SetDescription( _( "KiCad Default Libraries" ) );
                chained.SetURI( stockPath );
                table.Rows().insert( table.Rows().begin(), chained );
            }

            wxLogTrace( traceLibraries, wxT( "StartWizard libraries migration: removed %zu rows; saving" ),
                        toRemove.size() );

            table.Save();
        }
    }

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

