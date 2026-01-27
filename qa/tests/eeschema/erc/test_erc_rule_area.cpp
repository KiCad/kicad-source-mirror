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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

/**
 * @file test_erc_rule_area.cpp
 * Tests for ERC with rule areas, including regression test for issue 22854
 * (crash when running ERC twice with rule areas and modified items).
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_rule_area.h>
#include <sch_symbol.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_RULE_AREA_TEST_FIXTURE
{
    ERC_RULE_AREA_TEST_FIXTURE()
    {
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Regression test for issue 22854.
 *
 * When a schematic has rule areas and items are modified between ERC runs,
 * the second ERC run would crash due to dangling pointers in the rule area's
 * item cache. The fix ensures that items remove themselves from rule areas
 * when destroyed.
 */
BOOST_FIXTURE_TEST_CASE( ERCRuleAreaItemDeletion, ERC_RULE_AREA_TEST_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue22854/test", m_schematic );

    ERC_TESTER tester( m_schematic.get() );

    // First ERC run populates the rule area caches
    tester.RunTests( nullptr, nullptr, nullptr, &m_schematic->Project(), nullptr );

    // Find a symbol to delete and recreate (simulating a move operation)
    SCH_SCREEN* rootScreen = m_schematic->RootScreen();
    SCH_SYMBOL* symbolToModify = nullptr;

    for( SCH_ITEM* item : rootScreen->Items().OfType( SCH_SYMBOL_T ) )
    {
        symbolToModify = static_cast<SCH_SYMBOL*>( item );
        break;
    }

    BOOST_REQUIRE( symbolToModify != nullptr );

    // Clone the symbol, delete the original, then add the clone
    // This simulates what happens during a move operation
    SCH_SYMBOL* clonedSymbol = static_cast<SCH_SYMBOL*>( symbolToModify->Clone() );

    rootScreen->Remove( symbolToModify );
    delete symbolToModify;
    symbolToModify = nullptr;

    rootScreen->Append( clonedSymbol );

    // Second ERC run should not crash. Before the fix, the rule area still held
    // a pointer to the deleted symbol and would crash when clearing caches.
    BOOST_CHECK_NO_THROW( tester.RunTests( nullptr, nullptr, nullptr, &m_schematic->Project(), nullptr ) );
}
