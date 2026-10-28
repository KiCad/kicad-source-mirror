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

#include "odbc_backend.h"
#include "access_engine_mutex.h"
#include "db_worker_thread.h"

#include <algorithm>
#include <charconv>
#include <mutex>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <fmt/format.h>
#include <nanodbc/nanodbc.h>

#ifdef _WIN32
#include <windows.h> // for sql.h
#endif

#include <sql.h> // SQL_IDENTIFIER_QUOTE_CHAR
#include <sqlext.h> // SQL_LONGVARCHAR

#include <wx/log.h>

#include <database/access_fast_exit.h>
#include <database/database_value.h>
#include <core/profile.h>

/**
 * When Unicode support is enabled in nanodbc, string formats are used matching the appropriate
 * character set of the platform.  KiCad uses UTF-8 encoded strings internally, but different
 * platforms use different encodings for SQL strings.  Unicode mode must be enabled for compilation
 * on Windows, since Visual Studio forces the use of Unicode SQL headers if any part of the project
 * has Unicode enabled.
 */

/**
 * Converts a string from KiCad-native to nanodbc-native
 * @param aString is a UTF-8 encoded string
 * @return a string in nanodbc's platform-specific representation
 */
static nanodbc::string fromUTF8( const std::string& aString )
{
    return boost::locale::conv::utf_to_utf<nanodbc::string::value_type>( aString );
}


/**
 * Converts a string from nanodbc-native to KiCad-native
 * @param aString is a string encoded in nanodbc's platform-specific way
 * @return a string with UTF-8 encoding
 */
static std::string toUTF8( const nanodbc::string& aString )
{
    return boost::locale::conv::utf_to_utf<char>( aString );
}


struct RESULT_COLUMN
{
    std::string name;
    int         type;
};


/// Column names and types are the same for every row, so read them once per result set
static std::vector<RESULT_COLUMN> resultColumns( nanodbc::result& aResults )
{
    std::vector<RESULT_COLUMN> columns;

    for( short i = 0; i < aResults.columns(); ++i )
        columns.push_back( { toUTF8( aResults.column_name( i ) ), aResults.column_datatype( i ) } );

    return columns;
}


/**
 * Decodes the current row of a result set into values in column order.
 * Floating-point columns keep their full precision, exact numerics are trimmed decimal text and nulls become
 * empty strings.
 */
static std::vector<DB_VALUE> decodeRow( nanodbc::result& aResults, const std::vector<RESULT_COLUMN>& aColumns )
{
    std::vector<DB_VALUE> row;
    row.reserve( aColumns.size() );

    for( short i = 0; i < static_cast<short>( aColumns.size() ); ++i )
    {
        switch( aColumns[i].type )
        {
        case SQL_DOUBLE:
        case SQL_FLOAT:
        case SQL_REAL:
            try
            {
                row.emplace_back( aResults.get<double>( i ) );
            }
            catch( nanodbc::null_access_error& )
            {
                row.emplace_back( std::monostate() );
            }

            break;

        case SQL_DECIMAL:
        case SQL_NUMERIC:
            // Text avoids the double conversion that would round past 15 digits
            row.emplace_back(
                    NormalizeDecimalText( toUTF8( aResults.get<nanodbc::string>( i, NANODBC_TEXT( "" ) ) ) ) );
            break;

        default:
            row.emplace_back( toUTF8( aResults.get<nanodbc::string>( i, NANODBC_TEXT( "" ) ) ) );
        }
    }

    return row;
}


ODBC_BACKEND::ODBC_BACKEND( std::string aDsn, std::string aUser, std::string aPass, std::string aConnectionString,
                            long aTimeout ) :
        m_dsn( std::move( aDsn ) ),
        m_user( std::move( aUser ) ),
        m_pass( std::move( aPass ) ),
        m_connectionString( std::move( aConnectionString ) ),
        m_timeout( aTimeout )
{
#ifdef _WIN32
    m_accessDriver = NeedsDriverLock( m_dsn, m_connectionString,
                                      []( const std::string& aName ) -> std::optional<std::string>
                                      {
                                          for( const DATA_SOURCE& source : ListDataSourceDetails() )
                                          {
                                              if( boost::iequals( source.name, aName ) )
                                                  return source.driver;
                                          }

                                          return std::nullopt;
                                      } );
#endif
}


