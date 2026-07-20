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
#include <core/kicad_algo.h>
#include <footprint.h>
#include <geometry/eda_angle.h>
#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
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
    case SHAPE_T::BEZIER:
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

    case SHAPE_T::RECTANGLE:
    {
        // TL TR BR BL order must match frozen corner roles of adapter
        VECTOR2I s = aShape->GetStart();
        VECTOR2I e = aShape->GetEnd();
        VECTOR2I tl( std::min( s.x, e.x ), std::min( s.y, e.y ) );
        VECTOR2I br( std::max( s.x, e.x ), std::max( s.y, e.y ) );

        return { { CONSTRAINT_ANCHOR::VERTEX, tl, 0 },
                 { CONSTRAINT_ANCHOR::VERTEX, VECTOR2I( br.x, tl.y ), 1 },
                 { CONSTRAINT_ANCHOR::VERTEX, br, 2 },
                 { CONSTRAINT_ANCHOR::VERTEX, VECTOR2I( tl.x, br.y ), 3 } };
    }

    case SHAPE_T::POLY:
    {
        // Same eligibility gate as adapter ingestion so picker never offers unmappable anchor
        if( !ConstraintPolygonIsModelable( aShape ) )
            return anchors;

        const SHAPE_LINE_CHAIN& outline = aShape->GetPolyShape().COutline( 0 );

        for( int i = 0; i < outline.PointCount(); ++i )
            anchors.push_back( { CONSTRAINT_ANCHOR::VERTEX, outline.CPoint( i ), i } );

        return anchors;
    }

    default:
        return anchors;
    }
}


bool ConstraintPolygonIsModelable( const PCB_SHAPE* aShape )
{
    if( !aShape || aShape->GetShape() != SHAPE_T::POLY )
        return false;

    const SHAPE_POLY_SET& polySet = aShape->GetPolyShape();

    if( polySet.OutlineCount() != 1 || polySet.HoleCount( 0 ) > 0 )
        return false;

    const SHAPE_LINE_CHAIN& outline = polySet.COutline( 0 );

    return outline.PointCount() > 0 && outline.ArcCount() == 0;
}


std::optional<CONSTRAINT_ANCHOR_POINT> ConstraintShapeVertex( const PCB_SHAPE* aShape, int aIndex )
{
    if( !aShape || aIndex < 0 )
        return std::nullopt;

    if( aShape->GetShape() != SHAPE_T::RECTANGLE && aShape->GetShape() != SHAPE_T::POLY )
        return std::nullopt;

    std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( aShape );

    if( aIndex >= (int) anchors.size() )
        return std::nullopt;

    return anchors[aIndex];
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
                result = CONSTRAINT_MEMBER( shape->m_Uuid, a.anchor, a.index );
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
                                                          double aMaxDist,
                                                          const std::vector<CONSTRAINT_MEMBER>& aExclude )
{
    double                           best = aMaxDist;
    std::optional<CONSTRAINT_MEMBER> result;

    for( BOARD_ITEM* item : CollectConstrainableItems( aBoard ) )
    {
        for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintItemAnchors( item ) )
        {
            CONSTRAINT_MEMBER candidate( item->m_Uuid, a.anchor, a.index );

            // Skip already picked handle so distinct coincident endpoint stays reachable
            if( alg::contains( aExclude, candidate ) )
                continue;

            double dist = ( a.pos - aPos ).EuclideanNorm();

            if( dist <= best )
            {
                best = dist;
                result = candidate;
            }
        }
    }

    return result;
}


