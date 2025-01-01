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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_DATABASE_CONNECTION_H
#define KICAD_DATABASE_CONNECTION_H

#include <any>
#include <map>
#include <memory>
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

    bool CacheTableInfo( const std::string& aTable, const std::set<std::string>& aColumns );

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

    std::string GetLastError() const { return m_lastError; }

private:
    void init();

    bool getQuoteChar();

    std::string columnsFor( const std::string& aTable );

    bool selectAllAndCache( const std::string& aTable, const std::string& aKey );

    std::unique_ptr<nanodbc::connection> m_conn;

    std::string m_dsn;
    std::string m_user;
    std::string m_pass;
    std::string m_connectionString;

    std::string m_lastError;

    std::map<std::string, std::string> m_tables;

    /// Map of table -> map of column name -> data type
    std::map<std::string, std::map<std::string, int>> m_columnCache;

    long m_timeout;

    char m_quoteChar;

    typedef DATABASE_CACHE<std::map<std::string, ROW>> DB_CACHE_TYPE;

    std::unique_ptr<DB_CACHE_TYPE> m_cache;
};

#endif //KICAD_DATABASE_CONNECTION_H
