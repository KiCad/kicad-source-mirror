/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

/**
 * Issue #24477. KiCad crashed when network connectivity was lost while using an HTTP library.
 *
 * Opening a schematic that references an HTTP symbol library while the network is
 * unreachable previously crashed.  SCH_IO_HTTP_LIB::GetSubLibraryNames() and
 * GetSubLibraryDescription() dereferenced the connection pointer (m_conn) without first
 * establishing or null-checking it.  When the connection could not be created the pointer
 * was null and the access faulted (SIGSEGV) while the library tree was built.
 *
 * The query entry points now degrade to an empty result instead of crashing, so the
 * schematic still opens.
 */

#include <boost/test/unit_test.hpp>

#include <cstring>

#include <wx/filename.h>
#include <wx/wfstream.h>

#include <http_lib/http_lib_connection.h>
#include <sch_io/http_lib/sch_io_http_lib.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>
#include <lib_symbol.h>
#include <ki_exception.h>


BOOST_AUTO_TEST_SUITE( Issue24477HttpNetworkLoss )


namespace
{
/// Concrete adapter so the plugin can be primed through its real entry points.  The base
/// constructor is protected, so it is exposed through an inheriting using-declaration.
class TEST_ADAPTER : public SYMBOL_LIBRARY_ADAPTER
{
public:
    using SYMBOL_LIBRARY_ADAPTER::SYMBOL_LIBRARY_ADAPTER;
};
}


/// Write a .kicad_httplib config pointing at an unreachable endpoint.
///
/// Port 1 is reserved and refuses connections immediately on localhost, so the curl
/// connect attempt fails fast rather than waiting on the connect timeout.  This emulates
/// "no network connectivity" without an actual network round-trip.
static wxFileName writeUnreachableHttpLib()
{
    wxFileName settings( wxFileName::CreateTempFileName( "kicad_qa_issue24477_" ) );
    wxRemoveFile( settings.GetFullPath() );
    settings.SetExt( "kicad_httplib" );

    wxFFileOutputStream out( settings.GetFullPath() );
    BOOST_REQUIRE( out.IsOk() );

    const char* json =
            "{\n"
            "    \"meta\": { \"version\": 1.0 },\n"
            "    \"name\": \"Unreachable HTTP Library\",\n"
            "    \"source\": {\n"
            "        \"type\": \"REST_API\",\n"
            "        \"api_version\": \"v1\",\n"
            "        \"root_url\": \"http://localhost:1/\",\n"
            "        \"token\": \"\"\n"
            "    }\n"
            "}\n";

    out.WriteAll( json, std::strlen( json ) );
    out.Close();

    return settings;
}


/// The sub-library queries used while building the library tree must not crash when the
/// HTTP endpoint is unreachable.  Before the fix these dereferenced a null connection.
BOOST_AUTO_TEST_CASE( SubLibraryQueriesSurviveNetworkLoss )
{
    wxFileName settings = writeUnreachableHttpLib();

    SCH_IO_HTTP_LIB    plugin;
    LIBRARY_MANAGER    manager;
    TEST_ADAPTER       adapter( manager );

    plugin.SetLibraryManagerAdapter( &adapter );

    // Enumerating the configured library loads its settings and then fails to connect, which
    // reproduces "HTTP library configured, network down": settings present, connection null.
    std::vector<LIB_SYMBOL*> symbols;
    BOOST_CHECK_THROW( plugin.EnumerateSymbolLib( symbols, settings.GetFullPath(), nullptr ), IO_ERROR );

    // GetSubLibraryNames previously dereferenced the null connection while building the tree.
    // It must now fail gracefully and leave the result empty.
    std::vector<wxString> names = { wxS( "stale" ) };
    BOOST_REQUIRE_NO_THROW( plugin.GetSubLibraryNames( names ) );
    BOOST_CHECK( names.empty() );

    // GetSubLibraryDescription dereferenced the connection with no connection guard at all.
    // It must now return empty rather than crash.
    wxString desc;
    BOOST_REQUIRE_NO_THROW( desc = plugin.GetSubLibraryDescription( wxS( "anything" ) ) );
    BOOST_CHECK( desc.IsEmpty() );

    wxRemoveFile( settings.GetFullPath() );
}


/// The connection object itself must report an invalid endpoint (and not crash) when it
/// cannot reach the server.
BOOST_AUTO_TEST_CASE( ConnectionReportsInvalidEndpointOnNetworkLoss )
{
    HTTP_LIB_SOURCE source;
    source.root_url = "http://localhost:1/v1/";
    source.token = "";
    source.api_version = "v1";

    HTTP_LIB_CONNECTION conn( source, true );

    BOOST_CHECK( !conn.IsValidEndpoint() );
    BOOST_CHECK( !conn.GetLastError().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