namespace
{
// Handles that could not be freed under the driver lock are kept for good, since freeing them unlocked races
// the driver unload and waiting for the lock can hang exit
std::vector<std::unique_ptr<nanodbc::connection>>& leakedConnections()
{
    static auto* s_leaked = new std::vector<std::unique_ptr<nanodbc::connection>>;

    return *s_leaked;
}

std::mutex& leakedConnectionsMutex()
{
    static auto* s_mutex = new std::mutex;

    return *s_mutex;
}
} // namespace


ODBC_BACKEND::~ODBC_BACKEND()
{
    // An abandoned Access worker can hold the driver lock without bound, and an unopened backend frees nothing
    if( !m_conn )
        return;

    std::unique_lock<std::timed_mutex> driverLock;

    if( !takeAccessDriverLock( driverLock ) )
    {
        // A handle left inside the Access driver crashes its unload at exit
        MarkAccessWorkerAbandoned();

        std::lock_guard<std::mutex> guard( leakedConnectionsMutex() );
        leakedConnections().push_back( std::move( m_conn ) );
        return;
    }

    disconnectLocked();
    m_conn.reset();
}


// The Access driver loads Office libraries when it initializes and unloads, and two libraries doing so at once race
// Other drivers connect and disconnect in parallel
bool ODBC_BACKEND::takeAccessDriverLock( std::unique_lock<std::timed_mutex>& aLock )
{
    if( !m_accessDriver )
        return true;

    wxLogTrace( traceDatabase, wxT( "Taking the Access driver lock" ) );

    // Only a contended wait spends the deadline, so a free lock leaves the whole budget for the close
    aLock = std::unique_lock<std::timed_mutex>( AccessEngineMutex(), std::try_to_lock );

    if( aLock.owns_lock() )
        return true;

    if( !m_engineDeadline )
        m_engineDeadline = std::chrono::steady_clock::now() + DB_WORKER_THREAD::CallerBound( m_timeout );

    aLock = LockAccessEngine( *m_engineDeadline );

    if( !aLock.owns_lock() )
    {
        wxLogTrace( traceDatabase, wxT( "The Access driver lock stayed busy" ) );
        return false;
    }

    return true;
}


ODBC_BACKEND::SELECTOR ODBC_BACKEND::FirstSelector( const std::string& aConnectionString )
{
    size_t pos = 0;
    const size_t size = aConnectionString.size();

    while( pos < size )
    {
        size_t end = aConnectionString.find_first_of( "=;", pos );

        if( end == std::string::npos )
            break;

        std::string key = boost::trim_copy( aConnectionString.substr( pos, end - pos ) );

        if( aConnectionString[end] == ';' )
        {
            pos = end + 1;
            continue;
        }

        pos = aConnectionString.find_first_not_of( " \t", end + 1 );

        if( pos == std::string::npos )
            break;

        std::string value;

        if( aConnectionString[pos] == '{' )
        {
            // A doubled closing brace is a literal brace inside the value
            for( ++pos; pos < size; ++pos )
            {
                if( aConnectionString[pos] == '}' )
                {
                    if( pos + 1 < size && aConnectionString[pos + 1] == '}' )
                    {
                        value += '}';
                        ++pos;
                        continue;
                    }

                    ++pos;
                    break;
                }

                value += aConnectionString[pos];
            }

            pos = aConnectionString.find( ';', pos );
        }
        else
        {
            size_t stop = aConnectionString.find( ';', pos );
            value = boost::trim_copy( aConnectionString.substr( pos, stop - pos ) );
            pos = stop;
        }

        if( boost::iequals( key, "DSN" ) )
            return { SELECTOR::KIND::DSN, value };

        if( boost::iequals( key, "FILEDSN" ) )
            return { SELECTOR::KIND::FILE_DSN, value };

        if( boost::iequals( key, "DRIVER" ) )
            return { SELECTOR::KIND::DRIVER, value };

        if( pos == std::string::npos )
            break;

        ++pos;
    }

    return {};
}


