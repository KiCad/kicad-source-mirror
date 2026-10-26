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

#include <boost/algorithm/string.hpp>
#include <wx/log.h>

#include <database/database_connection.h>
#include <database/database_value.h>
#include <core/profile.h>


DATABASE_CONNECTION::DATABASE_CONNECTION( std::unique_ptr<DATABASE_BACKEND> aBackend, bool aConnectNow ) :
        m_backend( std::move( aBackend ) ),
        m_cache( std::make_unique<DB_CACHE_TYPE>( 10, 1 ) )
{
    wxASSERT_MSG( m_backend, "DATABASE_CONNECTION needs a backend" );

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::LOAD_CYCLE::LOAD_CYCLE( DATABASE_CONNECTION& aConnection, DB_STOP_CHECK aStopped ) :
        m_connection( aConnection )
{
    std::lock_guard lock( m_connection.m_queryMutex );

    m_connection.m_backend->BeginLoadCycle( aStopped );
}


DATABASE_CONNECTION::LOAD_CYCLE::~LOAD_CYCLE()
{
    std::lock_guard lock( m_connection.m_queryMutex );

    m_connection.m_backend->EndLoadCycle();
}


void DATABASE_CONNECTION::SetCacheParams( int aMaxSize, int aMaxAge )
{
    if( !m_cache )
        return;

    if( aMaxSize < 0 )
        aMaxSize = 0;

    if( aMaxAge < 0 )
        aMaxAge = 0;

    m_cache->SetMaxSize( static_cast<size_t>( aMaxSize ) );
    m_cache->SetMaxAge( static_cast<time_t>( aMaxAge ) );
}


bool DATABASE_CONNECTION::Connect()
{
    std::lock_guard lock( m_queryMutex );

    if( !m_backend->Connect() )
        return false;

    m_tables.clear();
    m_tableFailures.clear();

    return true;
}


bool DATABASE_CONNECTION::Disconnect()
{
    std::lock_guard lock( m_queryMutex );

    m_backend->Disconnect();

    return !m_backend->IsConnected();
}


bool DATABASE_CONNECTION::IsConnected() const
{
    return m_backend->IsConnected();
}


bool DATABASE_CONNECTION::CacheTableInfo( const std::string& aTable, const std::set<std::string>& aRequiredColumns,
                                          const std::set<std::string>& aOptionalColumns )
{
    std::lock_guard lock( m_queryMutex );

    if( !m_backend->IsConnected() )
        return false;

    std::optional<DB_TABLE_INFO> info = m_backend->DescribeTable( aTable );

    // Catalog names are lowercase; normalize here so differently-cased columns still match
    std::set<std::string> requiredLower;
    std::set<std::string> optionalLower;

    for( const std::string& col : aRequiredColumns )
        requiredLower.insert( boost::to_lower_copy( col ) );

    for( const std::string& col : aOptionalColumns )
        optionalLower.insert( boost::to_lower_copy( col ) );

    if( !info )
    {
        wxLogTrace( traceDatabase, wxT( "CacheTableInfo: table '%s' not found in catalog" ), aTable );

        // The next table's catalog read can clear the backend's error, so keep this table's own
        TABLE_FAILURE& failed = m_tableFailures[aTable];
        failed.failure = m_backend->GetLastFailure();
        failed.error = m_backend->GetLastError();
        failed.columns.insert( requiredLower.begin(), requiredLower.end() );
        failed.columns.insert( optionalLower.begin(), optionalLower.end() );
        return false;
    }

    // Another library entry on this table may still lack the columns its failed lookup asked for, and a describe
    // that lists no columns does not show they are readable
    if( auto failed = m_tableFailures.find( aTable ); failed != m_tableFailures.end() )
    {
        const bool covered = std::all_of( failed->second.columns.begin(), failed->second.columns.end(),
                                          [&]( const std::string& aColumn )
                                          {
                                              return requiredLower.count( aColumn ) || optionalLower.count( aColumn );
                                          } );

        if( covered && !info->columnsUnavailable )
            m_tableFailures.erase( failed );
    }

    TABLE_STATE& table = m_tables[info->name];
    table.info = *info;

    // The table is still usable with SELECT * when the catalog cannot list its columns
    if( info->columnsUnavailable )
    {
        wxLogTrace( traceDatabase, wxT( "CacheTableInfo: columns of table '%s' unavailable" ), info->name );
        return false;
    }

    std::set<std::string> columnsReported;

    for( const DB_COLUMN& column : info->columns )
    {
        const std::string lower = boost::to_lower_copy( column.name );

        if( !requiredLower.count( lower ) && !optionalLower.count( lower ) )
            continue;

        columnsReported.insert( lower );

        if( !column.supported )
        {
            wxLogTrace( traceDatabase, wxT( "CacheTableInfo: column '%s' of table '%s' cannot be read; dropped" ),
                        column.name, info->name );
            continue;
        }

        table.columns.insert( column.name );
    }

    // SQLite's ODBC driver doesn't report generated columns via SQLColumns, so required
    // columns are trusted and added even when absent from the catalog
    for( const std::string& requestedCol : aRequiredColumns )
    {
        const std::string requestedLower = boost::to_lower_copy( requestedCol );

        if( !columnsReported.count( requestedLower ) && !requestedCol.empty() )
        {
            wxLogTrace( traceDatabase,
                        wxT( "CacheTableInfo: column '%s' not found in catalog for table '%s', adding anyway" ),
                        requestedCol, info->name );

            table.columns.insert( requestedCol );
        }
    }

    // Unlike required columns, a missing optional column is dropped, not added, but is
    // still worth a warning since it usually means a misconfigured .kicad_dbl mapping
    for( const std::string& requestedCol : aOptionalColumns )
    {
        if( !columnsReported.count( boost::to_lower_copy( requestedCol ) ) && !requestedCol.empty() )
        {
            wxLogWarning( wxT( "Database table '%s' has no column '%s'; ignoring the "
                               "misconfigured properties mapping." ),
                          info->name, requestedCol );
        }
    }

    return true;
}


std::vector<std::string> DATABASE_CONNECTION::columnNames( const TABLE_STATE& aTable )
{
    std::vector<std::string> names;

    for( const std::string& spelling : aTable.columns )
        names.push_back( spelling );

    if( names.empty() )
        wxLogTrace( traceDatabase, wxT( "columnNames: table %s has no columns mapped!" ), aTable.info.name );

    return names;
}


std::optional<size_t> DATABASE_CONNECTION::findLabel( const std::vector<std::string>& aLabels,
                                                      const std::string& aKey )
{
    for( size_t i = 0; i < aLabels.size(); ++i )
    {
        if( aLabels[i] == aKey )
            return i;
    }

    for( size_t i = 0; i < aLabels.size(); ++i )
    {
        if( boost::iequals( aLabels[i], aKey ) )
            return i;
    }

    return std::nullopt;
}


DATABASE_CONNECTION::ROW DATABASE_CONNECTION::makeRow( const std::vector<std::string>& aLabels,
                                                       std::vector<DB_VALUE>&&         aValues )
{
    ROW row;

    for( size_t i = 0; i < aLabels.size() && i < aValues.size(); ++i )
    {
        if( std::string* text = std::get_if<std::string>( &aValues[i] ) )
            row[aLabels[i]] = std::move( *text );
        else
            row[aLabels[i]] = DatabaseValueToString( aValues[i] );
    }

    return row;
}


bool DATABASE_CONNECTION::readAndCache( const std::string& aTable, const std::string& aKey,
                                        DB_CACHE_TYPE::CACHE_VALUE& aEntry )
{
    const TABLE_STATE& table = m_tables.at( aTable );

    PROF_TIMER                 timer;
    DB_CACHE_TYPE::CACHE_VALUE built;
    std::vector<std::string>   labels;
    std::optional<size_t>      keyIndex;
    bool                       keyResolved = false;

    // Labels are known before the first row arrives
    auto sink =
            [&]( std::vector<DB_VALUE>&& aValues )
            {
                if( !keyResolved )
                {
                    keyResolved = true;
                    keyIndex = findLabel( labels, aKey );

                    if( !keyIndex )
                    {
                        wxLogTrace( traceDatabase, wxT( "readAndCache: warning: key %s not found in result set" ),
                                    aKey );
                    }
                }

                if( !keyIndex || *keyIndex >= aValues.size() )
                    return;

                std::string key = DatabaseValueToString( aValues[*keyIndex] );
                built[std::move( key )] = makeRow( labels, std::move( aValues ) );
            };

    // A pass the backend could not verify must leave nothing behind, not even rows whose keys a later pass lacks
    auto restart =
            [&]()
            {
                built.clear();
                keyResolved = false;
                keyIndex.reset();
            };

    if( !m_backend->ReadAllRows( table.info, columnNames( table ), labels, sink, restart ) )
        return false;

    timer.Stop();

    wxLogTrace( traceDatabase, wxT( "readAndCache from %s completed in %0.1f ms" ), aTable, timer.msecs() );

    // The cache is best effort; with no room for the table the rows are still returned
    m_cache->Put( aTable, built );
    aEntry = std::move( built );

    return true;
}


bool DATABASE_CONNECTION::SelectOne( const std::string& aTable, const std::pair<std::string, std::string>& aWhere,
                                     DATABASE_CONNECTION::ROW& aResult )
{
    std::lock_guard lock( m_queryMutex );

    if( !m_backend->IsConnected() )
    {
        wxLogTrace( traceDatabase, wxT( "Called SelectOne without valid connection!" ) );
        return false;
    }

    auto tableIter = m_tables.find( aTable );

    if( tableIter == m_tables.end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: requested table %s not found in cache" ), aTable );
        return false;
    }

    const std::string& tableName = tableIter->first;
    const TABLE_STATE& table = tableIter->second;
    DB_CACHE_TYPE::CACHE_VALUE cacheEntry;

    bool cached = m_cache->Get( tableName, cacheEntry );

    if( !cached )
        wxLogTrace( traceDatabase, wxT( "SelectOne: table `%s` not in row cache; will SelectAll" ), tableName );

    if( cached || readAndCache( tableName, aWhere.first, cacheEntry ) )
    {
        if( auto hit = cacheEntry.find( aWhere.second ); hit != cacheEntry.end() )
        {
            wxLogTrace( traceDatabase, wxT( "SelectOne: `%s` with parameter `%s` - cache hit" ), tableName,
                        aWhere.second );
            aResult = hit->second;
            return true;
        }
    }

    auto keyColumn = table.columns.find( aWhere.first );

    if( keyColumn == table.columns.end() )
    {
        keyColumn = std::find_if( table.columns.begin(), table.columns.end(),
                                  [&]( const std::string& aColumn )
                                  {
                                      return boost::iequals( aColumn, aWhere.first );
                                  } );
    }

    if( keyColumn == table.columns.end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: requested column %s not found in cache for %s" ), aWhere.first,
                    tableName );
        return false;
    }

    std::vector<std::string> labels;
    std::vector<DB_VALUE>    values;

    std::optional<bool> found = m_backend->ReadRow( table.info, columnNames( table ), *keyColumn,
                                                    aWhere.second, labels, values );

    if( found )
    {
        if( !*found )
            return false;

        aResult = makeRow( labels, std::move( values ) );
        return true;
    }

    // No indexed lookup, and the cache or a fresh read has no such key; the refresh bounds how stale the cache gets
    return false;
}


