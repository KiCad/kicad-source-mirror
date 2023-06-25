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
#include <unordered_set>
#include <wx/log.h>

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
                                              const STRING_UTF8_MAP* aProperties )
{
    std::vector<LIB_SYMBOL*> symbols;
    EnumerateSymbolLib( symbols, aLibraryPath, aProperties );

    for( LIB_SYMBOL* symbol : symbols )
        aSymbolNameList.Add( symbol->GetName() );
}


void SCH_DATABASE_PLUGIN::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                              const wxString&           aLibraryPath,
                                              const STRING_UTF8_MAP*         aProperties )
{
    wxCHECK_RET( m_libTable, "Database plugin missing library table handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    bool powerSymbolsOnly = ( aProperties &&
                              aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) !=
                              aProperties->end() );

    for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
    {
        std::vector<DATABASE_CONNECTION::ROW> results;

        if( !m_conn->SelectAll( table.table, table.key_col, results ) )
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

            std::string prefix = table.name.empty() ? "" : fmt::format( "{}/", table.name );
            wxString name( fmt::format( "{}{}", prefix,
                                        std::any_cast<std::string>( result[table.key_col] ) ) );

            LIB_SYMBOL* symbol = loadSymbolFromRow( name, table, result );

            if( symbol && ( !powerSymbolsOnly || symbol->IsPower() ) )
                aSymbolList.emplace_back( symbol );
        }
    }
}


LIB_SYMBOL* SCH_DATABASE_PLUGIN::LoadSymbol( const wxString&   aLibraryPath,
                                             const wxString&   aAliasName,
                                             const STRING_UTF8_MAP* aProperties )
{
    wxCHECK( m_libTable, nullptr );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    /*
     * Table names are tricky, in order to allow maximum flexibility to the user.
     * The slash character is used as a separator between a table name and symbol name, but symbol
     * names may also contain slashes and table names may now also be empty (which results in the
     * slash being dropped in the symbol name when placing a new symbol).  So, if a slash is found,
     * we check if the string before the slash is a valid table name.  If not, we assume the table
     * name is blank if our config has an entry for the null table.
     */

    std::string tableName = "";
    std::string symbolName( aAliasName.ToUTF8() );

    if( aAliasName.Contains( '/' ) )
    {
        tableName = std::string( aAliasName.BeforeFirst( '/' ).ToUTF8() );
        symbolName = std::string( aAliasName.AfterFirst( '/' ).ToUTF8() );
    }

    std::vector<const DATABASE_LIB_TABLE*> tablesToTry;

    for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
    {
        if( tableIter.name == tableName )
            tablesToTry.emplace_back( &tableIter );
    }

    if( tablesToTry.empty() )
    {
        wxLogTrace( traceDatabase, wxT( "LoadSymbol: table '%s' not found in config" ), tableName );
        return nullptr;
    }

    const DATABASE_LIB_TABLE* foundTable = nullptr;
    DATABASE_CONNECTION::ROW result;

    for( const DATABASE_LIB_TABLE* table : tablesToTry )
    {
        if( m_conn->SelectOne( table->table, std::make_pair( table->key_col, symbolName ),
                               result ) )
        {
            foundTable = table;
            wxLogTrace( traceDatabase, wxT( "LoadSymbol: SelectOne (%s, %s) found in %s" ),
                        table->key_col, symbolName, table->table );
        }
        else
        {
            wxLogTrace( traceDatabase, wxT( "LoadSymbol: SelectOne (%s, %s) failed for table %s" ),
                        table->key_col, symbolName, table->table );
        }
    }

    wxCHECK( foundTable, nullptr );

    return loadSymbolFromRow( aAliasName, *foundTable, result );
}


void SCH_DATABASE_PLUGIN::GetSubLibraryNames( std::vector<wxString>& aNames )
{
    ensureSettings( wxEmptyString );

    aNames.clear();

    std::set<wxString> tableNames;

    for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
    {
        if( tableNames.count( tableIter.name ) )
            continue;

        aNames.emplace_back( tableIter.name );
        tableNames.insert( tableIter.name );
    }
}


void SCH_DATABASE_PLUGIN::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
    std::copy( m_customFields.begin(), m_customFields.end(), std::back_inserter( aNames ) );
}