bool ODBC_BACKEND::NeedsDriverLock( const std::string& aDsn, const std::string& aConnectionString,
                                    const DSN_LOOKUP& aLookup )
{
    // A driver that cannot be named is locked too, since it may be the Access driver
    auto lockForDsn =
            [&]( const std::string& aName )
            {
                std::optional<std::string> driver = aLookup( aName );

                return !driver || driver->empty() || IsAccessDriver( *driver );
            };

    if( aConnectionString.empty() )
        return lockForDsn( aDsn );

    // ODBC honours the first DSN, FILEDSN or DRIVER keyword in the string
    SELECTOR selector = FirstSelector( aConnectionString );

    switch( selector.kind )
    {
    case SELECTOR::KIND::DRIVER: return selector.value.empty() || IsAccessDriver( selector.value );
    case SELECTOR::KIND::DSN: return lockForDsn( selector.value );
    case SELECTOR::KIND::FILE_DSN:
    case SELECTOR::KIND::NONE: break;
    }

    return true;
}


bool ODBC_BACKEND::IsAccessDriver( const std::string& aDriverName )
{
    return aDriverName.starts_with( "Microsoft Access Driver" );
}


bool ODBC_BACKEND::Connect()
{
    nanodbc::string dsn  = fromUTF8( m_dsn );
    nanodbc::string user = fromUTF8( m_user );
    nanodbc::string pass = fromUTF8( m_pass );
    nanodbc::string cs   = fromUTF8( m_connectionString );

    m_lastFailure = DB_FAILURE::UNKNOWN;
    m_engineDeadline.reset();

    std::unique_lock<std::timed_mutex> driverLock;

    if( !takeAccessDriverLock( driverLock ) )
    {
        m_lastError = AccessEngineBusyMessage();
        m_lastFailure = DB_FAILURE::TIMEOUT;
        return false;
    }

    try
    {
        if( cs.empty() )
        {
            wxLogTrace( traceDatabase, wxT( "Creating connection to DSN %s" ), m_dsn );
            m_conn = std::make_unique<nanodbc::connection>( dsn, user, pass, m_timeout );
        }
        else
        {
            wxLogTrace( traceDatabase, wxT( "Creating connection with connection string" ) );
            m_conn = std::make_unique<nanodbc::connection>( cs, m_timeout );
        }
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        m_lastFailure = ClassifyFailure( e.state(), m_lastError );
        return false;
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        m_lastFailure = ClassifyFailure( std::string(), m_lastError );
        return false;
    }

    if( IsConnected() )
    {
        getQuoteChar();

        try
        {
            m_patternEscape = toUTF8( m_conn->get_info<nanodbc::string>( SQL_SEARCH_PATTERN_ESCAPE ) );
        }
        catch( std::exception& e )
        {
            m_patternEscape.clear();
            wxLogTrace( traceDatabase, wxT( "Pattern escape unavailable: %s" ), e.what() );
        }
    }

    return IsConnected();
}


void ODBC_BACKEND::Disconnect()
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Note: Disconnect() called without valid connection" ) );
        return;
    }

    std::unique_lock<std::timed_mutex> driverLock;

    if( !takeAccessDriverLock( driverLock ) )
    {
        m_lastError = AccessEngineBusyMessage();
        m_lastFailure = DB_FAILURE::TIMEOUT;
        return;
    }

    disconnectLocked();
}


void ODBC_BACKEND::disconnectLocked()
{
    try
    {
        m_conn->disconnect();
    }
    catch( std::exception& exc )
    {
        wxLogTrace( traceDatabase, wxT( "Disconnect() error \"%s\" occured." ), exc.what() );
    }
}


bool ODBC_BACKEND::IsConnected() const
{
    if( !m_conn )
        return false;

    return m_conn->connected();
}


