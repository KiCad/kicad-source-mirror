/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_DATABASE_BACKEND_H
#define KICAD_DATABASE_BACKEND_H

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <wx/string.h>

#include <database/database_value.h>
#include <trace_helpers.h>

struct DATABASE_SOURCE;


/// Why a connection attempt failed, as far as the backend's report tells
enum class DB_FAILURE
{
    UNKNOWN,
    LOGIN,                     ///< The server rejected the user name or password
    UNREACHABLE,               ///< Nothing answered at the server address, or it refused the connection
    TIMEOUT,                   ///< The source did not answer within the timeout
    NO_DSN,                    ///< The data source name is not configured
    NO_DRIVER,                 ///< The driver named by the data source could not be loaded
    FILE_NOT_FOUND,            ///< The database file does not exist
    FILE_IN_USE,               ///< The database file is locked by another process
    PROVIDER_MISSING,          ///< The database engine needed to open the file is not installed
    NOT_SUPPORTED_ON_PLATFORM, ///< No backend for the source type exists on this platform
    INVALID_TYPE,              ///< The source type is not one KiCad knows
    NETWORK_WAL                ///< A write-ahead-log database on a network path cannot be read safely
};


/// A column as the backend reports it
struct DB_COLUMN
{
    std::string name;
    int         type;
    bool        supported = true; ///< False for columns the backend can never read, such as attachments
};


struct DB_TABLE_INFO
{
    std::string             name; ///< The table's name as the backend spells it
    std::vector<DB_COLUMN>  columns;

    /// True when the table exists but the backend could not read its column list, so only SELECT * works
    bool columnsUnavailable = false;
};


/// Receives one row at a time, in the order of the labels the read produced
using DB_ROW_SINK = std::function<void( std::vector<DB_VALUE>&& )>;

/// Told that the rows delivered so far are void, because the read starts the table again from its first row
using DB_RESTART_SINK = std::function<void()>;

/// True once the caller no longer wants what a read would deliver
using DB_STOP_CHECK = std::function<bool()>;


class DATABASE_BACKEND;


/// The outcome of building a backend; backend is null when failure and error say why
struct DB_BACKEND_RESULT
{
    std::unique_ptr<DATABASE_BACKEND> backend;
    DB_FAILURE                        failure = DB_FAILURE::UNKNOWN;
    std::string                       error;
};


/**
 * A read-only source of tables.  DATABASE_CONNECTION owns one and supplies the locking, row cache,
 * column matching and conversion to text, so a backend only talks to its database.  A backend is
 * never called from two threads at once.
 */
class DATABASE_BACKEND
{
public:
    /// Passed as ReadTable()'s row limit to read every row
    static constexpr size_t ALL_ROWS = std::numeric_limits<size_t>::max();

    virtual ~DATABASE_BACKEND() = default;

    virtual bool Connect() = 0;

    virtual void Disconnect() = 0;

    virtual bool IsConnected() const = 0;

    /// Lists the tables and views, sorted by name
    virtual bool ListTables( std::vector<std::string>& aTables ) = 0;

    /// @return the table's columns in catalog order, or nullopt if it was not found or the catalog failed
    virtual std::optional<DB_TABLE_INFO> DescribeTable( const std::string& aTable ) = 0;

    /// Lists the column names of a table as the catalog reports them, empty if it has none, false if the catalog failed
    virtual bool ListColumns( const std::string& aTable, std::vector<std::string>& aColumns );

    /**
     * Reads rows of a table, handing each to aSink.
     *
     * @param aColumns the columns to read; empty reads all of them
     * @param aMaxRows stops after this many rows; zero fills aLabels and fetches no rows. A bounded
     *                 read is ordered by the first column where the backend can.
     * @param aLabels is filled with the result-column labels as the source spells them, before the
     *                first call to aSink
     * @return false on failure, in which case the caller discards any rows already delivered
     */
    virtual bool ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns, size_t aMaxRows,
                            std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink ) = 0;

    /**
     * Reads every row of a table for a caller that keeps them, handing each to aSink.  A backend that checks one
     * read against another may deliver the table more than once, and calls aRestart before every delivery after
     * the first, so the caller drops the rows it already took.  The default reads once.
     *
     * @param aLabels is filled with the result-column labels before each delivery's first call to aSink
     * @return false on failure, in which case the caller discards any rows already delivered
     */
    virtual bool ReadAllRows( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                              std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink,
                              const DB_RESTART_SINK& aRestart )
    {
        return ReadTable( aTable, aColumns, ALL_ROWS, aLabels, aSink );
    }

    /**
     * Brackets a load cycle, in which the caller reads each table once.  A backend may keep what it opens for the
     * cycle's reads until the cycle ends.  Cycles may nest, and only the outermost begin and end
     * count.  Nesting is backend-wide and may cross threads, as when a lookup on one thread reads a table while a
     * refresh on another has a cycle open.  The nested read keeps the stop check and sessions of the outermost cycle.
     *
     * @param aStopped may be empty; a backend that retries a read gives up when it returns true
     */
    virtual void BeginLoadCycle( const DB_STOP_CHECK& aStopped ) {}

    virtual void EndLoadCycle() {}

    /**
     * Reads the one row whose key column equals aKey, using an indexed lookup if the backend has one.
     *
     * @return nullopt if the backend has no indexed lookup, otherwise true when aRow was filled
     */
    virtual std::optional<bool> ReadRow( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                                         const std::string& aKeyColumn, const std::string& aKey,
                                         std::vector<std::string>& aLabels, std::vector<DB_VALUE>& aRow )
    {
        return std::nullopt;
    }

    virtual std::string GetLastError() const = 0;

    virtual DB_FAILURE GetLastFailure() const = 0;

    /**
     * Builds the backend for a source.
     *
     * @param aSource the source as configured; ${CWD} is expanded in its connection string and path
     * @param aBasePath the directory ${CWD} stands for
     * @return the backend, or a null backend with the failure and a message naming the problem
     */
    static DB_BACKEND_RESULT Create( const DATABASE_SOURCE& aSource, const wxString& aBasePath );
};

#endif // KICAD_DATABASE_BACKEND_H
