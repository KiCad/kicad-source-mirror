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
#include <erc/erc.h>
#include <locale_io.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_CASE( ERCStackedPinSyntaxUsesSharedInstanceUnits )
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
                auto* candidate = static_cast<SCH_SYMBOL*>( item );

                if( candidate->GetUnitCount() > 1 )
                {
                    symbol = candidate;
                    break;
                }
            }

            BOOST_REQUIRE( symbol );
            symbol->SetUnitSelection( &paths[0], 1 );
            symbol->SetUnitSelection( &paths[1], 2 );
            SCH_PIN* pin = nullptr;

            for( const auto& candidate : symbol->GetRawPins() )
            {
                if( candidate->GetNumber() == wxString( "5" ) )
                {
                    pin = candidate.get();
                    break;
                }
            }

            BOOST_REQUIRE( pin );
            BOOST_REQUIRE_EQUAL( pin->GetUnit(), 2 );
            pin->SetNumber( "1[" );
            bool valid;
            pin->GetStackedPinNumbers( &valid );
            BOOST_REQUIRE( !valid );
            screen->Update( symbol, false );
            schematic->RebuildConnectivity();
            schematic->SetCurrentSheet( paths[0] );
            ERC_TESTER tester( schematic.get() );
            tester.TestStackedPinNotation();
            int count = 0;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
            {
                auto* marker = static_cast<SCH_MARKER*>( item );
                const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                if( error->GetErrorCode() != ERCE_STACKED_PIN_SYNTAX || error->GetMainItemID() != pin->m_Uuid )
                    continue;

                ++count;
                BOOST_CHECK( error->IsSheetSpecific() );

                if( error->IsSheetSpecific() )
                    BOOST_CHECK( error->GetSpecificSheetPath().PathRef() == paths[1].PathRef() );

                BOOST_CHECK( marker->GetPosition() == pin->GetPosition() );
            }

            BOOST_CHECK_EQUAL( count, 1 );
        }
    }
}