std::optional<DB_TABLE_INFO> ODBC_BACKEND::DescribeTable( const std::string& aTable )
{
    if( !m_conn )
        return std::nullopt;

    // A table that is simply absent leaves no error, so nothing of an earlier failure may remain
    m_lastError.clear();
    m_lastFailure = DB_FAILURE::UNKNOWN;

    try
    {
        nanodbc::catalog catalog( *m_conn );
        nanodbc::catalog::tables tables = catalog.find_tables( fromUTF8( escapePattern( aTable ) ) );

        if( !tables.next() )
        {
            wxLogTrace( traceDatabase, wxT( "DescribeTable: table '%s' not found in catalog" ), aTable );
            return std::nullopt;
        }

        DB_TABLE_INFO info;
        info.name = toUTF8( tables.table_name() );

        try
        {
            nanodbc::catalog::columns columns =
                    catalog.find_columns( NANODBC_TEXT( "" ), fromUTF8( escapePattern( info.name ) ) );

            std::map<std::string, COLUMN_INFO>& described = m_columnInfo[info.name];
            described.clear();

            while( columns.next() )
            {
                std::string name = toUTF8( columns.column_name() );

                info.columns.push_back( { name, columns.data_type() } );
                described[name] = { columns.data_type(), columns.column_size(), columns.decimal_digits() };
            }
        }
        catch( nanodbc::database_error& e )
        {
            m_lastError = e.what();
            wxLogTrace( traceDatabase, wxT( "Exception while syncing columns for table '%s': %s" ), info.name,
                        m_lastError );

            info.columns.clear();
            info.columnsUnavailable = true;
        }

        return info;
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while describing table: %s" ), m_lastError );
        return std::nullopt;
    }
}


bool ODBC_BACKEND::ListColumns( const std::string& aTable, std::vector<std::string>& aColumns )
{
    aColumns.clear();

    if( !m_conn )
        return false;

    try
    {
        nanodbc::catalog          catalog( *m_conn );
        nanodbc::catalog::columns columns = catalog.find_columns( NANODBC_TEXT( "" ),
                                                                  fromUTF8( escapePattern( aTable ) ) );

        while( columns.next() )
            aColumns.emplace_back( toUTF8( columns.column_name() ) );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "ListColumns for %s failed: %s" ), aTable, m_lastError );
        return false;
    }

    return true;
}


bool ODBC_BACKEND::getQuoteChar()
{
    if( !m_conn )
        return false;

    try
    {
        nanodbc::string qc = m_conn->get_info<nanodbc::string>( SQL_IDENTIFIER_QUOTE_CHAR );

        if( qc.empty() )
            return false;

        m_quoteChar = *toUTF8( qc ).begin();

        wxLogTrace( traceDatabase, wxT( "Quote char retrieved: %c" ), m_quoteChar );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while querying quote char: %s" ), m_lastError );
        return false;
    }

    return true;
}


std::string ODBC_BACKEND::quoted( const std::string& aIdentifier ) const
{
    std::string quote( 1, m_quoteChar );
    std::string escaped = aIdentifier;

    boost::replace_all( escaped, quote, quote + quote );

    return quote + escaped + quote;
}


std::string ODBC_BACKEND::escapePattern( const std::string& aName ) const
{
    // Without an escape character the driver offers no way to match a name literally
    if( m_patternEscape.empty() )
        return aName;

    std::string escaped;

    for( char c : aName )
    {
        if( c == '_' || c == '%' || c == m_patternEscape.front() )
            escaped += m_patternEscape;

        escaped += c;
    }

    return escaped;
}


std::string ODBC_BACKEND::columnList( const std::vector<std::string>& aColumns ) const
{
    if( aColumns.empty() )
        return "*";

    std::string ret;

    for( const std::string& column : aColumns )
        ret += quoted( column ) + ", ";

    // strip tailing ', '
    ret.resize( ret.length() - 2 );

    return ret;
}


// Fallback sizes for when the catalog reports none, and the longest character size worth declaring
static constexpr unsigned long DEFAULT_KEY_CHAR_SIZE = 255;
static constexpr unsigned long MAX_KEY_CHAR_SIZE = 8000;
static constexpr unsigned long DEFAULT_KEY_INTEGER_SIZE = 19; // nanodbc asserts on a zero size when binding numbers
static constexpr unsigned long DEFAULT_KEY_DECIMAL_SIZE = 38; // widest precision ODBC drivers commonly allow