std::vector<DIMENSION_ENDPOINT_BINDING>
SelectDimensionEndpointBindings( BOARD* aBoard, const KIID& aDimension, const VECTOR2I& aStart,
                                 const std::optional<VECTOR2I>& aEnd, double aMaxDist )
{
    std::vector<DIMENSION_ENDPOINT_BINDING> bindings;

    if( !aBoard )
        return bindings;

    // Best pair of distinct anchors on one item within aMaxDist minimizing summed distance
    // Distinct anchors required or endpoints merge and pairs judged jointly not per end nearest
    using ANCHOR_PAIR = std::pair<CONSTRAINT_MEMBER, CONSTRAINT_MEMBER>;

    auto bestPairOn = [&]( BOARD_ITEM* aItem ) -> std::optional<std::pair<ANCHOR_PAIR, double>>
    {
        std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintItemAnchors( aItem );

        // Sum decomposes per endpoint best and runner up END anchors computed once serve every
        // START candidate keeps a dense polygon linear in vertex count instead of quadratic
        const size_t        none = anchors.size();
        size_t              bestEnd = none;
        size_t              secondEnd = none;
        std::vector<double> dEnd( anchors.size(), 0.0 );

        for( size_t j = 0; j < anchors.size(); ++j )
        {
            dEnd[j] = ( anchors[j].pos - *aEnd ).EuclideanNorm();

            if( dEnd[j] > aMaxDist )
                continue;

            if( bestEnd == none || dEnd[j] < dEnd[bestEnd] )
            {
                secondEnd = bestEnd;
                bestEnd = j;
            }
            else if( secondEnd == none || dEnd[j] < dEnd[secondEnd] )
            {
                secondEnd = j;
            }
        }

        if( bestEnd == none )
            return std::nullopt;

        std::optional<std::pair<ANCHOR_PAIR, double>> best;

        for( size_t i = 0; i < anchors.size(); ++i )
        {
            double dStart = ( anchors[i].pos - aStart ).EuclideanNorm();

            if( dStart > aMaxDist )
                continue;

            size_t j = ( i == bestEnd ) ? secondEnd : bestEnd;

            if( j == none )
                continue;

            double sum = dStart + dEnd[j];

            if( !best || sum < best->second )
            {
                best = std::make_pair(
                        ANCHOR_PAIR{ CONSTRAINT_MEMBER( aItem->m_Uuid, anchors[i].anchor,
                                                        anchors[i].index ),
                                     CONSTRAINT_MEMBER( aItem->m_Uuid, anchors[j].anchor,
                                                        anchors[j].index ) },
                        sum );
            }
        }

        return best;
    };

    // Prefer single object reaching both endpoints so a single feature dimension stays bound at
    // both ends
    if( aEnd )
    {
        std::optional<ANCHOR_PAIR> bestPair;
        double                     bestSum = 0.0;

        for( BOARD_ITEM* item : CollectConstrainableItems( aBoard ) )
        {
            if( item->m_Uuid == aDimension )
                continue;

            auto pair = bestPairOn( item );

            if( !pair )
                continue;

            if( !bestPair || pair->second < bestSum )
            {
                bestSum = pair->second;
                bestPair = pair->first;
            }
        }

        if( bestPair )
        {
            bindings.push_back( { CONSTRAINT_ANCHOR::START, bestPair->first } );
            bindings.push_back( { CONSTRAINT_ANCHOR::END, bestPair->second } );
            return bindings;
        }
    }

    // Else bind each endpoint to its own nearest anchor the two may land on different objects
    // and either may find nothing
    std::vector<CONSTRAINT_MEMBER> exclude{ { aDimension, CONSTRAINT_ANCHOR::START },
                                            { aDimension, CONSTRAINT_ANCHOR::END } };

    if( auto startTarget = NearestConstraintAnchor( aBoard, aStart, aMaxDist, exclude ) )
    {
        bindings.push_back( { CONSTRAINT_ANCHOR::START, *startTarget } );
        exclude.push_back( *startTarget );
    }

    if( aEnd )
    {
        if( auto endTarget = NearestConstraintAnchor( aBoard, *aEnd, aMaxDist, exclude ) )
            bindings.push_back( { CONSTRAINT_ANCHOR::END, *endTarget } );
    }

    return bindings;
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
        // VERTEX anchor needs its ordinal too else every vertex member resolves to vertex 0
        if( a.anchor == aMember.m_anchor
            && ( a.anchor != CONSTRAINT_ANCHOR::VERTEX || a.index == aMember.m_index ) )
        {
            return a.pos;
        }
    }

    return std::nullopt;
}


