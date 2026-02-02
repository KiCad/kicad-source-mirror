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
 * @file test_sch_rule_area_destruction.cpp
 *
 * Regression test for issue 22822: crash when closing eeschema while background
 * library loading is in progress. The crash occurs in SCH_RULE_AREA::RemoveItem()
 * because FreeDrawList() deletes items in arbitrary order. If a rule area is
 * destroyed before items it contains, those items' destructors call RemoveItem()
 * on freed memory.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_line.h>
#include <sch_rule_area.h>


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
    ruleArea->m_items.insert( line1.get() );
    line1->AddRuleAreaToCache( ruleArea.get() );

    ruleArea->m_items.insert( line2.get() );
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

    ruleArea->m_items.insert( line1.get() );
    line1->AddRuleAreaToCache( ruleArea.get() );

    ruleArea->m_items.insert( line2.get() );
    line2->AddRuleAreaToCache( ruleArea.get() );

    BOOST_CHECK_EQUAL( ruleArea->m_items.size(), 2u );

    // Destroy items first (the normal ordering)
    line1.reset();
    line2.reset();

    // Rule area should have had items removed by their destructors
    BOOST_CHECK_EQUAL( ruleArea->m_items.size(), 0u );

    ruleArea.reset();
}


BOOST_AUTO_TEST_SUITE_END()
