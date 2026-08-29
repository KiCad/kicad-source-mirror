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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <thread>

#include <http_lib/http_lib_connection.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>
#include <lib_symbol.h>
#include <sch_io/http_lib/sch_io_http_lib.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include "http_lib_fake_server.h"


BOOST_AUTO_TEST_SUITE( HttpLibPlugin )


static const std::string RootUrl = "http://fake.test/v1/";
static const std::string SettingsFile = "test_http_lib.kicad_httplib";


static wxString httpLibSettingsPath()
{
    return wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() ) + SettingsFile;
}


static void useFastCacheLifetimes( SCH_IO_HTTP_LIB& aPlugin )
{
    aPlugin.SetSourcePatcher( []( HTTP_LIB_SOURCE& aSource )
                               {
                                   aSource.timeout_parts = 1;
                                   aSource.timeout_categories = 1;
                               } );
}


struct HTTP_PLUGIN_HARNESS
{
    HTTP_PLUGIN_HARNESS() : adapter( manager )
    {
        plugin.SetLibraryManagerAdapter( &adapter );
        server.InstallStandardLibrary( RootUrl );
        plugin.SetConnectionBuilder( [this]( const HTTP_LIB_SOURCE& aSource )
                                     {
                                         return std::make_unique<HTTP_LIB_CONNECTION>(
                                                 aSource, true, server.MakeRetriever() );
                                     } );
    }

    LIBRARY_MANAGER        manager;
    SYMBOL_LIBRARY_ADAPTER adapter;
    HTTP_LIB_FAKE_SERVER   server;
    SCH_IO_HTTP_LIB        plugin;
};


BOOST_AUTO_TEST_CASE( EnumerateSymbolLibListsAllParts )
{
    HTTP_PLUGIN_HARNESS harness;
    const wxString settingsPath = httpLibSettingsPath();

    wxArrayString names;
    harness.plugin.EnumerateSymbolLib( names, settingsPath );

    BOOST_CHECK( names.Index( wxS( "R_10K" ) ) != wxNOT_FOUND );
    BOOST_CHECK( names.Index( wxS( "R_100K" ) ) != wxNOT_FOUND );
    BOOST_CHECK( names.Index( wxS( "C_100n" ) ) != wxNOT_FOUND );

    BOOST_CHECK( harness.server.RequestCount( RootUrl + "categories.json" ) >= 1 );
    BOOST_CHECK( harness.server.RequestCount( RootUrl + "parts/category/1.json" ) >= 1 );

}


BOOST_AUTO_TEST_CASE( EnumerateSymbolLibMaterializesFields )
{
    HTTP_PLUGIN_HARNESS harness;
    const wxString settingsPath = httpLibSettingsPath();

    std::vector<LIB_SYMBOL*> symbols;
    harness.plugin.EnumerateSymbolLib( symbols, settingsPath );

    BOOST_REQUIRE_EQUAL( symbols.size(), 3u );

    const LIB_SYMBOL* r10k = nullptr;

    for( const LIB_SYMBOL* symbol : symbols )
    {
        if( symbol->GetName() == wxS( "R_10K" ) )
            r10k = symbol;
    }

    BOOST_REQUIRE( r10k );
    BOOST_CHECK_EQUAL( r10k->GetFootprintField().GetText(),
                       wxString( wxS( "Resistor_SMD:R_0603_1608Metric" ) ) );
    BOOST_CHECK_EQUAL( r10k->GetValueField().GetText(), wxString( wxS( "10k" ) ) );
    BOOST_CHECK_EQUAL( r10k->GetDescription(), wxString( wxS( "10 kOhms resistor" ) ) );

}


BOOST_AUTO_TEST_CASE( SubLibrariesExposeCategories )
{
    HTTP_PLUGIN_HARNESS harness;
    const wxString settingsPath = httpLibSettingsPath();

    // GetSubLibraryNames relies on settings established by an earlier connection.
    BOOST_REQUIRE_NO_THROW( harness.plugin.CheckLibrary( settingsPath ) );

    std::vector<wxString> names = { wxS( "stale" ) };
    harness.plugin.GetSubLibraryNames( names );

    BOOST_REQUIRE_EQUAL( names.size(), 2u );
    BOOST_CHECK_EQUAL( names[0], wxString( wxS( "Resistors" ) ) );
    BOOST_CHECK_EQUAL( names[1], wxString( wxS( "Capacitors" ) ) );

    BOOST_CHECK_EQUAL( harness.plugin.GetSubLibraryDescription( wxS( "Resistors" ) ),
                       wxString( wxS( "Resistors" ) ) );
}


