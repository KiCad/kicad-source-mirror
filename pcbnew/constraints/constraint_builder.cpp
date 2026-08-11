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
#include <set>

#include <base_units.h>
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


wxString ConstraintSelectionHint( PCB_CONSTRAINT_TYPE aType )
{
    wxString format;

    // No default: an added type must be classified here, or the build fails rather than shipping
    // a silently hintless constraint.  Each arm mirrors the matching BuildConstraintFromItems rule.
    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::PARALLEL:
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:
    case PCB_CONSTRAINT_TYPE::COLLINEAR:
        format = _( "%s needs two line segments.  Click them to constrain." );
        break;

    // A zero-length segment has no direction, so the corner angle would be singular
    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION:
        format = _( "%s needs two line segments of nonzero length.  Click them to constrain." );
        break;

    case PCB_CONSTRAINT_TYPE::HORIZONTAL:
    case PCB_CONSTRAINT_TYPE::VERTICAL:
        format = _( "%s needs one line segment, or two anchor points.  Click them to constrain." );
        break;

    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:
        format = _( "%s needs one line segment.  Click it to constrain." );
        break;

    // allCentered also accepts ellipses, which have a centre but no single radius
    case PCB_CONSTRAINT_TYPE::CONCENTRIC:
        format = _( "%s needs two arcs, circles or ellipses.  Click them to constrain." );
        break;

    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:
        format = _( "%s needs two arcs or circles.  Click them to constrain." );
        break;

    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
        format = _( "%s needs one arc or circle.  Click it to constrain." );
        break;

    case PCB_CONSTRAINT_TYPE::ARC_ANGLE:
        format = _( "%s needs one arc.  Click it to constrain." );
        break;

    // Two curves must both be arcs or circles; an ellipse pairs only with a line
    case PCB_CONSTRAINT_TYPE::TANGENT:
        format = _( "%s needs a line and a curve, or two arcs or circles.  Click them to constrain." );
        break;

    // Authored by clicking anchors, so there is never a selection to reject
    case PCB_CONSTRAINT_TYPE::COINCIDENT:
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:
    case PCB_CONSTRAINT_TYPE::MIDPOINT:
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:
    case PCB_CONSTRAINT_TYPE::FIXED_POSITION:
    case PCB_CONSTRAINT_TYPE::UNDEFINED:
        return wxEmptyString;
    }

    return wxString::Format( format, ConstraintTypeLabel( aType ) );
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
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ); shape && aBoard->IsLayerVisible( shape->GetLayer() ) )
        {
            shapes.push_back( shape );
        }
    }

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );
                shape && aBoard->IsLayerVisible( shape->GetLayer() ) )
            {
                shapes.push_back( shape );
            }
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
                    if( ( item->Type() == PCB_SHAPE_T || dynamic_cast<PCB_DIMENSION_BASE*>( item ) )
                        && aBoard->IsLayerVisible( item->GetLayer() ) )
                    {
                        items.push_back( item );
                    }
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