ODBC_BACKEND::KEY_PARAMETER ODBC_BACKEND::DescribeKeyParameter( int aColumnType, long aColumnSize,
                                                                short aColumnScale, const std::string& aKey )
{
    const short type = static_cast<short>( aColumnType );

    // Catalogs report a negative or NULL size for types where it does not apply
    const unsigned long columnSize = aColumnSize > 0 ? static_cast<unsigned long>( aColumnSize ) : 0;

    switch( type )
    {
    case SQL_TINYINT:
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIGINT:
    {
        int64_t     value = 0;
        const char* end = aKey.data() + aKey.size();
        auto        parsed = std::from_chars( aKey.data(), end, value );

        // A key that is not a whole number cannot match an integer column, so the driver is left to reject it
        if( parsed.ec == std::errc() && parsed.ptr == end )
            return { type, columnSize ? columnSize : DEFAULT_KEY_INTEGER_SIZE, 0, true, value };

        break;
    }

    case SQL_DECIMAL:
    case SQL_NUMERIC: return { type, columnSize ? columnSize : DEFAULT_KEY_DECIMAL_SIZE, aColumnScale, false, 0 };

    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
    {
        const bool          wide = type == SQL_WCHAR || type == SQL_WVARCHAR || type == SQL_WLONGVARCHAR;
        const unsigned long size = columnSize ? std::min( columnSize, MAX_KEY_CHAR_SIZE ) : DEFAULT_KEY_CHAR_SIZE;

        return { static_cast<short>( wide ? SQL_WVARCHAR : SQL_VARCHAR ),
                 std::max( size, static_cast<unsigned long>( aKey.size() ) ), 0, false, 0 };
    }

    default: break;
    }

    return { SQL_VARCHAR, DEFAULT_KEY_CHAR_SIZE, 0, false, 0 };
}


std::optional<bool> ODBC_BACKEND::ReadRow( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                                           const std::string& aKeyColumn, const std::string& aKey,
                                           std::vector<std::string>& aLabels, std::vector<DB_VALUE>& aRow )
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Called ReadRow without valid connection!" ) );
        return false;
    }

    COLUMN_INFO columnInfo = { SQL_VARCHAR, 0, 0 };

    if( auto table = m_columnInfo.find( aTable.name ); table != m_columnInfo.end() )
    {
        if( auto column = table->second.find( aKeyColumn ); column != table->second.end() )
            columnInfo = column->second;
    }

    std::string queryStr = fmt::format( "SELECT {} FROM {} WHERE {} = ?", columnList( aColumns ),
                                        quoted( aTable.name ), quoted( aKeyColumn ) );
    nanodbc::string query = fromUTF8( queryStr );

    PROF_TIMER timer;
    nanodbc::statement statement;

    try
    {
        statement.prepare( *m_conn, query );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while preparing statement for SelectOne: %s" ),
                    m_lastError );

        // Exception may be due to a connection error; nanodbc won't auto-reconnect
        Disconnect();

        return false;
    }

    // Pre-describe the parameter to avoid SQLDescribeParam, which some ODBC drivers (Microsoft Access,
    // Excel, CSV) don't implement
    const KEY_PARAMETER key = DescribeKeyParameter( columnInfo.type, columnInfo.size, columnInfo.scale, aKey );

    try
    {
        statement.describe_parameters( { 0 }, { key.type }, { key.size }, { key.scale } );

        if( key.asInteger )
            statement.bind( 0, &key.integerValue );
        else
            statement.bind( 0, aKey.c_str() );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while binding parameter for SelectOne: %s" ),
                    m_lastError );

        Disconnect();

        return false;
    }

    wxLogTrace( traceDatabase, wxT( "SelectOne: `%s` with parameter `%s`" ), toUTF8( query ), aKey );

    nanodbc::result results;

    try
    {
        results = nanodbc::execute( statement );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while executing statement for SelectOne: %s" ),
                    m_lastError );

        // Exception may be due to a connection error; nanodbc won't auto-reconnect
        Disconnect();

        return false;
    }

    timer.Stop();

    if( !results.first() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: no results returned from query" ) );
        return false;
    }

    wxLogTrace( traceDatabase, wxT( "SelectOne: %ld results returned from query in %0.1f ms" ),
                results.rows(), timer.msecs() );

    try
    {
        const std::vector<RESULT_COLUMN> columns = resultColumns( results );

        aLabels.clear();

        for( const RESULT_COLUMN& column : columns )
            aLabels.push_back( column.name );

        aRow = decodeRow( results, columns );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while parsing results from SelectOne: %s" ),
                    m_lastError );
        return false;
    }

    return true;
}


