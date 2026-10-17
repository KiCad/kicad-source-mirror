/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <chrono>
#include <iostream>
#include <string_view>
#include <set>
#include <utility>
#include <bs_thread_pool.hpp>
#include <wx/datetime.h>
#include <wx/log.h>
#include <wx/tokenzr.h>

#include <boost/algorithm/string.hpp>
#include <json_common.h>
#include <pin_map.h>

#include <libraries/symbol_library_adapter.h>
#include <database/database_connection.h>
#include <database/database_lib_settings.h>
#include <fmt.h>
#include <hash.h>
#include <ki_exception.h>
#include <lib_symbol.h>

#include "sch_io_database.h"

#include <dialog_database_lib_settings.h>


thread_local std::unordered_set<wxString> SCH_IO_DATABASE::s_inProgressLoads;


SCH_IO_DATABASE::SCH_IO_DATABASE() :
        SCH_IO( wxS( "Database library" ) ),
        m_adapter( nullptr ),
        m_settings(),
        m_conn()
{
    m_cacheTimestamp = 0;
    m_cacheSignature = 0;
}


SCH_IO_DATABASE::~SCH_IO_DATABASE()
{
    stopBackgroundRefresh();
}


void SCH_IO_DATABASE::EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                                          const wxString&   aLibraryPath,
                                          const std::map<std::string, UTF8>* aProperties )
{
    std::vector<LIB_SYMBOL*> symbols;
    EnumerateSymbolLib( symbols, aLibraryPath, aProperties );

    for( LIB_SYMBOL* symbol : symbols )
    {
        aSymbolNameList.Add( symbol->GetName() );
        delete symbol;
    }
}


void SCH_IO_DATABASE::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                          const wxString&           aLibraryPath,
                                          const std::map<std::string, UTF8>*         aProperties )
{
    wxCHECK_RET( m_adapter, "Database plugin missing library manager adapter handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();
    cacheLib();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    bool powerSymbolsOnly = ( aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly ) );

    std::shared_lock lock( m_cacheMutex );

    for( auto const& pair : m_nameToSymbolcache )
    {
        LIB_SYMBOL* symbol = pair.second.get();

        if( !powerSymbolsOnly || symbol->IsPower() )
            aSymbolList.emplace_back( symbol->Duplicate() );
    }
}


void SCH_IO_DATABASE::CheckLibrary( const wxString& aLibraryPath,
                                    const std::map<std::string, UTF8>* aProperties )
{
    ensureSettings( aLibraryPath );
    ensureConnection();
}


