/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <sch_table.h>
#include <sch_tablecell.h>


BOOST_AUTO_TEST_SUITE( SchTable )


static SCH_TABLE* createTable( int aColCount, int aCellCount, int aX, int aY )
{
    SCH_TABLE* table = new SCH_TABLE( schIUScale.mmToIU( 0.1 ) );
    table->SetColCount( aColCount );

    for( int i = 0; i < aCellCount; ++i )
    {
        SCH_TABLECELL* cell = new SCH_TABLECELL();

        if( i == 0 )
            cell->SetPosition( VECTOR2I( aX, aY ) );

        table->AddCell( cell );
    }

    return table;
}


BOOST_AUTO_TEST_CASE( TableCompareByPosition )
{
    // Issue 22559: Tables with different positions were not compared correctly because
    // operator< had bugs comparing positions (comparing self to self instead of other).

    std::unique_ptr<SCH_TABLE> table1( createTable( 2, 4, 100, 200 ) );
    std::unique_ptr<SCH_TABLE> table2( createTable( 2, 4, 300, 200 ) );
    std::unique_ptr<SCH_TABLE> table3( createTable( 2, 4, 100, 400 ) );

    // Table1 should be < Table2 (different X positions)
    BOOST_CHECK( *table1 < *table2 );
    BOOST_CHECK( !( *table2 < *table1 ) );

    // Table1 should be < Table3 (same X, different Y positions)
    BOOST_CHECK( *table1 < *table3 );
    BOOST_CHECK( !( *table3 < *table1 ) );

    // Table2 should be > Table3 (X comparison takes precedence: 300 > 100)
    BOOST_CHECK( *table3 < *table2 );
    BOOST_CHECK( !( *table2 < *table3 ) );

    // Test reflexivity: table should not be less than itself
    BOOST_CHECK( !( *table1 < *table1 ) );
    BOOST_CHECK( !( *table2 < *table2 ) );
    BOOST_CHECK( !( *table3 < *table3 ) );
}


BOOST_AUTO_TEST_CASE( TableCompareOrdering )
{
    // Test that table comparison produces consistent ordering for sorting.

    std::unique_ptr<SCH_TABLE> tableA( createTable( 2, 4, 100, 100 ) );
    std::unique_ptr<SCH_TABLE> tableB( createTable( 2, 4, 200, 100 ) );
    std::unique_ptr<SCH_TABLE> tableC( createTable( 2, 4, 100, 200 ) );
    std::unique_ptr<SCH_TABLE> tableD( createTable( 2, 4, 200, 200 ) );

    // Verify strict weak ordering for all pairs
    // Each table should compare consistently against all others

    // A < B (X: 100 < 200)
    BOOST_CHECK( *tableA < *tableB );
    BOOST_CHECK( !( *tableB < *tableA ) );

    // A < C (same X, Y: 100 < 200)
    BOOST_CHECK( *tableA < *tableC );
    BOOST_CHECK( !( *tableC < *tableA ) );

    // A < D (X: 100 < 200)
    BOOST_CHECK( *tableA < *tableD );
    BOOST_CHECK( !( *tableD < *tableA ) );

    // B > C (X: 200 > 100)
    BOOST_CHECK( *tableC < *tableB );
    BOOST_CHECK( !( *tableB < *tableC ) );

    // B < D (same X: 200, Y: 100 < 200)
    BOOST_CHECK( *tableB < *tableD );
    BOOST_CHECK( !( *tableD < *tableB ) );

    // C < D (X: 100 < 200)
    BOOST_CHECK( *tableC < *tableD );
    BOOST_CHECK( !( *tableD < *tableC ) );
}


BOOST_AUTO_TEST_CASE( TableCompareByCellCount )
{
    // Tables with different cell counts should compare by cell count first

    std::unique_ptr<SCH_TABLE> smallTable( createTable( 2, 4, 100, 100 ) );
    std::unique_ptr<SCH_TABLE> largeTable( createTable( 3, 9, 100, 100 ) );

    // Smaller table should be < larger table
    BOOST_CHECK( *smallTable < *largeTable );
    BOOST_CHECK( !( *largeTable < *smallTable ) );
}


BOOST_AUTO_TEST_SUITE_END()
