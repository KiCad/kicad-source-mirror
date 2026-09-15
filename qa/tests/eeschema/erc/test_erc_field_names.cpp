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

#include <array>
#include <set>
#include <advanced_config.h>
#include <connectivity/conn_facade.h>
#include <erc/erc.h>
#include <locale_io.h>
#include <sch_commit.h>
#include <sch_field.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_CASE( ERCFieldNamesRetainSymbolAndSheetSnapshots )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine=" << backend )
        {
            enabled = backend;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "legacy_hierarchy/legacy_hierarchy", schematic );
            std::vector<SCH_SHEET_PATH> paths;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                if( path.LastScreen()->GetFileName().EndsWith( "ampli_ht.kicad_sch" ) )
                    paths.push_back( path );
            }

            BOOST_REQUIRE_EQUAL( paths.size(), 2 );
            SCH_SCREEN* screen = paths[0].LastScreen();
            BOOST_REQUIRE( screen == paths[1].LastScreen() );
            SCH_SYMBOL* symbol = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                symbol = static_cast<SCH_SYMBOL*>( item );
                break;
            }

            BOOST_REQUIRE( symbol );
            SCH_SHEET* sheet = paths[0].Last();
            BOOST_REQUIRE( sheet );
            const wxString badName( " Snapshot Field " );
            symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, badName ) );
            sheet->AddField( SCH_FIELD( sheet, FIELD_T::USER, badName ) );
            SCH_FIELD* symbolField = symbol->GetField( badName );
            SCH_FIELD* sheetField = sheet->GetField( badName );
            BOOST_REQUIRE( symbolField );
            BOOST_REQUIRE( sheetField );
            const std::array<SCH_FIELD*, 2> fields{ symbolField, sheetField };
            const std::array<KIID, 2> owners{ symbol->m_Uuid, sheet->m_Uuid };
            std::array<VECTOR2I, 2> positions;

            for( size_t i = 0; i < fields.size(); ++i )
            {
                fields[i]->SetPosition( VECTOR2I( 1000000 + i * 1000000, 3000000 ) );
                positions[i] = fields[i]->GetPosition();
            }

            schematic->RebuildConnectivity();
            schematic->SetCurrentSheet( paths[1] );
            ERC_TESTER tester( schematic.get() );
            const auto check = [&]( bool present )
            {
                BOOST_CHECK_EQUAL( tester.TestFieldNameWhitespace(), present ? 3 : 0 );
                std::array<std::set<KIID_PATH>, 2> seen;
                std::vector<std::pair<SCH_SCREEN*, SCH_MARKER*>> markers;

                for( SCH_SCREEN* current : { schematic->RootScreen(), screen } )
                {
                    for( SCH_ITEM* item : current->Items().OfType( SCH_MARKER_T ) )
                    {
                        auto* marker = static_cast<SCH_MARKER*>( item );
                        const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );
                        markers.emplace_back( current, marker );

                        if( error->GetErrorCode() != ERCE_FIELD_NAME_WHITESPACE )
                            continue;

                        for( size_t i = 0; i < fields.size(); ++i )
                        {
                            if( error->GetMainItemID() != owners[i] )
                                continue;

                            BOOST_CHECK( error->GetAuxItemID() == fields[i]->m_Uuid );
                            BOOST_CHECK( marker->GetPosition() == positions[i] );
                            BOOST_CHECK( error->GetErrorMessage( true ).Contains( badName ) );
                            BOOST_REQUIRE( error->IsSheetSpecific() );
                            const KIID_PATH& path = error->GetSpecificSheetPath().PathRef();
                            BOOST_CHECK( error->GetMainItemSheetPath().PathRef() == path );
                            BOOST_CHECK( error->GetAuxItemSheetPath().PathRef() == path );
                            BOOST_CHECK( seen[i].insert( path ).second );
                        }
                    }
                }

                const std::set<KIID_PATH> symbolPaths = present
                        ? std::set<KIID_PATH>{ paths[0].PathRef(), paths[1].PathRef() } : std::set<KIID_PATH>{};
                const std::set<KIID_PATH> sheetPaths = present
                        ? std::set<KIID_PATH>{ schematic->Hierarchy().front().PathRef() } : std::set<KIID_PATH>{};
                BOOST_CHECK( seen[0] == symbolPaths );
                BOOST_CHECK( seen[1] == sheetPaths );

                for( const auto& [current, marker] : markers )
                    current->DeleteItem( marker );
            };
            check( true );

            for( SCH_FIELD* field : fields )
            {
                field->SetName( "Snapshot Field" );
                field->SetPosition( field->GetPosition() + VECTOR2I( 500000, 500000 ) );
            }

            schematic->RebuildConnectivity();
            check( false );

            for( size_t i = 0; i < fields.size(); ++i )
            {
                fields[i]->SetName( badName );
                positions[i] = fields[i]->GetPosition();
            }

            schematic->RebuildConnectivity();
            check( true );

            TOOL_MANAGER manager;

            for( size_t i = 0; i < fields.size(); ++i )
            {
                SCH_COMMIT commit( &manager );
                commit.Modify( fields[i], i == 0 ? screen : schematic->RootScreen() );
                fields[i]->SetPosition( fields[i]->GetPosition() + VECTOR2I( 500000, 0 ) );
                positions[i] = fields[i]->GetPosition();
            }

            if( backend )
                schematic->Connectivity().Recalculate( *schematic, false );
            else
                schematic->RebuildConnectivity();

            check( true );
        }
    }
}
