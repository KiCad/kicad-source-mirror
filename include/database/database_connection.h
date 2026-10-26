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

#ifndef KICAD_DATABASE_CONNECTION_H
#define KICAD_DATABASE_CONNECTION_H

#include <any>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <database/database_backend.h>
#include <database/database_cache.h>


/**
 * A read-only view of a database library source.  Owns the backend that talks to the database and adds
 * what every backend shares: serialised access, the row cache, column-name matching and the conversion of
 * values to the text KiCad stores.
 */
class DATABASE_CONNECTION
{
public:
    typedef std::map<std::string, std::any> ROW;

    /**
     * @param aBackend the backend to read through; must not be null
     * @param aConnectNow false defers connecting to the caller's Connect(), e.g. on a worker thread
     */
    DATABASE_CONNECTION( std::unique_ptr<DATABASE_BACKEND> aBackend, bool aConnectNow = true );

    /// Brackets the reads of one library load or refresh, so the backend can keep what it opens for them
    class LOAD_CYCLE
    {
    public:
        LOAD_CYCLE( DATABASE_CONNECTION& aConnection, DB_STOP_CHECK aStopped );

        ~LOAD_CYCLE();

        LOAD_CYCLE( const LOAD_CYCLE& ) = delete;
        LOAD_CYCLE& operator=( const LOAD_CYCLE& ) = delete;

    private:
        DATABASE_CONNECTION& m_connection;
    };

    void SetCacheParams( int aMaxSize, int aMaxAge );

    bool Connect();

    bool Disconnect();

    bool IsConnected() const;

    /**
     * Caches schema information for a table so that subsequent queries know which columns exist.
     *
     * Required columns are trusted even if the backend doesn't report them (e.g. SQLite generated
     * columns).  Optional columns are added only if confirmed present; otherwise they are dropped
     * with a warning rather than breaking every query against the table.  Matching is case-insensitive.
     * Columns the backend reports as unreadable are dropped even when required.
     *
     * @param aTable the name of a table in the database
     * @param aRequiredColumns columns that must be present in queries even if the backend does not
     *                         report them
     * @param aOptionalColumns columns that are added only when the backend confirms they exist
     * @return true on success
     */
    bool CacheTableInfo( const std::string& aTable, const std::set<std::string>& aRequiredColumns,
                         const std::set<std::string>& aOptionalColumns = {} );

    /**
     * Retrieves a single row from a database table.  Table and column names are cached when the
     * connection is created, so schema changes to the database won't be recognized unless the
     * connection is recreated.
     * @param aTable the name of a table in the database
     * @param aWhere column to search, and the value to search for
     * @param aResult will be filled with a row in the database if one was found
     * @return true if aResult was filled; false otherwise
     */
    bool SelectOne( const std::string& aTable, const std::pair<std::string, std::string>& aWhere,
                    ROW& aResult );

    /// What a failed query left behind, copied before another thread's query can replace it
    struct QUERY_STATUS
    {
        bool        connected = false;
        DB_FAILURE  failure = DB_FAILURE::UNKNOWN;
        std::string error;
    };

    /// Why CacheTableInfo could not describe a table
    struct TABLE_FAILURE
    {
        DB_FAILURE  failure = DB_FAILURE::UNKNOWN;
        std::string error;

        /// Lowercase columns the failed lookups asked for; only a lookup asking for all of them clears the failure
        std::set<std::string> columns;
    };

    /**
     * @return why CacheTableInfo could not describe aTable, or nothing if it was described
     */
    std::optional<TABLE_FAILURE> GetTableFailure( const std::string& aTable ) const;

    /**
     * Retrieves all rows from a database table.
     * @param aTable the name of a table in the database
     * @param aKey holds the column name of the primary key used for caching results
     * @param aResults will be filled with all rows in the table
     * @param aStatus if not null, receives the state of the failure when the query fails
     * @return true if the query succeeded and at least one ROW was found, false otherwise
     */
    bool SelectAll( const std::string& aTable, const std::string& aKey, std::vector<ROW>& aResults,
                    QUERY_STATUS* aStatus = nullptr );

    /**
     * Reads the first rows of a table, bypassing the row cache.
     * @param aTable the name of a table in the database
     * @param aMaxRows stop fetching after this many rows; zero fetches only the column names
     * @param aColumns will be filled with the column names in result order
     * @param aRows will be filled with up to aMaxRows rows
     * @return true if the query succeeded
     */
    bool SelectSample( const std::string& aTable, size_t aMaxRows, std::vector<std::string>& aColumns,
                       std::vector<ROW>& aRows );

    /**
     * Lists the tables and views visible through this connection, sorted by name.
     */
    bool GetTables( std::vector<std::string>& aTables );

    /**
     * Lists the columns of a table in catalog order.
     */
    bool GetColumns( const std::string& aTable, std::vector<std::string>& aColumns );

    void ClearCache( const std::string& aTable )
    {
        if( m_cache )
            m_cache->Clear( aTable );
    }

    std::string GetLastError() const { return m_backend->GetLastError(); }

    /// @return why the last connection attempt failed
    DB_FAILURE GetLastFailure() const { return m_backend->GetLastFailure(); }

private:
    typedef DATABASE_CACHE<std::map<std::string, ROW>> DB_CACHE_TYPE;

    /// A table the backend described, with the columns queries against it read
    struct TABLE_STATE
    {
        DB_TABLE_INFO info;

        /// The column spellings queries read.  Spellings differing only in case are distinct columns.
        std::set<std::string> columns;
    };

    /// @return the names of the columns to read from a table
    static std::vector<std::string> columnNames( const TABLE_STATE& aTable );

    /// @return the position in aLabels of the key column, matched exactly first and then ignoring case
    static std::optional<size_t> findLabel( const std::vector<std::string>& aLabels, const std::string& aKey );

    /// Converts one row delivered by the backend to text keyed by result-column label
    static ROW makeRow( const std::vector<std::string>& aLabels, std::vector<DB_VALUE>&& aValues );

    /**
     * Reads a whole table into aEntry and offers it to the row cache.  The caller must already hold
     * m_queryMutex.  aEntry is untouched and the cache unchanged unless the read succeeded.
     */
    bool readAndCache( const std::string& aTable, const std::string& aKey, DB_CACHE_TYPE::CACHE_VALUE& aEntry );

    std::unique_ptr<DATABASE_BACKEND> m_backend;

    std::map<std::string, TABLE_STATE> m_tables;

    /// Why CacheTableInfo could not describe each table, so a later query on it reports that table's own failure
    std::map<std::string, TABLE_FAILURE> m_tableFailures;

    mutable std::mutex m_queryMutex;

    std::unique_ptr<DB_CACHE_TYPE> m_cache;
};

#endif //KICAD_DATABASE_CONNECTION_H
