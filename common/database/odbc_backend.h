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

#ifndef KICAD_ODBC_BACKEND_H
#define KICAD_ODBC_BACKEND_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <database/database_backend.h>

namespace nanodbc
{
class connection;
}


/// Reads a database through an ODBC data source or connection string
class ODBC_BACKEND : public DATABASE_BACKEND
{
public:
    /**
     * @param aDsn the data source name, ignored when aConnectionString is set
     * @param aConnectionString a full ODBC connection string, or empty to use the data source name
     * @param aTimeout the login timeout and query timeout in seconds
     */
    ODBC_BACKEND( std::string aDsn, std::string aUser, std::string aPass, std::string aConnectionString,
                  long aTimeout );

    ~ODBC_BACKEND() override;

    bool Connect() override;

    void Disconnect() override;

    bool IsConnected() const override;

    bool ListTables( std::vector<std::string>& aTables ) override;

    std::optional<DB_TABLE_INFO> DescribeTable( const std::string& aTable ) override;

    bool ListColumns( const std::string& aTable, std::vector<std::string>& aColumns ) override;

    bool ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns, size_t aMaxRows,
                    std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink ) override;

    /// Runs a prepared query whose key parameter is described with the key column's catalog type
    std::optional<bool> ReadRow( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                                 const std::string& aKeyColumn, const std::string& aKey,
                                 std::vector<std::string>& aLabels, std::vector<DB_VALUE>& aRow ) override;

    std::string GetLastError() const override { return m_lastError; }

    DB_FAILURE GetLastFailure() const override { return m_lastFailure; }

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

    /// The keyword ODBC uses to choose where a connection string connects
    struct SELECTOR
    {
        enum class KIND
        {
            NONE,
            DSN,
            FILE_DSN,
            DRIVER
        };

        KIND        kind = KIND::NONE;
        std::string value; ///< The keyword's value without braces
    };

    /// @return the first DSN, FILEDSN or DRIVER keyword of a connection string, which ODBC honours over later ones
    static SELECTOR FirstSelector( const std::string& aConnectionString );

    /// Resolves a data source name to its driver, or nullopt if it is unknown
    using DSN_LOOKUP = std::function<std::optional<std::string>( const std::string& )>;

    /**
     * Decides whether connecting must be serialized because the effective driver is, or may be, the Microsoft Access
     * driver.  Anything that cannot be resolved without loading a driver is serialized.
     *
     * @param aDsn is the data source name, used when the connection string is empty
     * @param aLookup resolves a data source name to its driver
     */
    static bool NeedsDriverLock( const std::string& aDsn, const std::string& aConnectionString,
                                 const DSN_LOOKUP& aLookup );

    /// @return true for the Microsoft Access ODBC drivers, which must not initialize on two threads at once
    static bool IsAccessDriver( const std::string& aDriverName );

    /// @return true when a lowercase driver message is one of the Access driver's own locked-file messages
    static bool IsAccessFileInUse( const std::string& aLowerMessage );

    /// Classifies a driver error from its SQLSTATE, falling back to its text since drivers disagree on states
    static DB_FAILURE ClassifyFailure( const std::string& aState, const std::string& aMessage );

    /// How ReadRow describes its key parameter to the driver
    struct KEY_PARAMETER
    {
        short         type = 0;          ///< ODBC SQL data type
        unsigned long size = 0;          ///< Column size or precision
        short         scale = 0;         ///< Digits right of the decimal point
        bool          asInteger = false; ///< Bind integerValue as a 64-bit integer instead of the key text
        int64_t       integerValue = 0;  ///< The key as a number, set when asInteger is true
    };

    /**
     * Chooses the parameter description for a lookup key from its column's cached ODBC description.
     * Strict drivers (Microsoft Access) reject a key whose type does not match the column.
     *
     * @param aColumnType is the ODBC SQL data type of the key column
     * @param aColumnSize is its character length or numeric precision, zero or negative when unknown
     * @param aColumnScale is its number of digits right of the decimal point
     * @param aKey is the key text
     */
    static KEY_PARAMETER DescribeKeyParameter( int aColumnType, long aColumnSize, short aColumnScale,
                                               const std::string& aKey );

private:
    bool getQuoteChar();

    /**
     * Takes the lock that keeps the Access driver from initializing or unloading on two threads at once.
     * The lock is not taken for any other driver, and never off Windows.  Connect, Disconnect and destruction
     * share one deadline while the lock stays out of reach, so a stalled Access worker costs one wait.
     *
     * @return false if the lock was needed and could not be taken before the deadline
     */
    bool takeAccessDriverLock( std::unique_lock<std::timed_mutex>& aLock );

    /// Closes the connection with the driver lock held by the caller
    void disconnectLocked();

    /// Wraps an identifier in the driver's quote character, doubling any it contains
    std::string quoted( const std::string& aIdentifier ) const;

    /// Escapes catalog search pattern characters so a name matches only itself
    std::string escapePattern( const std::string& aName ) const;

    /// @return the quoted column list for a SELECT, or * when there are no columns
    std::string columnList( const std::vector<std::string>& aColumns ) const;

    std::unique_ptr<nanodbc::connection> m_conn;

    std::string m_dsn;
    std::string m_user;
    std::string m_pass;
    std::string m_connectionString;

    bool m_accessDriver = false;

    /// When the wait for the Access driver lock ends, fixed by the first step that finds the lock busy and never
    /// renewed, so it stays spent until the next Connect
    std::optional<std::chrono::steady_clock::time_point> m_engineDeadline;

    std::string m_lastError;
    DB_FAILURE  m_lastFailure = DB_FAILURE::UNKNOWN;

    /// ODBC description of a column, so a lookup key can be bound with the type the driver expects
    struct COLUMN_INFO
    {
        int   type = 0;  ///< SQL data type
        long  size = 0;  ///< Character length or numeric precision, zero or negative when unknown
        short scale = 0; ///< Digits right of the decimal point
    };

    /// Map of table -> map of column name -> column description
    std::map<std::string, std::map<std::string, COLUMN_INFO>> m_columnInfo;

    long m_timeout;

    char m_quoteChar = '"';

    std::string m_patternEscape;
};

#endif // KICAD_ODBC_BACKEND_H