double InitialConstraintValue( PCB_CONSTRAINT_TYPE aType, double aMeasured,
                               const std::map<PCB_CONSTRAINT_TYPE, double>& aRemembered )
{
    auto it = aRemembered.find( aType );

    return it != aRemembered.end() ? it->second : aMeasured;
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


std::optional<KIID> SelectRadialDimensionTarget( BOARD* aBoard, const KIID& aDimension,
                                                 const VECTOR2I& aCenter, const VECTOR2I& aRim,
                                                 double aMaxDist )
{
    if( !aBoard )
        return std::nullopt;

    std::optional<KIID> best;
    double              bestErr = 0.0;

    for( PCB_SHAPE* shape : CollectConstraintShapes( aBoard ) )
    {
        if( shape->m_Uuid == aDimension || !isCircleOrArc( shape ) )
            continue;

        // Centre and rim must land on the same circle or arc centre and circumference or else a
        // radial dimension over unrelated geometry would bind spuriously
        std::optional<VECTOR2I> centerPos;

        for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintShapeAnchors( shape ) )
        {
            if( a.anchor == CONSTRAINT_ANCHOR::CENTER )
                centerPos = a.pos;
        }

        if( !centerPos )
            continue;

        double centerErr = ( *centerPos - aCenter ).EuclideanNorm();

        if( centerErr > aMaxDist )
            continue;

        double rimErr = std::abs( ( aRim - *centerPos ).EuclideanNorm() - shape->GetRadius() );

        if( rimErr > aMaxDist )
            continue;

        // Arc outline is swept portion only not the whole circle so a rim point off the arc must
        // not bind
        if( shape->GetShape() == SHAPE_T::ARC && !shape->HitTest( aRim, KiROUND( aMaxDist ) ) )
            continue;

        double err = centerErr + rimErr;

        if( !best || err < bestErr )
        {
            bestErr = err;
            best = shape->m_Uuid;
        }
    }

    return best;
}


bool DimensionEndpointsBound( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension )
{
    if( !aBoard || !aDimension )
        return false;

    const CONSTRAINT_MEMBER startMember( aDimension->m_Uuid, CONSTRAINT_ANCHOR::START );
    const CONSTRAINT_MEMBER endMember( aDimension->m_Uuid, CONSTRAINT_ANCHOR::END );

    auto anyConstraint = [&]( const auto& aMatch )
    {
        if( std::ranges::any_of( aBoard->Constraints(), aMatch ) )
            return true;

        // Bindings are parented to the owning dimension footprint not necessarily the first so
        // every footprint must be scanned to match the write path
        return std::ranges::any_of( aBoard->Footprints(),
                                    [&]( const FOOTPRINT* aFootprint )
                                    { return std::ranges::any_of( aFootprint->Constraints(), aMatch ); } );
    };

    // Radial dimension binds centre coincident plus rim on outline of one circle or arc
    // Legs on different objects or an object that cannot play the radius role never offer Driving
    if( aDimension->Type() == PCB_DIM_RADIAL_T )
    {
        auto rimOnItem = [&]( const KIID& aItem )
        {
            return anyConstraint(
                    [&]( const PCB_CONSTRAINT* aConstraint )
                    {
                        if( aConstraint->GetConstraintType() != PCB_CONSTRAINT_TYPE::POINT_ON_LINE )
                            return false;

                        // Point on line binding is asymmetric the dimension rim point is member 0
                        // and the object outline WHOLE anchor is member 1
                        const std::vector<CONSTRAINT_MEMBER>& members = aConstraint->GetMembers();

                        return members.size() == 2 && members[0] == endMember
                               && members[1] == CONSTRAINT_MEMBER( aItem, CONSTRAINT_ANCHOR::WHOLE );
                    } );
        };

        return anyConstraint(
                [&]( const PCB_CONSTRAINT* aConstraint )
                {
                    if( aConstraint->GetConstraintType() != PCB_CONSTRAINT_TYPE::COINCIDENT )
                        return false;

                    const std::vector<CONSTRAINT_MEMBER>& members = aConstraint->GetMembers();

                    if( members.size() != 2 )
                        return false;

                    // Authored dimension first but coincident is symmetric so accept either order
                    const CONSTRAINT_MEMBER* target = nullptr;

                    if( members[0] == startMember )
                        target = &members[1];
                    else if( members[1] == startMember )
                        target = &members[0];

                    if( !target || target->m_anchor != CONSTRAINT_ANCHOR::CENTER )
                        return false;

                    BOARD_ITEM* item = ResolveConstrainableItem( aBoard, target->m_item );

                    return item && isCircleOrArc( item ) && rimOnItem( target->m_item );
                } );
    }

    // Aligned or orthogonal needs a coincident per endpoint whose target still resolves a target
    // pointing at a deleted item or a stale vertex index does not count
    auto hasCoincident = [&]( const CONSTRAINT_MEMBER& aMember )
    {
        return anyConstraint(
                [&]( const PCB_CONSTRAINT* aConstraint )
                {
                    if( aConstraint->GetConstraintType() != PCB_CONSTRAINT_TYPE::COINCIDENT )
                        return false;

                    const std::vector<CONSTRAINT_MEMBER>& members = aConstraint->GetMembers();

                    // Must pair with a distinct target not itself
                    if( members.size() != 2 || members[0].m_item == members[1].m_item )
                        return false;

                    if( members[0] == aMember )
                        return ConstraintAnchorPosition( aBoard, members[1] ).has_value();

                    return members[1] == aMember
                           && ConstraintAnchorPosition( aBoard, members[0] ).has_value();
                } );
    };

    return hasCoincident( startMember ) && hasCoincident( endMember );
}


