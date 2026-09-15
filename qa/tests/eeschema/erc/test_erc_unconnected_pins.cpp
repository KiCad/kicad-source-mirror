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
#include <connection_graph.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <locale_io.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <map>
#include <set>
#include <scoped_set_reset.h>


BOOST_AUTO_TEST_CASE( ERCUnconnectedPinsUsePublishedConnectivity )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    const std::vector<wxString> fixtures = {
        "NoConnectOnPin", "NoConnectOnLine", "NoConnectOnLineWithLabel",
        "NoConnectPinsConnectedByLine", "NoConnectPinsConnectedByLabel", "ground_pin_test_ok",
        "issue6588", "netlists/multinetclasses/multinetclasses", "issue23840/BusAndVectors"
    };

    for( const wxString& fixture : fixtures )
    {
        BOOST_TEST_CONTEXT( fixture.ToStdString() )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, fixture, schematic );

            for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
                severity = RPT_SEVERITY_IGNORE;

            schematic->ErcSettings().m_ERCSeverities[ERCE_PIN_NOT_CONNECTED] = RPT_SEVERITY_ERROR;
            schematic->ConnectionGraph()->RunERC();

            if( fixture == "issue6588" )
            {
                for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                {
                    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                        static_cast<SCH_MARKER*>( item )->SetExcluded( true, "Retained stacked-pin exclusion" );
                }

                schematic->RecordERCExclusions();
            }
            using DIAGNOSTICS = std::map<std::pair<KIID_PATH, KIID>, VECTOR2I>;
            const auto collect = [&]()
            {
                DIAGNOSTICS diagnostics;
                std::set<SCH_SCREEN*> screens;

                for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                    screens.insert( path.LastScreen() );

                for( SCH_SCREEN* screen : screens )
                {
                    std::vector<SCH_MARKER*> markers;

                    for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                    {
                        auto* marker = static_cast<SCH_MARKER*>( item );
                        markers.push_back( marker );
                        const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                        auto* pin = dynamic_cast<SCH_PIN*>( schematic->ResolveItem( error->GetMainItemID(), nullptr,
                                                                                   true ) );

                        if( !pin )
                            continue;

                        if( pin->GetParentSymbol()->IsPower() && pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_OUT )
                            continue;

                        if( fixture == "issue6588" )
                        {
                            BOOST_CHECK( marker->IsExcluded() );
                            BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained stacked-pin exclusion" ) );
                        }

                        BOOST_CHECK_EQUAL( error->GetErrorCode(), ERCE_PIN_NOT_CONNECTED );
                        BOOST_CHECK( error->MainItemHasSheetPath() );
                        BOOST_CHECK( !error->AuxItemHasSheetPath() );
                        BOOST_CHECK_EQUAL( error->GetIDs().size(), 1 );
                        const auto key = std::make_pair( error->GetSpecificSheetPath().PathRef(), pin->m_Uuid );
                        BOOST_CHECK( diagnostics.emplace( key, marker->GetPosition() ).second );
                    }

                    for( SCH_MARKER* marker : markers )
                        screen->DeleteItem( marker );
                }

                return diagnostics;
            };
            auto expected = collect();
            const size_t expectedCount = fixture == "issue6588" ? 1
                                         : fixture == "netlists/multinetclasses/multinetclasses" ? 12 : 0;
            BOOST_REQUIRE_EQUAL( expected.size(), expectedCount );

            if( fixture == "issue6588" )
            {
                // The legacy graph can select either stacked pin; the new witness must be visible
                auto key = expected.begin()->first;
                key.second = KIID( "c1301314-4db8-4c8f-a374-c89403debfd3" );
                const auto* witness = dynamic_cast<SCH_PIN*>( schematic->ResolveItem( key.second, nullptr, true ) );
                BOOST_REQUIRE( witness );
                BOOST_CHECK( witness->IsVisible() );
                BOOST_CHECK( witness->GetPosition() == expected.begin()->second );
                expected.clear();
                expected.emplace( key, witness->GetPosition() );
            }

            enabled = true;
            schematic->RebuildConnectivity();
            schematic->ConnectionGraph()->Reset();
            BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), expected.size() );
            schematic->ResolveERCExclusionsPostUpdate();
            const auto actual = collect();

            if( actual != expected )
            {
                for( const auto& [key, position] : expected )
                {
                    if( !actual.contains( key ) )
                        BOOST_TEST_MESSAGE( "Missing pin " << key.second.AsString().ToStdString()
                                            << " on " << key.first.AsString().ToStdString() );
                }

                for( const auto& [key, position] : actual )
                {
                    if( !expected.contains( key ) )
                        BOOST_TEST_MESSAGE( "Extra pin " << key.second.AsString().ToStdString()
                                            << " on " << key.first.AsString().ToStdString() );
                }
            }

            BOOST_CHECK( actual == expected );
            schematic->ErcSettings().m_ERCSeverities[ERCE_PIN_NOT_CONNECTED] = RPT_SEVERITY_IGNORE;
            BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), 0 );
            BOOST_CHECK( collect().empty() );
        }
    }
}
