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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Test suite for SCH_SHEET
 */

#include <base_units.h>
#include <core/ignore.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <qa_utils/uuid_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <sch_rtree.h>

#include <qa_utils/wx_utils/wx_assert.h>

class TEST_SCH_RTREE_FIXTURE
{
public:
    TEST_SCH_RTREE_FIXTURE() : m_tree()
    {
    }

    EE_RTREE m_tree;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchRtree, TEST_SCH_RTREE_FIXTURE )


/**
 * Check default iterators
 */
BOOST_AUTO_TEST_CASE( Default )
{
    BOOST_CHECK_EQUAL( m_tree.empty(), true );

    int count = 0;
    for( auto item : m_tree )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 0 );

    for( int type = 0; type <= MAX_STRUCT_TYPE_ID; type++ )
    {
        count = 0;
        for( auto item : m_tree.OfType( KICAD_T( type ) ) )
        {
            ignore_unused( item );
            count++;
        }

        BOOST_CHECK_EQUAL( count, 0 );
    }

    BOX2I bbox;

    for( int type = 0; type <= MAX_STRUCT_TYPE_ID; type++ )
    {
        count = 0;

        for( SCH_ITEM* item : m_tree.Overlapping( SCH_JUNCTION_T, bbox ) )
        {
            ignore_unused( item );
            count++;
        }

        BOOST_CHECK_EQUAL( count, 0 );
    }
}

BOOST_AUTO_TEST_CASE( Junctions )
{
    for( int i = 0; i < 100; i++ )
    {
        SCH_JUNCTION* junction = new SCH_JUNCTION(
                VECTOR2I( schIUScale.MilsToIU( 100 ) * i, schIUScale.MilsToIU( 100 ) * i ) );
        m_tree.insert( junction );
    }

    int count = 0;

    for( auto item : m_tree.OfType( SCH_JUNCTION_T ) )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 100 );

    count = 0;
    for( auto item : m_tree.OfType( SCH_NO_CONNECT_T ) )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 0 );

    BOX2I small_bbox( VECTOR2I( -1, -1 ),
                      VECTOR2I( schIUScale.MilsToIU( 2 ), schIUScale.MilsToIU( 2 ) ) );
    BOX2I med_bbox( VECTOR2I( 0, 0 ),
                    VECTOR2I( schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) ) );
    BOX2I big_bbox( VECTOR2I( 0, 0 ),
                    VECTOR2I( schIUScale.MilsToIU( 5000 ), schIUScale.MilsToIU( 5000 ) ) );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( small_bbox ) )
    {
        BOOST_CHECK( small_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 1 );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( SCH_JUNCTION_T, small_bbox ) )
    {
        BOOST_CHECK( small_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 1 );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( SCH_NO_CONNECT_T, small_bbox ) )
    {
        BOOST_CHECK( small_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 0 );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( med_bbox ) )
    {
        BOOST_CHECK( med_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 2 );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( big_bbox ) )
    {
        BOOST_CHECK( big_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 51 );

    for( SCH_ITEM* item : m_tree )
        delete item;
}

BOOST_AUTO_TEST_CASE( MixedElements )
{
    for( int i = 0; i < 100; i++ )
    {
        int x_sign = ( i % 2 == 0 ) ? -1 : 1;
        int y_sign = ( i % 3 == 0 ) ? -1 : 1;

        SCH_JUNCTION* junction = new SCH_JUNCTION( VECTOR2I( schIUScale.MilsToIU( 100 ) * i * x_sign,
                                                             schIUScale.MilsToIU( 100 ) * i * y_sign ) );
        m_tree.insert( junction );

        SCH_NO_CONNECT* nc = new SCH_NO_CONNECT( VECTOR2I( schIUScale.MilsToIU( 150 ) * i * y_sign,
                                                           schIUScale.MilsToIU( 150 ) * i * x_sign ) );
        m_tree.insert( nc );
    }

    int count = 0;

    for( SCH_ITEM* item : m_tree.OfType( SCH_JUNCTION_T ) )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 100 );

    count = 0;

    for( SCH_ITEM* item : m_tree.OfType( SCH_NO_CONNECT_T ) )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 100 );

    BOX2I small_bbox( VECTOR2I( -1, -1 ), VECTOR2I( schIUScale.MilsToIU( 2 ), schIUScale.MilsToIU( 2 ) ) );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( small_bbox ) )
    {
        BOOST_CHECK( small_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 2 );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( SCH_JUNCTION_T, small_bbox ) )
    {
        BOOST_CHECK( small_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 1 );

    count = 0;

    for( SCH_ITEM* item : m_tree.Overlapping( SCH_NO_CONNECT_T, small_bbox ) )
    {
        BOOST_CHECK( small_bbox.Intersects( item->GetBoundingBox() ) );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 1 );

    for( SCH_ITEM* item : m_tree )
        delete item;
}

// This tests the case where the tree has no branches but we want to iterator over a subset
// where the first case may or may not match
BOOST_AUTO_TEST_CASE( SingleElementTree )
{
    SCH_JUNCTION* junction = new SCH_JUNCTION( VECTOR2I( schIUScale.MilsToIU( 100 ), schIUScale.MilsToIU( 100 ) ) );
    m_tree.insert( junction );

    SCH_NO_CONNECT* nc = new SCH_NO_CONNECT( VECTOR2I( schIUScale.MilsToIU( 150 ), schIUScale.MilsToIU( 150 ) ) );
    m_tree.insert( nc );

    int count = 0;

    for( auto item : m_tree.OfType( SCH_JUNCTION_T ) )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 1 );

    count = 0;
    for( auto item : m_tree.OfType( SCH_NO_CONNECT_T ) )
    {
        ignore_unused( item );
        count++;
    }

    BOOST_CHECK_EQUAL( count, 1 );

    for( SCH_ITEM* item : m_tree )
        delete item;
}

BOOST_AUTO_TEST_SUITE_END()
