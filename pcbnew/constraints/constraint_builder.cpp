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

#include <constraints/constraint_builder.h>

#include <algorithm>
#include <cmath>
#include <ranges>

#include <board.h>
#include <footprint.h>
#include <geometry/eda_angle.h>
#include <geometry/seg.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>


namespace
{
bool isSegment( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_SHAPE_T
            && static_cast<const PCB_SHAPE*>( aItem )->GetShape() == SHAPE_T::SEGMENT;
}


bool isCircleOrArc( const BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_SHAPE_T )
        return false;

    SHAPE_T shape = static_cast<const PCB_SHAPE*>( aItem )->GetShape();
    return shape == SHAPE_T::CIRCLE || shape == SHAPE_T::ARC;
}


bool isArc( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_SHAPE_T
            && static_cast<const PCB_SHAPE*>( aItem )->GetShape() == SHAPE_T::ARC;
}


bool isEllipseKind( const BOARD_ITEM* aItem )
{
    if( aItem->Type() != PCB_SHAPE_T )
        return false;

    SHAPE_T shape = static_cast<const PCB_SHAPE*>( aItem )->GetShape();
    return shape == SHAPE_T::ELLIPSE || shape == SHAPE_T::ELLIPSE_ARC;
}


bool allSegments( const std::vector<BOARD_ITEM*>& aItems )
{
    return std::ranges::all_of( aItems, isSegment );
}


// Circles and arcs have a radius the solver can equate or fix.
bool allRadial( const std::vector<BOARD_ITEM*>& aItems )
{
    return std::ranges::all_of( aItems, isCircleOrArc );
}


// Circles, arcs and ellipses all have a centre the solver can make concentric.
bool allCentered( const std::vector<BOARD_ITEM*>& aItems )
{
    return std::ranges::all_of( aItems,
                                []( const BOARD_ITEM* aItem )
                                {
                                    return isCircleOrArc( aItem ) || isEllipseKind( aItem );
                                } );
}
}


EDA_ANGLE MeasureCornerAngle( const SEG& aA, const SEG& aB )
{
    const VECTOR2I aEnds[2] = { aA.A, aA.B };
    const VECTOR2I bEnds[2] = { aB.A, aB.B };

    // The vertex is the closest endpoint pair; the rays run from it toward each other endpoint.
    int         vA = 0, vB = 0;
    SEG::ecoord best = ( aEnds[0] - bEnds[0] ).SquaredEuclideanNorm();

    for( int i = 0; i < 2; ++i )
    {
        for( int j = 0; j < 2; ++j )
        {
            SEG::ecoord dist = ( aEnds[i] - bEnds[j] ).SquaredEuclideanNorm();

            if( dist < best )
            {
                best = dist;
                vA = i;
                vB = j;
            }
        }
    }

    // Orient both segments from the shared vertex outward so SEG::Angle reads the corner the rays
    // open.  It uses each segment's true direction (not a midpoint ray), so a small gap between the
    // near endpoints does not skew the measurement, and it returns [0, 180] without folding past 90.
    return SEG( aEnds[vA], aEnds[1 - vA] ).Angle( SEG( bEnds[vB], bEnds[1 - vB] ) );
}


