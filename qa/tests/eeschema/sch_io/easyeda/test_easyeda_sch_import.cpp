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

#include <connection_graph.h>
#include <json_common.h>
#include <lib_symbol.h>
#include <sch_connection.h>
#include <sch_io/easyeda/sch_io_easyeda.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <wx/filename.h>

#include <fstream>
#include <map>
#include <vector>


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


BOOST_AUTO_TEST_CASE( OffsetNetFlagsConnectAtTheirStubEnd )
{
    SETTINGS_MANAGER settings;
    settings.LoadProject( "" );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &settings.Prj() );

    SCH_IO_EASYEDA plugin;
    wxString       path = KI_TEST::GetEeschemaTestDataDir() + "io/easyeda/easy_sdr_coax_power_supply.json";

    BOOST_REQUIRE( plugin.LoadSchematicFile( path, &schematic ) );

    SCH_SHEET_LIST sheets = schematic.BuildSheetListSortedByPageNumbers();
    schematic.ConnectionGraph()->Recalculate( sheets, true );

    std::map<wxString, wxString> pinNets;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                if( SCH_CONNECTION* conn = pin->Connection( &sheet ) )
                    pinNets[symbol->GetRef( &sheet ) + wxS( "." ) + pin->GetNumber()] = conn->GetNetName();
            }
        }
    }

    // Each net is joined only through circle flags whose origin sits 10 mils off the wire
    const std::map<wxString, std::vector<wxString>> expected = {
        { wxS( "+5V" ), { wxS( "F1.2" ), wxS( "SW1.2" ) } },
        { wxS( "+3.3V" ), { wxS( "C3.2" ), wxS( "C4.2" ), wxS( "R2.2" ), wxS( "U1.5" ), wxS( "U2.3" ) } },
        { wxS( "POWER FILTER" ), { wxS( "C1.1" ), wxS( "C2.2" ), wxS( "L1.3" ), wxS( "U1.1" ), wxS( "U1.3" ) } },
    };

    for( const auto& [netName, pins] : expected )
    {
        for( const wxString& pin : pins )
        {
            BOOST_TEST_CONTEXT( pin )
            {
                BOOST_REQUIRE( pinNets.count( pin ) );
                BOOST_CHECK_EQUAL( pinNets[pin], netName );
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
