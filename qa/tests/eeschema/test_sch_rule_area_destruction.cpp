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

/**
 * @file test_sch_rule_area_destruction.cpp
 *
 * Regression test for issue 22822: crash when closing eeschema while background
 * library loading is in progress. The crash occurs in SCH_RULE_AREA::RemoveItem()
 * because FreeDrawList() deletes items in arbitrary order. If a rule area is
 * destroyed before items it contains, those items' destructors call RemoveItem()
 * on freed memory.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <schematic.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_symbol.h>


BOOST_AUTO_TEST_SUITE( SchRuleAreaDestruction )


/**
 * Verify that destroying a rule area before its contained items does not crash.
 *
 * This reproduces the exact destruction ordering that caused the issue 22822
 * segfault in FreeDrawList().
 */
BOOST_AUTO_TEST_CASE( RuleAreaDestroyedBeforeContainedItems )
{
    auto ruleArea = std::make_unique<SCH_RULE_AREA>();
    auto line1 = std::make_unique<SCH_LINE>( VECTOR2I( 0, 0 ), LAYER_WIRE );
    auto line2 = std::make_unique<SCH_LINE>( VECTOR2I( 100, 0 ), LAYER_WIRE );

    // Simulate what RefreshContainedItemsAndDirectives does
    ruleArea->_GetContainedItems().insert( line1.get() );
    line1->AddRuleAreaToCache( ruleArea.get() );

    ruleArea->_GetContainedItems().insert( line2.get() );
    line2->AddRuleAreaToCache( ruleArea.get() );

    BOOST_CHECK_EQUAL( line1->GetRuleAreaCache().size(), 1u );
    BOOST_CHECK_EQUAL( line2->GetRuleAreaCache().size(), 1u );

    // Destroy rule area first (the problematic ordering from FreeDrawList)
    ruleArea.reset();

    // Contained items should no longer reference the destroyed rule area
    BOOST_CHECK_EQUAL( line1->GetRuleAreaCache().size(), 0u );
    BOOST_CHECK_EQUAL( line2->GetRuleAreaCache().size(), 0u );

    // Destroying items should not crash (previously called RemoveItem on freed memory)
    line1.reset();
    line2.reset();
}


/**
 * Verify that destroying contained items before the rule area also works.
 *
 * The reverse ordering should also be safe since SCH_ITEM::~SCH_ITEM calls
 * RemoveItem on still-valid rule areas.
 */
BOOST_AUTO_TEST_CASE( ContainedItemsDestroyedBeforeRuleArea )
{
    auto ruleArea = std::make_unique<SCH_RULE_AREA>();
    auto line1 = std::make_unique<SCH_LINE>( VECTOR2I( 0, 0 ), LAYER_WIRE );
    auto line2 = std::make_unique<SCH_LINE>( VECTOR2I( 100, 0 ), LAYER_WIRE );

    ruleArea->_GetContainedItems().insert( line1.get() );
    line1->AddRuleAreaToCache( ruleArea.get() );

    ruleArea->_GetContainedItems().insert( line2.get() );
    line2->AddRuleAreaToCache( ruleArea.get() );

    BOOST_CHECK_EQUAL( ruleArea->_GetContainedItems().size(), 2u );

    // Destroy items first (the normal ordering)
    line1.reset();
    line2.reset();

    // Rule area should have had items removed by their destructors
    BOOST_CHECK_EQUAL( ruleArea->_GetContainedItems().size(), 0u );

    ruleArea.reset();
}


/**
 * Verify that rebuilding rule area caches clears reciprocal links on owned children and
 * directive labels which are no longer attached to the area.
 */
BOOST_AUTO_TEST_CASE( CacheRefreshClearsReciprocalLinks )
{
    SCHEMATIC  schematic( nullptr );
    SCH_SCREEN screen( &schematic );
    auto*      ruleArea = new SCH_RULE_AREA();
    auto*      symbol = new SCH_SYMBOL();
    auto*      directive = new SCH_DIRECTIVE_LABEL( VECTOR2I( 1000, 1000 ) );

    symbol->GetRawPins().push_back( std::make_unique<SCH_PIN>( symbol, wxT( "1" ), wxEmptyString, KIID() ) );
    SCH_PIN* pin = symbol->GetRawPins().front().get();

    ruleArea->_GetContainedItems().insert( pin );
    pin->AddRuleAreaToCache( ruleArea );
    ruleArea->_GetContainedDirectives().insert( directive );
    directive->AddConnectedRuleArea( ruleArea );

    screen.Append( ruleArea );
    screen.Append( symbol, false );
    screen.Append( directive );

    std::unordered_set<SCH_SCREEN*> screens = { &screen };
    SCH_RULE_AREA::UpdateRuleAreasInScreens( screens, nullptr );

    BOOST_CHECK( pin->GetRuleAreaCache().empty() );
    BOOST_CHECK( directive->GetConnectedRuleAreas().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
