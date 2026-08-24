/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <base_units.h>

#include <numeric>
#include <random>


BOOST_AUTO_TEST_SUITE( FootprintCourtyardChaining )


struct COURTYARD_SEGMENT
{
    double m_startX;
    double m_startY;
    double m_endX;
    double m_endY;
};


struct COURTYARD_ARC
{
    double m_startX;
    double m_startY;
    double m_midX;
    double m_midY;
    double m_endX;
    double m_endY;
};


/*
 * L5 (Don-Inductor:L_Wuerth_WE-HCI_Flat_1040) from the ticket 2491 board.  Four segments are
 * shorter than the 0.02mm chaining epsilon.
 */
static const std::vector<COURTYARD_SEGMENT> l5Segments = {
    { -6.025, -1.500000, -6.025, 1.500000 },
    { -5.502647, -2.214616, -5.500000, -2.225000 },
    { -5.502647, 2.214616, -5.500000, 2.225000 },
    { -5.500000, -5.250000, -5.500000, -2.225000 },
    { -5.500000, 5.250000, -5.500000, 2.225000 },
    { -5.250000, -5.500000, 5.250000, -5.500000 },
    { -5.250000, 5.500000, 5.250000, 5.500000 },
    { 5.500000, -5.250000, 5.500000, -2.221321 },
    { 5.500000, -2.221321, 5.502647, -2.214616 },
    { 5.500000, 5.250000, 5.500000, 2.225000 },
    { 5.502647, 2.214616, 5.500000, 2.225000 },
    { 6.025, 1.500000, 6.025, -1.500000 },
};


static const std::vector<COURTYARD_ARC> l5Arcs = {
    { -6.025000, -1.500000, -5.880489, -1.942586, -5.502647, -2.214616 },
    { -5.502647, 2.214616, -5.880490, 1.942586, -6.025000, 1.500000 },
    { -5.500000, -5.250000, -5.426777, -5.426777, -5.250000, -5.500000 },
    { -5.250000, 5.500000, -5.426777, 5.426777, -5.500000, 5.250000 },
    { 5.250000, -5.500000, 5.426777, -5.426777, 5.500000, -5.250000 },
    { 5.500000, 5.250000, 5.426777, 5.426777, 5.250000, 5.500000 },
    { 5.502647, -2.214616, 5.880489, -1.942586, 6.025000, -1.500000 },
    { 6.025000, 1.500000, 5.880490, 1.942586, 5.502647, 2.214616 },
};


static VECTOR2I mmPoint( double aX, double aY )
{
    return VECTOR2I( pcbIUScale.mmToIU( aX ), pcbIUScale.mmToIU( aY ) );
}


static void buildShapes( FOOTPRINT& aFootprint, const std::vector<COURTYARD_SEGMENT>& aSegments,
                         const std::vector<COURTYARD_ARC>& aArcs, const std::vector<size_t>& aOrder )
{
    std::vector<PCB_SHAPE*> shapes;

    for( const COURTYARD_SEGMENT& seg : aSegments )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( &aFootprint, SHAPE_T::SEGMENT );
        shape->SetLayer( B_CrtYd );
        shape->SetStart( mmPoint( seg.m_startX, seg.m_startY ) );
        shape->SetEnd( mmPoint( seg.m_endX, seg.m_endY ) );
        shape->SetWidth( pcbIUScale.mmToIU( 0.05 ) );
        shapes.push_back( shape );
    }

    for( const COURTYARD_ARC& arc : aArcs )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( &aFootprint, SHAPE_T::ARC );
        shape->SetLayer( B_CrtYd );
        shape->SetArcGeometry( mmPoint( arc.m_startX, arc.m_startY ), mmPoint( arc.m_midX, arc.m_midY ),
                               mmPoint( arc.m_endX, arc.m_endY ) );
        shape->SetWidth( pcbIUScale.mmToIU( 0.05 ) );
        shapes.push_back( shape );
    }

    for( size_t idx : aOrder )
        aFootprint.Add( shapes[idx] );
}


static void checkCourtyardCloses( const std::vector<COURTYARD_SEGMENT>& aSegments,
                                  const std::vector<COURTYARD_ARC>&     aArcs,
                                  const std::vector<size_t>&            aOrder )
{
    BOARD     board;
    FOOTPRINT footprint( &board );

    buildShapes( footprint, aSegments, aArcs, aOrder );
    footprint.BuildCourtyardCaches();

    BOOST_CHECK_EQUAL( footprint.GetFlags() & MALFORMED_COURTYARDS, 0u );
    BOOST_CHECK_EQUAL( footprint.GetCourtyard( B_CrtYd ).OutlineCount(), 1 );
}


/**
 * The courtyard closes whatever order its shapes arrive in.
 *
 * Shape order used to track allocation addresses, so ticket 2491 saw the error come and go.
 */
BOOST_AUTO_TEST_CASE( ClosesFromEverySeedShape )
{
    const size_t shapeCount = l5Segments.size() + l5Arcs.size();

    std::vector<size_t> order( shapeCount );
    std::iota( order.begin(), order.end(), 0 );

    // Each rotation seeds from a different shape
    for( size_t rotation = 0; rotation < shapeCount; ++rotation )
    {
        BOOST_TEST_CONTEXT( "rotation " << rotation )
        checkCourtyardCloses( l5Segments, l5Arcs, order );

        std::rotate( order.begin(), order.begin() + 1, order.end() );
    }
}


BOOST_AUTO_TEST_CASE( ClosesInArbitraryShapeOrder )
{
    const size_t shapeCount = l5Segments.size() + l5Arcs.size();

    std::vector<size_t> order( shapeCount );
    std::iota( order.begin(), order.end(), 0 );

    // Rotations only move the seed, so shuffle the whole list
    std::mt19937 rng( 42 );

    for( size_t trial = 0; trial < 64; ++trial )
    {
        std::shuffle( order.begin(), order.end(), rng );

        BOOST_TEST_CONTEXT( "trial " << trial )
        checkCourtyardCloses( l5Segments, l5Arcs, order );
    }
}


BOOST_AUTO_TEST_SUITE_END()
