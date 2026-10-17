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
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include <database/database_cache.h>

extern const char* const traceDatabase;


namespace nanodbc
{
    class connection;
}


class DATABASE_CONNECTION
{
public:
    static const long DEFAULT_TIMEOUT = 10;

    typedef std::map<std::string, std::any> ROW;

    /// Why a connection attempt failed, as far as the driver's report tells
    enum class FAILURE
    {
        UNKNOWN,
        LOGIN,       ///< The server rejected the user name or password
        UNREACHABLE, ///< Nothing answered at the server address, or it refused the connection
        TIMEOUT,     ///< The server did not answer within the timeout
        NO_DSN,      ///< The data source name is not configured
        NO_DRIVER    ///< The driver named by the data source could not be loaded
    };

    DATABASE_CONNECTION( const std::string& aDataSourceName, const std::string& aUsername,
                         const std::string& aPassword, int aTimeoutSeconds = DEFAULT_TIMEOUT,
                         bool aConnectNow = true );

    DATABASE_CONNECTION( const std::string& aConnectionString,
                         int aTimeoutSeconds = DEFAULT_TIMEOUT,
                         bool aConnectNow = true );

    ~DATABASE_CONNECTION();

    void SetCacheParams( int aMaxSize, int aMaxAge );

    bool Connect();

    bool Disconnect();

    bool IsConnected() const;

    /**
     * Caches schema information for a table so that subsequent queries know which columns exist.
     *
     * Required columns are trusted even if the driver doesn't report them (e.g. SQLite generated
     * columns).  Optional columns are added only if confirmed present; otherwise they are dropped
     * with a warning rather than breaking every query against the table.  Matching is case-insensitive.
     *
     * @param aTable the name of a table in the database
     * @param aRequiredColumns columns that must be present in queries even if the driver does not
     *                         report them
     * @param aOptionalColumns columns that are added only when the driver confirms they exist
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

    /**
     * Retrieves all rows from a database table.
     * @param aTable the name of a table in the database
     * @param aKey holds the column name of the primary key used for caching results
     * @param aResults will be filled with all rows in the table
     * @return true if the query succeeded and at least one ROW was found, false otherwise
     */
    bool SelectAll( const std::string& aTable, const std::string& aKey,
                    std::vector<ROW>& aResults );

    /**
     * Reads the first rows of a table with a forward-only cursor, bypassing the row cache.
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

    /// A data source configured in the ODBC driver manager
    struct DATA_SOURCE
    {
        std::string name;
        std::string driver;
    };

    /// @return the data sources configured in the ODBC driver manager, with the driver each one uses
    static std::vector<DATA_SOURCE> ListDataSourceDetails();

    /// @return the driver names installed in the ODBC driver manager
    static std::vector<std::string> ListDrivers();

    void ClearCache( const std::string& aTable )
    {
        if( m_cache )
            m_cache->Clear( aTable );
    }

    std::string GetLastError() const { return m_lastError; }

    /// @return the SQLSTATE of the last driver error, or an empty string
    std::string GetLastState() const { return m_lastState; }

    /// Classifies a driver error from its SQLSTATE, falling back to its text since drivers disagree on states
    static FAILURE ClassifyFailure( const std::string& aState, const std::string& aMessage );

private:
    void init();

    bool getQuoteChar();

    /// Wraps an identifier in the driver's quote character, doubling any it contains
    std::string quoted( const std::string& aIdentifier ) const;

    /// Escapes catalog search pattern characters so a name matches only itself
    std::string escapePattern( const std::string& aName ) const;

    std::string columnsFor( const std::string& aTable );

    /// The caller must already hold m_queryMutex.
    bool selectAllAndCache( const std::string& aTable, const std::string& aKey );

    std::unique_ptr<nanodbc::connection> m_conn;

    std::string m_dsn;
    std::string m_user;
    std::string m_pass;
    std::string m_connectionString;

    std::string m_lastError;
    std::string m_lastState;

    std::map<std::string, std::string> m_tables;

    /// Map of table -> map of column name -> data type
    std::map<std::string, std::map<std::string, int>> m_columnCache;

    long m_timeout;

    char m_quoteChar;

    std::string m_patternEscape;

    mutable std::mutex m_queryMutex;

    typedef DATABASE_CACHE<std::map<std::string, ROW>> DB_CACHE_TYPE;

    std::unique_ptr<DB_CACHE_TYPE> m_cache;
};

#endif //KICAD_DATABASE_CONNECTION_H
