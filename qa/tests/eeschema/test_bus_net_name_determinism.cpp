/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_bus_net_name_determinism.cpp
 * Test for issue 18606: net name of shorted nets should be deterministic
 * when part of a higher-level bus definition.
 *
 * When multiple nets from a bus (e.g., A0, A1, A2, A3 from bus A[0..3]) are
 * shorted together, the resulting combined net name should be deterministically
 * chosen (alphabetically first: A0).
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <sch_io/orcad/sch_io_orcad.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <richio.h>
#include <wx/filename.h>
#include <filesystem>
#include <cstdlib>

struct BUS_NET_NAME_DETERMINISM_FIXTURE
{
    BUS_NET_NAME_DETERMINISM_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test that when bus member nets (A0, A1, A2, A3) are shorted together,
 * the resulting net name is deterministic (should be "A0" - alphabetically first).
 *
 * The test schematic has:
 * - Parent sheet with bus A[0..3] connected to a hierarchical sheet
 * - Child sheet with hierarchical label A[0..3], bus entries to A0, A1, A2, A3
 * - All four nets (A0, A1, A2, A3) are shorted together via junctions
 * - A resistor R201 is connected to the shorted nets
 */
BOOST_FIXTURE_TEST_CASE( ShortedBusNetsHaveDeterministicName, BUS_NET_NAME_DETERMINISM_FIXTURE )
{
    LOCALE_IO dummy;

    // Load the test schematic multiple times to verify determinism
    for( int iteration = 0; iteration < 3; ++iteration )
    {
        KI_TEST::LoadSchematic( m_settingsManager, "issue18606/issue18606", m_schematic );

        SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

        // Find the resistor R201 in the child sheet and check its pin's net name
        wxString foundNetName;
        bool     foundResistor = false;

        for( const SCH_SHEET_PATH& path : sheets )
        {
            SCH_SCREEN* screen = path.LastScreen();

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &path ) == "R201" )
                {
                    foundResistor = true;

                    for( SCH_PIN* pin : symbol->GetPins( &path ) )
                    {
                        SCH_CONNECTION* conn = pin->Connection( &path );

                        if( conn )
                        {
                            foundNetName = conn->Name();
                            break;
                        }
                    }

                    break;
                }
            }

            if( foundResistor )
                break;
        }

        BOOST_CHECK_MESSAGE( foundResistor, "R201 should be found in the schematic" );

        // The net name should be deterministic - alphabetically "A0" should win
        // when A0, A1, A2, A3 are shorted together.
        // The path is "/" (not "/test/") because the net name is inherited from the
        // parent sheet's bus A[0..3] during hierarchical propagation.
        BOOST_CHECK_MESSAGE( foundNetName == "/A0",
                             "Net name should be '/A0' (alphabetically first bus member), "
                             "but got '" << foundNetName.ToStdString() << "' on iteration "
                             << iteration );
    }
}