bool ODBC_BACKEND::ReadTable( const DB_TABLE_INFO& aTable, const std::vector<std::string>& aColumns,
                              size_t aMaxRows, std::vector<std::string>& aLabels, const DB_ROW_SINK& aSink )
{
    aLabels.clear();

    if( !m_conn )
        return false;

    const bool all = aMaxRows == ALL_ROWS;

    try
    {
        const std::string select = fmt::format( "SELECT {} FROM {}", columnList( aColumns ), quoted( aTable.name ) );
        PROF_TIMER        timer;
        nanodbc::result   results;

        if( all )
        {
            nanodbc::statement statement( *m_conn );

            statement.prepare( fromUTF8( select ) );
            results = nanodbc::execute( statement );
        }
        else
        {
            // Direct execution keeps the default forward-only cursor; prepare() would force a keyset
            // cursor that some drivers build over the whole table before returning a row
            try
            {
                // Ordered so every server returns the same rows; the first column is usually the indexed key
                results = nanodbc::execute( *m_conn, fromUTF8( select + " ORDER BY 1" ), 1, m_timeout );
            }
            catch( const nanodbc::database_error& e )
            {
                // Some column types cannot be ordered (e.g. Postgres json), so fall back to server order
                wxLogTrace( traceDatabase, wxT( "ReadTable: ordered query on %s failed (%s); retrying unordered" ),
                            aTable.name, e.what() );
                results = nanodbc::execute( *m_conn, fromUTF8( select ), 1, m_timeout );
            }
        }

        timer.Stop();

        const std::vector<RESULT_COLUMN> columns = resultColumns( results );

        for( const RESULT_COLUMN& column : columns )
            aLabels.push_back( column.name );

        size_t rows = 0;

        while( rows < aMaxRows && results.next() )
        {
            try
            {
                aSink( decodeRow( results, columns ) );
            }
            catch( std::runtime_error& e )
            {
                m_lastError = e.what();
                wxLogTrace( traceDatabase, wxT( "Exception while parsing result %zu from ReadTable: %s" ), rows,
                            m_lastError );
                return false;
            }

            ++rows;
        }

        wxLogTrace( traceDatabase, wxT( "ReadTable from %s completed in %0.1f ms" ), aTable.name, timer.msecs() );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "ReadTable from %s failed: %s" ), aTable.name, m_lastError );

        // Exception may be due to a connection error; nanodbc won't auto-reconnect
        if( all )
            Disconnect();

        return false;
    }

    return true;
}


bool ODBC_BACKEND::ListTables( std::vector<std::string>& aTables )
{
    aTables.clear();

    if( !m_conn )
        return false;

    try
    {
        nanodbc::catalog catalog( *m_conn );
        nanodbc::string  current;

        try
        {
            current = m_conn->catalog_name();
        }
        catch( const std::exception& )
        {
            // Without a current catalog every listed table is offered
        }

        auto listTables =
                [&]( const nanodbc::string& aCatalog )
                {
                    nanodbc::catalog::tables tables = catalog.find_tables( NANODBC_TEXT( "" ),
                                                                           NANODBC_TEXT( "TABLE,VIEW" ),
                                                                           NANODBC_TEXT( "" ), aCatalog );

                    while( tables.next() )
                        aTables.emplace_back( toUTF8( tables.table_name() ) );
                };

        // MariaDB Connector/ODBC 3.1 lists every visible database but queries name bare tables in the
        // current one; the driver applies its case rules to this restriction, a pattern under ODBC 3
        listTables( fromUTF8( escapePattern( toUTF8( current ) ) ) );

        // A catalog name the driver reported truncated or spelled differently matches nothing
        if( aTables.empty() && !current.empty() )
            listTables( NANODBC_TEXT( "" ) );
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "ListTables failed: %s" ), m_lastError );
        return false;
    }

    std::sort( aTables.begin(), aTables.end() );
    return true;
}