BOOST_AUTO_TEST_CASE( LoadSymbolReturnsMaterializedPart )
{
    HTTP_PLUGIN_HARNESS harness;
    const wxString settingsPath = httpLibSettingsPath();

    BOOST_REQUIRE_NO_THROW( harness.plugin.CheckLibrary( settingsPath ) );

    LIB_SYMBOL* symbol = harness.plugin.LoadSymbol( settingsPath, wxS( "R_10K" ) );
    BOOST_REQUIRE( symbol );

    BOOST_CHECK_EQUAL( symbol->GetFootprintField().GetText(),
                       wxString( wxS( "Resistor_SMD:R_0603_1608Metric" ) ) );
    BOOST_CHECK_EQUAL( symbol->GetValueField().GetText(), wxString( wxS( "10k" ) ) );

    delete symbol;
}


BOOST_AUTO_TEST_CASE( CheckLibraryDoesNotEnumerateParts )
{
    HTTP_PLUGIN_HARNESS harness;
    const wxString settingsPath = httpLibSettingsPath();

    BOOST_REQUIRE_NO_THROW( harness.plugin.CheckLibrary( settingsPath ) );

    // Category sync happens as part of endpoint validation; part enumeration must not.
    BOOST_CHECK_EQUAL( harness.server.RequestCount( RootUrl + "categories.json" ), 1 );
    BOOST_CHECK_EQUAL( harness.server.RequestCount( RootUrl + "parts/category/1.json" ), 0 );
    BOOST_CHECK_EQUAL( harness.server.RequestCount( RootUrl + "parts/res-10k.json" ), 0 );
}


/// Re-querying with unchanged server data must not re-materialize the symbol cache.
BOOST_AUTO_TEST_CASE( IdenticalMetadataDoesNotRematerialize )
{
    HTTP_PLUGIN_HARNESS harness;
    useFastCacheLifetimes( harness.plugin );
    const wxString settingsPath = httpLibSettingsPath();

    wxArrayString names;
    harness.plugin.EnumerateSymbolLib( names, settingsPath );
    BOOST_REQUIRE( names.Index( wxS( "R_10K" ) ) != wxNOT_FOUND );

    const int hashBefore = harness.plugin.GetModifyHash();

    // Give the background refresh at least one full cycle with identical data.
    std::this_thread::sleep_for( std::chrono::milliseconds( 1100 ) );

    BOOST_CHECK_EQUAL( harness.plugin.GetModifyHash(), hashBefore );

    wxArrayString namesAgain;
    harness.plugin.EnumerateSymbolLib( namesAgain, settingsPath );
    BOOST_CHECK( namesAgain.Index( wxS( "R_10K" ) ) != wxNOT_FOUND );
}


BOOST_AUTO_TEST_CASE( MetadataChangeRematerializes )
{
    HTTP_PLUGIN_HARNESS harness;
    useFastCacheLifetimes( harness.plugin );
    const wxString settingsPath = httpLibSettingsPath();

    wxArrayString names;
    harness.plugin.EnumerateSymbolLib( names, settingsPath );
    BOOST_REQUIRE( names.Index( wxS( "R_10K" ) ) != wxNOT_FOUND );

    const int hashBefore = harness.plugin.GetModifyHash();

    harness.server.SetResponse( RootUrl + "parts/category/1.json",
                                { 200, R"([
  { "id": "res-20k",  "name": "R_20K",  "description": "20 kOhms resistor" },
  { "id": "res-100k", "name": "R_100K", "description": "100 kOhms resistor" }
])" } );

    harness.server.SetResponse( RootUrl + "parts/res-20k.json",
                                { 200, R"({
  "id": "res-20k",
  "name": "R_20K",
  "description": "20 kOhms resistor",
  "fields": {
    "footprint": { "value": "Resistor_SMD:R_0603_1608Metric", "visible": "False" },
    "value":     { "value": "20k" },
    "reference": { "value": "R" }
  }
})" } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 1100 ) );

    LIB_SYMBOL* newSymbol = harness.plugin.LoadSymbol( settingsPath, wxS( "R_20K" ) );

    BOOST_REQUIRE( newSymbol );
    BOOST_CHECK_EQUAL( newSymbol->GetValueField().GetText(), wxString( wxS( "20k" ) ) );
    delete newSymbol;

    BOOST_CHECK( harness.plugin.LoadSymbol( settingsPath, wxS( "R_10K" ) ) == nullptr );
    BOOST_CHECK( harness.plugin.GetModifyHash() != hashBefore );
}


BOOST_AUTO_TEST_SUITE_END()