bool DimensionHasValueMode( const PCB_DIMENSION_BASE* aDimension )
{
    if( !aDimension )
        return false;

    switch( aDimension->Type() )
    {
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_RADIAL_T:
        return true;

    default:
        return false;
    }
}


PCB_CONSTRAINT* FindDimensionLengthConstraint( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension )
{
    if( !aBoard || !aDimension )
        return nullptr;

    const CONSTRAINT_MEMBER startMember( aDimension->m_Uuid, CONSTRAINT_ANCHOR::START );
    const CONSTRAINT_MEMBER endMember( aDimension->m_Uuid, CONSTRAINT_ANCHOR::END );

    auto matches = [&]( const PCB_CONSTRAINT* aConstraint )
    {
        if( aConstraint->GetConstraintType() != PCB_CONSTRAINT_TYPE::FIXED_LENGTH )
            return false;

        const std::vector<CONSTRAINT_MEMBER>& members = aConstraint->GetMembers();

        return members.size() == 2
               && ( ( members[0] == startMember && members[1] == endMember )
                    || ( members[0] == endMember && members[1] == startMember ) );
    };

    auto scan = [&]( const CONSTRAINTS& aList ) -> PCB_CONSTRAINT*
    {
        auto it = std::ranges::find_if( aList, matches );
        return it != aList.end() ? *it : nullptr;
    };

    if( PCB_CONSTRAINT* c = scan( aBoard->Constraints() ) )
        return c;

    // Driving length is parented to the owning dimension footprint not necessarily the first so
    // scan every footprint to match the write
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        if( PCB_CONSTRAINT* c = scan( footprint->Constraints() ) )
            return c;
    }

    return nullptr;
}


bool DimensionCanDrive( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension )
{
    if( !DimensionHasValueMode( aDimension ) )
        return false;

    if( DimensionEndpointsBound( aBoard, aDimension ) )
        return true;

    PCB_CONSTRAINT* existing = FindDimensionLengthConstraint( aBoard, aDimension );

    return existing && existing->IsDriving();
}


DIM_VALUE_MODE DimensionValueMode( BOARD* aBoard, const PCB_DIMENSION_BASE* aDimension )
{
    PCB_CONSTRAINT* lengthConstraint = FindDimensionLengthConstraint( aBoard, aDimension );

    if( lengthConstraint && lengthConstraint->IsDriving() )
        return DIM_VALUE_MODE::DRIVING;

    if( aDimension && aDimension->GetOverrideTextEnabled() )
        return DIM_VALUE_MODE::ARBITRARY;

    return DIM_VALUE_MODE::DRIVEN;
}


