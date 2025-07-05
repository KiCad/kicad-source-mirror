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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <algorithm>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>
#include <thread_pool.h>

struct BUS_ENTRY_CONCURRENCY_FIXTURE
{
    BUS_ENTRY_CONCURRENCY_FIXTURE() :
            m_mgr()
    {
        m_mgr.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_mgr.Prj() );
        m_screen = new SCH_SCREEN( m_schematic.get() );
        m_sheet = new SCH_SHEET( m_schematic.get() );
        m_sheet->SetScreen( m_screen );
        m_schematic->SetRoot( m_sheet );
    }

    SETTINGS_MANAGER           m_mgr;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_SCREEN*                m_screen;
    SCH_SHEET*                 m_sheet;
};

static SCH_LINE* make_bus( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    SCH_LINE* const line = new SCH_LINE{ aStart, LAYER_BUS };
    line->SetEndPoint( aEnd );
    return line;
}

static SCH_BUS_BUS_ENTRY* make_entry( const VECTOR2I& aPos, int aDy )
{
    SCH_BUS_BUS_ENTRY* entry = new SCH_BUS_BUS_ENTRY( aPos );
    entry->SetSize( VECTOR2I( 0, aDy ) );
    return entry;
}

BOOST_FIXTURE_TEST_CASE( BusEntryConcurrency, BUS_ENTRY_CONCURRENCY_FIXTURE )
{
    thread_pool& tp = GetKiCadThreadPool();
    tp.reset( 2 );

    SCH_LINE* bus1 = make_bus( { 0, 0 }, { 5000000, 0 } );
    SCH_LINE* bus2 = make_bus( { 0, 10000 }, { 5000000, 10000 } );

    m_screen->Append( bus1 );
    m_screen->Append( bus2 );

    std::vector<SCH_BUS_BUS_ENTRY*> entries;

    for( int ii = 0; ii < 50; ++ii )
    {
        int x = ii * 10000;
        SCH_BUS_BUS_ENTRY* entry = make_entry( { x, 0 }, 10000 );
        m_screen->Append( entry );
        entries.push_back( entry );
    }

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    SCH_SHEET_PATH path = sheets[0];

    for( SCH_BUS_BUS_ENTRY* entry : entries )
    {
        BOOST_CHECK( entry->m_connected_bus_items[0] == bus1 );
        BOOST_CHECK( entry->m_connected_bus_items[1] == bus2 );
    }

    SCH_ITEM_VEC bus1_items = bus1->ConnectedItems( path );
    SCH_ITEM_VEC bus2_items = bus2->ConnectedItems( path );

    BOOST_CHECK_EQUAL( bus1_items.size(), entries.size() );
    BOOST_CHECK_EQUAL( bus2_items.size(), entries.size() );

    for( SCH_BUS_BUS_ENTRY* entry : entries )
    {
        BOOST_CHECK( std::find( bus1_items.begin(), bus1_items.end(), entry ) != bus1_items.end() );
        BOOST_CHECK( std::find( bus2_items.begin(), bus2_items.end(), entry ) != bus2_items.end() );
    }
}
