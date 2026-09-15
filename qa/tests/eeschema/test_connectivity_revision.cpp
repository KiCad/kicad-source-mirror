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
#include <connectivity/conn_dump.h>
#include <connectivity/conn_facade.h>
#include <lib_symbol.h>
#include <sch_commit.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <scoped_set_reset.h>


struct CONNECTIVITY_REVISION_FIXTURE
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
};


BOOST_FIXTURE_TEST_SUITE( ConnectivityRevision, CONNECTIVITY_REVISION_FIXTURE )

BOOST_AUTO_TEST_CASE( ScreenIndexTracksSymbolPinReplacementAndUndo )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET restoreEngine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET restoreIncremental( config.m_IncrementalConnectivity, config.m_IncrementalConnectivity );
    config.m_ConnectivityEngine = true;
    config.m_IncrementalConnectivity = true;

    KI_TEST::LoadSchematic( settings, "issue7203", schematic );
    SCH_SCREEN* screen = schematic->GetTopLevelSheets().front()->GetScreen();
    SCH_SYMBOL* symbol = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        if( candidate->GetLibSymbolRef() && candidate->GetPins().size() >= 2 )
        {
            symbol = candidate;
            break;
        }
    }

    BOOST_REQUIRE( symbol );
    TOOL_MANAGER manager;
    manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
    schematic->RebuildConnectivity();
    SCH_CONNECTIVITY::FACADE rebuilt;
    const auto verify = [&]()
    {
        const std::string incremental = SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() );
        rebuilt.Update( *schematic, true );
        BOOST_CHECK_EQUAL( incremental, SCH_CONNECTIVITY::Dump( *schematic, rebuilt ) );
    };
    verify();

    SCH_PIN* pin = symbol->GetPins().front();
    const KIID id = pin->m_Uuid;
    const wxString number = pin->GetNumber();
    const KIID_PATH path = schematic->Hierarchy().front().Path();
    BOOST_REQUIRE( schematic->Connectivity().Connection( id, path ) );
    BOOST_REQUIRE( screen->GetConnectivityItem( id ) == pin );
    std::unique_ptr<SCH_ITEM> undo( static_cast<SCH_ITEM*>( symbol->Clone() ) );
    auto updated = symbol->GetLibSymbolRef()->Flatten();

    for( SCH_PIN* libPin : updated->GetPinsByNumber( number ) )
        updated->RemoveDrawItem( libPin );

    SCH_COMMIT replacement( &manager );
    replacement.Modify( symbol, screen );
    symbol->SetLibSymbol( updated.release() );
    BOOST_CHECK( !screen->GetConnectivityItem( id ) );
    replacement.Push( "Replace native symbol pin set", SKIP_UNDO );
    BOOST_CHECK( !schematic->Connectivity().Connection( id, path ) );
    verify();

    SCH_COMMIT restoration( &manager );
    restoration.Modify( symbol, screen );
    symbol->SwapItemData( undo.get() );
    SCH_ITEM* restored = screen->GetConnectivityItem( id );
    BOOST_REQUIRE( restored );
    BOOST_CHECK( restored->GetParent() == symbol );
    BOOST_CHECK( restored == symbol->GetPin( number ) );
    restoration.Push( "Restore native symbol pin set", SKIP_UNDO );
    BOOST_REQUIRE( schematic->Connectivity().Connection( id, path ) );
    verify();
}


BOOST_AUTO_TEST_CASE( AssigningSymbolPreservesAlternatePinDefinitions )
{
    KI_TEST::LoadSchematic( settings, "issue22286/bugtest", schematic );
    SCH_SCREEN* screen = schematic->GetTopLevelSheets().front()->GetScreen();
    SCH_SYMBOL* symbol = nullptr;
    SCH_PIN* pin = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* candidate = static_cast<SCH_SYMBOL*>( item );

        for( SCH_PIN* candidatePin : candidate->GetPins() )
        {
            if( !candidatePin->GetAlt().IsEmpty() )
            {
                symbol = candidate;
                pin = candidatePin;
                break;
            }
        }

        if( symbol )
            break;
    }

    BOOST_REQUIRE( symbol );
    const KIID id = pin->m_Uuid;
    const wxString alternate = pin->GetAlt();
    const auto type = pin->GetType();
    BOOST_REQUIRE( screen->GetConnectivityItem( id ) == pin );
    auto copy = std::make_unique<SCH_SYMBOL>( *symbol );
    *symbol = *copy;
    auto* updated = dynamic_cast<SCH_PIN*>( screen->GetConnectivityItem( id ) );
    BOOST_REQUIRE( updated );
    BOOST_REQUIRE( updated->GetLibPin() );
    BOOST_CHECK_EQUAL( updated->GetAlt(), alternate );
    BOOST_CHECK( updated->GetType() == type );
    BOOST_CHECK( updated->GetLibPin()->GetParent() == symbol->GetLibSymbolRef().get() );
}

BOOST_AUTO_TEST_SUITE_END()