std::optional<DATABASE_CONNECTION::TABLE_FAILURE>
DATABASE_CONNECTION::GetTableFailure( const std::string& aTable ) const
{
    std::lock_guard lock( m_queryMutex );

    if( auto failed = m_tableFailures.find( aTable ); failed != m_tableFailures.end() )
        return failed->second;

    return std::nullopt;
}


bool DATABASE_CONNECTION::SelectAll( const std::string& aTable, const std::string& aKey, std::vector<ROW>& aResults,
                                     QUERY_STATUS* aStatus )
{
    std::lock_guard lock( m_queryMutex );

    // Copied under the query lock, so another thread's query cannot replace the failure first
    auto fail =
            [&]()
            {
                if( aStatus )
                {
                    *aStatus = QUERY_STATUS{ m_backend->IsConnected(), m_backend->GetLastFailure(),
                                             m_backend->GetLastError() };
                }

                return false;
            };

    if( !m_backend->IsConnected() )
    {
        wxLogTrace( traceDatabase, wxT( "Called SelectAll without valid connection!" ) );
        return fail();
    }

    // A table another library entry described is in m_tables too, so its failure must win
    if( auto failed = m_tableFailures.find( aTable ); failed != m_tableFailures.end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectAll: table %s could not be described" ), aTable );

        if( aStatus )
            *aStatus = QUERY_STATUS{ m_backend->IsConnected(), failed->second.failure, failed->second.error };

        return false;
    }

    if( !m_tables.count( aTable ) )
    {
        wxLogTrace( traceDatabase, wxT( "SelectAll: requested table %s not found in cache" ), aTable );

        if( aStatus )
            *aStatus = QUERY_STATUS{ m_backend->IsConnected(), DB_FAILURE::UNKNOWN, std::string() };

        return false;
    }

    DB_CACHE_TYPE::CACHE_VALUE cacheEntry;

    if( !m_cache->Get( aTable, cacheEntry ) )
    {
        if( !readAndCache( aTable, aKey, cacheEntry ) )
        {
            wxLogTrace( traceDatabase, wxT( "SelectAll: `%s` cache fill failed" ), aTable );
            return fail();
        }
    }
    else
    {
        wxLogTrace( traceDatabase, wxT( "SelectAll: `%s` - returning cached results" ), aTable );
    }

    aResults.reserve( cacheEntry.size() );

    for( auto& [key, row] : cacheEntry )
        aResults.emplace_back( std::move( row ) );

    return true;
}


