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

#include <advanced_config.h>
#include <scoped_set_reset.h>
#include <sch_marker.h>
#include <connection_graph.h>
#include <schematic.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <set>
#include <tuple>


struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCNoConnect, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    // Check for Errors related to no connect flag

    std::vector<std::pair<wxString, int>> tests = {
        { "NoConnectOnPin", 0 },
        { "NoConnectOnLine", 0 },
        { "NoConnectOnLineWithLabel", 1 },
        { "NoConnectOnLineWithGlobalLabel", 1 },
        { "NoConnectOnLineWithHierarchicalLabel", 3 },
        { "NoConnectPinsConnectedByLine", 1 },
        { "NoConnectPinsConnectedByLabel", 1 },
        { "issue24201/issue24201", 0 },
        { "issue24201_label/issue24201", 1 },
    };

    for( bool useEngine : { false, true } )
    {
        enabled = useEngine;

        for( const std::pair<wxString, int>& test : tests )
        {
            BOOST_TEST_CONTEXT( test.first.ToStdString() << " engine=" << useEngine )
            {
                KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

                ERC_SETTINGS& settings = m_schematic->ErcSettings();
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
                                     "Expected " << test.second << " errors in " <<  test.first.ToStdString()
                                                 << " but got " << errors.GetCount() << "\n"
                                                 << reportWriter.GetTextReport() );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCNoConnectFlagsUsePublishedNets )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    const std::vector<wxString> fixtures = {
        "NoConnectOnPin", "NoConnectOnLine", "NoConnectOnLineWithLabel",
        "NoConnectOnLineWithGlobalLabel", "NoConnectOnLineWithHierarchicalLabel",
        "NoConnectPinsConnectedByLine", "NoConnectPinsConnectedByLabel",
        "issue24201/issue24201", "issue24201_label/issue24201", "NoConnectOnPin"
    };

    // The repeated NoConnectOnPin entry moves its flag off the pin to exercise the unconnected-flag check
    const size_t movedFlagVariant = fixtures.size() - 1;

    for( size_t variant = 0; variant < fixtures.size(); ++variant )
    {
        const wxString& fixture = fixtures[variant];

        BOOST_TEST_CONTEXT( fixture.ToStdString() << " variant=" << variant )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, fixture, schematic );

            for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
                severity = RPT_SEVERITY_IGNORE;

            schematic->ErcSettings().m_ERCSeverities[ERCE_NOCONNECT_CONNECTED] = RPT_SEVERITY_ERROR;
            schematic->ErcSettings().m_ERCSeverities[ERCE_NOCONNECT_NOT_CONNECTED] = RPT_SEVERITY_ERROR;

            if( variant == movedFlagVariant )
            {
                for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                {
                    for( SCH_ITEM* flag : path.LastScreen()->Items().OfType( SCH_NO_CONNECT_T ) )
                    {
                        flag->Move( VECTOR2I( 10000000, 10000000 ) );
                        path.LastScreen()->Update( flag );
                    }
                }

                schematic->RebuildConnectivity();
            }

            schematic->ConnectionGraph()->RunERC();

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                    static_cast<SCH_MARKER*>( item )->SetExcluded( true, "Retained NC flag" );
            }

            schematic->RecordERCExclusions();
            using DIAGNOSTIC = std::tuple<int, KIID_PATH, std::vector<KIID>, VECTOR2I>;
            const auto collect = [&]()
            {
                std::vector<DIAGNOSTIC> result;
                std::set<SCH_SCREEN*> screens;

                for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                    screens.insert( path.LastScreen() );

                for( SCH_SCREEN* screen : screens )
                {
                    std::vector<SCH_MARKER*> markers;

                    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                    {
                        auto* marker = static_cast<SCH_MARKER*>( item );
                        const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                        BOOST_CHECK( marker->IsExcluded() );
                        BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained NC flag" ) );
                        BOOST_CHECK( error->MainItemHasSheetPath() );
                        BOOST_CHECK( !error->AuxItemHasSheetPath() );
                        result.emplace_back( error->GetErrorCode(), error->GetSpecificSheetPath().PathRef(),
                                             error->GetIDs(), marker->GetPosition() );
                        markers.push_back( marker );
                    }

                    for( SCH_MARKER* marker : markers )
                        screen->DeleteItem( marker );
                }

                std::sort( result.begin(), result.end() );
                return result;
            };
            const auto expected = collect();

            if( variant == movedFlagVariant )
            {
                BOOST_REQUIRE_EQUAL( expected.size(), 1 );
                BOOST_CHECK_EQUAL( std::get<0>( expected.front() ), ERCE_NOCONNECT_NOT_CONNECTED );
            }

            enabled = true;
            schematic->RebuildConnectivity();
            schematic->ConnectionGraph()->Reset();
            BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), expected.size() );
            schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK( collect() == expected );
            schematic->ErcSettings().m_ERCSeverities[ERCE_NOCONNECT_CONNECTED] = RPT_SEVERITY_IGNORE;
            schematic->ErcSettings().m_ERCSeverities[ERCE_NOCONNECT_NOT_CONNECTED] = RPT_SEVERITY_IGNORE;
            BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), 0 );
            BOOST_CHECK( collect().empty() );
        }
    }
}
