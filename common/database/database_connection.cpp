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

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <fmt/core.h>
#include <nanodbc/nanodbc.h>

// Some outdated definitions are used in sql.h
// We need to define them for "recent" dev tools
#define INT64 int64_t
#define UINT64 uint64_t

#ifdef __MINGW32__
#define BYTE uint8_t
#define WORD uint16_t
#define DWORD uint32_t
#define HWND uint32_t   /* dummy define */
#endif

#ifdef WIN32
#include <windows.h> // for sql.h
#endif

#include <sql.h> // SQL_IDENTIFIER_QUOTE_CHAR

#include <wx/log.h>

#include <database/database_connection.h>
#include <database/database_cache.h>
#include <core/profile.h>


const char* const traceDatabase = "KICAD_DATABASE";

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
nanodbc::string fromUTF8( const std::string& aString )
{
    return boost::locale::conv::utf_to_utf<nanodbc::string::value_type>( aString );
}


/**
 * Converts a string from nanodbc-native to KiCad-native
 * @param aString is a string encoded in nanodbc's platform-specific way
 * @return a string with UTF-8 encoding
 */
std::string toUTF8( const nanodbc::string& aString )
{
    return boost::locale::conv::utf_to_utf<char>( aString );
}


DATABASE_CONNECTION::DATABASE_CONNECTION( const std::string& aDataSourceName,
                                          const std::string& aUsername,
                                          const std::string& aPassword, int aTimeoutSeconds,
                                          bool aConnectNow ) :
        m_quoteChar( '"' )
{
    m_dsn     = aDataSourceName;
    m_user    = aUsername;
    m_pass    = aPassword;
    m_timeout = aTimeoutSeconds;

    init();

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::DATABASE_CONNECTION( const std::string& aConnectionString,
                                          int aTimeoutSeconds, bool aConnectNow ) :
        m_quoteChar( '"' )
{
    m_connectionString = aConnectionString;
    m_timeout          = aTimeoutSeconds;

    init();

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::~DATABASE_CONNECTION()
{
    Disconnect();
    m_conn.reset();
}


void DATABASE_CONNECTION::init()
{
    m_cache = std::make_unique<DB_CACHE_TYPE>( 10, 1 );
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
    nanodbc::string dsn  = fromUTF8( m_dsn );
    nanodbc::string user = fromUTF8( m_user );
    nanodbc::string pass = fromUTF8( m_pass );
    nanodbc::string cs   = fromUTF8( m_connectionString );

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
    catch( std::exception& e )
    {
        m_lastError = e.what();
        return false;
    }

    m_tables.clear();

    if( IsConnected() )
        getQuoteChar();

    return IsConnected();
}


bool DATABASE_CONNECTION::Disconnect()
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Note: Disconnect() called without valid connection" ) );
        return false;
    }

    try
    {
        m_conn->disconnect();
    }
    catch( std::exception& exc )
    {
        wxLogTrace( traceDatabase, wxT( "Disconnect() error \"%s\" occured." ), exc.what() );
        return false;
    }

    return !m_conn->connected();
}


bool DATABASE_CONNECTION::IsConnected() const
{
    if( !m_conn )
        return false;

    return m_conn->connected();
}


bool DATABASE_CONNECTION::CacheTableInfo( const std::string& aTable,
                                          const std::set<std::string>& aColumns )
{
    if( !m_conn )
        return false;

    try
    {
        nanodbc::catalog catalog( *m_conn );
        nanodbc::catalog::tables tables = catalog.find_tables( fromUTF8( aTable ) );

        if( !tables.next() )
        {
            wxLogTrace( traceDatabase, wxT( "CacheTableInfo: table '%s' not found in catalog" ),
                        aTable );
            return false;
        }

        std::string key = toUTF8( tables.table_name() );
        m_tables[key] = toUTF8( tables.table_type() );

        try
        {
            nanodbc::catalog::columns columns =
                    catalog.find_columns( NANODBC_TEXT( "" ), tables.table_name() );

            while( columns.next() )
            {
                std::string columnKey = toUTF8( columns.column_name() );

                if( aColumns.count( boost::to_lower_copy( columnKey ) ) )
                    m_columnCache[key][columnKey] = columns.data_type();
            }

        }
        catch( nanodbc::database_error& e )
        {
            m_lastError = e.what();
            wxLogTrace( traceDatabase, wxT( "Exception while syncing columns for table '%s': %s" ),
                        key, m_lastError );
            return false;
        }
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while caching table info: %s" ), m_lastError );
        return false;
    }

    return true;
}


bool DATABASE_CONNECTION::getQuoteChar()
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