std::vector<ENDPOINT_BINDING> SelectEndpointBindings( BOARD* aBoard, const KIID& aItem, const VECTOR2I& aStart,
                                                      const std::optional<VECTOR2I>& aEnd, double aMaxDist )
{
    std::vector<ENDPOINT_BINDING> bindings;

    if( !aBoard )
        return bindings;

    // Best pair of distinct anchors on one item within aMaxDist minimizing summed distance
    // Distinct anchors required or endpoints merge and pairs judged jointly not per end nearest
    using ANCHOR_PAIR = std::pair<CONSTRAINT_MEMBER, CONSTRAINT_MEMBER>;

    auto bestPairOn = [&]( BOARD_ITEM* aCandidate ) -> std::optional<std::pair<ANCHOR_PAIR, double>>
    {
        std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintItemAnchors( aCandidate );

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
                        ANCHOR_PAIR{ CONSTRAINT_MEMBER( aCandidate->m_Uuid, anchors[i].anchor, anchors[i].index ),
                                     CONSTRAINT_MEMBER( aCandidate->m_Uuid, anchors[j].anchor, anchors[j].index ) },
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
            if( item->m_Uuid == aItem )
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
    std::vector<CONSTRAINT_MEMBER> exclude{ { aItem, CONSTRAINT_ANCHOR::START }, { aItem, CONSTRAINT_ANCHOR::END } };

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


std::optional<KIID> NearestOutlineShape( BOARD* aBoard, const VECTOR2I& aPos, double aMaxDist, bool aAllowCircle )
{
    double              best = aMaxDist;
    std::optional<KIID> result;

    for( PCB_SHAPE* shape : CollectConstraintShapes( aBoard ) )
    {
        const SHAPE_T shapeType = shape->GetShape();
        double        dist = 0;

        if( shapeType == SHAPE_T::SEGMENT )
        {
            dist = SEG( shape->GetStart(), shape->GetEnd() ).Distance( aPos );
        }
        else if( aAllowCircle && ( shapeType == SHAPE_T::CIRCLE || shapeType == SHAPE_T::ARC ) )
        {
            dist = std::abs( ( aPos - shape->GetCenter() ).EuclideanNorm() - shape->GetRadius() );
        }
        else if( aAllowCircle && ( shapeType == SHAPE_T::ELLIPSE || shapeType == SHAPE_T::ELLIPSE_ARC ) )
        {
            // Radial distance to the outline at the click's polar angle in the ellipse frame.
            // Not the exact outline distance, but exact on the outline, which is all a snap needs.
            double   a = shape->GetEllipseMajorRadius();
            double   b = shape->GetEllipseMinorRadius();
            double   phi = shape->GetEllipseRotation().AsRadians();
            VECTOR2D d = VECTOR2D( aPos - shape->GetEllipseCenter() );
            double   lx = d.x * std::cos( phi ) + d.y * std::sin( phi );
            double   ly = -d.x * std::sin( phi ) + d.y * std::cos( phi );
            double   r = std::hypot( lx, ly );

            if( a <= 0 || b <= 0 )
                continue;

            double theta = std::atan2( ly, lx );
            double re = a * b / std::hypot( b * std::cos( theta ), a * std::sin( theta ) );

            dist = std::abs( r - re );
        }
        else
        {
            continue;
        }

        if( dist <= best )
        {
            best = dist;
            result = shape->m_Uuid;
        }
    }

    return result;
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


bool ConstraintIsDuplicateOnBoard( BOARD* aBoard, const PCB_CONSTRAINT* aConstraint )
{
    auto scan = [&]( const CONSTRAINTS& aList )
    {
        return std::ranges::any_of( aList,
                                    [&]( const PCB_CONSTRAINT* aExisting )
                                    {
                                        return ConstraintsAreDuplicate( *aExisting, *aConstraint );
                                    } );
    };

    if( scan( aBoard->Constraints() ) )
        return true;

    return std::ranges::any_of( aBoard->Footprints(),
                                [&]( FOOTPRINT* aFootprint )
                                {
                                    return scan( aFootprint->Constraints() );
                                } );
}


namespace
{
// Tuning knobs for draw time auto constraints
// The bind tolerance accepts only exact landings while the corridor also captures near misses
constexpr double AUTO_BIND_TOL_MM = 0.01;
constexpr double AUTO_CORRIDOR_MM = 0.25;
constexpr double AUTO_TANGENT_TOL_DEG = 10.0;


// Tangent direction of a segment or circular shape at aPos or nullopt for other kinds
std::optional<double> tangentDirAt( const PCB_SHAPE* aShape, const VECTOR2I& aPos )
{
    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    {
        VECTOR2D dir( aShape->GetEnd() - aShape->GetStart() );

        if( dir.EuclideanNorm() == 0 )
            return std::nullopt;

        return std::atan2( dir.y, dir.x );
    }

    case SHAPE_T::ARC:
    case SHAPE_T::CIRCLE:
    {
        VECTOR2D radial( aPos - aShape->GetCenter() );

        if( radial.EuclideanNorm() == 0 )
            return std::nullopt;

        return std::atan2( radial.y, radial.x ) + M_PI / 2;
    }

    default: return std::nullopt;
    }
}


// A drawn shape meeting aTarget within a few degrees of tangency gets a tangent constraint
// The target is the first member so the snap solve moves the drawn shape not the board
std::unique_ptr<PCB_CONSTRAINT> makeTangent( const PCB_SHAPE* aShape, const PCB_SHAPE* aTarget, const VECTOR2I& aPos,
                                             BOARD_ITEM* aParent )
{
    const double tangentTol = AUTO_TANGENT_TOL_DEG * M_PI / 180.0;

    bool curveInvolved = aShape->GetShape() == SHAPE_T::ARC || aTarget->GetShape() == SHAPE_T::ARC
                         || aTarget->GetShape() == SHAPE_T::CIRCLE;

    if( !curveInvolved )
        return nullptr;

    std::optional<double> myDir = tangentDirAt( aShape, aPos );
    std::optional<double> otherDir = tangentDirAt( aTarget, aPos );

    if( !myDir || !otherDir )
        return nullptr;

    double diff = std::fabs( std::fmod( *myDir - *otherDir, M_PI ) );
    diff = std::min( diff, M_PI - diff );

    if( diff > tangentTol )
        return nullptr;

    auto constraint = std::make_unique<PCB_CONSTRAINT>( aParent, PCB_CONSTRAINT_TYPE::TANGENT );
    constraint->AddMember( aTarget->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    constraint->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );

    return constraint;
}


// Append unless an equal constraint exists on the board or already in this batch
// One draw can touch the same target twice so the batch check matters
void addUnlessDuplicate( BOARD* aBoard, std::vector<AUTO_CONSTRAINT>& aResult,
                         std::unique_ptr<PCB_CONSTRAINT> aConstraint, bool aNeedsSolve )
{
    if( ConstraintIsDuplicateOnBoard( aBoard, aConstraint.get() ) )
        return;

    if( std::ranges::any_of( aResult,
                             [&]( const AUTO_CONSTRAINT& aEntry )
                             {
                                 return ConstraintsAreDuplicate( *aEntry.constraint, *aConstraint );
                             } ) )
    {
        return;
    }

    aResult.push_back( { std::move( aConstraint ), aNeedsSolve } );
}


// A circle or closed ellipse has no endpoints so only its centre binds
// Another curve centre reads as concentric an anchor coincides and an outline holds the centre
std::vector<AUTO_CONSTRAINT> selectCenterBindings( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent )
{
    std::vector<AUTO_CONSTRAINT> result;

    const double tol = pcbIUScale.mmToIU( AUTO_BIND_TOL_MM );
    VECTOR2I     center = aShape->GetCenter();

    std::vector<CONSTRAINT_MEMBER> exclude = { { aShape->m_Uuid, CONSTRAINT_ANCHOR::CENTER } };

    if( std::optional<CONSTRAINT_MEMBER> target = NearestConstraintAnchor( aBoard, center, tol, exclude ) )
    {
        std::unique_ptr<PCB_CONSTRAINT> constraint;

        if( target->m_anchor == CONSTRAINT_ANCHOR::CENTER )
        {
            constraint = std::make_unique<PCB_CONSTRAINT>( aParent, PCB_CONSTRAINT_TYPE::CONCENTRIC );
            constraint->AddMember( target->m_item, CONSTRAINT_ANCHOR::WHOLE );
            constraint->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
        }
        else
        {
            constraint = std::make_unique<PCB_CONSTRAINT>( aParent, PCB_CONSTRAINT_TYPE::COINCIDENT );
            constraint->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::CENTER );
            constraint->AddMember( target->m_item, target->m_anchor, target->m_index );
        }

        addUnlessDuplicate( aBoard, result, std::move( constraint ), false );
    }
    else if( std::optional<KIID> outline = NearestOutlineShape( aBoard, center, tol, true ) )
    {
        if( *outline != aShape->m_Uuid )
        {
            auto constraint = std::make_unique<PCB_CONSTRAINT>( aParent, PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
            constraint->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::CENTER );
            constraint->AddMember( *outline, CONSTRAINT_ANCHOR::WHOLE );

            addUnlessDuplicate( aBoard, result, std::move( constraint ), false );
        }
    }

    return result;
}


// Endpoints landing on existing anchors bind coincident
void selectEndpointCoincidents( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent,
                                std::vector<AUTO_CONSTRAINT>& aResult, std::set<CONSTRAINT_ANCHOR>& aBound,
                                std::vector<std::pair<VECTOR2I, KIID>>& aTouches )
{
    const double tol = pcbIUScale.mmToIU( AUTO_BIND_TOL_MM );

    std::vector<ENDPOINT_BINDING> bindings =
            SelectEndpointBindings( aBoard, aShape->m_Uuid, aShape->GetStart(), aShape->GetEnd(), tol );

    for( const ENDPOINT_BINDING& binding : bindings )
    {
        auto constraint = std::make_unique<PCB_CONSTRAINT>( aParent, PCB_CONSTRAINT_TYPE::COINCIDENT );
        constraint->AddMember( aShape->m_Uuid, binding.sourceAnchor );
        constraint->AddMember( binding.target.m_item, binding.target.m_anchor, binding.target.m_index );

        aBound.insert( binding.sourceAnchor );

        VECTOR2I pos = binding.sourceAnchor == CONSTRAINT_ANCHOR::START ? aShape->GetStart() : aShape->GetEnd();
        aTouches.emplace_back( pos, binding.target.m_item );

        addUnlessDuplicate( aBoard, aResult, std::move( constraint ), false );
    }
}


// An endpoint with no anchor to coincide with but sitting on an outline binds point on line
// Landing on a segment midpoint means the midpoint snap was used so bind midpoint instead
void selectOutlineFallbacks( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent,
                             std::vector<AUTO_CONSTRAINT>& aResult, const std::set<CONSTRAINT_ANCHOR>& aBound,
                             std::vector<std::pair<VECTOR2I, KIID>>& aTouches )
{
    const double tol = pcbIUScale.mmToIU( AUTO_BIND_TOL_MM );

    for( CONSTRAINT_ANCHOR anchor : { CONSTRAINT_ANCHOR::START, CONSTRAINT_ANCHOR::END } )
    {
        if( aBound.contains( anchor ) )
            continue;

        VECTOR2I            pos = anchor == CONSTRAINT_ANCHOR::START ? aShape->GetStart() : aShape->GetEnd();
        std::optional<KIID> target = NearestOutlineShape( aBoard, pos, tol, true );

        if( !target || *target == aShape->m_Uuid )
            continue;

        PCB_CONSTRAINT_TYPE type = PCB_CONSTRAINT_TYPE::POINT_ON_LINE;
        PCB_SHAPE*          targetShape = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( *target, true ) );

        if( targetShape && targetShape->GetShape() == SHAPE_T::SEGMENT )
        {
            VECTOR2I mid = targetShape->GetStart() + ( targetShape->GetEnd() - targetShape->GetStart() ) / 2;

            if( ( pos - mid ).EuclideanNorm() <= tol )
                type = PCB_CONSTRAINT_TYPE::MIDPOINT;
        }

        auto constraint = std::make_unique<PCB_CONSTRAINT>( aParent, type );
        constraint->AddMember( aShape->m_Uuid, anchor );
        constraint->AddMember( *target, CONSTRAINT_ANCHOR::WHOLE );

        aTouches.emplace_back( pos, *target );

        addUnlessDuplicate( aBoard, aResult, std::move( constraint ), false );
    }
}


// A segment drawn through or near an existing shape anchor pins that point on the new segment
// Near misses within the corridor bind too and the post push solve pulls the line onto them
void selectCorridorPins( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent,
                         std::vector<AUTO_CONSTRAINT>& aResult )
{
    if( aShape->GetShape() != SHAPE_T::SEGMENT || aShape->GetStart() == aShape->GetEnd() )
        return;

    const double tol = pcbIUScale.mmToIU( AUTO_BIND_TOL_MM );
    const double corridor = pcbIUScale.mmToIU( AUTO_CORRIDOR_MM );

    SEG                   span( aShape->GetStart(), aShape->GetEnd() );
    std::vector<VECTOR2I> boundPos;

    for( BOARD_ITEM* item : CollectConstrainableItems( aBoard ) )
    {
        if( item->m_Uuid == aShape->m_Uuid || item->Type() != PCB_SHAPE_T )
            continue;

        for( const CONSTRAINT_ANCHOR_POINT& anchor : ConstraintItemAnchors( item ) )
        {
            if( span.Distance( anchor.pos ) > corridor )
                continue;

            // The drawn endpoints already bound above so only true mid span hits count
            if( ( anchor.pos - aShape->GetStart() ).EuclideanNorm() <= corridor
                || ( anchor.pos - aShape->GetEnd() ).EuclideanNorm() <= corridor )
            {
                continue;
            }

            // Chained corners stack two anchors on one spot and one binding is enough
            if( std::ranges::any_of( boundPos,
                                     [&]( const VECTOR2I& aP )
                                     {
                                         return ( anchor.pos - aP ).EuclideanNorm() <= tol;
                                     } ) )
            {
                continue;
            }

            boundPos.push_back( anchor.pos );

            auto constraint = std::make_unique<PCB_CONSTRAINT>( aParent, PCB_CONSTRAINT_TYPE::POINT_ON_LINE );
            constraint->AddMember( item->m_Uuid, anchor.anchor, anchor.index );
            constraint->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );

            addUnlessDuplicate( aBoard, aResult, std::move( constraint ), true );
        }
    }
}


