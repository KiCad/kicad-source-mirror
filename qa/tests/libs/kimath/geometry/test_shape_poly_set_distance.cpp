/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <geometry/shape_poly_set.h>

#include <qa_utils/geometry/poly_set_construction.h>
#include <qa_utils/geometry/seg_construction.h>

#include <qa_utils/geometry/geometry.h>

/// Mock up a conversion function
constexpr static double IU_PER_MM = 1e3;

constexpr static inline int Millimeter2iu( double mm )
{
    return (int) ( mm < 0 ? mm * IU_PER_MM - 0.5 : mm * IU_PER_MM + 0.5 );
}


/**
 * Declares the Boost test suite fixture.
 */
BOOST_AUTO_TEST_SUITE( SPSDistance )

struct SPS_DISTANCE_TO_SEG_CASE
{
    std::string m_case_name;

    /// list of lists of polygon points
    SHAPE_POLY_SET m_polyset;

    /// the segment to check distance to
    SEG m_seg;
    int m_seg_width;

    /// The expected answer
    int m_exp_dist;
};

static std::vector<SPS_DISTANCE_TO_SEG_CASE> GetSPSSegDistCases()
{
    namespace KT = KI_TEST;
    std::vector<SPS_DISTANCE_TO_SEG_CASE> cases;

    // Single 10mm square at origin
    const SHAPE_POLY_SET square_10mm_0_0 = KT::BuildPolyset( {
            KT::BuildSquareChain( Millimeter2iu( 10 ) ),
    } );

    // Double square: 10mm each, one at (0, 0), one at (10, 0)
    const SHAPE_POLY_SET squares_10mm_0_0_and_20_0 = KT::BuildPolyset( {
            KT::BuildSquareChain( Millimeter2iu( 10 ) ),
            KT::BuildSquareChain( Millimeter2iu( 10 ), //
                    { Millimeter2iu( 20 ), Millimeter2iu( 0 ) } ),
    } );

    // Hollow square: 10mm hole in 20mm square, at origin
    const SHAPE_POLY_SET hollow_square_20_10_at_0_0 =
            KT::BuildHollowSquare( Millimeter2iu( 20 ), Millimeter2iu( 10 ) );

    cases.push_back( {
            "Square poly -> 1D segment",
            square_10mm_0_0,
            KT::BuildHSeg( { Millimeter2iu( 0 ), Millimeter2iu( 15 ) }, Millimeter2iu( 10 ) ),
            Millimeter2iu( 0 ), // 1-d segment
            Millimeter2iu( 10 ),
    } );

    cases.push_back( {
            "Square poly -> 2D (thick) segment",
            square_10mm_0_0,
            KT::BuildHSeg( { Millimeter2iu( 0 ), Millimeter2iu( 15 ) }, Millimeter2iu( 10 ) ),
            Millimeter2iu( 2 ), // thick segment
            Millimeter2iu( 9 ),
    } );

    cases.push_back( {
            "Two Squares poly -> 2D segment (nearest second square)", squares_10mm_0_0_and_20_0,
            KT::BuildHSeg( { Millimeter2iu( 15 ), Millimeter2iu( 15 ) }, Millimeter2iu( 10 ) ),
            Millimeter2iu( 2 ), // thick segment
            Millimeter2iu( 9 ), // from line to second square
    } );

    cases.push_back( {
            "Square poly -> one intersect", square_10mm_0_0,
            KT::BuildHSeg( { Millimeter2iu( -5 ), Millimeter2iu( 0 ) }, Millimeter2iu( 10 ) ),
            Millimeter2iu( 0 ), // 1-d segment
            Millimeter2iu( 0 ), // intersect
    } );

    cases.push_back( {
            "Square poly -> multiple intersection", square_10mm_0_0,
            KT::BuildHSeg( { Millimeter2iu( -5 ), Millimeter2iu( 0 ) }, Millimeter2iu( 20 ) ),
            Millimeter2iu( 0 ), // 1-d segment
            Millimeter2iu( 0 ), // intersect
    } );

    cases.push_back( {
            "Square poly -> 1D seg touching", square_10mm_0_0,
            // touch left side at (-5, 0)
            KT::BuildHSeg( { Millimeter2iu( -10 ), Millimeter2iu( 0 ) }, Millimeter2iu( 5 ) ),
            Millimeter2iu( 0 ), // 2D segment
            Millimeter2iu( 0 ), // intersect
    } );

    cases.push_back( {
            "Square poly -> 2D seg (end cap is nearest)", square_10mm_0_0,
            KT::BuildHSeg( { Millimeter2iu( -20 ), Millimeter2iu( 0 ) }, Millimeter2iu( 10 ) ),
            Millimeter2iu( 2 ), // 2D segment, 1mm cap radius
            Millimeter2iu( 4 ), // 4mm short, 5mm to wire end, -1mm radius
    } );

    return cases;
};

/**
 * Check segment distances
 */
BOOST_AUTO_TEST_CASE( SegDistance )
{
    for( const auto& c : GetSPSSegDistCases() )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            SHAPE_POLY_SET polyset = c.m_polyset;

            int dist = sqrt( polyset.SquaredDistanceToSeg( c.m_seg ) ) - ( c.m_seg_width / 2 );

            // right answer?
            BOOST_CHECK_PREDICATE( KI_TEST::IsWithin<int>, ( dist )( c.m_exp_dist )( 1 ) );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