bool DATABASE_CONNECTION::SelectSample( const std::string& aTable, size_t aMaxRows,
                                        std::vector<std::string>& aColumns, std::vector<ROW>& aRows )
{
    std::lock_guard lock( m_queryMutex );

    aColumns.clear();
    aRows.clear();

    if( !m_backend->IsConnected() )
        return false;

    // A table nobody described yet is read by the name it was given
    auto          tableIter = m_tables.find( aTable );
    DB_TABLE_INFO info = tableIter != m_tables.end() ? tableIter->second.info : DB_TABLE_INFO{ aTable, {} };

    auto sink =
            [&]( std::vector<DB_VALUE>&& aValues )
            {
                aRows.emplace_back( makeRow( aColumns, std::move( aValues ) ) );
            };

    if( !m_backend->ReadTable( info, {}, aMaxRows, aColumns, sink ) )
    {
        aColumns.clear();
        aRows.clear();
        return false;
    }

    return true;
}


bool DATABASE_CONNECTION::GetTables( std::vector<std::string>& aTables )
{
    std::lock_guard lock( m_queryMutex );

    aTables.clear();

    if( !m_backend->IsConnected() )
        return false;

    return m_backend->ListTables( aTables );
}


bool DATABASE_CONNECTION::GetColumns( const std::string& aTable, std::vector<std::string>& aColumns )
{
    std::lock_guard lock( m_queryMutex );

    aColumns.clear();

    if( !m_backend->IsConnected() )
        return false;

    return m_backend->ListColumns( aTable, aColumns );
}