void SCH_DATABASE_PLUGIN::GetDefaultSymbolFields( std::vector<wxString>& aNames )
{
    std::copy( m_defaultShownFields.begin(), m_defaultShownFields.end(),
               std::back_inserter( aNames ) );
}


bool SCH_DATABASE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // TODO: Implement this sometime; but CheckHeader isn't even called...
    return true;
}


bool SCH_DATABASE_PLUGIN::TestConnection( wxString* aErrorMsg )
{
    if( m_conn && m_conn->IsConnected() )
        return true;

    connect();

    if( aErrorMsg && ( !m_conn || !m_conn->IsConnected() ) )
        *aErrorMsg = m_lastError;

    return m_conn && m_conn->IsConnected();
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

    connect();

    if( !m_conn || !m_conn->IsConnected() )
    {
        wxString msg = wxString::Format(
                    _( "Could not load database library: could not connect to database %s (%s)" ),
                    m_settings->m_Source.dsn, m_lastError );

        THROW_IO_ERROR( msg );
    }
}


void SCH_DATABASE_PLUGIN::connect()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before connect()!" );

    if( m_conn && !m_conn->IsConnected() )
        m_conn.reset();

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
            // for specifying on-disk databases that live next to the kicad_dbl file
            boost::replace_all( cs, "${CWD}", basePath );

            m_conn = std::make_unique<DATABASE_CONNECTION>( cs, m_settings->m_Source.timeout );
        }

        if( !m_conn->IsConnected() )
        {
            m_lastError = m_conn->GetLastError();
            m_conn.reset();
            return;
        }

        for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
        {
            std::set<std::string> columns;

            columns.insert( tableIter.key_col );
            columns.insert( tableIter.footprints_col );
            columns.insert( tableIter.symbols_col );

            columns.insert( tableIter.properties.description );
            columns.insert( tableIter.properties.footprint_filters );
            columns.insert( tableIter.properties.keywords );
            columns.insert( tableIter.properties.exclude_from_bom );
            columns.insert( tableIter.properties.exclude_from_board );

            for( const DATABASE_FIELD_MAPPING& field : tableIter.fields )
                columns.insert( field.column );

            m_conn->CacheTableInfo( tableIter.table, columns );
        }

        m_conn->SetCacheParams( m_settings->m_Cache.max_size, m_settings->m_Cache.max_age );
    }
}


std::optional<bool> SCH_DATABASE_PLUGIN::boolFromAny( const std::any& aVal )
{
    try
    {
        bool val = std::any_cast<bool>( aVal );
        return val;
    }
    catch( const std::bad_any_cast& )
    {
    }

    try
    {
        int val = std::any_cast<int>( aVal );
        return static_cast<bool>( val );
    }
    catch( const std::bad_any_cast& )
    {
    }

    try
    {
        wxString strval( std::any_cast<std::string>( aVal ).c_str(), wxConvUTF8 );

        if( strval.IsEmpty() )
            return std::nullopt;

        strval.MakeLower();

        for( const auto& trueVal : { wxS( "true" ), wxS( "yes" ), wxS( "y" ), wxS( "1" ) } )
        {
            if( strval.Matches( trueVal ) )
                return true;
        }

        for( const auto& falseVal : { wxS( "false" ), wxS( "no" ), wxS( "n" ), wxS( "0" ) } )
        {
            if( strval.Matches( falseVal ) )
                return false;
        }
    }
    catch( const std::bad_any_cast& )
    {
    }

    return std::nullopt;
}


