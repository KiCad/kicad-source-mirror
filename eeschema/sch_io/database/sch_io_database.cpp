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

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string_view>
#include <set>
#include <utility>
#include <vector>
#include <bs_thread_pool.hpp>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/tokenzr.h>

#include <boost/algorithm/string.hpp>
#include <json_common.h>
#include <pin_map.h>

#include <libraries/symbol_library_adapter.h>
#include <database/database_connection.h>
#include <database/database_lib_settings.h>
#include <database/file_in_use_backoff.h>
#include <fmt.h>
#include <gestfich.h>
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


void SCH_IO_DATABASE::CancelLoad()
{
    // Set under the wait mutex so a waiter between its check and its sleep cannot miss the wakeup
    {
        std::lock_guard lock( m_refreshMutex );
        m_loadCancelled = true;
    }

    m_refreshCV.notify_all();
}


void SCH_IO_DATABASE::ResumeLoad() noexcept
{
    std::lock_guard lock( m_refreshMutex );
    m_loadCancelled = false;
}


void SCH_IO_DATABASE::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                          const wxString&           aLibraryPath,
                                          const std::map<std::string, UTF8>*         aProperties )
{
    wxCHECK_RET( m_adapter, "Database plugin missing library manager adapter handle!" );
    ensureSettings( aLibraryPath );
    cacheLib( ensureConnection() );

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
    std::shared_ptr<DATABASE_CONNECTION> conn = ensureConnection();

    cacheLib( conn );

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
        if( conn->SelectOne( table->table, std::make_pair( table->key_col, symbolName ), result ) )
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


/// The directory ${CWD} stands for, which is the settings file's unless the caller supplies one
static wxString cwdDirFor( const DATABASE_LIB_SETTINGS& aSettings, const wxString& aBaseDir )
{
    return aBaseDir.IsEmpty() ? wxFileName( aSettings.GetFilename() ).GetPath() : aBaseDir;
}


SCH_IO_DATABASE::CONNECTION_RESULT SCH_IO_DATABASE::CreateConnection( const DATABASE_LIB_SETTINGS& aSettings,
                                                                      bool aConnectNow, const wxString& aBaseDir )
{
    return connectionFor( DATABASE_BACKEND::Create( aSettings.m_Source, cwdDirFor( aSettings, aBaseDir ) ),
                          aConnectNow );
}


SCH_IO_DATABASE::CONNECTION_RESULT SCH_IO_DATABASE::connectionFor( DB_BACKEND_RESULT&& aBackend, bool aConnectNow )
{
    if( !aBackend.backend )
        return { nullptr, aBackend.failure, std::move( aBackend.error ) };

    return { std::make_unique<DATABASE_CONNECTION>( std::move( aBackend.backend ), aConnectNow ), DB_FAILURE::UNKNOWN,
             std::string() };
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

    return wxString::FromUTF8( fmt::format( "{}{}", prefix, sanitizedName.c_str() ) );
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


void SCH_IO_DATABASE::cacheLib( const std::shared_ptr<DATABASE_CONNECTION>& aConn )
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

    // A backend may open sessions for the load, so they are closed before the slow materialization
    {
        DATABASE_CONNECTION::LOAD_CYCLE cycle( *aConn,
                                               [this]()
                                               {
                                                   return m_loadCancelled.load();
                                               } );

        for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
        {
            // Nothing is published from a load that was cancelled
            if( m_loadCancelled.load() )
                THROW_IO_ERROR( _( "Database library load cancelled" ) );

            std::vector<DATABASE_CONNECTION::ROW> results;
            DATABASE_CONNECTION::QUERY_STATUS     status;

            if( !aConn->SelectAll( table.table, table.key_col, results, &status ) )
            {
                // A read the cancel stopped says nothing about the table
                if( m_loadCancelled.load() )
                    THROW_IO_ERROR( _( "Database library load cancelled" ) );

                if( !status.error.empty() )
                    THROW_IO_ERRORF( _( "Error reading database table %s: %s" ), table.table, status.error );

                continue;
            }

            tableResults.emplace_back( &table, std::move( results ) );
        }
    }

    if( m_loadCancelled.load() )
        THROW_IO_ERROR( _( "Database library load cancelled" ) );

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
    // The flag check and the thread install stay together so a stop cannot slip between them
    std::lock_guard lock( m_refreshMutex );

    if( m_stopping.load() || m_refreshRunning.load() )
        return;

    m_refreshRunning = true;

    wxLogTrace( traceDatabase, wxT( "Starting background refresh thread" ) );
    m_refreshThread = std::thread( &SCH_IO_DATABASE::backgroundRefreshWorker, this );
}


void SCH_IO_DATABASE::stopBackgroundRefresh()
{
    std::thread worker;

    // Set under the wait mutex so a waiter between its check and its sleep cannot miss the wakeup
    {
        std::lock_guard lock( m_refreshMutex );

        m_stopping = true;
        m_refreshRunning = false;
        worker = std::move( m_refreshThread );
    }

    m_refreshCV.notify_all();

    if( worker.joinable() )
        worker.join();
}


std::chrono::milliseconds SCH_IO_DATABASE::busyRetryWait( long long aMaxAgeSeconds, int aAttempt,
                                                          std::minstd_rand& aRng )
{
    // A writer holds its lock for seconds, so waiting a whole max_age after one leaves the cache stale for longer
    static constexpr long long FIRST_RETRY_MS = 3000;

    const long long ceiling =
            std::min( aMaxAgeSeconds * 1000, FIRST_RETRY_MS << std::clamp( aAttempt, 0, MAX_BUSY_ATTEMPT ) );

    // Readers that failed on one lock must not all come back together and keep the writer out
    std::uniform_int_distribution<long long> spread( ceiling / 2, ceiling );

    return std::chrono::milliseconds( spread( aRng ) );
}


bool SCH_IO_DATABASE::connectionStopped() const
{
    return m_stopping.load() || m_loadCancelled.load();
}


void SCH_IO_DATABASE::backgroundRefreshWorker()
{
    BS::this_thread::set_os_thread_name( "dblib bg" );

    std::minstd_rand jitter( std::random_device{}() );
    int              busyRetries = 0;

    while( m_refreshRunning.load() )
    {
        long long maxAge = 0;
        bool      busy = false;

        {
            std::unique_lock lock( m_cacheMutex );

            if( m_settings )
                maxAge = m_settings->m_Cache.max_age;
        }

        if( maxAge <= 0 )
            maxAge = 1;

        std::shared_ptr<DATABASE_CONNECTION> conn = currentConnection();
        bool                                 dropped = m_cachePopulated.load() && ( !conn || !conn->IsConnected() );

        // A failed read leaves the backend disconnected, and only a new connection can end that
        if( dropped && m_refreshRunning.load() )
        {
            // A catalog a writer holds is retried on the busy backoff, like a locked read
            busy = connect().failure == DB_FAILURE::FILE_IN_USE;
            conn = currentConnection();
        }

        // The pass owns its connection, so a reconnect cannot free it and no lock is held across the reads
        if( conn && conn->IsConnected() && m_cachePopulated.load() )
        {
            wxLogTrace( traceDatabase, wxT( "Initiating background refresh" ) );

            try
            {
                TABLE_RESULT_LIST tableResults;
                bool              querySuccess = true;

                {
                    DATABASE_CONNECTION::LOAD_CYCLE cycle( *conn,
                                                           [this]()
                                                           {
                                                               return !m_refreshRunning.load() || m_stopping.load();
                                                           } );

                    for( const DATABASE_LIB_TABLE& table : m_settings->m_Tables )
                    {
                        // A close must not wait for the tables left in the pass
                        if( !m_refreshRunning.load() )
                        {
                            querySuccess = false;
                            break;
                        }

                        conn->ClearCache( table.table );

                        std::vector<DATABASE_CONNECTION::ROW> results;
                        DATABASE_CONNECTION::QUERY_STATUS     status;

                        if( !conn->SelectAll( table.table, table.key_col, results, &status ) )
                        {
                            wxLogTrace( traceDatabase, wxT( "Background refresh: SelectAll failed for table %s" ),
                                        table.table );
                            querySuccess = false;

                            // A lost connection goes to the reconnect at the top of the next pass instead
                            busy = status.connected && status.failure == DB_FAILURE::FILE_IN_USE;
                            break;
                        }

                        tableResults.emplace_back( &table, std::move( results ) );
                    }
                }

                // A stop during the last table must not wait for a publish
                if( !m_refreshRunning.load() )
                    querySuccess = false;

                if( querySuccess )
                {
                    size_t signature = computeSignature( tableResults );

                    bool dataChanged = false;

                    {
                        std::unique_lock lock( m_cacheMutex );

                        if( signature == m_cacheSignature )
                            m_cacheTimestamp = wxDateTime::Now().GetValue().GetValue() / 1000;
                        else
                            dataChanged = true;
                    }

                    if( dataChanged )
                    {
                        std::map<wxString, std::unique_ptr<LIB_SYMBOL>>         newSymbolCache;
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

        // A connection the foreground replaced must not stay open through the wait
        conn.reset();

        std::chrono::milliseconds wait = std::chrono::seconds( maxAge );

        if( busy )
        {
            wait = busyRetryWait( maxAge, busyRetries, jitter );
            busyRetries = std::min( busyRetries + 1, MAX_BUSY_ATTEMPT );
        }
        else
        {
            busyRetries = 0;
        }

        {
            std::unique_lock lock( m_refreshMutex );
            m_refreshCV.wait_for( lock, wait,
                                  [this]()
                                  {
                                      return !m_refreshRunning.load();
                                  } );
        }
    }
}


void SCH_IO_DATABASE::ensureSettings( const wxString& aSettingsPath )
{
    if( !m_settings && !aSettingsPath.IsEmpty() )
    {
        std::string path( aSettingsPath.ToUTF8() );
        m_settings = std::make_unique<DATABASE_LIB_SETTINGS>( path );
        m_settings->SetReadOnly( true );

        // A read that fails leaves these defaults to size the next attempt's wait
        m_settings->Load();

        loadSettingsBounded( m_settings->m_Source.timeout );
    }
    else if( !currentConnection() && m_settings && !m_cachePopulated )
    {
        // Reload in case the user is editing; the read is bounded because the share may be down
        loadSettingsBounded( m_settings->m_Source.timeout );
    }
    else if( currentConnection() && m_settings && !aSettingsPath.IsEmpty() )
    {
        wxASSERT_MSG( aSettingsPath == m_settings->GetFilename(),
                      "Path changed for database library without re-initializing plugin!" );
    }
    else if( !m_settings )
    {
        wxLogTrace( traceDatabase, wxT( "ensureSettings: no settings but no valid path!" ) );
    }
}


namespace
{
// Owned jointly by the callers and the reader thread, which is all the thread touches
struct SETTINGS_READ
{
    std::mutex              m;
    std::condition_variable cv;
    bool                    done = false;
    bool                    ok = false;
    std::string             bytes;
};


// Reached only from callers, so its static lifetime cannot race a detached reader
struct SETTINGS_READ_REGISTRY
{
    std::mutex                                          m;
    std::map<std::string, std::weak_ptr<SETTINGS_READ>> reads;
};


SETTINGS_READ_REGISTRY& settingsReadRegistry()
{
    static SETTINGS_READ_REGISTRY* instance = new SETTINGS_READ_REGISTRY;

    return *instance;
}


// Runs detached, so it uses no wx state and never throws
void readSettingsFile( const std::shared_ptr<SETTINGS_READ>& aRead, const std::filesystem::path& aPath )
{
    std::string bytes;
    bool        ok = false;

    try
    {
        std::ifstream     in( aPath, std::ios::binary );
        std::vector<char> chunk( 1 << 16 );

        while( in )
        {
            in.read( chunk.data(), chunk.size() );
            bytes.append( chunk.data(), static_cast<size_t>( in.gcount() ) );
        }

        ok = in.eof() && !in.bad();
    }
    catch( ... )
    {
        bytes.clear();
        ok = false;
    }

    std::lock_guard lock( aRead->m );
    aRead->bytes = std::move( bytes );
    aRead->ok = ok;
    aRead->done = true;
    aRead->cv.notify_all();
}
} // namespace


void SCH_IO_DATABASE::loadSettingsBounded( int aTimeoutSeconds )
{
    const wxString path = m_settings->GetLoadPath();

    if( m_loadCancelled.load() )
        THROW_IO_ERRORF( _( "Database library settings file %s could not be reached" ), path );

    std::shared_ptr<SETTINGS_READ> read;

    {
        SETTINGS_READ_REGISTRY& registry = settingsReadRegistry();
        std::lock_guard         registryLock( registry.m );

        for( auto it = registry.reads.begin(); it != registry.reads.end(); )
        {
            it = it->second.expired() ? registry.reads.erase( it ) : std::next( it );
        }

        // Normalised without touching the file system, since the share may be down
        wxFileName file( path );
        file.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE );

        const std::filesystem::path nativePath = ToFsPath( file.GetFullPath() );

        file.Normalize( wxPATH_NORM_CASE );

        const std::string key( file.GetFullPath().ToUTF8() );

        if( std::shared_ptr<SETTINGS_READ> pending = registry.reads[key].lock() )
        {
            std::lock_guard readLock( pending->m );

            if( !pending->done )
                read = pending;
        }

        if( !read )
        {
            read = std::make_shared<SETTINGS_READ>();
            registry.reads[key] = read;

            try
            {
                std::thread( readSettingsFile, read, nativePath ).detach();
            }
            catch( ... )
            {
                registry.reads.erase( key );
                THROW_IO_ERRORF( _( "Database library settings file %s could not be reached" ), path );
            }
        }
    }

    const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds( DatabaseFileWaitSeconds( aTimeoutSeconds ) );

    std::unique_lock lock( read->m );

    while( !read->done )
    {
        if( m_loadCancelled.load() || std::chrono::steady_clock::now() >= deadline )
            THROW_IO_ERRORF( _( "Database library settings file %s could not be reached" ), path );

        read->cv.wait_for( lock, std::chrono::milliseconds( 50 ) );
    }

    const bool        ok = read->ok;
    const std::string bytes = read->bytes;

    lock.unlock();

    if( !ok || !m_settings->LoadFromBuffer( bytes ) )
        THROW_IO_ERRORF( _( "Could not load database library: settings file %s missing or invalid" ), path );

    m_settings->ResolvePassword();
}


std::shared_ptr<DATABASE_CONNECTION> SCH_IO_DATABASE::currentConnection()
{
    std::shared_lock lock( m_cacheMutex );
    return m_conn;
}


std::shared_ptr<DATABASE_CONNECTION> SCH_IO_DATABASE::ensureConnection()
{
    wxCHECK_MSG( m_settings, nullptr, "Call ensureSettings before ensureConnection!" );

    CONNECT_STATUS status = connect();

    std::shared_ptr<DATABASE_CONNECTION> conn = currentConnection();

    if( !conn || !conn->IsConnected() )
    {
        // No backend exists for the source, so there is no database to name
        if( status.failure == DB_FAILURE::INVALID_TYPE || status.failure == DB_FAILURE::NOT_SUPPORTED_ON_PLATFORM )
            THROW_IO_ERRORF( _( "Could not load database library: %s" ), status.error );

        const DATABASE_SOURCE source = m_settings->ExpandedSource();
        const std::string     name = source.Describe();
        const bool            configured = !source.dsn.empty() || !source.connection_string.empty();

        // Nothing safe to name means the source was withheld, unless none is configured at all
        const wxString shown = !name.empty() ? wxString::FromUTF8( name )
                               : configured  ? _( "connection string" )
                                             : _( "(not configured)" );

        THROW_IO_ERRORF( _( "Could not load database library: could not connect to database %s (%s)" ), shown,
                         status.error );
    }

    return conn;
}


SCH_IO_DATABASE::CONNECT_STATUS SCH_IO_DATABASE::connect()
{
    wxCHECK_MSG( m_settings, CONNECT_STATUS(), "Call ensureSettings before connect()!" );

    // Every load and symbol lookup comes through here, so a live connection returns before any other work
    if( std::shared_ptr<DATABASE_CONNECTION> live = currentConnection(); live && live->IsConnected() )
        return CONNECT_STATUS();

    auto now = std::chrono::steady_clock::now;

    const auto                start = now();
    const auto                deadline = start + FIRST_CONNECT_BUDGET;
    std::minstd_rand          jitter( std::random_device{}() );
    bool                      firstOpen = false;
    std::chrono::milliseconds lastAttempt( 0 );
    CONNECT_STATUS            status;

    for( int attempt = 0;; ++attempt )
    {
        {
            // The refresh thread reconnects too, and two connections must not be built at once
            std::lock_guard connectGuard( m_connectMutex );

            std::shared_ptr<DATABASE_CONNECTION> existing = currentConnection();

            if( existing && existing->IsConnected() )
                return CONNECT_STATUS();

            if( attempt == 0 )
                firstOpen = !existing;

            // The wait or the mutex may have run past the budget the last check allowed for
            if( attempt > 0 && ( connectionStopped() || now() + lastAttempt > deadline ) )
                return status;

            const auto attemptStart = now();

            status = connectOnce();
            lastAttempt = std::chrono::duration_cast<std::chrono::milliseconds>( now() - attemptStart );
        }

        // A reconnect has the refresh backoff, so only a library that never opened waits here
        if( !firstOpen || status.failure != DB_FAILURE::FILE_IN_USE || connectionStopped() )
            return status;

        // Another attempt must also end inside the budget, as the file was in use for as long as the last one took
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>( deadline - now() ) - lastAttempt;

        if( remaining <= std::chrono::milliseconds( 0 ) )
            return status;

        const std::chrono::milliseconds wait = std::min( FileInUseBackoff( attempt, jitter ), remaining );

        {
            std::unique_lock lock( m_refreshMutex );

            m_refreshCV.wait_for( lock, wait,
                                  [this]()
                                  {
                                      return connectionStopped();
                                  } );
        }

        if( connectionStopped() )
            return status;
    }
}


SCH_IO_DATABASE::CONNECT_STATUS SCH_IO_DATABASE::connectOnce()
{
    // The old connection stays in place until the new one is ready, as other threads may still be reading from it
    CONNECTION_RESULT created = connectionFor(
            DATABASE_BACKEND::Create( m_settings->m_Source, cwdDirFor( *m_settings, wxEmptyString ) ), true );

    std::shared_ptr<DATABASE_CONNECTION> newConn = std::move( created.connection );

    if( !newConn )
        return { created.failure, wxString::FromUTF8( created.error ) };

    if( !newConn->IsConnected() )
        return { newConn->GetLastFailure(), newConn->GetLastError() };

    for( const DATABASE_LIB_TABLE& tableIter : m_settings->m_Tables )
    {
        // Trusted even if unreported by the driver, to support generated columns (#16952)
        std::set<std::string> requiredColumns{ tableIter.key_col, tableIter.footprints_col, tableIter.symbols_col };

        for( const DATABASE_FIELD_MAPPING& field : tableIter.fields )
            requiredColumns.insert( field.column );

        if( !tableIter.pins_col.empty() )
            requiredColumns.insert( tableIter.pins_col );

        // Only used if confirmed present, so a misconfigured mapping can't break the table (#23532)
        std::set<std::string> optionalColumns{
            tableIter.properties.description,      tableIter.properties.footprint_filters,
            tableIter.properties.keywords,         tableIter.properties.exclude_from_sim,
            tableIter.properties.exclude_from_bom, tableIter.properties.exclude_from_board
        };

        if( newConn->CacheTableInfo( tableIter.table, requiredColumns, optionalColumns ) )
            continue;

        std::optional<DATABASE_CONNECTION::TABLE_FAILURE> failed = newConn->GetTableFailure( tableIter.table );

        // A connection without a table that was only locked or out of reach would keep it missing until a reload
        if( failed && ( failed->failure == DB_FAILURE::FILE_IN_USE || failed->failure == DB_FAILURE::UNREACHABLE ) )
        {
            return { failed->failure,
                     wxString::Format( _( "Error reading database table %s: %s" ),
                                       wxString::FromUTF8( tableIter.table ), wxString::FromUTF8( failed->error ) ) };
        }
    }

    newConn->SetCacheParams( m_settings->m_Cache.max_size, m_settings->m_Cache.max_age );

    std::unique_lock connLock( m_cacheMutex );
    m_conn = std::move( newConn );

    return CONNECT_STATUS();
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
