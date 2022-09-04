/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <iostream>

#include <boost/algorithm/string.hpp>

#include <database/database_connection.h>
#include <database/database_lib_settings.h>
#include <fmt.h>
#include <lib_symbol.h>
#include <symbol_lib_table.h>

#include "sch_database_plugin.h"


SCH_DATABASE_PLUGIN::SCH_DATABASE_PLUGIN() :
        m_libTable( nullptr ),
        m_settings(),
        m_conn()
{
}


SCH_DATABASE_PLUGIN::~SCH_DATABASE_PLUGIN()
{
}


void SCH_DATABASE_PLUGIN::EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                                              const wxString&   aLibraryPath,
                                              const PROPERTIES* aProperties )
{
    std::vector<LIB_SYMBOL*> symbols;
    EnumerateSymbolLib( symbols, aLibraryPath, aProperties );

    for( LIB_SYMBOL* symbol : symbols )
        aSymbolNameList.Add( symbol->GetName() );
}


void SCH_DATABASE_PLUGIN::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                              const wxString&           aLibraryPath,
                                              const PROPERTIES*         aProperties )
{
    wxCHECK_RET( m_libTable, "Database plugin missing library table handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) !=
                              aProperties->end() );

    for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
    {
        std::vector<DATABASE_CONNECTION::ROW> results;

        if( !m_conn->SelectAll( table.table, results ) )
        {
            if( !m_conn->GetLastError().empty() )
            {
                wxString msg = wxString::Format( _( "Error reading database table %s: %s" ),
                                                 table.table, m_conn->GetLastError() );
                THROW_IO_ERROR( msg );
            }

            continue;
        }

        for( DATABASE_CONNECTION::ROW& result : results )
        {
            if( !result.count( table.key_col ) )
                continue;

            wxString name( fmt::format( "{}/{}", table.name,
                                        std::any_cast<std::string>( result[table.key_col] ) ) );

            LIB_SYMBOL* symbol = loadSymbolFromRow( name, table, result );

            if( symbol && ( !powerSymbolsOnly || symbol->IsPower() ) )
                aSymbolList.emplace_back( symbol );
        }
    }
}


LIB_SYMBOL* SCH_DATABASE_PLUGIN::LoadSymbol( const wxString&   aLibraryPath,
                                             const wxString&   aAliasName,
                                             const PROPERTIES* aProperties )
{
    wxCHECK( m_libTable, nullptr );
    ensureSettings( aLibraryPath );
    ensureConnection();

    const DATABASE_LIB_TABLE* table = nullptr;

    std::string tableName( aAliasName.BeforeFirst( '/' ).ToUTF8() );
    std::string symbolName( aAliasName.AfterFirst( '/' ).ToUTF8() );

    for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
    {
        if( tableIter.name == tableName )
        {
            table = &tableIter;
            break;
        }
    }

    if( !table )
    {
        wxLogTrace( traceDatabase, wxT( "LoadSymbol: table %s not found in config" ), tableName );
        return nullptr;
    }

    DATABASE_CONNECTION::ROW result;

    if( !m_conn->SelectOne( table->table, std::make_pair( table->key_col, symbolName ), result ) )
    {
        wxLogTrace( traceDatabase, wxT( "LoadSymbol: SelectOne (%s, %s) failed" ), table->key_col,
                    symbolName );
        return nullptr;
    }

    return loadSymbolFromRow( aAliasName, *table, result );
}


void SCH_DATABASE_PLUGIN::GetSubLibraryNames( std::vector<wxString>& aNames )
{
    ensureSettings( wxEmptyString );

    aNames.clear();

    for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
        aNames.emplace_back( tableIter.name );
}


bool SCH_DATABASE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // TODO: Implement this sometime; but CheckHeader isn't even called...
    return true;
}


void SCH_DATABASE_PLUGIN::ensureSettings( const wxString& aSettingsPath )
{
    auto tryLoad =
            [&]()
            {
                if( !m_settings->LoadFromFile() )
                {
                    wxString msg = wxString::Format(
                        _( "Could not load database library: settings file %s missing or invalid" ),
                        aSettingsPath );

                    THROW_IO_ERROR( msg );
                }
            };

    if( !m_settings && !aSettingsPath.IsEmpty() )
    {
        std::string path( aSettingsPath.ToUTF8() );
        m_settings = std::make_unique<DATABASE_LIB_SETTINGS>( path );
        m_settings->SetReadOnly( true );

        tryLoad();
    }
    else if( !m_conn && m_settings )
    {
        // If we have valid settings but no connection yet; reload settings in case user is editing
        tryLoad();
    }
    else if( m_conn && m_settings && !aSettingsPath.IsEmpty() )
    {
        wxASSERT_MSG( aSettingsPath == m_settings->GetFilename(),
                      "Path changed for database library without re-initializing plugin!" );
    }
    else if( !m_settings )
    {
        wxLogTrace( traceDatabase, wxT( "ensureSettings: no settings but no valid path!" ) );
    }
}


