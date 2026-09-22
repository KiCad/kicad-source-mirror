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
 * @file test_erc_shared_pin_multiunit.cpp
 * Test ERC and netlist export for shared pins on multi-unit symbols.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/1768
 *
 * The issue describes multi-unit symbols where a shared pin (same pin number appears
 * in all units) is connected to different nets in different unit instances. The tests
 * verify that:
 * 1. ERC correctly detects when shared pins are on different nets
 * 2. The netlist exporter prefers user-assigned nets over auto-generated nets
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <algorithm>

#include <advanced_config.h>
#include <sch_marker.h>
#include <erc/erc_item.h>
#include <sch_symbol.h>
#include <connection_graph.h>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_exclusion.h>
#include <erc/erc_report.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <scoped_set_reset.h>


struct ERC_SHARED_PIN_TEST_FIXTURE
{
    ERC_SHARED_PIN_TEST_FIXTURE() = default;

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( Issue1768_SharedPinDifferentNets, ERC_SHARED_PIN_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Load the issue 1768 test schematic
    // This schematic has a 4-unit diode symbol (ESDAxx-SC5-V) where pin 2 is the
    // common anode shared across all units. In the test case, only unit 2 has
    // pin 2 connected to GND, while the other units have pin 2 unconnected
    // (which results in auto-generated net names).
    KI_TEST::LoadSchematic( m_settingsManager, "issue1768/issue1768", m_schematic );

    ERC_SETTINGS&                settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    // Ignore library symbol warnings since we're using old/rescue symbols
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Build connectivity and run ERC
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );

    // This is the key test - TestMultUnitPinConflicts should detect that the shared
    // pin 2 is connected to different nets across different unit instances
    int multiUnitErrors = tester.TestMultUnitPinConflicts();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    // We expect at least one ERCE_DIFFERENT_UNIT_NET error because pin 2
    // is connected to GND in one unit but has auto-generated nets in others
    BOOST_CHECK_MESSAGE( multiUnitErrors > 0,
                         "Expected ERC to detect shared pin on different nets.\n"
                         << reportWriter.GetTextReport() );

    // Verify the specific error type is present
    bool foundDifferentUnitNetError = false;

    for( int i = 0; i < errors.GetCount(); ++i )
    {
        std::shared_ptr<RC_ITEM> item = errors.GetItem( i );

        if( item && item->GetErrorCode() == ERCE_DIFFERENT_UNIT_NET )
        {
            foundDifferentUnitNetError = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundDifferentUnitNetError,
                         "Expected ERCE_DIFFERENT_UNIT_NET error for shared pin 2.\n"
                         << reportWriter.GetTextReport() );
}


BOOST_AUTO_TEST_CASE( Issue1768_NetlistPreferUserNet )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    struct EXPORTER : NETLIST_EXPORTER_KICAD
    {
        using NETLIST_EXPORTER_KICAD::NETLIST_EXPORTER_KICAD;
        using NETLIST_EXPORTER_BASE::CONNECTIVITY_SCOPE;
        using NETLIST_EXPORTER_BASE::CreatePinList;
        using NETLIST_EXPORTER_BASE::eraseDuplicatePins;
        using NETLIST_EXPORTER_BASE::findAllUnitsOfSymbol;
    };

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine=" << backend )
        {
            enabled = backend;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "issue1768/issue1768", schematic );
            schematic->RebuildConnectivity();
            const SCH_SHEET_PATH path = schematic->Hierarchy().front();
            std::vector<SCH_SYMBOL*> units;

            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &path ) != "VD1" )
                    continue;

                units.push_back( symbol );
            }

            BOOST_REQUIRE_EQUAL( units.size(), 4u );
            EXPORTER exporter( schematic.get(), nullptr );
            EXPORTER::CONNECTIVITY_SCOPE connectivity( exporter );
            size_t sharedPins = 0;

            for( const PIN_INFO& pin : exporter.CreatePinList( units.front(), path ) )
            {
                if( pin.num == "2" )
                {
                    ++sharedPins;
                    BOOST_CHECK_EQUAL( pin.netName, wxString( "GND" ) );
                }
            }

            BOOST_CHECK_EQUAL( sharedPins, 1u );
            std::vector<PIN_INFO> nativePins;
            exporter.findAllUnitsOfSymbol( units.front(), path, nativePins );
            std::erase_if( nativePins, []( const PIN_INFO& pin ) { return pin.num != "2"; } );
            BOOST_REQUIRE_EQUAL( nativePins.size(), 4u );

            for( bool userFirst : { false, true } )
            {
                auto pins = nativePins;
                std::stable_partition( pins.begin(), pins.end(), [&]( const PIN_INFO& pin )
                {
                    return ( pin.netName == "GND" ) == userFirst;
                } );
                BOOST_REQUIRE_EQUAL( pins.front().netName == "GND", userFirst );
                exporter.eraseDuplicatePins( pins );
                std::erase_if( pins, []( const PIN_INFO& pin ) { return pin.num.empty(); } );
                BOOST_REQUIRE_EQUAL( pins.size(), 1u );
                BOOST_CHECK_EQUAL( pins.front().netName, wxString( "GND" ) );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCMultiUnitPinConflictsMatchLegacy )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue1768/issue1768", schematic );
    schematic->RebuildConnectivity();
    ERC_TESTER tester( schematic.get() );
    const auto run = [&]()
    {
        const int count = tester.TestMultUnitPinConflicts();
        std::vector<std::string> markers;
        SCH_SCREENS screens( schematic->Root() );

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        {
            for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );
                BOOST_CHECK_EQUAL( marker->GetRCItem()->GetErrorCode(), ERCE_DIFFERENT_UNIT_NET );
                BOOST_CHECK_EQUAL( marker->GetRCItem()->GetIDs().size(), 2 );
                const auto ercItem = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                BOOST_CHECK( ercItem->GetSpecificSheetPath().LastScreen() == screen );
                markers.push_back( ERC_EXCLUSION::FromMarker( *marker ).GetSortKey()
                                   + marker->GetRCItem()->GetErrorMessage( true ).ToStdString() );
            }
        }

        BOOST_CHECK_EQUAL( count, markers.size() );
        screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );
        std::sort( markers.begin(), markers.end() );
        return markers;
    };
    const auto legacy = run();
    BOOST_REQUIRE( !legacy.empty() );
    enabled = true;
    schematic->RebuildConnectivity();
    schematic->ConnectionGraph()->Reset();
    const auto captured = run();
    BOOST_TEST( captured == legacy, boost::test_tools::per_element() );
}