LIB_SYMBOL* SCH_IO_DATABASE::LoadSymbol( const wxString&   aLibraryPath,
                                         const wxString&   aAliasName,
                                         const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK_MSG( m_adapter, nullptr, "Database plugin missing library manager adapter handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    cacheLib();

    std::string tableName;
    std::string symbolName( aAliasName.ToUTF8() );

    {
        std::shared_lock lock( m_cacheMutex );

        if( auto cacheIt = m_nameToSymbolcache.find( aAliasName ); cacheIt != m_nameToSymbolcache.end() )
        {
            LIB_SYMBOL* cached = cacheIt->second.get();
            return cached->Duplicate();
        }

        auto sanitizedIt = m_sanitizedNameMap.find( aAliasName );

        if( sanitizedIt != m_sanitizedNameMap.end() )
        {
            tableName = sanitizedIt->second.first;
            symbolName = sanitizedIt->second.second;
        }
    }

    /*
     * Table names are tricky, in order to allow maximum flexibility to the user.
     * The slash character is used as a separator between a table name and symbol name, but symbol
     * names may also contain slashes and table names may now also be empty (which results in the
     * slash being dropped in the symbol name when placing a new symbol).  So, if a slash is found,
     * we check if the string before the slash is a valid table name.  If not, we assume the table
     * name is blank if our config has an entry for the null table.
     */

    if( tableName.empty() && aAliasName.Contains( '/' ) )
    {
        tableName = std::string( aAliasName.BeforeFirst( '/' ).ToUTF8() );
        symbolName = std::string( aAliasName.AfterFirst( '/' ).ToUTF8() );
    }

    std::vector<const DATABASE_LIB_TABLE*> tablesToTry;

    for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
    {
        // no table means globally unique keys, try all tables
        if( tableName.empty() || tableIter.name == tableName )
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
        if( m_conn->SelectOne( table->table, std::make_pair( table->key_col, symbolName ), result ) )
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

    if( !foundTable )
        return nullptr;

    return loadSymbolFromRow( aAliasName, *foundTable, result ).release();
}


void SCH_IO_DATABASE::GetSubLibraryNames( std::vector<wxString>& aNames )
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


void SCH_IO_DATABASE::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
    std::lock_guard<std::mutex> lock( m_symbolLoadMutex );

    std::copy( m_customFields.begin(), m_customFields.end(), std::back_inserter( aNames ) );
}


void SCH_IO_DATABASE::GetDefaultSymbolFields( std::vector<wxString>& aNames )
{
    std::lock_guard<std::mutex> lock( m_symbolLoadMutex );

    std::copy( m_defaultShownFields.begin(), m_defaultShownFields.end(), std::back_inserter( aNames ) );
}


size_t SCH_IO_DATABASE::computeSignature( const TABLE_RESULT_LIST& aTableResults ) const
{
    size_t signature = 0;

    for( const auto& [table, results] : aTableResults )
    {
        for( const DATABASE_CONNECTION::ROW& result : results )
        {
            size_t rowSignature = 0;

            for( const auto& [column, value] : result )
            {
                hash_combine( signature, std::string_view( column ) );

                if( const std::string* str = std::any_cast<std::string>( &value ) )
                    hash_combine( rowSignature, std::string_view( *str ) );
            }

            hash_combine( signature, rowSignature );

            if( auto it = result.find( table->symbols_col ); it != result.end() )
            {
                if( const std::string* str = std::any_cast<std::string>( &it->second ) )
                {
                    LIB_ID symbolId;
                    symbolId.Parse( *str );

                    if( symbolId.IsValid() )
                    {
                        const UTF8& nickname = symbolId.GetLibNickname();
                        hash_combine( signature, std::string_view( nickname.c_str() ) );

                        if( std::optional<int> libHash = m_adapter->GetLibraryModifyHash( nickname ) )
                            hash_combine( signature, *libHash );
                    }
                }
            }
        }
    }

    return signature;
}


LIB_SYMBOL* SCH_IO_DATABASE::resolveSourceSymbol( const LIB_ID& aId )
{
    if( !m_adapter )
        return nullptr;

    if( !m_previewMode )
        return m_adapter->LoadSymbol( aId );

    std::optional<LIBRARY_TABLE_ROW*> row = m_adapter->GetRow( aId.GetLibNickname() );

    // A database library would query its own tables, which the preview must not do
    if( row && SCH_IO_MGR::EnumFromStr( ( *row )->Type() ) == SCH_IO_MGR::SCH_DATABASE )
    {
        reportIssue( PREVIEW_ISSUE::FIELD::SYMBOLS,
                     wxString::Format( _( "Symbol '%s' is in a database library and is not previewed." ),
                                       aId.Format().wx_str() ),
                     RPT_SEVERITY_WARNING );
        return nullptr;
    }

    // Not memoized; the pointer belongs to another library's cache, which a later load may rebuild
    LIB_SYMBOL* symbol = m_adapter->LoadSymbol( aId );

    if( !symbol )
    {
        reportIssue( PREVIEW_ISSUE::FIELD::SYMBOLS,
                     wxString::Format( _( "Symbol '%s' was not found." ), aId.Format().wx_str() ),
                     RPT_SEVERITY_WARNING );
    }

    return symbol;
}


std::unique_ptr<DATABASE_CONNECTION> SCH_IO_DATABASE::CreateConnection( const DATABASE_LIB_SETTINGS& aSettings,
                                                                        bool aConnectNow, const wxString& aBaseDir )
{
    const DATABASE_SOURCE& source = aSettings.m_Source;

    if( source.connection_string.empty() )
    {
        return std::make_unique<DATABASE_CONNECTION>( source.dsn, source.username, source.password,
                                                      source.timeout, aConnectNow );
    }

    std::string cs = source.connection_string;
    wxString    baseDir = aBaseDir.IsEmpty() ? wxFileName( aSettings.GetFilename() ).GetPath() : aBaseDir;
    std::string basePath( baseDir.ToUTF8() );

    // Database drivers that use files operate on absolute paths, so provide a mechanism
    // for specifying on-disk databases that live next to the kicad_dbl file
    boost::replace_all( cs, "${CWD}", basePath );

    return std::make_unique<DATABASE_CONNECTION>( cs, source.timeout, aConnectNow );
}


wxString SCH_IO_DATABASE::SymbolNameForRow( const DATABASE_LIB_TABLE& aTable, bool aGloballyUniqueKeys,
                                            const DATABASE_CONNECTION::ROW& aRow, std::string* aRawKey )
{
    auto it = aRow.find( aTable.key_col );

    if( it == aRow.end() )
        return wxEmptyString;

    std::string rawName = std::any_cast<std::string>( it->second );
    UTF8        sanitizedName = LIB_ID::FixIllegalChars( rawName, false );
    std::string prefix = ( aGloballyUniqueKeys || aTable.name.empty() ) ? "" : fmt::format( "{}/", aTable.name );

    if( aRawKey )
        *aRawKey = rawName;

    return wxString( fmt::format( "{}{}", prefix, sanitizedName.c_str() ) );
}


std::vector<std::unique_ptr<LIB_SYMBOL>>
SCH_IO_DATABASE::BuildPreviewSymbols( SYMBOL_LIBRARY_ADAPTER* aLibs, const DATABASE_LIB_TABLE& aTable,
                                      bool aGloballyUniqueKeys, const std::vector<DATABASE_CONNECTION::ROW>& aRows,
                                      PREVIEW_ISSUES* aIssues )
{
    SCH_IO_DATABASE preview;
    preview.m_adapter = aLibs;
    preview.m_previewIssues = aIssues;
    preview.m_previewMode = true;

    std::vector<std::unique_ptr<LIB_SYMBOL>> symbols;
    std::map<wxString, std::pair<size_t, std::string>> listed;

    for( const DATABASE_CONNECTION::ROW& row : aRows )
    {
        std::string rawKey;
        wxString    name = SymbolNameForRow( aTable, aGloballyUniqueKeys, row, &rawKey );

        if( name.IsEmpty() )
            continue;

        try
        {
            std::unique_ptr<LIB_SYMBOL> symbol = preview.loadSymbolFromRow( name, aTable, row );

            if( !symbol )
                continue;

            // The library keeps the last row listed under a name, so the preview does the same
            if( auto it = listed.find( name ); it != listed.end() )
            {
                if( it->second.second == rawKey )
                {
                    preview.reportIssue( PREVIEW_ISSUE::FIELD::REPEATED_KEY,
                                         wxString::Format( _( "Key '%s' appears in more than one row; only the last "
                                                              "row is kept." ),
                                                           wxString::FromUTF8( rawKey ) ),
                                         RPT_SEVERITY_WARNING );
                }
                else
                {
                    preview.reportIssue( PREVIEW_ISSUE::FIELD::KEY,
                                         wxString::Format( _( "Keys '%s' and '%s' are both listed as '%s'; only the "
                                                              "last is kept." ),
                                                           wxString::FromUTF8( it->second.second ),
                                                           wxString::FromUTF8( rawKey ), name ),
                                         RPT_SEVERITY_WARNING );
                }

                symbols[it->second.first] = std::move( symbol );
                it->second.second = rawKey;
                continue;
            }

            listed[name] = { symbols.size(), rawKey };
            symbols.push_back( std::move( symbol ) );
        }
        catch( const IO_ERROR& e )
        {
            preview.reportIssue( PREVIEW_ISSUE::FIELD::KEY, e.What(), RPT_SEVERITY_ERROR );
        }
    }

    return symbols;
}


void SCH_IO_DATABASE::reportIssue( PREVIEW_ISSUE::FIELD aField, const wxString& aMessage, SEVERITY aSeverity ) const
{
    if( m_previewIssues )
        m_previewIssues->push_back( { aField, aMessage } );
    else
        Report( aMessage, aSeverity );
}


bool SCH_IO_DATABASE::materializeCache( const TABLE_RESULT_LIST& aTableResults,
        std::map<wxString, std::unique_ptr<LIB_SYMBOL>>& aSymbolCache,
        std::map<wxString, std::pair<std::string, std::string>>& aSanitizedNameMap )
{
    for( const auto& [table, results] : aTableResults )
    {
        for( const DATABASE_CONNECTION::ROW& result : results )
        {
            std::string rawName;
            wxString    name = SymbolNameForRow( *table, m_settings->m_GloballyUniqueKeys, result, &rawName );

            if( name.IsEmpty() )
                continue;

            aSanitizedNameMap[name] = std::make_pair( table->name, rawName );

            std::unique_ptr<LIB_SYMBOL> symbol = loadSymbolFromRow( name, *table, result );

            if( symbol )
                aSymbolCache[symbol->GetName()] = std::move( symbol );
        }
    }

    return !aSymbolCache.empty();
}


void SCH_IO_DATABASE::cacheLib()
{
    // Guard against re-entrant cacheLib() calls. A self-referential symbol row (issue #24249)
    // causes m_adapter->LoadSymbol to route back into SCH_IO_DATABASE::LoadSymbol, which would
    // otherwise call cacheLib() again while it is in the middle of populating its caches.
    if( m_inCacheLib )
        return;

    long long currentTimestampSeconds = wxDateTime::Now().GetValue().GetValue() / 1000;

    // After the initial load, the background refresh thread handles all cache updates.
    {
        std::shared_lock lock( m_cacheMutex );

        if( m_cachePopulated )
            return;
    }

    m_inCacheLib = true;

    struct CACHE_LIB_GUARD
    {
        bool* flag;
        ~CACHE_LIB_GUARD() { *flag = false; }
    } cacheLibGuard{ &m_inCacheLib };

    // Re-query the database (the connection layer caches results subject to its own max_age) and
    // compute a lightweight signature of the raw rows so we can skip the costly materialization
    // when nothing relevant has changed.
    TABLE_RESULT_LIST tableResults;

    for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
    {
        std::vector<DATABASE_CONNECTION::ROW> results;

        if( !m_conn->SelectAll( table.table, table.key_col, results ) )
        {
            if( !m_conn->GetLastError().empty() )
                THROW_IO_ERRORF( _( "Error reading database table %s: %s" ), table.table, m_conn->GetLastError() );

            continue;
        }

        tableResults.emplace_back( &table, std::move( results ) );
    }

    size_t signature = computeSignature( tableResults );

    {
        std::unique_lock lock( m_cacheMutex );

        if( m_cachePopulated && signature == m_cacheSignature )
        {
            m_cacheTimestamp = currentTimestampSeconds;
            return;
        }
    }

    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> newSymbolCache;
    std::map<wxString, std::pair<std::string, std::string>> newSanitizedNameMap;

    materializeCache( tableResults, newSymbolCache, newSanitizedNameMap );

    {
        std::unique_lock lock( m_cacheMutex );

        m_nameToSymbolcache = std::move( newSymbolCache );
        m_sanitizedNameMap = std::move( newSanitizedNameMap );

        m_cacheTimestamp = currentTimestampSeconds;
        m_cacheSignature = signature;
        m_cachePopulated = true;
        m_modifyHash++;
    }

    if( !m_refreshRunning.load() )
        startBackgroundRefresh();
}


void SCH_IO_DATABASE::startBackgroundRefresh()
{
    if( m_refreshRunning.exchange( true ) )
        return;

    wxLogTrace( traceDatabase, wxT( "Starting background refresh thread" ) );
    m_refreshThread = std::thread( &SCH_IO_DATABASE::backgroundRefreshWorker, this );
}


void SCH_IO_DATABASE::stopBackgroundRefresh()
{
    m_refreshRunning = false;
    m_refreshCV.notify_all();

    if( m_refreshThread.joinable() )
        m_refreshThread.join();
}


void SCH_IO_DATABASE::backgroundRefreshWorker()
{
    BS::this_thread::set_os_thread_name( "dblib bg" );

    while( m_refreshRunning.load() )
    {
        long long maxAge = 0;

        {
            std::unique_lock lock( m_cacheMutex );

            if( m_settings )
                maxAge = m_settings->m_Cache.max_age;
        }

        if( maxAge <= 0 )
            maxAge = 1;

        std::shared_lock connGuard( m_cacheMutex );

        if( m_conn && m_cachePopulated.load() )
        {
            wxLogTrace( traceDatabase, wxT( "Initiating background refresh" ) );

            try
            {
                std::vector<std::pair<const DATABASE_LIB_TABLE*, std::vector<DATABASE_CONNECTION::ROW>>> tableResults;
                bool querySuccess = true;

                for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
                {
                    m_conn->ClearCache( table.table );

                    std::vector<DATABASE_CONNECTION::ROW> results;

                    if( !m_conn->SelectAll( table.table, table.key_col, results ) )
                    {
                        wxLogTrace( traceDatabase, wxT( "Background refresh: SelectAll failed for table %s" ),
                                    table.table );
                        querySuccess = false;
                        break;
                    }

                    tableResults.emplace_back( &table, std::move( results ) );
                }

                if( querySuccess )
                {
                    size_t signature = computeSignature( tableResults );

                    bool dataChanged = false;

                    {
                        // Upgrade to unique lock for cache mutation
                        connGuard.unlock();
                        std::unique_lock lock( m_cacheMutex );

                        if( signature == m_cacheSignature )
                            m_cacheTimestamp = wxDateTime::Now().GetValue().GetValue() / 1000;
                        else
                            dataChanged = true;
                    }

                    if( dataChanged )
                    {
                        std::map<wxString, std::unique_ptr<LIB_SYMBOL>> newSymbolCache;
                        std::map<wxString, std::pair<std::string, std::string>> newSanitizedNameMap;

                        materializeCache( tableResults, newSymbolCache, newSanitizedNameMap );

                        wxLogTrace( traceDatabase, wxT( "Background refresh: new data" ) );

                        {
                            std::unique_lock lock( m_cacheMutex );

                            m_nameToSymbolcache = std::move( newSymbolCache );
                            m_sanitizedNameMap = std::move( newSanitizedNameMap );

                            m_cacheTimestamp = wxDateTime::Now().GetValue().GetValue() / 1000;
                            m_cacheSignature = signature;
                            m_cachePopulated = true;
                            m_modifyHash++;
                        }
                    }
                    else
                    {
                        wxLogTrace( traceDatabase, wxT( "Background refresh: no new data" ) );
                    }
                }
            }
            catch( const IO_ERROR& e )
            {
                wxLogTrace( traceDatabase, wxT( "Background refresh failed: %s; cache preserved" ), e.What() );
            }
            catch( const std::exception& e )
            {
                wxLogTrace( traceDatabase, wxT( "Background refresh failed: %s; cache preserved" ), e.what() );
            }
        }

        {
            std::unique_lock lock( m_refreshMutex );
            m_refreshCV.wait_for( lock, std::chrono::seconds( maxAge ),
                                  [this]()
                                  {
                                      return !m_refreshRunning.load();
                                  } );
        }
    }
}


void SCH_IO_DATABASE::ensureSettings( const wxString& aSettingsPath )
{
    auto tryLoad =
            [&]()
            {
                if( !m_settings->LoadFromFile() )
                {
                    THROW_IO_ERRORF( _( "Could not load database library: settings file %s missing or invalid" ),
                                     aSettingsPath );
                }

                m_settings->ResolvePassword();
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


void SCH_IO_DATABASE::ensureConnection()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before ensureConnection!" );

    connect();

    if( !m_conn || !m_conn->IsConnected() )
    {
        THROW_IO_ERRORF( _( "Could not load database library: could not connect to database %s (%s)" ),
                         m_settings->m_Source.dsn, m_lastError );
    }
}


void SCH_IO_DATABASE::connect()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before connect()!" );

    {
        std::unique_lock connLock( m_cacheMutex );

        if( m_conn && !m_conn->IsConnected() )
            m_conn.reset();
    }

    if( !m_conn )
    {
        std::unique_ptr<DATABASE_CONNECTION> newConn = CreateConnection( *m_settings );

        if( !newConn->IsConnected() )
        {
            m_lastError = newConn->GetLastError();
            return;
        }

        for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
        {
            // Trusted even if unreported by the driver, to support generated columns (#16952)
            std::set<std::string> requiredColumns{ tableIter.key_col,
                                                   tableIter.footprints_col,
                                                   tableIter.symbols_col };

            for( const DATABASE_FIELD_MAPPING& field : tableIter.fields )
                requiredColumns.insert( field.column );

            if( !tableIter.pins_col.empty() )
                requiredColumns.insert( tableIter.pins_col );

            // Only used if confirmed present, so a misconfigured mapping can't break the table (#23532)
            std::set<std::string> optionalColumns{ tableIter.properties.description,
                                                   tableIter.properties.footprint_filters,
                                                   tableIter.properties.keywords,
                                                   tableIter.properties.exclude_from_sim,
                                                   tableIter.properties.exclude_from_bom,
                                                   tableIter.properties.exclude_from_board };

            newConn->CacheTableInfo( tableIter.table, requiredColumns, optionalColumns );
        }

        newConn->SetCacheParams( m_settings->m_Cache.max_size, m_settings->m_Cache.max_age );

        std::unique_lock connLock( m_cacheMutex );
        m_conn = std::move( newConn );
    }
}


std::optional<bool> SCH_IO_DATABASE::boolFromAny( const std::any& aVal )
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


std::unique_ptr<LIB_SYMBOL>  SCH_IO_DATABASE::loadSymbolFromRow( const wxString& aSymbolName,
                                                const DATABASE_LIB_TABLE& aTable,
                                                const DATABASE_CONNECTION::ROW& aRow )
{
    std::unique_ptr<LIB_SYMBOL> symbol = nullptr;

    if( aRow.contains( aTable.symbols_col ) )
    {
        std::string symbols = std::any_cast<std::string>( aRow.at( aTable.symbols_col ) );
        wxString    symbolsStr = wxString( symbols.c_str(), wxConvUTF8 );
        wxStringTokenizer tokenizer( symbolsStr, ";\t\r\n", wxTOKEN_STRTOK );

        std::vector<LIB_ID> symbolIds;

        while( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken();
            LIB_ID   id;
            id.Parse( std::string( token.ToUTF8() ) );

            if( id.IsValid() )
            {
                symbolIds.push_back( id );
            }
            else
            {
                reportIssue( PREVIEW_ISSUE::FIELD::SYMBOLS,
                             wxString::Format( _( "'%s' is not a symbol; expected Library:Symbol." ), token ),
                             RPT_SEVERITY_WARNING );
            }
        }

        // A row's Symbols column may resolve back into the same database library (issue #24249,
        // e.g. a mistyped library nickname). The adapter would route that lookup back into
        // SCH_IO_DATABASE::LoadSymbol and re-enter loadSymbolFromRow on the same row until the
        // stack overflows. Track in-flight LIB_IDs and skip the recursive load on re-entry.
        std::vector<LIB_SYMBOL*> sourceSymbols;
        std::vector<wxString>    sourceSymbolNames;

        for( const LIB_ID& symbolId : symbolIds )
        {
            wxString symbolIdStr = symbolId.Format().wx_str();

            if( !s_inProgressLoads.insert( symbolIdStr ).second )
            {
                wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: cycle detected resolving '%s' "
                                                "(row '%s' in table '%s'); skipping recursive load" ),
                            symbolIdStr, aSymbolName, aTable.name );
                continue;
            }

            struct CYCLE_GUARD
            {
                std::unordered_set<wxString>* set;
                wxString                      key;
                ~CYCLE_GUARD() { set->erase( key ); }
            } guard{ &s_inProgressLoads, symbolIdStr };

            LIB_SYMBOL* src = nullptr;

            {
                std::lock_guard lock( m_symbolLoadMutex );
                src = resolveSourceSymbol( symbolId );
            }

            if( src )
            {
                sourceSymbols.push_back( src );
                sourceSymbolNames.push_back( src->GetName() );
            }
            else
            {
                wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: source symbol '%s' not found" ), symbolIdStr );
            }
        }

        if( sourceSymbols.empty() )
        {
            // Actual symbol not found: return metadata only; error will be indicated in the
            // symbol chooser
            symbol.reset( new LIB_SYMBOL( aSymbolName ) );
        }
        else if( sourceSymbols.size() == 1 )
        {
            symbol.reset( sourceSymbols[0]->Duplicate() );
            symbol->SetSourceLibId( symbolIds[0] );
            symbol->SetName( aSymbolName );
        }
        else
        {
            // If the database row specifies multiple symbols, put together a composite,
            // but since source symbols may already have multiple body styles, we
            // need to flatten out any common to all body styles items on source symbols.
            // Since we don't support varying all properties, take those from the first one.

            symbol.reset( new LIB_SYMBOL( aSymbolName ) );
            symbol->SetSourceLibId( symbolIds[0] );

            symbol->SetUnitCount( sourceSymbols[0]->GetUnitCount(), false );
            symbol->SetPowerSymbolProp( sourceSymbols[0]->IsPower() );
            symbol->SetShowPinNames( sourceSymbols[0]->GetShowPinNames() );
            symbol->SetShowPinNumbers( sourceSymbols[0]->GetShowPinNumbers() );
            symbol->SetPinNameOffset( sourceSymbols[0]->GetPinNameOffset() );

            for( FIELD_T fieldId : MANDATORY_FIELDS )
            {
                if( SCH_FIELD* srcField = sourceSymbols[0]->GetField( fieldId ) )
                {
                    SCH_FIELD* dstField = symbol->GetField( fieldId );
                    *dstField = *srcField;
                    dstField->SetParent( symbol.get() );
                }
            }

            std::vector<wxString> bodyStyleNames;
            std::vector<int> sourceBodyStyleCounts;
            std::vector<int> compositeBodyStyleBase;

            int nextBodyStyle = 1;

            for( size_t i = 0; i < sourceSymbols.size(); ++i )
            {
                int srcCount = std::max( 1, sourceSymbols[i]->GetBodyStyleCount() );
                sourceBodyStyleCounts.push_back( srcCount );
                compositeBodyStyleBase.push_back( nextBodyStyle );

                if( srcCount == 1 )
                {
                    bodyStyleNames.push_back( sourceSymbolNames[i] );
                }
                else
                {
                    for( int style = 1; style <= srcCount; ++style )
                    {
                        wxString styleName = sourceSymbols[i]->GetBodyStyleDescription( style, false );
                        bodyStyleNames.push_back( sourceSymbolNames[i] + wxT( " (" ) + styleName + wxT( ")" ) );
                    }
                }

                nextBodyStyle += srcCount;
            }

            int totalBodyStyles = nextBodyStyle - 1;

            symbol->SetHasDeMorganBodyStyles( false );
            symbol->SetBodyStyleNames( bodyStyleNames );

            std::set<wxString> mergedFieldNames;

            for( size_t i = 0; i < sourceSymbols.size(); ++i )
            {
                std::unique_ptr<LIB_SYMBOL> srcSymbol = sourceSymbols[i]->Flatten();
                int srcCount = sourceBodyStyleCounts[i];
                int base = compositeBodyStyleBase[i];

                for( SCH_ITEM& item : srcSymbol->GetDrawItems() )
                {
                    if( item.Type() == SCH_FIELD_T )
                    {
                        SCH_FIELD& field = static_cast<SCH_FIELD&>( item );

                        if( field.IsMandatory() || !mergedFieldNames.insert( field.GetName() ).second )
                            continue;

                        int targetStyle;

                        if( item.GetBodyStyle() == ALL_BODY_STYLES )
                            targetStyle = base;
                        else
                            targetStyle = base + ( item.GetBodyStyle() - 1 );

                        if( targetStyle > totalBodyStyles )
                            continue;

                        SCH_ITEM* newItem = item.Duplicate( IGNORE_PARENT_GROUP );
                        newItem->SetParent( symbol.get() );
                        newItem->SetBodyStyle( targetStyle );
                        symbol->AddDrawItem( newItem, false );
                    }
                    else if( item.GetBodyStyle() == ALL_BODY_STYLES )
                    {
                        for( int bodyStyle = 0; bodyStyle < srcCount; ++bodyStyle )
                        {
                            SCH_ITEM* newItem = item.Duplicate( IGNORE_PARENT_GROUP );
                            newItem->SetParent( symbol.get() );
                            newItem->SetBodyStyle( base + bodyStyle );
                            symbol->AddDrawItem( newItem, false );
                        }
                    }
                    else
                    {
                        int targetStyle = base + ( item.GetBodyStyle() - 1 );

                        if( targetStyle > totalBodyStyles )
                            continue;

                        SCH_ITEM* newItem = item.Duplicate( IGNORE_PARENT_GROUP );
                        newItem->SetParent( symbol.get() );
                        newItem->SetBodyStyle( targetStyle );
                        symbol->AddDrawItem( newItem, false );
                    }
                }
            }

            symbol->GetDrawItems().sort();
        }
    }
    else
    {
        symbol.reset( new LIB_SYMBOL( aSymbolName ) );
    }

    LIB_ID libId = symbol->GetLibId();
    libId.SetSubLibraryName( aTable.name );
    symbol->SetLibId( libId );

    wxArrayString footprintsList;

    if( aRow.count( aTable.footprints_col ) )
    {
        std::string footprints = std::any_cast<std::string>( aRow.at( aTable.footprints_col ) );

        wxString footprintsStr = wxString( footprints.c_str(), wxConvUTF8 );
        wxStringTokenizer tokenizer( footprintsStr, ";\t\r\n", wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
            footprintsList.Add( tokenizer.GetNextToken() );

        if( footprintsList.size() > 0 )
            symbol->GetFootprintField().SetText( footprintsList[0] );
    }
    else
    {
        wxLogTrace( traceDatabase, wxT( "loadSymboFromRow: footprint field %s not found." ), aTable.footprints_col );
    }

    // Pin-to-pad maps (issue #2282): attach non-destructively.  The pins column carries either the
    // spec-form named object { "pin_maps": [...], "associated_footprints": [...] } or the legacy
    // flat MR !2540 array (read for one release, bound to the row's footprints).
    if( !aTable.pins_col.empty() && aRow.count( aTable.pins_col ) )
    {
        try
        {
            std::string jsonStr = std::any_cast<std::string>( aRow.at( aTable.pins_col ) );

            if( !jsonStr.empty() )
            {
                nlohmann::json json = nlohmann::json::parse( jsonStr );

                if( json.is_object() && json.contains( "pin_maps" ) )
                {
                    symbol->SetPinMaps( ParsePinMapSet( json ) );
                    symbol->SetAssociatedFootprints( ParseAssociatedFootprints( json ) );
                }
                else if( !footprintsList.IsEmpty() )
                {
                    std::unordered_map<wxString, std::vector<wxString>> assignments = ParseLegacyPinAssignments( json );

                    if( !assignments.empty() )
                    {
                        const wxString mapName = wxS( "Database" );

                        symbol->PinMaps().AddOrReplace( MakeLegacyPinMap( mapName, assignments ) );

                        // Bind the one named map to every footprint the row offers, mirroring the
                        // old behaviour where the assignment applied regardless of footprint.
                        std::vector<ASSOCIATED_FOOTPRINT> associations;

                        for( const wxString& footprint : footprintsList )
                        {
                            LIB_ID fpId;
                            fpId.Parse( footprint );
                            associations.push_back( { fpId, mapName } );
                        }

                        symbol->SetAssociatedFootprints( std::move( associations ) );
                    }
                }
            }
        }
        catch( const std::exception& e )
        {
            // Surface a malformed pin-map payload to the user instead of silently dropping it; the
            // symbol still loads without the map (issue #2282).
            reportIssue( PREVIEW_ISSUE::FIELD::PIN_MAP,
                         wxString::Format( _( "Error parsing pin map for database symbol '%s': %s" ), aSymbolName,
                                           e.what() ),
                         RPT_SEVERITY_ERROR );
        }
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
        footprintsList.push_back( value );
    }

    symbol->SetFPFilters( footprintsList );

    if( !aTable.properties.exclude_from_sim.empty()
        && aRow.count( aTable.properties.exclude_from_sim ) )
    {
        std::optional<bool> val = boolFromAny( aRow.at( aTable.properties.exclude_from_sim ) );

        if( val )
        {
            symbol->SetExcludedFromSim( *val );
        }
        else
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: exclude_from_sim value for %s "
                                            "could not be cast to a boolean" ), aSymbolName );
        }
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

    std::vector<SCH_FIELD*> fields;
    symbol->GetFields( fields );

    std::unordered_map<wxString, SCH_FIELD*> fieldsMap;

    for( SCH_FIELD* field : fields )
        fieldsMap[field->GetName()] = field;

    static const wxString c_valueFieldName( wxS( "Value" ) );
    static const wxString c_datasheetFieldName( wxS( "Datasheet" ) );
    static const wxString c_footprintFieldName( wxS( "Footprint" ) );

    // User fields produced from the database mapping must appear in the order they are
    // declared in the .kicad_dbl file, not in lexicographic order of their values. Start
    // assigning ordinals above whatever the source LIB_SYMBOL already uses so that any
    // pre-existing user fields keep their relative position before the database fields.
    int dbFieldOrdinal = symbol->GetNextFieldOrdinal();

    for( const DATABASE_FIELD_MAPPING& mapping : aTable.fields )
    {
        if( !aRow.count( mapping.column ) )
        {
            wxLogTrace( traceDatabase, wxT( "loadSymbolFromRow: field %s not found in result" ), mapping.column );
            continue;
        }

        // Skip footprint field if it maps to the footprints column, since that column is
        // already processed above with tokenization for semicolon-separated multiple footprints.
        if( mapping.name_wx == c_footprintFieldName && mapping.column == aTable.footprints_col )
            continue;

        std::string strValue;

        try
        {
            strValue = std::any_cast<std::string>( aRow.at( mapping.column ) );
        }
        catch( std::bad_any_cast& )
        {
        }

        wxString value( strValue.c_str(), wxConvUTF8 );

        if( mapping.name_wx == c_valueFieldName )
        {
            SCH_FIELD& field = symbol->GetValueField();
            field.SetText( value );

            if( !mapping.inherit_properties )
            {
                field.SetVisible( mapping.visible_on_add );
                field.SetNameShown( mapping.show_name );
            }

            continue;
        }
        else if( mapping.name_wx == c_datasheetFieldName )
        {
            SCH_FIELD& field = symbol->GetDatasheetField();
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

        SCH_FIELD* field;
        bool isNew = false;

        if( fieldsMap.count( mapping.name_wx ) )
        {
            field = fieldsMap[mapping.name_wx];
        }
        else
        {
            field = new SCH_FIELD( nullptr, FIELD_T::USER );
            field->SetName( mapping.name_wx );
            isNew = true;
            fieldsMap[mapping.name_wx] = field;
        }

        // Assign a sort-order ordinal so the property editor and BOM see the fields in the
        // order declared in the .kicad_dbl file. Without this, all USER fields share the
        // same FIELD_T::USER id and fall through to value-based comparison in operator<.
        // NB: a DB mapping that lands on a mandatory field by name (e.g. Reference or
        // Description) must keep its FIELD_T identity for downstream lookups.
        field->SetOrdinal( dbFieldOrdinal++, field->IsMandatory() ? field->GetId() : FIELD_T::USER );

        if( !mapping.inherit_properties || isNew )
        {
            field->SetVisible( mapping.visible_on_add );
            field->SetAutoAdded( true );
            field->SetNameShown( mapping.show_name );
        }

        field->SetText( value );

        if( isNew )
            symbol->AddDrawItem( field, false );

        m_customFields.insert( mapping.name_wx );

        if( mapping.visible_in_chooser )
            m_defaultShownFields.insert( mapping.name_wx );
    }

    symbol->GetDrawItems().sort();

    // Field mappings (including Description) are applied with SCH_FIELD::SetText, which does not
    // refresh the cached values the library tree and chooser read. Without this the upper chooser
    // panel keeps the source symbol's description while the details panel shows the database value.
    symbol->RefreshLibraryTreeCaches();

    return symbol;
}


DIALOG_SHIM* SCH_IO_DATABASE::CreateConfigurationDialog( wxWindow* aParent, const wxString& aLibraryPath )
{
    return new DIALOG_DATABASE_LIB_SETTINGS( aParent, aLibraryPath, m_adapter );
}
