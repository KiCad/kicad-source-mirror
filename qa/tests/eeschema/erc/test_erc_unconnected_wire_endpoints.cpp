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
#include <sch_marker.h>
#include <algorithm>
#include <map>
#include <tuple>
#include <set>
#include <scoped_set_reset.h>

struct ERC_REGRESSION_TEST_FIXTURE
{
    ERC_REGRESSION_TEST_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCWireEndpointsUsePublishedState, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    using DIAGNOSTIC = std::tuple<KIID, KIID_PATH, int, int, wxString>;

    for( const wxString& fixture : { wxString( "erc_wire_endpoints" ), wxString( "unconnected_bus_entry_qa" ) } )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            enabled = false;
            KI_TEST::LoadSchematic( m_settingsManager, fixture, m_schematic );

            for( auto& [code, severity] : m_schematic->ErcSettings().m_ERCSeverities )
                severity = RPT_SEVERITY_IGNORE;

            m_schematic->ErcSettings().m_ERCSeverities[ERCE_UNCONNECTED_WIRE_ENDPOINT] = RPT_SEVERITY_ERROR;
            m_schematic->ConnectionGraph()->RunERC();
            SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );
            const auto diagnostics = [&]( bool aRecordExclusions = false )
            {
                errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING | RPT_SEVERITY_EXCLUSION );
                std::multiset<DIAGNOSTIC> result;
                std::vector<std::pair<SCH_SCREEN*, SCH_MARKER*>> markers;

                for( int i = 0; i < errors.GetCount(); ++i )
                {
                    const auto item = std::static_pointer_cast<ERC_ITEM>( errors.GetItem( i ) );
                    auto* marker = static_cast<SCH_MARKER*>( item->GetParent() );

                    if( aRecordExclusions )
                        marker->SetExcluded( true, "Retained endpoint" );

                    BOOST_CHECK( marker->IsExcluded() );
                    BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained endpoint" ) );
                    BOOST_CHECK_EQUAL( item->GetErrorCode(), ERCE_UNCONNECTED_WIRE_ENDPOINT );
                    result.emplace( item->GetMainItemID(), item->GetSpecificSheetPath().PathRef(),
                                    marker->GetPosition().x, marker->GetPosition().y, item->GetErrorMessage( false ) );
                    markers.emplace_back( item->GetSpecificSheetPath().LastScreen(), marker );
                }

                if( aRecordExclusions )
                    m_schematic->RecordERCExclusions();

                for( const auto& [screen, marker] : markers )
                    screen->DeleteItem( marker );

                return result;
            };
            const auto expected = diagnostics( true );
            BOOST_REQUIRE( !expected.empty() );
            enabled = true;
            m_schematic->RebuildConnectivity();
            BOOST_CHECK_EQUAL( m_schematic->ConnectionGraph()->RunERC(), expected.size() );
            std::set<SCH_SCREEN*> screens;
            size_t markerCount = 0;

            for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
            {
                if( screens.insert( path.LastScreen() ).second )
                {
                    for( SCH_ITEM* marker : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                        ++markerCount;
                }
            }

            BOOST_CHECK_EQUAL( markerCount, expected.size() );
            m_schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK( diagnostics() == expected );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( ERCFloatingWiresUsePublishedConnectivity, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    using GROUP = std::pair<KIID_PATH, std::vector<KIID>>;

    for( const wxString& fixture : { wxString( "erc_wire_endpoints" ), wxString( "unconnected_bus_entry_qa" ) } )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            enabled = false;
            KI_TEST::LoadSchematic( m_settingsManager, fixture, m_schematic );

            for( auto& [code, severity] : m_schematic->ErcSettings().m_ERCSeverities )
                severity = RPT_SEVERITY_IGNORE;

            m_schematic->ErcSettings().m_ERCSeverities[ERCE_WIRE_DANGLING] = RPT_SEVERITY_ERROR;
            std::map<KIID, VECTOR2I> positions;

            for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items() )
                    positions.emplace( item->m_Uuid, item->GetPosition() );
            }

            SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );
            const auto diagnostics = [&]( bool aCanonical )
            {
                errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING | RPT_SEVERITY_EXCLUSION );
                std::multiset<GROUP> result;
                std::vector<std::pair<SCH_SCREEN*, SCH_MARKER*>> markers;

                for( int i = 0; i < errors.GetCount(); ++i )
                {
                    const auto item = std::static_pointer_cast<ERC_ITEM>( errors.GetItem( i ) );
                    auto* marker = static_cast<SCH_MARKER*>( item->GetParent() );
                    BOOST_CHECK_EQUAL( item->GetErrorCode(), ERCE_WIRE_DANGLING );
                    auto ids = item->GetIDs();
                    std::erase( ids, niluuid );
                    BOOST_REQUIRE( !ids.empty() );

                    if( aCanonical )
                    {
                        BOOST_CHECK( std::is_sorted( ids.begin(), ids.end() ) );
                        BOOST_CHECK( marker->GetPosition() == positions.at( ids.front() ) );
                        BOOST_CHECK( marker->IsExcluded() );
                        BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained floating wire" ) );
                    }
                    else
                    {
                        marker->SetExcluded( true, "Retained floating wire" );
                    }

                    std::sort( ids.begin(), ids.end() );
                    result.emplace( item->GetSpecificSheetPath().PathRef(), std::move( ids ) );
                    markers.emplace_back( item->GetSpecificSheetPath().LastScreen(), marker );
                }

                if( !aCanonical )
                    m_schematic->RecordERCExclusions();

                for( const auto& [screen, marker] : markers )
                    screen->DeleteItem( marker );

                return result;
            };
            m_schematic->ConnectionGraph()->RunERC();
            const auto expected = diagnostics( false );
            BOOST_REQUIRE( !expected.empty() );
            enabled = true;
            m_schematic->RebuildConnectivity();
            m_schematic->ConnectionGraph()->Reset();
            BOOST_CHECK_EQUAL( m_schematic->ConnectionGraph()->RunERC(), expected.size() );
            m_schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK( diagnostics( true ) == expected );
            m_schematic->ErcSettings().m_ERCSeverities[ERCE_WIRE_DANGLING] = RPT_SEVERITY_IGNORE;
            BOOST_CHECK_EQUAL( m_schematic->ConnectionGraph()->RunERC(), 0 );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( ERCUnconnectedWireEndpoints, ERC_REGRESSION_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    // Check for Errors when using rule area netclass directives
    std::vector<std::pair<wxString, int>> tests = { { "erc_wire_endpoints", 4 } };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadSchematic( m_settingsManager, test.first, m_schematic );

        ERC_SETTINGS&                settings = m_schematic->ErcSettings();
        SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

        // Skip the "Modified symbol" warning
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
        settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

        // Configure the rules under test
        settings.m_ERCSeverities[ERCE_UNCONNECTED_WIRE_ENDPOINT] = RPT_SEVERITY_ERROR;
        settings.m_ERCSeverities[ERCE_WIRE_DANGLING] = RPT_SEVERITY_IGNORE;

        m_schematic->ConnectionGraph()->RunERC();

        errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

        ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

        BOOST_CHECK_MESSAGE( errors.GetCount() == test.second,
                             "Expected " << test.second << " errors in " << test.first.ToStdString()
                                         << " but got " << errors.GetCount() << "\n"
                                         << reportWriter.GetTextReport() );
    }
}
