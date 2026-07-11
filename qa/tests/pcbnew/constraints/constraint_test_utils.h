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

#include <cmath>
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


inline PCB_SHAPE* addEllipse( BOARD& aBoard, const VECTOR2I& aCenter, int aMajor, int aMinor,
                              const EDA_ANGLE& aRotation )
{
    PCB_SHAPE* ellipse = new PCB_SHAPE( &aBoard, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( aCenter );
    ellipse->SetEllipseMajorRadius( aMajor );
    ellipse->SetEllipseMinorRadius( aMinor );
    ellipse->SetEllipseRotation( aRotation );
    aBoard.Add( ellipse );
    return ellipse;
}


inline PCB_SHAPE* addEllipseArc( BOARD& aBoard, const VECTOR2I& aCenter, int aMajor, int aMinor,
                                 const EDA_ANGLE& aRotation, const EDA_ANGLE& aStart, const EDA_ANGLE& aEnd )
{
    PCB_SHAPE* arc = new PCB_SHAPE( &aBoard, SHAPE_T::ELLIPSE_ARC );
    arc->SetEllipseCenter( aCenter );
    arc->SetEllipseMajorRadius( aMajor );
    arc->SetEllipseMinorRadius( aMinor );
    arc->SetEllipseRotation( aRotation );
    arc->SetEllipseStartAngle( aStart );
    arc->SetEllipseEndAngle( aEnd );
    aBoard.Add( arc );
    return arc;
}


/// Squared-normalized ellipse equation value at aPos: 1.0 exactly on the outline.
inline double ellipseEquationAt( const PCB_SHAPE* aEllipse, const VECTOR2I& aPos )
{
    double   a = aEllipse->GetEllipseMajorRadius();
    double   b = aEllipse->GetEllipseMinorRadius();
    double   phi = aEllipse->GetEllipseRotation().AsRadians();
    VECTOR2D d = VECTOR2D( aPos - aEllipse->GetEllipseCenter() );
    double   lx = d.x * std::cos( phi ) + d.y * std::sin( phi );
    double   ly = -d.x * std::sin( phi ) + d.y * std::cos( phi );

    return ( lx / a ) * ( lx / a ) + ( ly / b ) * ( ly / b );
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
