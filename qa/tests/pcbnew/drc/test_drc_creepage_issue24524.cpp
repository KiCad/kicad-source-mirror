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

/**
 * @file test_drc_creepage_issue24524.cpp
 *
 * Regression test for issue #24524: creepage DRC computed an incorrect path
 * around the rounded end of a fully-rounded (stadium) rectangle slot on
 * Edge.Cuts. A horizontal stadium models its rounded ends as semicircular
 * caps; if a cap is swept over the inner half of its circle it bulges into the
 * slot instead of away from it, and the creepage path then cuts through the
 * wrong side of the rounded end.
 *
 * These tests drive the two production code paths that consume the cap
 * geometry: SegmentIntersectsBoard (validates candidate paths) and
 * CREEPAGE_GRAPH::TransformEdgeToCreepShapes (builds the boundary arcs the
 * graph routes around). Both must place the caps on the outer side.
 */

#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <board_item.h>
#include <pcb_shape.h>
#include <base_units.h>
#include <geometry/eda_angle.h>

#include <drc/drc_creepage_utils.h>


namespace
{
// Build a horizontal stadium slot: 20 mm wide, 2 mm tall, 1 mm corner radius.
// The left cap is centered at (1, 1) mm and the right cap at (19, 1) mm.
PCB_SHAPE MakeHorizontalStadium()
{
    PCB_SHAPE slot( nullptr, SHAPE_T::RECTANGLE );

    slot.SetStart( VECTOR2I( 0, 0 ) );
    slot.SetEnd( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 2 ) ) );
    slot.SetCornerRadius( pcbIUScale.mmToIU( 1 ) );

    return slot;
}
} // namespace


// A path segment crossing the outer apex of a cap must be reported as crossing
// the board edge (return value false), while a segment grazing the inner half
// of the same circle stays inside the slot and must read as clear (true).
BOOST_AUTO_TEST_CASE( CreepageStadiumCapsBlockOuterSide )
{
    PCB_SHAPE                       slot = MakeHorizontalStadium();
    std::vector<BOARD_ITEM*>        edges = { &slot };
    std::vector<const BOARD_ITEM*>  dontTest;

    const int oneMM = pcbIUScale.mmToIU( 1 );
    const int halfMM = pcbIUScale.mmToIU( 0.5 );

    // Left cap apex sits at (0, 1) mm. A horizontal probe through it must be blocked.
    VECTOR2I leftOuterA( -halfMM, oneMM );
    VECTOR2I leftOuterB( halfMM, oneMM );
    BOOST_CHECK( !SegmentIntersectsBoard( leftOuterA, leftOuterB, edges, dontTest, 0 ) );

    // Right cap apex sits at (20, 1). A horizontal probe through it must be blocked.
    VECTOR2I rightOuterA( pcbIUScale.mmToIU( 19.5 ), oneMM );
    VECTOR2I rightOuterB( pcbIUScale.mmToIU( 20.5 ), oneMM );
    BOOST_CHECK( !SegmentIntersectsBoard( rightOuterA, rightOuterB, edges, dontTest, 0 ) );

    // A probe across the inner half of the left cap's circle (around (2, 1)) lies
    // inside the slot, away from any real boundary, so it must read as clear. A cap
    // mistakenly swept over the inner half would model a boundary here and block it.
    VECTOR2I innerA( pcbIUScale.mmToIU( 1.5 ), oneMM );
    VECTOR2I innerB( pcbIUScale.mmToIU( 2.5 ), oneMM );
    BOOST_CHECK( SegmentIntersectsBoard( innerA, innerB, edges, dontTest, 0 ) );
}


// The decomposition feeding the routing graph must emit two cap arcs whose
// swept midpoints fall on the outer side of each cap center.
BOOST_AUTO_TEST_CASE( CreepageStadiumCapsArcOutward )
{
    BOARD          board;
    CREEPAGE_GRAPH graph( board );
    PCB_SHAPE      slot = MakeHorizontalStadium();

    graph.m_boardEdge.push_back( &slot );
    graph.TransformEdgeToCreepShapes();

    std::vector<CREEP_SHAPE*> arcs;

    for( CREEP_SHAPE* shape : graph.m_shapeCollection )
    {
        if( shape && shape->GetType() == CREEP_SHAPE::TYPE::ARC )
            arcs.push_back( shape );
    }

    BOOST_REQUIRE_EQUAL( arcs.size(), 2u );

    const int oneMM = pcbIUScale.mmToIU( 1 );

    for( CREEP_SHAPE* arc : arcs )
    {
        VECTOR2I  center = arc->GetPos();
        EDA_ANGLE midAngle = ( arc->GetStartAngle() + arc->GetEndAngle() ) / 2.0;
        double    apexX = center.x + arc->GetRadius() * midAngle.Cos();

        BOOST_CHECK_EQUAL( arc->GetRadius(), oneMM );

        if( center.x < pcbIUScale.mmToIU( 10 ) )
            BOOST_CHECK_LT( apexX, center.x ); // left cap bulges left
        else
            BOOST_CHECK_GT( apexX, center.x ); // right cap bulges right
    }
}