PCB_CONSTRAINT* SetDimensionValueMode( BOARD* aBoard, PCB_DIMENSION_BASE* aDimension, DIM_VALUE_MODE aMode,
                                       std::optional<int>                        aDrivingLengthIU,
                                       const std::optional<wxString>&            aOverrideText,
                                       const std::function<void( BOARD_ITEM* )>& aBeforeModify,
                                       const std::function<void( BOARD_ITEM* )>& aStageAdd,
                                       const std::function<void( BOARD_ITEM* )>& aBeforeRemove )
{
    if( !aBoard || !DimensionHasValueMode( aDimension ) )
        return nullptr;

    PCB_CONSTRAINT* existing = FindDimensionLengthConstraint( aBoard, aDimension );

    if( aMode == DIM_VALUE_MODE::DRIVING )
    {
        // Unbound dimension has no geometry to drive and a non positive length would collapse the
        // constraint so the transition rejects with the board untouched
        if( !aDrivingLengthIU || *aDrivingLengthIU <= 0 || !DimensionCanDrive( aBoard, aDimension ) )
            return nullptr;

        aBeforeModify( aDimension );
        aDimension->SetOverrideTextEnabled( false );
        aDimension->Update();

        if( existing )
        {
            aBeforeModify( existing );
            existing->SetValue( *aDrivingLengthIU );
            existing->SetDriving( true );
            return existing;
        }

        BOARD_ITEM* parent = aDimension->GetParentFootprint()
                                     ? static_cast<BOARD_ITEM*>( aDimension->GetParentFootprint() )
                                     : static_cast<BOARD_ITEM*>( aBoard );

        auto constraint = std::make_unique<PCB_CONSTRAINT>( parent, PCB_CONSTRAINT_TYPE::FIXED_LENGTH );
        constraint->AddMember( aDimension->m_Uuid, CONSTRAINT_ANCHOR::START );
        constraint->AddMember( aDimension->m_Uuid, CONSTRAINT_ANCHOR::END );
        constraint->SetValue( *aDrivingLengthIU );
        constraint->SetDriving( true );

        PCB_CONSTRAINT* added = constraint.get();
        aStageAdd( constraint.release() );
        return added;
    }

    aBeforeModify( aDimension );
    aDimension->SetOverrideTextEnabled( aMode == DIM_VALUE_MODE::ARBITRARY );

    if( aMode == DIM_VALUE_MODE::ARBITRARY && aOverrideText )
        aDimension->SetOverrideText( *aOverrideText );

    aDimension->Update();

    // Driven and Arbitrary both measure geometry natively so any driving length is dropped
    if( existing )
        aBeforeRemove( existing );

    return nullptr;
}


void RemapPolygonVertexMembers( BOARD* aBoard, const KIID& aPoly, int aChangedIndex, int aDelta,
                                const std::function<void( BOARD_ITEM* )>& aBeforeModify,
                                const std::function<void( BOARD_ITEM* )>& aBeforeRemove )
{
    if( !aBoard || aDelta == 0 )
        return;

    auto remapIn = [&]( const CONSTRAINTS& aConstraints )
    {
        for( PCB_CONSTRAINT* constraint : aConstraints )
        {
            bool shifts = false;
            bool doomed = false;

            for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
            {
                if( member.m_item != aPoly || member.m_anchor != CONSTRAINT_ANCHOR::VERTEX )
                    continue;

                if( aDelta < 0 && member.m_index == aChangedIndex )
                    doomed = true;
                else if( member.m_index >= aChangedIndex )
                    shifts = true;
            }

            // Deleted vertex drags its bound member down and no fixed arity solver form survives
            // losing one so the whole constraint retires left unedited the staged removal image
            // keeps the authored members for undo
            if( doomed )
            {
                aBeforeRemove( constraint );
                continue;
            }

            if( !shifts )
                continue;

            aBeforeModify( constraint );

            for( CONSTRAINT_MEMBER& member : constraint->Members() )
            {
                if( member.m_item == aPoly && member.m_anchor == CONSTRAINT_ANCHOR::VERTEX
                        && member.m_index >= aChangedIndex )
                {
                    member.m_index += aDelta;
                }
            }
        }
    };

    remapIn( aBoard->Constraints() );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        remapIn( footprint->Constraints() );
}