// Tangents where the drawn shape touched another shape near tangency
void selectTangents( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent, std::vector<AUTO_CONSTRAINT>& aResult,
                     const std::vector<std::pair<VECTOR2I, KIID>>& aTouches )
{
    for( const auto& [pos, id] : aTouches )
    {
        PCB_SHAPE* target = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( id, true ) );

        if( target && target != aShape )
        {
            if( std::unique_ptr<PCB_CONSTRAINT> tangent = makeTangent( aShape, target, pos, aParent ) )
                addUnlessDuplicate( aBoard, aResult, std::move( tangent ), true );
        }
    }
}


// A segment drawn in a constrained line mode keeps its axis
void selectAxisConstraint( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent,
                           std::vector<AUTO_CONSTRAINT>& aResult )
{
    if( aShape->GetShape() != SHAPE_T::SEGMENT || aShape->GetStart() == aShape->GetEnd() )
        return;

    bool horizontal = aShape->GetStart().y == aShape->GetEnd().y;
    bool vertical = aShape->GetStart().x == aShape->GetEnd().x;

    if( !horizontal && !vertical )
        return;

    auto constraint = std::make_unique<PCB_CONSTRAINT>( aParent, horizontal ? PCB_CONSTRAINT_TYPE::HORIZONTAL
                                                                            : PCB_CONSTRAINT_TYPE::VERTICAL );
    constraint->AddMember( aShape->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );

    addUnlessDuplicate( aBoard, aResult, std::move( constraint ), false );
}
} // namespace


std::vector<AUTO_CONSTRAINT> SelectShapeAutoConstraints( BOARD* aBoard, const PCB_SHAPE* aShape, BOARD_ITEM* aParent,
                                                         bool aAxisConstraint )
{
    std::vector<AUTO_CONSTRAINT> result;

    if( !aBoard || !aShape )
        return result;

    if( aShape->GetShape() == SHAPE_T::CIRCLE || aShape->GetShape() == SHAPE_T::ELLIPSE )
        return selectCenterBindings( aBoard, aShape, aParent );

    std::set<CONSTRAINT_ANCHOR>            bound;
    std::vector<std::pair<VECTOR2I, KIID>> touches;

    selectEndpointCoincidents( aBoard, aShape, aParent, result, bound, touches );
    selectOutlineFallbacks( aBoard, aShape, aParent, result, bound, touches );
    selectCorridorPins( aBoard, aShape, aParent, result );
    selectTangents( aBoard, aShape, aParent, result, touches );

    if( aAxisConstraint )
        selectAxisConstraint( aBoard, aShape, aParent, result );

    return result;
}
