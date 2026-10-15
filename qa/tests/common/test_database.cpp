/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
* Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <filesystem>
#include <random>

#include <fmt/format.h>
#include <boost/test/unit_test.hpp>
#include <nanodbc/nanodbc.h>

#include <database/database_connection.h>

BOOST_AUTO_TEST_SUITE( Database )


BOOST_AUTO_TEST_CASE( Connect )
{
    std::string cs = fmt::format( "Driver={{SQLite3}};Database={}/database.sqlite",
                                  QA_DATABASE_FILE_LOCATION );

    // Construct and connect
    DATABASE_CONNECTION dc( cs, 2, false );
    BOOST_CHECK_NO_THROW( dc.Connect() );
    BOOST_CHECK( dc.IsConnected() );

    dc.Disconnect();
    BOOST_CHECK( !dc.IsConnected() );

    dc.Connect();
    BOOST_CHECK( dc.IsConnected() );

    dc.Disconnect();

    // Scoped connection should self-disconnect
    {
        DATABASE_CONNECTION dc2( cs, 2 );
    }

    dc.Connect();
    dc.CacheTableInfo( "Resistors", { "Part ID", "MPN" } );
    dc.CacheTableInfo( "Capacitors", { "Part ID", "Cost" } );
    BOOST_CHECK( dc.IsConnected() );

    DATABASE_CONNECTION::ROW result;

    BOOST_CHECK( dc.SelectOne( "Resistors", std::make_pair( "Part ID", "RES-001" ), result ) );

    BOOST_CHECK( !result.empty() );
    BOOST_CHECK( result.count( "MPN" ) );
    BOOST_CHECK_NO_THROW( std::any_cast<std::string>( result.at( "MPN" ) ) );
    BOOST_CHECK_EQUAL( std::any_cast<std::string>( result.at( "MPN" ) ), "RC0603FR-0710KL" );

    BOOST_CHECK( dc.SelectOne( "Capacitors", std::make_pair( "Part ID", "CAP-003" ), result ) );

    BOOST_CHECK( !result.empty() );
    BOOST_CHECK( result.count( "Cost" ) );
    BOOST_CHECK_NO_THROW( std::any_cast<std::string>( result.at( "Cost" ) ) );
    BOOST_CHECK_EQUAL( std::any_cast<std::string>( result.at( "Cost" ) ), "1.95" );
}


/**
 * Test that generated columns in SQLite are properly handled.
 *
 * SQLite ODBC drivers use PRAGMA table_info to implement SQLColumns, which doesn't
 * return generated columns. This test verifies that KiCad can still query tables
 * with generated columns by trusting user-configured column names.
 *
 * See https://gitlab.com/kicad/code/kicad/-/issues/16952
 */
BOOST_AUTO_TEST_CASE( GeneratedColumns )
{
    std::string cs = fmt::format( "Driver={{SQLite3}};Database={}/database.sqlite",
                                  QA_DATABASE_FILE_LOCATION );

    DATABASE_CONNECTION dc( cs, 2 );
    BOOST_CHECK( dc.IsConnected() );

    // Cache table info including the generated column "FullDesc"
    // The SQLite ODBC driver won't report this column via SQLColumns,
    // but our fix should add it to the cache anyway
    dc.CacheTableInfo( "GeneratedTest", { "part id", "value", "resistance", "fulldesc" } );

    DATABASE_CONNECTION::ROW result;

    BOOST_CHECK( dc.SelectOne( "GeneratedTest", std::make_pair( "Part ID", "GEN-001" ), result ) );
    BOOST_CHECK( !result.empty() );

    // Verify we can read the generated column value
    BOOST_CHECK( result.count( "FullDesc" ) );
    BOOST_CHECK_NO_THROW( std::any_cast<std::string>( result.at( "FullDesc" ) ) );
    BOOST_CHECK_EQUAL( std::any_cast<std::string>( result.at( "FullDesc" ) ), "10K - 10000ohm" );

    // Also verify regular columns still work
    BOOST_CHECK( result.count( "Value" ) );
    BOOST_CHECK_EQUAL( std::any_cast<std::string>( result.at( "Value" ) ), "10K" );
}


