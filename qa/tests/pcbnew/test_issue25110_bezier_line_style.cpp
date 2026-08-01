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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cmath>
#include <numeric>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <geometry/shape_line_chain.h>
#include <pcb_painter.h>
#include <pcb_shape.h>
#include <stroke_params.h>


BOOST_AUTO_TEST_SUITE( Issue25110 )


// A dash that spans a vertex of the flattened curve arrives as several touching pieces, so
// glue the pieces back together before measuring anything.
struct DASH_RUNS
{
    std::vector<double> drawn; // length of each dash actually drawn
    std::vector<double> gaps;  // distance from the end of one dash to the start of the next
};


static DASH_RUNS collectRuns( const SHAPE* aShape, LINE_STYLE aStyle, int aWidth,
                              const KIGFX::RENDER_SETTINGS* aSettings )
{
    std::vector<SEG> pieces;

    STROKE_PARAMS::Stroke( aShape, aStyle, aWidth, aSettings,
                           [&pieces]( const VECTOR2I& a, const VECTOR2I& b )
                           {
                               pieces.emplace_back( a, b );
                           } );

    DASH_RUNS runs;

    for( size_t ii = 0; ii < pieces.size(); )
    {
        double   length = VECTOR2D( pieces[ii].B - pieces[ii].A ).EuclideanNorm();
        VECTOR2I end = pieces[ii].B;
        size_t   jj = ii + 1;

        while( jj < pieces.size() && pieces[jj].A == end )
        {
            length += VECTOR2D( pieces[jj].B - pieces[jj].A ).EuclideanNorm();
            end = pieces[jj].B;
            jj++;
        }

        runs.drawn.push_back( length );

        if( jj < pieces.size() )
            runs.gaps.push_back( VECTOR2D( pieces[jj].A - end ).EuclideanNorm() );

        ii = jj;
    }

    return runs;
}


static SHAPE_LINE_CHAIN makeBezierChain()
{
    PCB_SHAPE shape( nullptr, SHAPE_T::BEZIER );

    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( -8 ) } );
    shape.SetBezierC2( { pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 8 ) } );
    shape.SetEnd( { pcbIUScale.mmToIU( 20 ), 0 } );
    shape.RebuildBezierToSegmentsPointsList( ARC_HIGH_DEF );

    return SHAPE_LINE_CHAIN( shape.GetBezierPoints() );
}


// A Bezier is flattened into segments far shorter than one dash, so stroking them one at a
// time drew each of them in full and the curve came out solid.
BOOST_AUTO_TEST_CASE( BezierDashedLineHasGaps )
{
    KIGFX::PCB_RENDER_SETTINGS settings;

    const int        width = pcbIUScale.mmToIU( 0.1 );
    SHAPE_LINE_CHAIN chain = makeBezierChain();

    double curveLength = 0.0;

    for( int ii = 0; ii < chain.SegmentCount(); ++ii )
        curveLength += VECTOR2D( chain.CSegment( ii ).B - chain.CSegment( ii ).A ).EuclideanNorm();

    const double dashLength = settings.GetDashLength( width );
    const double gapLength = settings.GetGapLength( width );

    DASH_RUNS runs = collectRuns( &chain, LINE_STYLE::DASH, width, &settings );

    BOOST_REQUIRE_MESSAGE( runs.drawn.size() > 5,
                           "expected the curve to be broken into dashes, got " << runs.drawn.size() << " run(s)" );

    double drawn = std::accumulate( runs.drawn.begin(), runs.drawn.end(), 0.0 );
    double expected = curveLength * dashLength / ( dashLength + gapLength );

    BOOST_CHECK_MESSAGE( std::abs( drawn - expected ) < 0.02 * curveLength,
                         "dashes should cover " << expected << " IU of the " << curveLength << " IU curve, they cover "
                                                << drawn );
}


// Restarting the pattern on every flattened segment put a dot on every vertex, so the dots
// followed the tessellation instead of being evenly spaced.
BOOST_AUTO_TEST_CASE( BezierDottedLineIsEvenlySpaced )
{
    KIGFX::PCB_RENDER_SETTINGS settings;

    const int        width = pcbIUScale.mmToIU( 0.1 );
    SHAPE_LINE_CHAIN chain = makeBezierChain();

    const double gapLength = settings.GetGapLength( width );

    DASH_RUNS runs = collectRuns( &chain, LINE_STYLE::DOT, width, &settings );

    BOOST_REQUIRE_MESSAGE( runs.gaps.size() > 5, "expected a row of dots, got " << runs.drawn.size() << " run(s)" );

    for( size_t ii = 0; ii < runs.gaps.size(); ++ii )
    {
        BOOST_CHECK_MESSAGE( std::abs( runs.gaps[ii] - gapLength ) < 0.03 * gapLength,
                             "dot " << ii << " sits " << runs.gaps[ii] << " IU from the previous one, expected "
                                    << gapLength );
    }
}


BOOST_AUTO_TEST_SUITE_END()