BOOST_FIXTURE_TEST_CASE( WeakPinNetNameSuffixesSurviveReload, BUS_NET_NAME_DETERMINISM_FIXTURE )
{
    const char* corpus = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpus || !*corpus )
        return;

    for( const char* relativePath : { "PADS/adi-eval/DC1366B/DC1366B-2.DSN",
                                     "OrCAD/_zulip-dm/S-593487-REV-B.DSN" } )
    {
        std::filesystem::path source = std::filesystem::path( corpus ) / relativePath;
        bool equivalentDrivers = source.filename() == "S-593487-REV-B.DSN";

        if( !std::filesystem::exists( source ) )
        {
            BOOST_TEST_MESSAGE( source.filename().string() << " not present; skipping weak-driver reload check." );
            continue;
        }

        LOCALE_IO locale;
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_settingsManager.Prj() );
        SCH_IO_ORCAD importer;
        importer.LoadSchematicFile( source.string(), m_schematic.get() );

        auto names = [&]()
        {
            SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
            m_schematic->ConnectionGraph()->Recalculate( sheets, true );
            std::map<std::pair<wxString, wxString>, wxString> terminals;

            for( const SCH_SHEET_PATH& sheet : sheets )
            {
                for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                    wxString reference = symbol->GetRef( &sheet, false );

                    if( reference != wxS( "J4" ) && reference != wxS( "T2" )
                        && reference != wxS( "T3" ) && reference != wxS( "T4" ) && reference != wxS( "U9" ) )
                    {
                        continue;
                    }

                    for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                    {
                        SCH_CONNECTION* connection = pin->Connection( &sheet );

                        if( connection )
                            terminals[{ reference, pin->GetNumber() }] = connection->Name();
                    }
                }
            }

            std::map<wxString, wxString> result;

            if( equivalentDrivers )
            {
                for( const wxString& number : { wxString( "3" ), wxString( "5" ), wxString( "10" ), wxString( "12" ) } )
                {
                    auto terminal = terminals.find( { wxS( "U9" ), number } );
                    BOOST_REQUIRE( terminal != terminals.end() );
                    BOOST_CHECK( terminal->second.StartsWith( wxS( "Net-(U9-IN+)" ) ) );
                    result[number] = terminal->second;
                }

                BOOST_CHECK_EQUAL( result.at( wxS( "3" ) ), result.at( wxS( "10" ) ) );
                BOOST_CHECK_NE( result.at( wxS( "3" ) ), result.at( wxS( "5" ) ) );
                BOOST_CHECK_NE( result.at( wxS( "3" ) ), result.at( wxS( "12" ) ) );
                BOOST_CHECK_NE( result.at( wxS( "5" ) ), result.at( wxS( "12" ) ) );
                return result;
            }

            for( const auto& [number, transformer] :
                 { std::pair{ wxString( "9" ), wxString( "T2" ) },
                   std::pair{ wxString( "17" ), wxString( "T3" ) },
                   std::pair{ wxString( "25" ), wxString( "T4" ) } } )
            {
                auto connector = terminals.find( { wxS( "J4" ), number } );
                auto peer = terminals.find( { transformer, wxS( "1" ) } );
                BOOST_REQUIRE( connector != terminals.end() );
                BOOST_REQUIRE( peer != terminals.end() );
                BOOST_CHECK_EQUAL( connector->second, peer->second );
                BOOST_CHECK( connector->second.StartsWith( wxS( "Net-(J4-1)" ) ) );
                result[number] = connector->second;
            }

            std::set<wxString> distinct;

            for( const auto& [number, name] : result )
                distinct.insert( name );

            BOOST_CHECK_EQUAL( distinct.size(), 3u );
            return result;
        };

        auto before = names();
        SCH_IO_KICAD_SEXPR io;
        std::vector<wxString> files;
        std::vector<std::pair<KIID, wxString>> identities;

        for( SCH_SHEET* sheet : m_schematic->GetTopLevelSheets() )
        {
            wxString file = wxFileName::CreateTempFileName( wxS( "weak_net_reload_" ) );
            io.SaveSchematicFile( file, sheet, m_schematic.get() );
            files.push_back( file );
            identities.emplace_back( sheet->m_Uuid, sheet->GetName() );
        }

        m_schematic->Reset();
        std::vector<SCH_SHEET*> reloaded;

        for( size_t i = 0; i < files.size(); ++i )
        {
            SCH_SHEET* sheet = io.LoadSchematicFile( files[i], m_schematic.get() );
            BOOST_REQUIRE( sheet );
            const_cast<KIID&>( sheet->m_Uuid ) = identities[i].first;
            sheet->SetName( identities[i].second );
            reloaded.push_back( sheet );
        }

        m_schematic->SetTopLevelSheets( reloaded );
        m_schematic->RefreshHierarchy();

        for( const SCH_SHEET_PATH& sheet : m_schematic->BuildSheetListSortedByPageNumbers() )
            sheet.LastScreen()->UpdateLocalLibSymbolLinks();

        auto after = names();

        for( const auto& [number, name] : before )
            BOOST_CHECK_EQUAL( after.at( number ), name );

        for( const wxString& file : files )
            wxRemoveFile( file );
    }
}