std::vector<ODBC_BACKEND::DATA_SOURCE> ODBC_BACKEND::ListDataSourceDetails()
{
    std::vector<DATA_SOURCE> sources;

    try
    {
        for( const nanodbc::datasource& source : nanodbc::list_datasources() )
            sources.push_back( { toUTF8( source.name ), toUTF8( source.driver ) } );
    }
    catch( std::exception& e )
    {
        wxLogTrace( traceDatabase, wxT( "ListDataSourceDetails failed: %s" ), e.what() );
    }

    return sources;
}


std::vector<std::string> ODBC_BACKEND::ListDrivers()
{
    std::vector<std::string> names;

    try
    {
        for( const nanodbc::driver& driver : nanodbc::list_drivers() )
            names.emplace_back( toUTF8( driver.name ) );
    }
    catch( std::exception& e )
    {
        wxLogTrace( traceDatabase, wxT( "ListDrivers failed: %s" ), e.what() );
    }

    return names;
}


bool ODBC_BACKEND::IsAccessFileInUse( const std::string& aLowerMessage )
{
    // Only the driver's own whole message counts, so a name echoed by another driver cannot imitate it.  The
    // templates are the English driver's, so a localized Office keeps the failure unclassified
    static const std::string ACCESS_TAG = "[odbc microsoft access driver]";

    // A path or user name may hold an apostrophe, so the substitutions take any text up to the next literal
    static const std::regex LOCKED(
            R"(\s*(could not use '[^\r\n]*'; file already in use)"
            R"(|could not lock file)"
            R"(|database '[^\r\n]*' is exclusively locked)"
            R"(|you attempted to open a database that is already opened (exclusively )?by user '[^\r\n]*')"
            R"( on machine '[^\r\n]*')"
            R"(|the database '[^\r\n]*' is already opened by user '[^\r\n]*' on machine '[^\r\n]*')"
            R"(|the database has been placed in a state by user '[^\r\n]*' on machine '[^\r\n]*')"
            R"( that prevents it from being opened or locked)\.?\s*)"
            R"((try again when the database is available\.?\s*)?)" );

    const size_t tag = aLowerMessage.rfind( ACCESS_TAG );

    return tag != std::string::npos
           && std::regex_match( aLowerMessage.begin() + tag + ACCESS_TAG.size(), aLowerMessage.end(), LOCKED );
}


DB_FAILURE ODBC_BACKEND::ClassifyFailure( const std::string& aState, const std::string& aMessage )
{
    std::string message = boost::to_lower_copy( aMessage );

    auto mentions =
            [&]( std::initializer_list<const char*> aPhrases )
            {
                for( const char* phrase : aPhrases )
                {
                    if( message.find( phrase ) != std::string::npos )
                        return true;
                }

                return false;
            };

    // Postgres reports rejected logins as 08001, so the text decides; MySQL's database-level
    // "access denied" lacks the password clause and is not a login failure
    if( aState.starts_with( "28" )
        || mentions( { "password authentication failed", "(using password:", "login failed", "authentication failed",
                       "no password supplied" } ) )
    {
        return DB_FAILURE::LOGIN;
    }

    if( aState == "IM002" || mentions( { "data source name not found" } ) )
        return DB_FAILURE::NO_DSN;

    if( aState == "IM003" || mentions( { "can't open lib", "could not be loaded" } ) )
        return DB_FAILURE::NO_DRIVER;

    if( aState.starts_with( "HYT" ) || mentions( { "timeout expired", "timed out" } ) )
        return DB_FAILURE::TIMEOUT;

    if( IsAccessFileInUse( message ) )
        return DB_FAILURE::FILE_IN_USE;

    // Class 08 alone also covers failures after the server answered, such as certificate checks
    if( mentions( { "connection refused", "could not connect", "can't connect", "unknown host",
                    "could not translate host name", "no route to host", "network is unreachable" } ) )
    {
        return DB_FAILURE::UNREACHABLE;
    }

    return DB_FAILURE::UNKNOWN;
}
