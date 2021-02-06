/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include <geometry/poly_grid_partition.h>

struct PGPartitionFixture
{
    SHAPE_POLY_SET testPolys[2];

    std::vector<VECTOR2I> testPoints;

    PGPartitionFixture()
    {

        testPoints.push_back( VECTOR2I( -2794000,  47830000 ) );
        testPoints.push_back( VECTOR2I( -3730000,  47820000 ) );

        // auto-generated code. don't complain about formatting...
        { auto tmp = SHAPE_LINE_CHAIN( { VECTOR2I( -1902094, 47420002), VECTOR2I( -1855601, 47473658), VECTOR2I( -1845000, 47513474), VECTOR2I( -1845000, 47517499), VECTOR2I( -1843938, 47523201), VECTOR2I( -1826178, 47618564), VECTOR2I( -1826177, 47618567), VECTOR2I( -1824048, 47629998), VECTOR2I( -1764001, 47727412), VECTOR2I( -1754742, 47734452), VECTOR2I( -1754741, 47734454), VECTOR2I( -1707035, 47770730), VECTOR2I( -1672912, 47796678), VECTOR2I( -1562995, 47828508), VECTOR2I( -1555663, 47828866), VECTOR2I( -1555660, 47828867), VECTOR2I( -1546053, 47829336), VECTOR2I( -1532500, 47829999), VECTOR2I( -1455410, 47829999), VECTOR2I( -1173301, 47830000), VECTOR2I( -1167501, 47830000), VECTOR2I( -1095939, 47816672), VECTOR2I( -1006088, 47833695), VECTOR2I( -921000, 47886876), VECTOR2I( -885528, 47920101), VECTOR2I( -257059, 48792973), VECTOR2I( -105269, 49003793), VECTOR2I( -86131, 49009997), VECTOR2I( 458321, 49186489), VECTOR2I( 524305, 49236457), VECTOR2I( 782412, 49623618), VECTOR2I( 803006, 49681564), VECTOR2I( 899432, 50694036), VECTOR2I( 900000, 50705982), VECTOR2I( 900000, 51374000), VECTOR2I( 879998, 51442121), VECTOR2I( 826342, 51488614), VECTOR2I( 774000, 51500000), VECTOR2I( -174000, 51500000), VECTOR2I( -242121, 51479998), VECTOR2I( -288614, 51426342), VECTOR2I( -300000, 51374000), VECTOR2I( -300000, 50500000), VECTOR2I( -1400000, 50500000), VECTOR2I( -1407434, 50517346), VECTOR2I( -1667272, 51123634), VECTOR2I( -1712491, 51178368), VECTOR2I( -1783084, 51200000), VECTOR2I( -4974000, 51200000), VECTOR2I( -5042121, 51179998), VECTOR2I( -5088614, 51126342), VECTOR2I( -5100000, 51074000), VECTOR2I( -5100000, 47526000), VECTOR2I( -5079998, 47457879), VECTOR2I( -5026342, 47411386), VECTOR2I( -4974000, 47400000), VECTOR2I( -1970215, 47400000)}, true );;
        testPolys[0].AddOutline(tmp); testPolys[0].Unfracture( SHAPE_POLY_SET::PM_FAST ); }


        { auto tmp = SHAPE_LINE_CHAIN( { VECTOR2I( -1669357, 47419152), VECTOR2I( -921001, 47886875), VECTOR2I( -885528, 47920101), VECTOR2I( -257059, 48792973), VECTOR2I( -105269, 49003793), VECTOR2I( 478113, 49192905), VECTOR2I( 517969, 49214375), VECTOR2I( 716789, 49373432), VECTOR2I( 760315, 49441261), VECTOR2I( 792698, 49570792), VECTOR2I( 789815, 49641728), VECTOR2I( 768344, 49676295), VECTOR2I( 768537, 49676413), VECTOR2I( 760684, 49689316), VECTOR2I( 750000, 49700000), VECTOR2I( 745420, 49714395), VECTOR2I( 427935, 50712204), VECTOR2I( 388220, 50771053), VECTOR2I( 307866, 50800000), VECTOR2I( 234899, 50800000), VECTOR2I( 170073, 50782044), VECTOR2I( -285313, 50508812), VECTOR2I( -285314, 50508812), VECTOR2I( -300000, 50500000), VECTOR2I( -1400000, 50500000), VECTOR2I( -1407434, 50517346), VECTOR2I( -1667272, 51123634), VECTOR2I( -1712491, 51178368), VECTOR2I( -1783084, 51200000), VECTOR2I( -4974000, 51200000), VECTOR2I( -5042121, 51179998), VECTOR2I( -5088614, 51126342), VECTOR2I( -5100000, 51074000), VECTOR2I( -5100000, 47526000), VECTOR2I( -5079998, 47457879), VECTOR2I( -5026342, 47411386), VECTOR2I( -4974000, 47400000), VECTOR2I( -1736137, 47400000)}, true );;
        testPolys[1].AddOutline(tmp); testPolys[1].Unfracture( SHAPE_POLY_SET::PM_FAST ); }
    }

    ~PGPartitionFixture()
    {
    }
};

BOOST_FIXTURE_TEST_SUITE( PGPartitionTest, PGPartitionFixture )

BOOST_AUTO_TEST_CASE( PointInside )
{
    for( auto p : testPoints )
    {
        for ( auto& poly : testPolys )
        {
            bool fallback_in = poly.Contains( p );
            poly.Fracture( SHAPE_POLY_SET::PM_FAST );
            POLY_GRID_PARTITION part( poly.COutline(0), 16 );
            bool pgp_in = part.ContainsPoint( p );

            // compare vanilla point-in-polygon with the grid partitioning.
            BOOST_CHECK_EQUAL( fallback_in, pgp_in );
        }
    }
}



BOOST_AUTO_TEST_SUITE_END()
