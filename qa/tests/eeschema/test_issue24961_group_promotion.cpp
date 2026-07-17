/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <sch_symbol.h>
#include <sch_group.h>
#include <sch_collectors.h>
#include <tools/sch_selection_tool.h>
#include <tool/tool_manager.h>
#include <view/view.h>

// Issue 24961: requesting a symbols-only selection over a grouped symbol
// returned the group instead, and callers crashed casting it to SCH_SYMBOL

struct GROUP_PROMOTION_FIXTURE
{
    GROUP_PROMOTION_FIXTURE() :
            m_tool( new SCH_SELECTION_TOOL() )
    {
        m_mgr.SetEnvironment( nullptr, &m_view, nullptr, nullptr, nullptr );
        m_mgr.RegisterTool( m_tool );

        m_group.AddItem( &m_symbol );
    }

    ~GROUP_PROMOTION_FIXTURE() { m_group.RemoveAll(); }

    SCH_COLLECTOR makeCollector( const std::vector<KICAD_T>& aScanTypes )
    {
        SCH_COLLECTOR collector;
        collector.SetScanTypes( aScanTypes );
        collector.Append( &m_symbol );
        return collector;
    }

    KIGFX::VIEW         m_view;
    TOOL_MANAGER        m_mgr;
    SCH_SELECTION_TOOL* m_tool;
    SCH_SYMBOL          m_symbol;
    SCH_GROUP           m_group;
};

BOOST_FIXTURE_TEST_SUITE( Issue24961GroupPromotion, GROUP_PROMOTION_FIXTURE )

BOOST_AUTO_TEST_CASE( SymbolOnlyScanTypesDoNotPromote )
{
    SCH_COLLECTOR collector = makeCollector( { SCH_SYMBOL_T } );

    m_tool->FilterCollectorForHierarchy( collector, false );

    BOOST_REQUIRE_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK( collector[0] == &m_symbol );
}

BOOST_AUTO_TEST_CASE( LocateAnyPromotesToGroup )
{
    SCH_COLLECTOR collector = makeCollector( { SCH_LOCATE_ANY_T } );

    m_tool->FilterCollectorForHierarchy( collector, false );

    BOOST_REQUIRE_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK( collector[0] == &m_group );
}

BOOST_AUTO_TEST_CASE( EmptyScanTypesPromoteToGroup )
{
    // SelectAll and box selection build collectors by hand with no scan types
    SCH_COLLECTOR collector = makeCollector( {} );

    m_tool->FilterCollectorForHierarchy( collector, false );

    BOOST_REQUIRE_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK( collector[0] == &m_group );
}

BOOST_AUTO_TEST_CASE( GroupScanTypesPromoteToGroup )
{
    SCH_COLLECTOR collector = makeCollector( SCH_COLLECTOR::MovableItems );

    m_tool->FilterCollectorForHierarchy( collector, false );

    BOOST_REQUIRE_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK( collector[0] == &m_group );
}

BOOST_AUTO_TEST_SUITE_END()
