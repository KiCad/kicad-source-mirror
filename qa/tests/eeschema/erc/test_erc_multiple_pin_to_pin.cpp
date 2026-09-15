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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <advanced_config.h>
#include <scoped_set_reset.h>
#include <erc/erc_item.h>
#include <sch_symbol.h>
#include <lib_symbol.h>
#include <set>
#include <tuple>


struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCMultiplePinToPin, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    std::vector<std::pair<wxString, int>> tests = { { "erc_multiple_pin_to_pin", 2 } };

    for( bool useEngine : { false, true } )
    {
        enabled = useEngine;

        for( const std::pair<wxString, int>& test : tests )
        {
            BOOST_TEST_CONTEXT( test.first.ToStdString() << " engine=" << useEngine )
            {
                KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

                const SCH_SHEET_PATH root = m_schematic->Hierarchy().front();
                std::set<wxString> connectors;
                size_t powerSymbols = 0;

                for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
                {
                    const auto* symbol = static_cast<SCH_SYMBOL*>( item );
                    const auto pins = symbol->GetPins( &root );
                    BOOST_REQUIRE_EQUAL( pins.size(), 1 );

                    if( symbol->IsPower() )
                        ++powerSymbols;
                    else
                    {
                        BOOST_CHECK( pins.front()->GetType() == ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
                        connectors.insert( symbol->GetRef( &root ) );
                    }
                }

                BOOST_REQUIRE_EQUAL( powerSymbols, 14 );
                const std::set<wxString> expectedConnectors{ "J1", "J2" };
                BOOST_REQUIRE( connectors == expectedConnectors );

                ERC_SETTINGS&                settings = m_schematic->ErcSettings();
                SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

                // Skip the "Modified symbol" warning
                settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
                settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

                m_schematic->ConnectionGraph()->RunERC();

                ERC_TESTER tester( m_schematic.get() );
                tester.TestMultUnitPinConflicts();
                tester.TestMultiunitFootprints();
                tester.TestNoConnectPins();
                tester.TestPinToPin();
                tester.TestSimilarLabels();

                errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

                ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

                BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                                     "Expected " << test.second << " errors in " << test.first.ToStdString()
                                                 << " but got " << errors.GetCount() << "\n"
                                                 << reportWriter.GetTextReport() );
            }
        }
    }
}


BOOST_FIXTURE_TEST_CASE( ERCPinConflictsUsePublishedNets, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    using DIAGNOSTIC = std::tuple<int, KIID, KIID, KIID_PATH>;
    using CHECK = int ( ERC_TESTER::* )();
    const std::vector<std::pair<wxString, CHECK>> cases = {
        { "issue1768/issue1768", &ERC_TESTER::TestMultUnitPinConflicts },
        { "similar_labels", &ERC_TESTER::TestSimilarLabels },
        { "same_local_global_label", &ERC_TESTER::TestSameLocalGlobalLabel }
    };

    for( const auto& [fixture, check] : cases )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            std::multiset<DIAGNOSTIC> expected;

            for( bool published : { false, true } )
            {
                enabled = published;
                KI_TEST::LoadSchematic( m_settingsManager, fixture, m_schematic );
                m_schematic->ErcSettings().m_ERCSeverities[ERCE_SAME_LOCAL_GLOBAL_LABEL] = RPT_SEVERITY_ERROR;
                m_schematic->RebuildConnectivity();

                if( published )
                    m_schematic->ConnectionGraph()->Reset();

                ERC_TESTER tester( m_schematic.get() );
                ( tester.*check )();
                SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );
                errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );
                std::multiset<DIAGNOSTIC> actual;

                for( size_t i = 0; i < errors.GetCount(); ++i )
                {
                    const auto item = std::static_pointer_cast<ERC_ITEM>( errors.GetItem( i ) );
                    actual.emplace( item->GetErrorCode(), item->GetMainItemID(), item->GetAuxItemID(),
                                    item->IsSheetSpecific() ? item->GetSpecificSheetPath().Path() : KIID_PATH() );
                }

                if( !published )
                {
                    BOOST_REQUIRE( !actual.empty() );
                    expected = actual;
                }
                else
                {
                    ERC_REPORT report( m_schematic.get(), EDA_UNITS::MM );
                    BOOST_CHECK_MESSAGE( actual == expected,
                                         "Published diagnostics: " << actual.size() << ", legacy: " << expected.size()
                                         << "\n" << report.GetTextReport() );
                }
            }
        }
    }
}


BOOST_FIXTURE_TEST_CASE( ERCPublishedNetsRespectNoConnectFlags, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool published : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Published connectivity: " << published )
        {
            enabled = published;
            KI_TEST::LoadSchematic( m_settingsManager, "NoConnectOnLine", m_schematic );
            const SCH_SHEET_PATH path = m_schematic->Hierarchy().front();
            SCH_SCREEN* screen = path.LastScreen();
            std::vector<SCH_PIN*> pins;
            std::vector<SCH_ITEM*> flags;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins( &path ) )
                    pins.push_back( pin );
            }

            for( SCH_ITEM* item : screen->Items().OfType( SCH_NO_CONNECT_T ) )
                flags.push_back( item );

            BOOST_REQUIRE_EQUAL( pins.size(), 1 );
            BOOST_REQUIRE_EQUAL( flags.size(), 1 );
            pins.front()->SetType( ELECTRICAL_PINTYPE::PT_INPUT );

            for( bool flagged : { true, false } )
            {
                if( !flagged )
                    screen->DeleteItem( flags.front() );

                m_schematic->RebuildConnectivity();

                if( published )
                    m_schematic->ConnectionGraph()->Reset();

                ERC_TESTER tester( m_schematic.get() );
                BOOST_CHECK_EQUAL( tester.TestPinToPin(), flagged ? 0 : 1 );
                SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );
                errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );
                BOOST_REQUIRE_EQUAL( errors.GetCount(), flagged ? 0 : 1 );

                if( !flagged )
                    BOOST_CHECK_EQUAL( errors.GetItem( 0 )->GetErrorCode(), ERCE_PIN_NOT_DRIVEN );
            }
        }
    }
}


BOOST_FIXTURE_TEST_CASE( ERCDuplicatePinNumbersJoinOnlyWhenJumpered, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    KI_TEST::LoadSchematic( m_settingsManager, "ground_pin_test_error", m_schematic );
    const SCH_SHEET_PATH path = m_schematic->Hierarchy().front();
    SCH_SCREEN* screen = path.LastScreen();
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        if( candidate->GetRef( &path ) == wxS( "U1" ) )
            symbol = candidate;
    }

    BOOST_REQUIRE( symbol );
    const auto pins = symbol->GetPins( &path );
    BOOST_REQUIRE_EQUAL( pins.size(), 2 );
    pins[1]->SetNumber( pins[0]->GetNumber() );

    for( bool backend : { false, true } )
    {
        for( bool jumpered : { false, true } )
        {
            BOOST_TEST_CONTEXT( "engine=" << backend << ", jumpered=" << jumpered )
            {
                enabled = backend;
                symbol->GetLibSymbolRef()->SetDuplicatePinNumbersAreJumpers( jumpered );
                m_schematic->RebuildConnectivity();
                ERC_TESTER tester( m_schematic.get() );
                BOOST_CHECK_EQUAL( tester.TestDuplicatePinNets(), jumpered ? 0 : 1 );
                SCH_SCREENS( m_schematic->Root() ).DeleteAllMarkers( MARKER_BASE::MARKER_ERC, true );
            }
        }
    }
}