void SCH_DATABASE_PLUGIN::ensureConnection()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before ensureConnection!" );

    if( !m_conn )
    {
        if( m_settings->m_Source.connection_string.empty() )
        {
            m_conn = std::make_unique<DATABASE_CONNECTION>( m_settings->m_Source.dsn,
                                                            m_settings->m_Source.username,
                                                            m_settings->m_Source.password,
                                                            m_settings->m_Source.timeout );
        }
        else
        {
            std::string cs = m_settings->m_Source.connection_string;
            std::string basePath( wxFileName( m_settings->GetFilename() ).GetPath().ToUTF8() );

            // Database drivers that use files operate on absolute paths, so provide a mechanism
            // for specifing on-disk databases that live next to the kicad_dbl file
            boost::replace_all( cs, "${CWD}", basePath );

            m_conn = std::make_unique<DATABASE_CONNECTION>( cs, m_settings->m_Source.timeout );
        }

        if( !m_conn->IsConnected() )
        {
            wxString msg = wxString::Format(
                    _( "Could not load database library: could not connect to database %s (%s)" ),
                    m_settings->m_Source.dsn,
                    m_conn->GetLastError() );

            m_conn.reset();

            THROW_IO_ERROR( msg );
        }

        m_conn->SetCacheParams( m_settings->m_Cache.max_size, m_settings->m_Cache.max_age );
    }
}


LIB_SYMBOL* SCH_DATABASE_PLUGIN::loadSymbolFromRow( const wxString& aSymbolName,
                                                    const DATABASE_LIB_TABLE& aTable,
                                                    const DATABASE_CONNECTION::ROW& aRow )
{
    LIB_SYMBOL* symbol = nullptr;

    if( aRow.count( aTable.symbols_col ) )
    {
        // TODO: Support multiple options for symbol
        std::string symbolIdStr = std::any_cast<std::string>( aRow.at( aTable.symbols_col ) );
        LIB_ID symbolId;
        symbolId.Parse( std::any_cast<std::string>( aRow.at( aTable.symbols_col ) ) );

        LIB_SYMBOL* originalSymbol = m_libTable->LoadSymbol( symbolId );

        if( originalSymbol )
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: found original symbol '%s'" ),
                        symbolIdStr );
            symbol = originalSymbol->Duplicate();
        }
        else
        {
            wxLogTrace( traceDatabase, wxT( "loadSymboFromRow: source symbol '%s' not found, "
                                            "will create empty symbol" ), symbolIdStr );
        }
    }

    if( !symbol )
    {
        // Actual symbol not found: return metadata only; error will be indicated in the
        // symbol chooser
        symbol = new LIB_SYMBOL( aSymbolName );
    }
    else
    {
        symbol->SetName( aSymbolName );
    }

    symbol->LibId().SetSubLibraryName( aTable.name );

    if( aRow.count( aTable.footprints_col ) )
    {
        // TODO: Support multiple footprint choices
        std::string footprints = std::any_cast<std::string>( aRow.at( aTable.footprints_col ) );
        wxString footprint = wxString( footprints.c_str(), wxConvUTF8 ).BeforeFirst( ';' );
        symbol->GetFootprintField().SetText( footprint );
    }
    else
    {
        wxLogTrace( traceDatabase, wxT( "loadSymboFromRow: footprint field %s not found." ),
                    aTable.footprints_col );
    }

    for( const DATABASE_FIELD_MAPPING& mapping : aTable.fields )
    {
        if( !aRow.count( mapping.column ) )
        {
            wxLogTrace( traceDatabase, wxT( "loadSymboFromRow: field %s not found in result" ),
                        mapping.column );
            continue;
        }

        wxString value( std::any_cast<std::string>( aRow.at( mapping.column ) ).c_str(),
                        wxConvUTF8 );

        if( mapping.name == wxT( "ki_description" ) )
        {
            symbol->SetDescription( value );
            continue;
        }
        else if( mapping.name == wxT( "ki_keywords" ) )
        {
            symbol->SetKeyWords( value );
            continue;
        }
        else if( mapping.name == wxT( "ki_fp_filters" ) )
        {
            // TODO: Handle this here?
            continue;
        }
        else if( mapping.name == wxT( "Value" ) )
        {
            LIB_FIELD& field = symbol->GetValueField();
            field.SetText( value );
            field.SetVisible( mapping.visible_on_add );
            field.SetNameShown( mapping.show_name );
            continue;
        }
        else if( mapping.name == wxT( "Datasheet" ) )
        {
            LIB_FIELD& field = symbol->GetDatasheetField();
            field.SetText( value );
            field.SetVisible( mapping.visible_on_add );
            field.SetNameShown( mapping.show_name );

            if( mapping.visible_on_add )
                field.SetAutoAdded( true );

            continue;
        }

        LIB_FIELD* field = new LIB_FIELD( symbol->GetNextAvailableFieldId() );
        field->SetName( mapping.name );
        field->SetText( value );
        field->SetVisible( mapping.visible_on_add );
        field->SetAutoAdded( true );
        field->SetNameShown( mapping.show_name );

        symbol->AddField( field );
    }

    return symbol;
}
