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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>
#include <advanced_config.h>
#include <erc/erc.h>
#include <locale_io.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_CASE( ERCWiredImplicitPowerReportsWiredHiddenPin )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/legacy_power/legacy_power", schematic );
    const SCH_SHEET_PATH path = schematic->Hierarchy().front();
    SCH_SCREEN* screen = path.LastScreen();
    SCH_PIN* pin = nullptr;
    std::unique_ptr<SCH_LINE> wire;

    for( SCH_ITEM* item : screen->Items() )
    {
        if( !wire && item->Type() == SCH_LINE_T && item->GetLayer() == LAYER_WIRE )
            wire.reset( static_cast<SCH_LINE*>( item->Duplicate( false ) ) );

        if( pin || item->Type() != SCH_SYMBOL_T )
            continue;

        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->IsPower() )
            continue;

        for( SCH_PIN* candidate : symbol->GetPins( &path ) )
        {
            if( candidate->IsGlobalPower() && !candidate->IsVisible()
                && candidate->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN )
            {
                pin = candidate;
                break;
            }
        }
    }

    BOOST_REQUIRE( pin );
    BOOST_REQUIRE( wire );
    auto* symbol = static_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

    // Move clear of the drawing so only the added wire touches the hidden pin
    symbol->SetPosition( VECTOR2I( 100000000, 100000000 ) );
    screen->Update( symbol, false );
    const VECTOR2I position = pin->GetPosition();
    wire->SetStartPoint( position );
    wire->SetEndPoint( position + VECTOR2I( 1000000, 0 ) );
    screen->Append( wire.release() );

    for( auto& [code, severity] : schematic->ErcSettings().m_ERCSeverities )
        severity = RPT_SEVERITY_IGNORE;

    schematic->ErcSettings().m_ERCSeverities[ERCE_WIRED_IMPLICIT_POWER] = RPT_SEVERITY_WARNING;
    schematic->RebuildConnectivity();
    const auto collect = [&]()
    {
        ERC_TESTER::TestConnectivity( *schematic );
        std::vector<SCH_MARKER*> result;
        std::vector<SCH_ITEM*> markers;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == ERCE_WIRED_IMPLICIT_POWER
                && marker->GetRCItem()->GetMainItemID() == pin->m_Uuid )
            {
                result.push_back( marker );
            }

            markers.push_back( item );
        }

        return std::make_pair( result, markers );
    };

    const auto [found, all] = collect();
    BOOST_REQUIRE_EQUAL( found.size(), 1 );
    BOOST_CHECK( found.front()->GetPosition() == position );

    for( SCH_ITEM* marker : all )
        screen->DeleteItem( marker );

    schematic->ErcSettings().m_ERCSeverities[ERCE_WIRED_IMPLICIT_POWER] = RPT_SEVERITY_IGNORE;
    BOOST_CHECK( collect().first.empty() );
}