LIB_SYMBOL* SCH_DATABASE_PLUGIN::loadSymbolFromRow( const wxString& aSymbolName,
                                                    const DATABASE_LIB_TABLE& aTable,
                                                    const DATABASE_CONNECTION::ROW& aRow )
{
    LIB_SYMBOL* symbol = nullptr;

    if( aRow.count( aTable.symbols_col ) )
    {
        LIB_SYMBOL* originalSymbol = nullptr;

        // TODO: Support multiple options for symbol
        std::string symbolIdStr = std::any_cast<std::string>( aRow.at( aTable.symbols_col ) );
        LIB_ID symbolId;
        symbolId.Parse( std::any_cast<std::string>( aRow.at( aTable.symbols_col ) ) );

        if( symbolId.IsValid() )
            originalSymbol = m_libTable->LoadSymbol( symbolId );

        if( originalSymbol )
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: found original symbol '%s'" ),
                        symbolIdStr );
            symbol = originalSymbol->Duplicate();
            symbol->SetSourceLibId( symbolId );
        }
        else if( !symbolId.IsValid() )
        {
            wxLogTrace( traceDatabase, wxT( "loadSymboFromRow: source symbol id '%s' is invalid, "
                                            "will create empty symbol" ), symbolIdStr );
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

    if( !aTable.properties.description.empty() && aRow.count( aTable.properties.description ) )
    {
        wxString value(
                std::any_cast<std::string>( aRow.at( aTable.properties.description ) ).c_str(),
                wxConvUTF8 );
        symbol->SetDescription( value );
    }

    if( !aTable.properties.keywords.empty() && aRow.count( aTable.properties.keywords ) )
    {
        wxString value( std::any_cast<std::string>( aRow.at( aTable.properties.keywords ) ).c_str(),
                        wxConvUTF8 );
        symbol->SetKeyWords( value );
    }

    if( !aTable.properties.footprint_filters.empty()
        && aRow.count( aTable.properties.footprint_filters ) )
    {
        wxString value( std::any_cast<std::string>( aRow.at( aTable.properties.footprint_filters ) )
                                .c_str(),
                        wxConvUTF8 );
        wxArrayString filters;
        filters.push_back( value );
        symbol->SetFPFilters( filters );
    }

    if( !aTable.properties.exclude_from_board.empty()
        && aRow.count( aTable.properties.exclude_from_board ) )
    {
        std::optional<bool> val = boolFromAny( aRow.at( aTable.properties.exclude_from_board ) );

        if( val )
        {
            symbol->SetExcludedFromBoard( *val );
        }
        else
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: exclude_from_board value for %s "
                                            "could not be cast to a boolean" ), aSymbolName );
        }
    }

    if( !aTable.properties.exclude_from_bom.empty()
        && aRow.count( aTable.properties.exclude_from_bom ) )
    {
        std::optional<bool> val = boolFromAny( aRow.at( aTable.properties.exclude_from_bom ) );

        if( val )
        {
            symbol->SetExcludedFromBOM( *val );
        }
        else
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: exclude_from_bom value for %s "
                                            "could not be cast to a boolean" ), aSymbolName );
        }
    }

    std::vector<LIB_FIELD*> fields;
    symbol->GetFields( fields );

    std::unordered_map<wxString, LIB_FIELD*> fieldsMap;

    for( LIB_FIELD* field : fields )
        fieldsMap[field->GetName()] = field;

    for( const DATABASE_FIELD_MAPPING& mapping : aTable.fields )
    {
        if( !aRow.count( mapping.column ) )
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: field %s not found in result" ),
                        mapping.column );
            continue;
        }

        wxString value( std::any_cast<std::string>( aRow.at( mapping.column ) ).c_str(),
                        wxConvUTF8 );

        if( mapping.name == wxT( "Value" ) )
        {
            LIB_FIELD& field = symbol->GetValueField();
            field.SetText( value );

            if( !mapping.inherit_properties )
            {
                field.SetVisible( mapping.visible_on_add );
                field.SetNameShown( mapping.show_name );
            }
            continue;
        }
        else if( mapping.name == wxT( "Datasheet" ) )
        {
            LIB_FIELD& field = symbol->GetDatasheetField();
            field.SetText( value );

            if( !mapping.inherit_properties )
            {
                field.SetVisible( mapping.visible_on_add );
                field.SetNameShown( mapping.show_name );

                if( mapping.visible_on_add )
                    field.SetAutoAdded( true );
            }

            continue;
        }

        LIB_FIELD* field;
        bool isNew = false;

        if( fieldsMap.count( mapping.name ) )
        {
            field = fieldsMap[mapping.name];
        }
        else
        {
            field = new LIB_FIELD( symbol->GetNextAvailableFieldId() );
            field->SetName( mapping.name );
            isNew = true;
            fieldsMap[mapping.name] = field;
        }

        if( !mapping.inherit_properties || isNew )
        {
            field->SetVisible( mapping.visible_on_add );
            field->SetAutoAdded( true );
            field->SetNameShown( mapping.show_name );
        }

        field->SetText( value );

        if( isNew )
            symbol->AddField( field );

        m_customFields.insert( mapping.name );

        if( mapping.visible_in_chooser )
            m_defaultShownFields.insert( mapping.name );
    }

    return symbol;
}