std::string DATABASE_CONNECTION::columnsFor( const std::string& aTable )
{
    if( !m_columnCache.count( aTable ) )
    {
        wxLogTrace( traceDatabase, wxT( "columnsFor: requested table %s missing from cache!" ),
                    aTable );
        return "*";
    }

    if( m_columnCache[aTable].empty() )
    {
        wxLogTrace( traceDatabase, wxT( "columnsFor: requested table %s has no columns mapped!" ),
                    aTable );
        return "*";
    }

    std::string ret;

    for( const auto& [ columnName, columnType ] : m_columnCache[aTable] )
        ret += fmt::format( "{}{}{}, ", m_quoteChar, columnName, m_quoteChar );

    // strip tailing ', '
    ret.resize( ret.length() - 2 );

    return ret;
}

//next step, make SelectOne take from the SelectAll cache if the SelectOne cache is missing.
//To do this, need to build a map of PK->ROW for the cache result.
bool DATABASE_CONNECTION::SelectOne( const std::string& aTable,
                                     const std::pair<std::string, std::string>& aWhere,
                                     DATABASE_CONNECTION::ROW& aResult )
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Called SelectOne without valid connection!" ) );
        return false;
    }

    auto tableMapIter = m_tables.find( aTable );

    if( tableMapIter == m_tables.end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: requested table %s not found in cache" ),
                    aTable );
        return false;
    }

    const std::string& tableName = tableMapIter->first;
    DB_CACHE_TYPE::CACHE_VALUE cacheEntry;

    if( m_cache->Get( tableName, cacheEntry ) )
    {
        if( cacheEntry.count( aWhere.second ) )
        {
            wxLogTrace( traceDatabase, wxT( "SelectOne: `%s` with parameter `%s` - cache hit" ),
                        tableName, aWhere.second );
            aResult = cacheEntry.at( aWhere.second );
            return true;
        }
    }
    else
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: table `%s` not in row cache; will SelectAll" ),
                    tableName, aWhere.second );

        selectAllAndCache( tableName, aWhere.first );

        if( m_cache->Get( tableName, cacheEntry ) )
        {
            if( cacheEntry.count( aWhere.second ) )
            {
                wxLogTrace( traceDatabase, wxT( "SelectOne: `%s` with parameter `%s` - cache hit" ),
                            tableName, aWhere.second );
                aResult = cacheEntry.at( aWhere.second );
                return true;
            }
        }
    }

    if( !m_columnCache.count( tableName ) )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: requested table %s missing from column cache" ),
                    tableName );
        return false;
    }

    auto columnCacheIter = m_columnCache.at( tableName ).find( aWhere.first );

    if( columnCacheIter == m_columnCache.at( tableName ).end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: requested column %s not found in cache for %s" ),
                    aWhere.first, tableName );
        return false;
    }

    const std::string& columnName = columnCacheIter->first;

    std::string cacheKey = fmt::format( "{}{}{}", tableName, columnName, aWhere.second );

    std::string queryStr = fmt::format( "SELECT {} FROM {}{}{} WHERE {}{}{} = ?",
                                        columnsFor( tableName ),
                                        m_quoteChar, tableName, m_quoteChar,
                                        m_quoteChar, columnName, m_quoteChar );
    nanodbc::string query = fromUTF8( queryStr );

    PROF_TIMER timer;
    nanodbc::statement statement;

    try
    {
        statement.prepare( *m_conn, query );
        statement.bind( 0, aWhere.second.c_str() );
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

    wxLogTrace( traceDatabase, wxT( "SelectOne: `%s` with parameter `%s`" ), toUTF8( query ),
                aWhere.second );

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

    aResult.clear();

    try
    {
        for( short i = 0; i < results.columns(); ++i )
        {
            std::string column = toUTF8( results.column_name( i ) );

            switch( results.column_datatype( i ) )
            {
            case SQL_DOUBLE:
            case SQL_FLOAT:
            case SQL_REAL:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            {
                try
                {
                    aResult[column] = fmt::format( "{:G}", results.get<double>( i ) );
                }
                catch( nanodbc::null_access_error& )
                {
                    // Column was empty (null)
                    aResult[column] = std::string();
                }

                break;
            }

            default:
                aResult[column] = toUTF8( results.get<nanodbc::string>( i, NANODBC_TEXT( "" ) ) );
            }
        }
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


bool DATABASE_CONNECTION::selectAllAndCache( const std::string& aTable, const std::string& aKey )
{
    try
    {
        nanodbc::statement statement( *m_conn );

        nanodbc::string query = fromUTF8( fmt::format( "SELECT {} FROM {}{}{}",
                                                       columnsFor( aTable ),
                                                       m_quoteChar, aTable, m_quoteChar ) );

        wxLogTrace( traceDatabase, wxT( "selectAllAndCache: `%s`" ), toUTF8( query ) );

        PROF_TIMER timer;

        try
        {
            statement.prepare( query );
        }
        catch( std::exception& e )
        {
            m_lastError = e.what();
            wxLogTrace( traceDatabase,
                        wxT( "Exception while preparing query for selectAllAndCache: %s" ),
                        m_lastError );

            // Exception may be due to a connection error; nanodbc won't auto-reconnect
            Disconnect();

            return false;
        }

        nanodbc::result results;

        try
        {
            results = nanodbc::execute( statement );
        }
        catch( std::exception& e )
        {
            m_lastError = e.what();
            wxLogTrace( traceDatabase,
                        wxT( "Exception while executing query for selectAllAndCache: %s" ),
                        m_lastError );

            // Exception may be due to a connection error; nanodbc won't auto-reconnect
            Disconnect();

            return false;
        }

        timer.Stop();

        DB_CACHE_TYPE::CACHE_VALUE cacheEntry;

        auto handleException =
                [&]( std::runtime_error& aException, const std::string& aExtraContext = "" )
                {
                    m_lastError = aException.what();
                    std::string extra = aExtraContext.empty() ? "" : ": " + aExtraContext;
                    wxLogTrace( traceDatabase,
                                wxT( "Exception while parsing result %d from selectAllAndCache: %s%s" ),
                                cacheEntry.size(), m_lastError, extra );
                };

        while( results.next() )
        {
            short columnCount = 0;
            ROW result;

            try
            {
                columnCount = results.columns();
            }
            catch( nanodbc::database_error& e )
            {
                handleException( e );
                return false;
            }

            for( short j = 0; j < columnCount; ++j )
            {
                std::string column;
                std::string columnExtraDbgInfo;
                int datatype = SQL_UNKNOWN_TYPE;

                try
                {
                    column = toUTF8( results.column_name( j ) );
                    datatype = results.column_datatype( j );
                    columnExtraDbgInfo = fmt::format( "column index {}, name '{}', type {}",
                                                      j,
                                                      column,
                                                      datatype );
                }
                catch( nanodbc::index_range_error& e )
                {
                    handleException( e, columnExtraDbgInfo );
                    return false;
                }

                switch( datatype )
                {
                case SQL_DOUBLE:
                case SQL_FLOAT:
                case SQL_REAL:
                case SQL_DECIMAL:
                case SQL_NUMERIC:
                    try
                    {
                        result[column] = fmt::format( "{:G}", results.get<double>( j ) );
                    }
                    catch( nanodbc::null_access_error& )
                    {
                        // Column was empty (null)
                        result[column] = std::string();
                    }
                    catch( std::runtime_error& e )
                    {
                        handleException( e, columnExtraDbgInfo );
                        return false;
                    }

                    break;

                default:
                    try
                    {
                        result[column] = toUTF8( results.get<nanodbc::string>( j, NANODBC_TEXT( "" ) ) );
                    }
                    catch( std::runtime_error& e )
                    {
                        handleException( e, columnExtraDbgInfo );
                        return false;
                    }
                }
            }

            if( !result.count( aKey ) )
            {
                wxLogTrace( traceDatabase,
                            wxT( "selectAllAndCache: warning: key %s not found in result set" ), aKey );
                continue;
            }

            std::string keyStr = std::any_cast<std::string>( result.at( aKey ) );
            cacheEntry[keyStr] = result;
        }

        wxLogTrace( traceDatabase, wxT( "selectAllAndCache from %s completed in %0.1f ms" ), aTable,
                    timer.msecs() );

        m_cache->Put( aTable, cacheEntry );
        return true;
    }
    catch( std::exception& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception in selectAllAndCache: %s" ), m_lastError );

        // Exception may be due to a connection error; nanodbc won't auto-reconnect
        Disconnect();

        return false;
    }
}


bool DATABASE_CONNECTION::SelectAll( const std::string& aTable, const std::string& aKey, std::vector<ROW>& aResults )
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Called SelectAll without valid connection!" ) );
        return false;
    }

    auto tableMapIter = m_tables.find( aTable );

    if( tableMapIter == m_tables.end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectAll: requested table %s not found in cache" ), aTable );
        return false;
    }

    DB_CACHE_TYPE::CACHE_VALUE cacheEntry;

    if( !m_cache->Get( aTable, cacheEntry ) )
    {
        if( !selectAllAndCache( aTable, aKey ) )
        {
            wxLogTrace( traceDatabase, wxT( "SelectAll: `%s` cache fill failed" ), aTable );
            return false;
        }

        // Now it should be filled
        m_cache->Get( aTable, cacheEntry );
    }

    if( !m_cache->Get( aTable, cacheEntry ) )
    {
        wxLogTrace( traceDatabase, wxT( "SelectAll: `%s` failed to get results from cache!" ), aTable );
        return false;
    }

    wxLogTrace( traceDatabase, wxT( "SelectAll: `%s` - returning cached results" ), aTable );

    aResults.reserve( cacheEntry.size() );

    for( auto &[ key, row ] : cacheEntry )
        aResults.emplace_back( row );

    return true;
}