/// A throwaway SQLite database whose tables are created by the test
struct SCRATCH_DATABASE
{
    SCRATCH_DATABASE() :
            m_path( std::filesystem::temp_directory_path()
                    / fmt::format( "kicad_qa_{}.sqlite", std::random_device()() ) )
    {
    }

    ~SCRATCH_DATABASE()
    {
        std::error_code ec;
        std::filesystem::remove( m_path, ec );
    }

    std::string ConnectionString() const
    {
        return fmt::format( "Driver={{SQLite3}};Database={}", m_path.string() );
    }

    void Execute( const std::string& aSql ) const
    {
        std::string         cs = ConnectionString();
        nanodbc::connection conn( nanodbc::string( cs.begin(), cs.end() ) );
        nanodbc::execute( conn, nanodbc::string( aSql.begin(), aSql.end() ) );
    }

    std::filesystem::path m_path;
};


// Embedded quote characters in table and column names must be doubled in the generated SQL
BOOST_AUTO_TEST_CASE( QuoteCharactersInIdentifiers )
{
    SCRATCH_DATABASE db;
    db.Execute( "CREATE TABLE \"Part \"\"A\"\"\" (\"Part ID\" TEXT, \"Va\"\"lue\" TEXT)" );
    db.Execute( "INSERT INTO \"Part \"\"A\"\"\" VALUES ('P1', '10K'), ('P2', '22K')" );

    const std::string table = "Part \"A\"";
    const std::string column = "Va\"lue";

    {
        DATABASE_CONNECTION dc( db.ConnectionString(), 2 );
        BOOST_REQUIRE( dc.IsConnected() );
        BOOST_REQUIRE( dc.CacheTableInfo( table, { "Part ID", column } ) );

        std::vector<DATABASE_CONNECTION::ROW> rows;
        BOOST_REQUIRE_MESSAGE( dc.SelectAll( table, "Part ID", rows ), dc.GetLastError() );
        BOOST_CHECK_EQUAL( rows.size(), 2 );
    }

    {
        DATABASE_CONNECTION dc( db.ConnectionString(), 2 );
        BOOST_REQUIRE( dc.IsConnected() );
        BOOST_REQUIRE( dc.CacheTableInfo( table, { "Part ID", column } ) );

        // A zero-size row cache forces the direct query
        dc.SetCacheParams( 0, 0 );

        DATABASE_CONNECTION::ROW result;
        BOOST_REQUIRE_MESSAGE( dc.SelectOne( table, std::make_pair( "Part ID", "P2" ), result ), dc.GetLastError() );
        BOOST_CHECK_EQUAL( std::any_cast<std::string>( result.at( column ) ), "22K" );
    }
}


// A table name is a catalog search pattern, so underscore and percent must match only themselves
BOOST_AUTO_TEST_CASE( PatternCharactersInTableNames )
{
    SCRATCH_DATABASE db;

    // The distractors come first and match the unescaped pattern "Part_%Data"
    db.Execute( "CREATE TABLE PartAData (\"Part ID\" TEXT, Value TEXT)" );
    db.Execute( "CREATE TABLE PartABCData (\"Part ID\" TEXT, Value TEXT)" );
    db.Execute( "CREATE TABLE \"Part_%Data\" (\"Part ID\" TEXT, Value TEXT)" );
    db.Execute( "INSERT INTO PartAData VALUES ('P1', 'wrong A')" );
    db.Execute( "INSERT INTO PartABCData VALUES ('P1', 'wrong ABC')" );
    db.Execute( "INSERT INTO \"Part_%Data\" VALUES ('P1', 'right')" );

    DATABASE_CONNECTION dc( db.ConnectionString(), 2 );
    BOOST_REQUIRE( dc.IsConnected() );
    BOOST_REQUIRE( dc.CacheTableInfo( "Part_%Data", { "Part ID", "Value" } ) );

    DATABASE_CONNECTION::ROW result;
    BOOST_REQUIRE_MESSAGE( dc.SelectOne( "Part_%Data", std::make_pair( "Part ID", "P1" ), result ), dc.GetLastError() );
    BOOST_CHECK_EQUAL( std::any_cast<std::string>( result.at( "Value" ) ), "right" );
}


BOOST_AUTO_TEST_SUITE_END()
