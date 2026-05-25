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
 * Test for issue #24297: Migrate http library fails.
 *
 * SCH_IO_MGR::ConvertLibrary previously accepted live backends (HTTP, DATABASE)
 * and silently produced an empty .kicad_sym file, replacing the original table
 * entry and destroying the user's library reference.  The converter must now
 * reject these backends so the caller can surface an error instead.
 */

#include <boost/test/unit_test.hpp>

#include <cstring>

#include <wx/filename.h>
#include <wx/wfstream.h>

#include <sch_io/sch_io_mgr.h>


BOOST_AUTO_TEST_SUITE( Issue24297HttpMigrate )


/// A .kicad_httplib config (HTTP source) is not migratable to a .kicad_sym snapshot.
///
/// Before the fix, ConvertLibrary would dispatch to SCH_IO_HTTP_LIB::EnumerateSymbolLib
/// which silently returned no symbols when the library manager adapter was unset (as it
/// always is from this code path) and then wrote an empty .kicad_sym, allowing the caller
/// to clobber the user's table entry.  The guard in ConvertLibrary makes this fast-fail.
BOOST_AUTO_TEST_CASE( HttpLibraryRejectedByConvertLibrary )
{
    wxFileName settings( wxFileName::CreateTempFileName( "kicad_qa_issue24297_" ) );
    wxRemoveFile( settings.GetFullPath() );
    settings.SetExt( "kicad_httplib" );

    {
        wxFFileOutputStream out( settings.GetFullPath() );
        BOOST_REQUIRE( out.IsOk() );
        const char* json =
                "{\n"
                "    \"meta\": { \"version\": 1.0 },\n"
                "    \"name\": \"Test HTTP Library\",\n"
                "    \"source\": {\n"
                "        \"type\": \"REST_API\",\n"
                "        \"api_version\": \"v1\",\n"
                "        \"root_url\": \"http://localhost:1/\",\n"
                "        \"token\": \"\"\n"
                "    }\n"
                "}\n";
        out.WriteAll( json, std::strlen( json ) );
    }

    BOOST_REQUIRE_EQUAL( SCH_IO_MGR::GuessPluginTypeFromLibPath( settings.GetFullPath() ),
                         SCH_IO_MGR::SCH_HTTP );

    wxFileName outLib( settings );
    outLib.SetExt( "kicad_sym" );
    wxRemoveFile( outLib.GetFullPath() );

    // Previously this returned true and wrote an empty .kicad_sym, clobbering the table entry.
    BOOST_CHECK( !SCH_IO_MGR::ConvertLibrary( nullptr, settings.GetFullPath(),
                                              outLib.GetFullPath() ) );

    // And must not have produced an empty replacement file.
    BOOST_CHECK( !outLib.Exists() );

    wxRemoveFile( settings.GetFullPath() );
    wxRemoveFile( outLib.GetFullPath() );
}


/// Database libraries are also non-file backends and must be rejected by ConvertLibrary.
BOOST_AUTO_TEST_CASE( DatabaseLibraryRejectedByConvertLibrary )
{
    wxFileName settings( wxFileName::CreateTempFileName( "kicad_qa_issue24297_db_" ) );
    wxRemoveFile( settings.GetFullPath() );
    settings.SetExt( "kicad_dbl" );

    {
        wxFFileOutputStream out( settings.GetFullPath() );
        BOOST_REQUIRE( out.IsOk() );
        const char* json =
                "{\n"
                "    \"meta\": { \"version\": 0 },\n"
                "    \"name\": \"Test DB\",\n"
                "    \"source\": { \"type\": \"odbc\", \"dsn\": \"\", \"username\": \"\","
                " \"password\": \"\", \"connection_string\": \"\" },\n"
                "    \"libraries\": []\n"
                "}\n";
        out.WriteAll( json, std::strlen( json ) );
    }

    BOOST_REQUIRE_EQUAL( SCH_IO_MGR::GuessPluginTypeFromLibPath( settings.GetFullPath() ),
                         SCH_IO_MGR::SCH_DATABASE );

    wxFileName outLib( settings );
    outLib.SetExt( "kicad_sym" );
    wxRemoveFile( outLib.GetFullPath() );

    BOOST_CHECK( !SCH_IO_MGR::ConvertLibrary( nullptr, settings.GetFullPath(),
                                              outLib.GetFullPath() ) );
    BOOST_CHECK( !outLib.Exists() );

    wxRemoveFile( settings.GetFullPath() );
    wxRemoveFile( outLib.GetFullPath() );
}


BOOST_AUTO_TEST_SUITE_END()
