/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/locale.hpp>
#include <fmt/core.h>
#include <nanodbc/nanodbc.h>
#include <sql.h> // SQL_IDENTIFIER_QUOTE_CHAR
#include <wx/log.h>

#include <database/database_connection.h>


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

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::DATABASE_CONNECTION( const std::string& aConnectionString,
                                          int aTimeoutSeconds, bool aConnectNow ) :
        m_quoteChar( '"' )
{
    m_connectionString = aConnectionString;
    m_timeout          = aTimeoutSeconds;

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::~DATABASE_CONNECTION()
{
    Disconnect();
    m_conn.reset();
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
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        return false;
    }

    if( IsConnected() )
    {
        syncTables();
        cacheColumns();
        getQuoteChar();
    }

    return IsConnected();
}


bool DATABASE_CONNECTION::Disconnect()
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Note: Disconnect() called without valid connection" ) );
        return false;
    }

    m_conn->disconnect();

    return !m_conn->connected();
}


bool DATABASE_CONNECTION::IsConnected() const
{
    if( !m_conn )
        return false;

    return m_conn->connected();
}


bool DATABASE_CONNECTION::syncTables()
{
    if( !m_conn )
        return false;

    m_tables.clear();

    try
    {
        nanodbc::catalog catalog( *m_conn );
        nanodbc::catalog::tables tables = catalog.find_tables();

        while( tables.next() )
        {
            std::string key = toUTF8( tables.table_name() );
            m_tables[key] = toUTF8( tables.table_type() );
        }
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while syncing tables: %s" ), m_lastError );
        return false;
    }

    return true;
}


bool DATABASE_CONNECTION::cacheColumns()
{
    if( !m_conn )
        return false;

    m_columnCache.clear();

    for( const auto& tableIter : m_tables )
    {
        try
        {
            nanodbc::catalog catalog( *m_conn );
            nanodbc::catalog::columns columns =
                    catalog.find_columns( NANODBC_TEXT( "" ), fromUTF8( tableIter.first ) );

            while( columns.next() )
            {
                std::string columnKey = toUTF8( columns.column_name() );
                m_columnCache[tableIter.first][columnKey] = columns.data_type();
            }
                
        }
        catch( nanodbc::database_error& e )
        {
            m_lastError = e.what();
            wxLogTrace( traceDatabase, wxT( "Exception while syncing columns for table %s: %s" ),
                        tableIter.first, m_lastError );
            return false;
        }
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
    catch( nanodbc::database_error& e )
    {
        wxLogTrace( traceDatabase, wxT( "Exception while querying quote char: %s" ), m_lastError );
        return false;
    }

    return true;
}


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

    nanodbc::statement statement( *m_conn );

    nanodbc::string query = fromUTF8( fmt::format( "SELECT * FROM {}{}{} WHERE {}{}{} = ?",
                                                   m_quoteChar, tableName, m_quoteChar,
                                                   m_quoteChar, columnName, m_quoteChar ) );

    try
    {
        statement.prepare( query );
        statement.bind( 0, aWhere.second.c_str() );
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while preparing statement for SelectOne: %s" ),
                    m_lastError );
        return false;
    }

    wxLogTrace( traceDatabase, wxT( "SelectOne: `%s` with parameter `%s`" ), toUTF8( query ),
                aWhere.second );

    nanodbc::result results;

    try
    {
        results = nanodbc::execute( statement );
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while executing statement for SelectOne: %s" ),
                    m_lastError );
        return false;
    }

    if( !results.first() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectOne: no results returned from query" ) );
        return false;
    }

    wxLogTrace( traceDatabase, wxT( "SelectOne: %ld results returned from query" ),
                results.rows() );

    aResult.clear();

    try
    {
        for( short i = 0; i < results.columns(); ++i )
        {
            std::string column = toUTF8( results.column_name( i ) );
            aResult[ column ] = toUTF8( results.get<nanodbc::string>( i, NANODBC_TEXT( "" ) ) );
        }
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while parsing results from SelectOne: %s" ),
                    m_lastError );
        return false;
    }

    return true;
}


bool DATABASE_CONNECTION::SelectAll( const std::string& aTable, std::vector<ROW>& aResults )
{
    if( !m_conn )
    {
        wxLogTrace( traceDatabase, wxT( "Called SelectAll without valid connection!" ) );
        return false;
    }

    auto tableMapIter = m_tables.find( aTable );

    if( tableMapIter == m_tables.end() )
    {
        wxLogTrace( traceDatabase, wxT( "SelectAll: requested table %s not found in cache" ),
                    aTable );
        return false;
    }

    nanodbc::statement statement( *m_conn );

    nanodbc::string query = fromUTF8( fmt::format( "SELECT * FROM {}{}{}", m_quoteChar,
                                                   tableMapIter->first, m_quoteChar ) );

    wxLogTrace( traceDatabase, wxT( "SelectAll: `%s`" ), toUTF8( query ) );

    try
    {
        statement.prepare( query );
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while preparing query for SelectAll: %s" ),
                    m_lastError );
        return false;
    }

    nanodbc::result results;

    try
    {
        results = nanodbc::execute( statement );
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while executing query for SelectAll: %s" ),
                    m_lastError );
        return false;
    }

    if( !results.first() )
        return false;

    try
    {
        do
        {
            ROW result;

            for( short j = 0; j < results.columns(); ++j )
            {
                std::string column = toUTF8( results.column_name( j ) );
                result[column] = toUTF8( results.get<nanodbc::string>( j, NANODBC_TEXT( "" ) ) );
            }

            aResults.emplace_back( std::move( result ) );

        } while( results.next() );
    }
    catch( nanodbc::database_error& e )
    {
        m_lastError = e.what();
        wxLogTrace( traceDatabase, wxT( "Exception while parsing results from SelectAll: %s" ),
                    m_lastError );
        return false;
    }

    return true;
}
