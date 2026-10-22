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

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <json_common.h>
#include <lib_symbol.h>
#include <sch_io/easyeda/sch_io_easyeda.h>

#include <wx/filename.h>

#include <fstream>


BOOST_AUTO_TEST_SUITE( EasyedaSchImport )


BOOST_AUTO_TEST_CASE( WrappedSingleSheetEnumeratesItsSymbols )
{
    const wxString source = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                            + wxS( "../pcbnew/plugins/easyeda/SCH_ESP32-PICO-D4 smart watch_2023-09-02.json" );

    std::ifstream  in( source.ToStdString() );
    nlohmann::json doc = nlohmann::json::parse( in, nullptr, false );

    BOOST_REQUIRE( !doc.is_discarded() );

    // The server wraps a lone sheet in an envelope that carries the sheet under dataStr
    nlohmann::json wrapped = { { "docType", "1" },
                               { "title", "sheet" },
                               { "dataStr", doc["schematics"][0]["dataStr"] } };

    wxFileName path( wxFileName::CreateTempFileName( wxS( "kicad_easyeda_wrapped_" ) ) );
    wxFileName jsonPath( path );
    jsonPath.SetExt( wxS( "json" ) );

    {
        std::ofstream out( jsonPath.GetFullPath().ToStdString() );
        out << wrapped.dump();
    }

    SCH_IO_EASYEDA plugin;
    wxArrayString  names;

    plugin.EnumerateSymbolLib( names, jsonPath.GetFullPath() );

    wxRemoveFile( jsonPath.GetFullPath() );
    wxRemoveFile( path.GetFullPath() );

    BOOST_CHECK( !names.IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
