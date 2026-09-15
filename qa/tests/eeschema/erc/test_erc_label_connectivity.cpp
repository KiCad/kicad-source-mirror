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
#include <schematic.h>
#include <settings/settings_manager.h>
#include <map>
#include <set>
#include <tuple>
#include <scoped_set_reset.h>


BOOST_AUTO_TEST_CASE( ERCLabelPoliciesUseCapturedConnectivity )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );
    const std::vector<std::tuple<wxString, int, size_t>> fixtures = {
        { "erc_directive_label_not_connected", ERCE_LABEL_NOT_CONNECTED, 1 },
        { "issue22854/test", ERCE_LABEL_NOT_CONNECTED, 0 },
        { "issue13212", ERCE_SINGLE_GLOBAL_LABEL, 3 },
        { "erc_label_test", ERCE_SINGLE_GLOBAL_LABEL, 4 },
        { "issue23840/BusAndVectors", ERCE_SINGLE_GLOBAL_LABEL, 2 }
    };

    for( const auto& [fixture, code, expectedCount] : fixtures )
    {
        BOOST_TEST_CONTEXT( fixture.ToStdString() )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, fixture, schematic );

            for( auto& [errorCode, severity] : schematic->ErcSettings().m_ERCSeverities )
                severity = errorCode == code ? RPT_SEVERITY_ERROR : RPT_SEVERITY_IGNORE;

            using DIAGNOSTICS = std::map<std::pair<KIID_PATH, KIID>, VECTOR2I>;
            const auto collect = [&]()
            {
                DIAGNOSTICS result;
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
                        const SCH_ITEM* label = schematic->ResolveItem( error->GetMainItemID(), nullptr, true );
                        BOOST_REQUIRE( label );

                        if( code == ERCE_LABEL_NOT_CONNECTED && label->Type() != SCH_DIRECTIVE_LABEL_T )
                            continue;

                        BOOST_CHECK( marker->IsExcluded() );
                        BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained label policy" ) );
                        BOOST_CHECK_EQUAL( error->GetErrorCode(), code );
                        BOOST_CHECK_EQUAL( error->MainItemHasSheetPath(), code == ERCE_SINGLE_GLOBAL_LABEL );
                        BOOST_CHECK( !error->AuxItemHasSheetPath() );
                        BOOST_CHECK( error->GetAuxItemID() == niluuid );
                        BOOST_CHECK_EQUAL( error->GetIDs().size(), 1 );
                        const auto key = std::make_pair( error->GetSpecificSheetPath().PathRef(), label->m_Uuid );
                        BOOST_CHECK( result.emplace( key, marker->GetPosition() ).second );
                    }

                    for( SCH_MARKER* marker : markers )
                        screen->DeleteItem( marker );
                }

                return result;
            };
            schematic->ConnectionGraph()->RunERC();

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_MARKER_T ) )
                    static_cast<SCH_MARKER*>( item )->SetExcluded( true, "Retained label policy" );
            }

            schematic->RecordERCExclusions();
            const auto expected = collect();
            BOOST_REQUIRE_EQUAL( expected.size(), expectedCount );

            enabled = true;
            schematic->RebuildConnectivity();
            schematic->ConnectionGraph()->Reset();
            BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), expected.size() );
            schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK( collect() == expected );
            BOOST_CHECK_EQUAL( schematic->ConnectionGraph()->RunERC(), expected.size() );
            schematic->ResolveERCExclusionsPostUpdate();
            BOOST_CHECK( collect() == expected );
            schematic->ErcSettings().m_ERCSeverities[code] = RPT_SEVERITY_IGNORE;
            BOOST_CHECK_EQUAL( ERC_TESTER::TestConnectivity( *schematic ), 0 );
            BOOST_CHECK( collect().empty() );
        }
    }
}
