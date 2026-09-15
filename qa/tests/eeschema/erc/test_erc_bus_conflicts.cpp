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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <advanced_config.h>
#include <connection_graph.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <locale_io.h>
#include <schematic.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <settings/settings_manager.h>
#include <set>
#include <tuple>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_CASE( ERCBusNetConflictsUsePublishedInstances )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( int variant = 0; variant < 6; ++variant )
    {
        BOOST_TEST_CONTEXT( "variant=" << variant )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "issue23840/BusAndVectors", schematic );
            SCH_SCREEN* screen = nullptr;
            std::set<KIID_PATH> paths;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                if( path.LastScreen()->GetFileName().EndsWith( "LEDs.kicad_sch" ) )
                {
                    BOOST_REQUIRE( !screen || screen == path.LastScreen() );
                    screen = path.LastScreen();
                    paths.insert( path.PathRef() );
                }
            }

            BOOST_REQUIRE( screen );
            BOOST_REQUIRE_EQUAL( paths.size(), 2 );
            SCH_LABEL* labelSource = nullptr;
            SCH_LINE* wireSource = nullptr;
            BOX2I bounds;

            for( SCH_ITEM* item : screen->Items() )
            {
                bounds.Merge( item->GetBoundingBox() );

                if( item->Type() == SCH_LABEL_T && ( !labelSource || item->m_Uuid < labelSource->m_Uuid ) )
                    labelSource = static_cast<SCH_LABEL*>( item );

                if( item->Type() == SCH_LINE_T && item->GetLayer() == LAYER_WIRE )
                {
                    auto* line = static_cast<SCH_LINE*>( item );

                    if( line->GetStartPoint() != line->GetEndPoint()
                        && ( !wireSource || item->m_Uuid < wireSource->m_Uuid ) )
                    {
                        wireSource = line;
                    }
                }
            }

            BOOST_REQUIRE( labelSource );
            BOOST_REQUIRE( wireSource );
            const VECTOR2I anchor = bounds.GetEnd() + VECTOR2I( 100000, 100000 );
            auto* label = static_cast<SCH_LABEL*>( labelSource->Duplicate( false ) );
            label->SetPosition( anchor );
            label->SetText( variant == 1 || variant == 4 ? "ERC_BUS[0..1]" : "ERC_SIGNAL" );
            screen->Append( label );
            SCH_ITEM* other = nullptr;

            if( variant == 0 )
            {
                auto* bus = static_cast<SCH_LABEL*>( labelSource->Duplicate( false ) );
                bus->SetText( "ERC_BUS[0..1]" );
                bus->SetPosition( anchor );
                other = bus;
            }
            else
            {
                auto* line = static_cast<SCH_LINE*>( wireSource->Duplicate( false ) );
                line->Move( anchor - ( line->GetStartPoint() + line->GetEndPoint() ) / 2 );
                line->SetLayer( variant == 2 || variant >= 4 ? LAYER_BUS : LAYER_WIRE );
                other = line;
            }

            screen->Append( other );

            if( variant == 5 )
            {
                auto* extraBus = static_cast<SCH_LABEL*>( labelSource->Duplicate( false ) );
                extraBus->SetText( "ERC_BUS[0..1]" );
                extraBus->SetPosition( anchor );
                screen->Append( extraBus );
            }

            for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
                severity = RPT_SEVERITY_IGNORE;

            schematic->ErcSettings().m_ERCSeverities[ERCE_BUS_TO_NET_CONFLICT] = RPT_SEVERITY_ERROR;
            using DIAGNOSTIC = std::tuple<KIID_PATH, KIID, KIID, int, int>;
            std::set<KIID_PATH> excludedPaths;
            const auto diagnostics = [&]( bool aRecordExclusions = false )
            {
                std::multiset<DIAGNOSTIC> result;
                std::vector<SCH_MARKER*> markers;

                for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                    if( error->GetMainItemID() == label->m_Uuid || error->GetMainItemID() == other->m_Uuid )
                    {
                        BOOST_CHECK_EQUAL( error->GetErrorCode(), ERCE_BUS_TO_NET_CONFLICT );
                        BOOST_CHECK( !error->MainItemHasSheetPath() );

                        if( aRecordExclusions )
                        {
                            marker->SetExcluded( true, "Retained bus/net conflict" );
                            excludedPaths.insert( error->GetSpecificSheetPath().PathRef() );
                        }

                        const bool excluded = excludedPaths.contains( error->GetSpecificSheetPath().PathRef() );
                        BOOST_CHECK_EQUAL( marker->IsExcluded(), excluded );

                        if( excluded )
                            BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Retained bus/net conflict" ) );

                        result.emplace( error->GetSpecificSheetPath().PathRef(), error->GetMainItemID(),
                                        error->GetAuxItemID(), marker->GetPosition().x, marker->GetPosition().y );
                    }

                    markers.push_back( marker );
                }

                if( aRecordExclusions )
                    schematic->RecordERCExclusions();

                for( SCH_MARKER* marker : markers )
                    screen->DeleteItem( marker );

                return result;
            };
            schematic->RebuildConnectivity();
            schematic->ConnectionGraph()->RunERC();
            const auto expected = diagnostics( true );

            if( variant < 3 || variant == 5 )
                BOOST_REQUIRE_EQUAL( expected.size(), 1 );
            else
                BOOST_REQUIRE( expected.empty() );

            for( bool backend : { false, true } )
            {
                enabled = backend;
                schematic->RebuildConnectivity();

                for( bool clearLegacy : { false, true } )
                {
                    // Only the engine can report without the legacy graph
                    if( clearLegacy && !backend )
                        continue;

                    if( clearLegacy )
                        schematic->ConnectionGraph()->Reset();

                    BOOST_TEST_CONTEXT( "backend=" << backend << "; clearLegacy=" << clearLegacy )
                    {
                        schematic->ConnectionGraph()->RunERC();
                        schematic->ResolveERCExclusionsPostUpdate();
                        BOOST_CHECK( diagnostics() == expected );
                    }
                }
            }
        }
    }
}
