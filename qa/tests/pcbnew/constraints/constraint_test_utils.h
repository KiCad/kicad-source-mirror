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

#ifndef QA_PCBNEW_CONSTRAINT_TEST_UTILS_H
#define QA_PCBNEW_CONSTRAINT_TEST_UTILS_H

#include <optional>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pcb_shape.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>

namespace KI_TEST
{
constexpr int MM = 1000000;   // 1 mm in IU (nanometres)


inline PCB_SHAPE* addSegment( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    PCB_SHAPE* seg = new PCB_SHAPE( &aBoard, SHAPE_T::SEGMENT );
    seg->SetStart( aStart );
    seg->SetEnd( aEnd );
    aBoard.Add( seg );
    return seg;
}


inline PCB_SHAPE* addCircle( BOARD& aBoard, const VECTOR2I& aCenter, int aRadius )
{
    PCB_SHAPE* circle = new PCB_SHAPE( &aBoard, SHAPE_T::CIRCLE );
    circle->SetCenter( aCenter );
    circle->SetRadius( aRadius );
    aBoard.Add( circle );
    return circle;
}


inline PCB_SHAPE* addArc( BOARD& aBoard, const VECTOR2I& aStart, const VECTOR2I& aMid,
                          const VECTOR2I& aEnd )
{
    PCB_SHAPE* arc = new PCB_SHAPE( &aBoard, SHAPE_T::ARC );
    arc->SetArcGeometry( aStart, aMid, aEnd );
    aBoard.Add( arc );
    return arc;
}


inline PCB_CONSTRAINT* addConstraint( BOARD& aBoard, PCB_CONSTRAINT_TYPE aType,
                                      const std::vector<CONSTRAINT_MEMBER>& aMembers,
                                      std::optional<double> aValue = std::nullopt )
{
    PCB_CONSTRAINT* c = new PCB_CONSTRAINT( &aBoard, aType );

    for( const CONSTRAINT_MEMBER& m : aMembers )
        c->AddMember( m.m_item, m.m_anchor );

    c->SetValue( aValue );
    aBoard.Add( c );
    return c;
}


/// Build the cluster of @p aShapes against every constraint on @p aBoard, solve it, and write the
/// result back -- the common adapter-test happy path, which REQUIREs the build and solve to succeed.
inline void solveAndApply( BOARD& aBoard, const std::vector<PCB_SHAPE*>& aShapes )
{
    std::vector<PCB_CONSTRAINT*> constraints( aBoard.Constraints().begin(), aBoard.Constraints().end() );

    BOARD_CONSTRAINT_ADAPTER adapter;
    BOOST_REQUIRE( adapter.Build( aShapes, constraints ) );
    BOOST_REQUIRE( adapter.Solve() );
    adapter.Apply();
}

} // namespace KI_TEST

#endif // QA_PCBNEW_CONSTRAINT_TEST_UTILS_H