std::unique_ptr<PCB_CONSTRAINT> BuildConstraintFromItems( BOARD_ITEM* aParent,
                                                          PCB_CONSTRAINT_TYPE aType,
                                                          const std::vector<BOARD_ITEM*>& aItems )
{
    // Build a constraint of aType with every selected item bound by its WHOLE anchor.
    auto makeWhole = [&]()
    {
        std::unique_ptr<PCB_CONSTRAINT> c = std::make_unique<PCB_CONSTRAINT>( aParent, aType );

        for( BOARD_ITEM* item : aItems )
            c->AddMember( item->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );

        return c;
    };

    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::PARALLEL:
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:
    case PCB_CONSTRAINT_TYPE::COLLINEAR:
    {
        if( aItems.size() != 2 || !allSegments( aItems ) )
            return nullptr;

        return makeWhole();
    }

    case PCB_CONSTRAINT_TYPE::HORIZONTAL:
    case PCB_CONSTRAINT_TYPE::VERTICAL:
    {
        if( aItems.size() != 1 || !isSegment( aItems[0] ) )
            return nullptr;

        return makeWhole();
    }

    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:
    {
        if( aItems.size() != 1 || !isSegment( aItems[0] ) )
            return nullptr;

        const PCB_SHAPE* seg = static_cast<const PCB_SHAPE*>( aItems[0] );

        std::unique_ptr<PCB_CONSTRAINT> c = makeWhole();
        c->SetValue( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::CONCENTRIC:
    {
        if( aItems.size() != 2 || !allCentered( aItems ) )
            return nullptr;

        return makeWhole();
    }

    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:
    {
        if( aItems.size() != 2 || !allRadial( aItems ) )
            return nullptr;

        return makeWhole();
    }

    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION:
    {
        if( aItems.size() != 2 || !allSegments( aItems ) )
            return nullptr;

        const PCB_SHAPE* a = static_cast<const PCB_SHAPE*>( aItems[0] );
        const PCB_SHAPE* b = static_cast<const PCB_SHAPE*>( aItems[1] );

        // A zero-length segment has no direction, so the corner angle is undefined and the solver's
        // angle equation is singular.
        if( a->GetStart() == a->GetEnd() || b->GetStart() == b->GetEnd() )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = makeWhole();
        c->SetValue( MeasureCornerAngle( SEG( a->GetStart(), a->GetEnd() ),
                                         SEG( b->GetStart(), b->GetEnd() ) ).AsDegrees() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
    {
        if( aItems.size() != 1 || !isCircleOrArc( aItems[0] ) )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = makeWhole();
        c->SetValue( static_cast<const PCB_SHAPE*>( aItems[0] )->GetRadius() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::ARC_ANGLE:
    {
        if( aItems.size() != 1 || !isArc( aItems[0] ) )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = makeWhole();
        c->SetValue( static_cast<const PCB_SHAPE*>( aItems[0] )->GetArcAngle().AsDegrees() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::TANGENT:
    {
        if( aItems.size() != 2 )
            return nullptr;

        const BOARD_ITEM* a = aItems[0];
        const BOARD_ITEM* b = aItems[1];

        auto isCurve = []( const BOARD_ITEM* aItem )
        {
            return isCircleOrArc( aItem ) || isEllipseKind( aItem );
        };

        bool lineCurve = ( isSegment( a ) && isCurve( b ) ) || ( isSegment( b ) && isCurve( a ) );
        bool curveCurve = isCircleOrArc( a ) && isCircleOrArc( b );

        if( !lineCurve && !curveCurve )
            return nullptr;

        return makeWhole();
    }

    default:
        // Point-anchored families (coincident, midpoint, symmetric, ...) need point selection,
        // which the whole-shape authoring tool does not yet provide.
        return nullptr;
    }
}


std::vector<CONSTRAINT_ANCHOR_POINT> ConstraintShapeAnchors( const PCB_SHAPE* aShape )
{
    std::vector<CONSTRAINT_ANCHOR_POINT> anchors;

    if( !aShape )
        return anchors;

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
        return { { CONSTRAINT_ANCHOR::START, aShape->GetStart() },
                 { CONSTRAINT_ANCHOR::END, aShape->GetEnd() } };

    case SHAPE_T::ARC:
    case SHAPE_T::ELLIPSE_ARC:
        return { { CONSTRAINT_ANCHOR::START, aShape->GetStart() },
                 { CONSTRAINT_ANCHOR::END, aShape->GetEnd() },
                 { CONSTRAINT_ANCHOR::CENTER, aShape->GetCenter() } };

    case SHAPE_T::CIRCLE:
    case SHAPE_T::ELLIPSE:
        return { { CONSTRAINT_ANCHOR::CENTER, aShape->GetCenter() } };

    default:
        return anchors;
    }
}


std::optional<CONSTRAINT_MEMBER> NearestAnchorAmong( const std::vector<PCB_SHAPE*>& aShapes,
                                                     const VECTOR2I& aPos, double aMaxDist )
{
    double                           best = aMaxDist;
    std::optional<CONSTRAINT_MEMBER> result;

    for( const PCB_SHAPE* shape : aShapes )
    {
        for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintShapeAnchors( shape ) )
        {
            double dist = ( a.pos - aPos ).EuclideanNorm();

            if( dist <= best )
            {
                best = dist;
                result = CONSTRAINT_MEMBER( shape->m_Uuid, a.anchor );
            }
        }
    }

    return result;
}


std::vector<PCB_SHAPE*> CollectConstraintShapes( BOARD* aBoard )
{
    std::vector<PCB_SHAPE*> shapes;

    if( !aBoard )
        return shapes;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
            shapes.push_back( shape );
    }

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
                shapes.push_back( shape );
        }
    }

    return shapes;
}


std::vector<BOARD_ITEM*> CollectConstrainableItems( BOARD* aBoard )
{
    std::vector<BOARD_ITEM*> items;

    if( !aBoard )
        return items;

    auto collect =
            [&]( const auto& aContainer )
            {
                for( BOARD_ITEM* item : aContainer )
                {
                    if( item->Type() == PCB_SHAPE_T || dynamic_cast<PCB_DIMENSION_BASE*>( item ) )
                        items.push_back( item );
                }
            };

    collect( aBoard->Drawings() );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        collect( footprint->GraphicalItems() );

    return items;
}


std::optional<CONSTRAINT_MEMBER> NearestConstraintAnchor( BOARD* aBoard, const VECTOR2I& aPos,
                                                          double aMaxDist )
{
    double                           best = aMaxDist;
    std::optional<CONSTRAINT_MEMBER> result;

    for( BOARD_ITEM* item : CollectConstrainableItems( aBoard ) )
    {
        for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintItemAnchors( item ) )
        {
            double dist = ( a.pos - aPos ).EuclideanNorm();

            if( dist <= best )
            {
                best = dist;
                result = CONSTRAINT_MEMBER( item->m_Uuid, a.anchor );
            }
        }
    }

    return result;
}


BOARD_ITEM* ResolveConstrainableItem( BOARD* aBoard, const KIID& aId )
{
    if( !aBoard )
        return nullptr;

    BOARD_ITEM* item = aBoard->ResolveItem( aId, true );

    return item && ( item->Type() == PCB_SHAPE_T || dynamic_cast<PCB_DIMENSION_BASE*>( item ) )
                   ? item
                   : nullptr;
}


std::vector<CONSTRAINT_ANCHOR_POINT> ConstraintItemAnchors( const BOARD_ITEM* aItem )
{
    if( !aItem )
        return {};

    if( aItem->Type() == PCB_SHAPE_T )
        return ConstraintShapeAnchors( static_cast<const PCB_SHAPE*>( aItem ) );

    if( const PCB_DIMENSION_BASE* dim = dynamic_cast<const PCB_DIMENSION_BASE*>( aItem ) )
    {
        std::vector<CONSTRAINT_ANCHOR_POINT> anchors;
        anchors.push_back( { CONSTRAINT_ANCHOR::START, dim->GetStart() } );

        // Only aligned/orthogonal/radial dimensions have a second measured feature point; a leader
        // or centre mark's second point is a control point.
        switch( aItem->Type() )
        {
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_RADIAL_T:
            anchors.push_back( { CONSTRAINT_ANCHOR::END, dim->GetEnd() } );
            break;

        default:
            break;
        }

        return anchors;
    }

    return {};
}


std::optional<VECTOR2I> ConstraintAnchorPosition( BOARD* aBoard, const CONSTRAINT_MEMBER& aMember )
{
    for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintItemAnchors( ResolveConstrainableItem( aBoard, aMember.m_item ) ) )
    {
        if( a.anchor == aMember.m_anchor )
            return a.pos;
    }

    return std::nullopt;
}


std::optional<KIID> NearestConstrainedShape( const std::vector<PCB_SHAPE*>& aCandidates,
                                             const VECTOR2I& aPos, int aMaxDist )
{
    auto it = std::ranges::find_if( aCandidates,
                                    [&]( const PCB_SHAPE* aShape )
                                    {
                                        return aShape && aShape->HitTest( aPos, aMaxDist );
                                    } );

    return it == aCandidates.end() ? std::nullopt : std::optional<KIID>( ( *it )->m_Uuid );
}
