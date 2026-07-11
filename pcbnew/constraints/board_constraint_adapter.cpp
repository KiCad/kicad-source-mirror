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

#include <constraints/board_constraint_adapter.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <board.h>
#include <footprint.h>
#include <math/util.h>
#include <pcb_shape.h>

#include <constraints/constraint_builder.h>

#include <GCS.h>


// IU are nanometres; one normalized unit is one millimetre (1e6 IU).  Squaring raw IU in the
// residuals (~1e16) is badly conditioned, so the cluster is solved in this millimetre frame.
static constexpr double IU_PER_NORM_UNIT = 1e6;

// Bound the solver so an interactive drag can never stall the UI thread.
static constexpr int MAX_SOLVE_ITERATIONS = 100;

// A non-temporary tag for the hard radius hold during a resize solve.
static constexpr int RESIZE_RADIUS_TAG = 1000;


// Every constraint owned by the board or by any of its footprints.  In the footprint editor the
// "board" is a footprint holder, so footprint-scoped constraints must be gathered too.
static std::vector<PCB_CONSTRAINT*> collectAllConstraints( BOARD* aBoard )
{
    std::vector<PCB_CONSTRAINT*> all( aBoard->Constraints().begin(), aBoard->Constraints().end() );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        all.insert( all.end(), footprint->Constraints().begin(), footprint->Constraints().end() );

    return all;
}


// Adjacency from a shape KIID to the constraints touching it.  When @p aErrored is given, any
// constraint that cannot be satisfied -- it has no members, or a member that does not resolve to a
// live PCB_SHAPE -- is recorded there (error state), since such a constraint never reaches the
// solver's per-cluster mapping where the remaining error cases are caught.
static std::unordered_map<KIID, std::vector<PCB_CONSTRAINT*>>
buildShapeConstraintMap( BOARD* aBoard, const std::vector<PCB_CONSTRAINT*>& aConstraints,
                         std::vector<KIID>* aErrored = nullptr )
{
    std::unordered_map<KIID, std::vector<PCB_CONSTRAINT*>> map;

    for( PCB_CONSTRAINT* constraint : aConstraints )
    {
        bool errored = constraint->GetMembers().empty();

        for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
        {
            map[member.m_item].push_back( constraint );

            // Members must resolve to a live PCB_SHAPE; a deleted or wrong-typed item is an error.
            if( aErrored && !dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( member.m_item, true ) ) )
                errored = true;
        }

        if( errored && aErrored )
            aErrored->push_back( constraint->m_Uuid );
    }

    return map;
}


// Walk the connected component (shapes + constraints) that contains @p aSeed.  Shapes reached are
// added to @p aVisited when provided, so a caller iterating seeds can skip an already-walked one.
static void collectConstraintCluster(
        const std::unordered_map<KIID, std::vector<PCB_CONSTRAINT*>>& aMap, const KIID& aSeed,
        std::unordered_set<KIID>& aClusterShapes, std::vector<PCB_CONSTRAINT*>& aClusterConstraints,
        std::set<KIID>* aVisited = nullptr )
{
    std::set<KIID>    used;
    std::vector<KIID> frontier{ aSeed };
    aClusterShapes.insert( aSeed );

    while( !frontier.empty() )
    {
        KIID shapeId = frontier.back();
        frontier.pop_back();

        if( aVisited )
            aVisited->insert( shapeId );

        auto it = aMap.find( shapeId );

        if( it == aMap.end() )
            continue;

        for( PCB_CONSTRAINT* constraint : it->second )
        {
            if( used.insert( constraint->m_Uuid ).second )
                aClusterConstraints.push_back( constraint );

            for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
            {
                if( aClusterShapes.insert( member.m_item ).second )
                    frontier.push_back( member.m_item );
            }
        }
    }
}


// Resolve a set of shape KIIDs to the live PCB_SHAPEs, dropping any that no longer exist.
static std::vector<PCB_SHAPE*> resolveClusterShapes( BOARD* aBoard,
                                                     const std::unordered_set<KIID>& aIds )
{
    std::vector<PCB_SHAPE*> shapes;

    for( const KIID& id : aIds )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( id, true ) ) )
            shapes.push_back( shape );
    }

    return shapes;
}


BOARD_CONSTRAINT_ADAPTER::BOARD_CONSTRAINT_ADAPTER() :
        m_gcs( std::make_unique<GCS::System>() )
{
}


BOARD_CONSTRAINT_ADAPTER::~BOARD_CONSTRAINT_ADAPTER()
{
    // GCS::System frees the Constraint objects it owns; m_params outlives it (member order).
    if( m_gcs )
        m_gcs->clear();
}


int BOARD_CONSTRAINT_ADAPTER::pushParam( double aValue )
{
    m_params.push_back( aValue );
    return static_cast<int>( m_params.size() ) - 1;
}


int BOARD_CONSTRAINT_ADAPTER::anchorParamIndex( const CONSTRAINT_MEMBER& aMember ) const
{
    auto it = m_shapeVars.find( aMember.m_item );

    if( it == m_shapeVars.end() )
        return -1;

    const SHAPE_VARS& vars = it->second;

    switch( aMember.m_anchor )
    {
    case CONSTRAINT_ANCHOR::START:
        if( vars.kind == SHAPE_KIND::ARC || vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            return vars.arcStartX;

        // A circle or closed ellipse has no endpoints; startX holds the centre, so never alias them.
        return vars.kind == SHAPE_KIND::SEGMENT ? vars.startX : -1;
    case CONSTRAINT_ANCHOR::END:
        if( vars.kind == SHAPE_KIND::ARC || vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            return vars.arcEndX;

        return vars.kind == SHAPE_KIND::SEGMENT ? vars.endX : -1;
    case CONSTRAINT_ANCHOR::CENTER: return vars.kind == SHAPE_KIND::SEGMENT ? -1 : vars.startX;
    default:
        return -1;
    }
}


bool BOARD_CONSTRAINT_ADAPTER::Build( const std::vector<PCB_SHAPE*>&      aShapes,
                                      const std::vector<PCB_CONSTRAINT*>& aConstraints,
                                      const std::set<KIID>*               aFixedShapes )
{
    m_params.clear();
    m_shapeVars.clear();
    m_tagToConstraint.clear();
    m_unmapped.clear();
    m_gcs->clear();
    m_built = false;

    if( aShapes.empty() )
        return false;

    // Centre on the first start point so normalized coordinates stay small for a board far from
    // the origin.
    m_originX = aShapes.front()->GetStart().x;
    m_originY = aShapes.front()->GetStart().y;
    m_scale = IU_PER_NORM_UNIT;
    m_invScale = 1.0 / m_scale;

    // [first, last) parameter spans of locked shapes, folded into fixedParams below so the solver
    // treats a locked shape as an immovable reference.
    std::vector<std::pair<int, int>> lockedRanges;

    // Fixed focus-offset params of ellipses, folded into fixedParams below.
    std::vector<int> ellipseOffsetParams;

    for( PCB_SHAPE* shape : aShapes )
    {
        SHAPE_VARS vars;
        vars.shape = shape;
        int firstParam = static_cast<int>( m_params.size() );

        if( shape->GetShape() == SHAPE_T::SEGMENT )
        {
            vars.kind = SHAPE_KIND::SEGMENT;
            vars.startX = pushParam( normalizeX( shape->GetStart().x ) );
            pushParam( normalizeY( shape->GetStart().y ) );
            vars.endX = pushParam( normalizeX( shape->GetEnd().x ) );
            pushParam( normalizeY( shape->GetEnd().y ) );
        }
        else if( shape->GetShape() == SHAPE_T::CIRCLE )
        {
            vars.kind = SHAPE_KIND::CIRCLE;
            vars.startX = pushParam( normalizeX( shape->GetCenter().x ) );
            pushParam( normalizeY( shape->GetCenter().y ) );
            vars.radius = pushParam( shape->GetRadius() * m_invScale );
        }
        else if( shape->GetShape() == SHAPE_T::ARC )
        {
            vars.kind = SHAPE_KIND::ARC;

            VECTOR2I center = shape->GetCenter();
            VECTOR2I start = shape->GetStart();
            VECTOR2I end = shape->GetEnd();

            double cx = normalizeX( center.x );
            double cy = normalizeY( center.y );
            double sx = normalizeX( start.x );
            double sy = normalizeY( start.y );
            double ex = normalizeX( end.x );
            double ey = normalizeY( end.y );

            vars.startX = pushParam( cx );
            pushParam( cy );
            vars.radius = pushParam( shape->GetRadius() * m_invScale );
            vars.arcStartX = pushParam( sx );
            pushParam( sy );
            vars.arcEndX = pushParam( ex );
            pushParam( ey );
            vars.startAngle = pushParam( std::atan2( sy - cy, sx - cx ) );
            vars.endAngle = pushParam( std::atan2( ey - cy, ex - cx ) );

            GCS::Arc arc;
            arc.center = GCS::Point{ &m_params[vars.startX], &m_params[vars.startX + 1] };
            arc.rad = &m_params[vars.radius];
            arc.start = GCS::Point{ &m_params[vars.arcStartX], &m_params[vars.arcStartX + 1] };
            arc.end = GCS::Point{ &m_params[vars.arcEndX], &m_params[vars.arcEndX + 1] };
            arc.startAngle = &m_params[vars.startAngle];
            arc.endAngle = &m_params[vars.endAngle];
            m_gcs->addConstraintArcRules( arc );
        }
        else if( shape->GetShape() == SHAPE_T::ELLIPSE || shape->GetShape() == SHAPE_T::ELLIPSE_ARC )
        {
            vars.kind = shape->GetShape() == SHAPE_T::ELLIPSE ? SHAPE_KIND::ELLIPSE : SHAPE_KIND::ELLIPSE_ARC;

            VECTOR2I center = shape->GetEllipseCenter();
            double   major = shape->GetEllipseMajorRadius();
            double   minor = shape->GetEllipseMinorRadius();
            double   phi = shape->GetEllipseRotation().AsRadians();

            // GCS parameterizes an ellipse as center + first focus + minor radius.
            double focal = major > minor ? std::sqrt( major * major - minor * minor ) : 0.0;
            double cx = normalizeX( center.x );
            double cy = normalizeY( center.y );

            vars.startX = pushParam( cx );
            pushParam( cy );
            vars.focusX = pushParam( cx + focal * m_invScale * std::cos( phi ) );
            pushParam( cy + focal * m_invScale * std::sin( phi ) );
            vars.radius = pushParam( std::min( major, minor ) * m_invScale );

            // Tie the focus to the center, or a solve moving the center would leave the focus
            // behind and distort the ellipse.
            int offX = pushParam( focal * m_invScale * std::cos( phi ) );
            int offY = pushParam( focal * m_invScale * std::sin( phi ) );
            ellipseOffsetParams.push_back( offX );
            ellipseOffsetParams.push_back( offY );

            m_gcs->addConstraintDifference( &m_params[vars.startX], &m_params[vars.focusX], &m_params[offX] );
            m_gcs->addConstraintDifference( &m_params[vars.startX + 1], &m_params[vars.focusX + 1], &m_params[offY] );

            if( vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            {
                VECTOR2I start = shape->GetStart();
                VECTOR2I end = shape->GetEnd();

                vars.arcStartX = pushParam( normalizeX( start.x ) );
                pushParam( normalizeY( start.y ) );
                vars.arcEndX = pushParam( normalizeX( end.x ) );
                pushParam( normalizeY( end.y ) );
                vars.startAngle = pushParam( shape->GetEllipseStartAngle().AsRadians() );
                vars.endAngle = pushParam( shape->GetEllipseEndAngle().AsRadians() );

                GCS::ArcOfEllipse arc;
                arc.center = GCS::Point{ &m_params[vars.startX], &m_params[vars.startX + 1] };
                arc.focus1 = GCS::Point{ &m_params[vars.focusX], &m_params[vars.focusX + 1] };
                arc.radmin = &m_params[vars.radius];
                arc.start = GCS::Point{ &m_params[vars.arcStartX], &m_params[vars.arcStartX + 1] };
                arc.end = GCS::Point{ &m_params[vars.arcEndX], &m_params[vars.arcEndX + 1] };
                arc.startAngle = &m_params[vars.startAngle];
                arc.endAngle = &m_params[vars.endAngle];
                m_gcs->addConstraintArcOfEllipseRules( arc );
            }
        }
        else
        {
            continue;   // other shapes are not mapped
        }

        // A locked shape, or one the caller pinned, is an immovable reference: record its parameter
        // span so the solver moves only the rest of the cluster.
        if( shape->IsLocked() || ( aFixedShapes && aFixedShapes->count( shape->m_Uuid ) ) )
            lockedRanges.emplace_back( firstParam, static_cast<int>( m_params.size() ) );

        m_shapeVars[shape->m_Uuid] = vars;
    }

    // Param indices the solver may not change: grounded (fixed-position) points, driving constants
    // (lengths, radii), and every parameter of a locked shape.  Everything else is an unknown.
    std::set<int> fixedParams;

    for( const auto& [first, last] : lockedRanges )
    {
        for( int i = first; i < last; ++i )
            fixedParams.insert( i );
    }

    for( int i : ellipseOffsetParams )
        fixedParams.insert( i );

    auto pointAt = [&]( int aXIndex ) -> GCS::Point
    {
        return GCS::Point{ &m_params[aXIndex], &m_params[aXIndex + 1] };
    };

    auto lineFor = [&]( const CONSTRAINT_MEMBER& aMember, GCS::Line& aLine ) -> bool
    {
        auto it = m_shapeVars.find( aMember.m_item );

        if( it == m_shapeVars.end() || it->second.kind != SHAPE_KIND::SEGMENT
                || aMember.m_anchor != CONSTRAINT_ANCHOR::WHOLE )
        {
            return false;
        }

        aLine.p1 = pointAt( it->second.startX );
        aLine.p2 = pointAt( it->second.endX );
        return true;
    };

    // A circle for radial constraints; an arc is accepted too (its center + radius are shared
    // with the Circle base, which is all addConstraintEqualRadius/CircleRadius/concentric need).
    auto circleFor = [&]( const CONSTRAINT_MEMBER& aMember, GCS::Circle& aCircle ) -> bool
    {
        auto it = m_shapeVars.find( aMember.m_item );

        if( it == m_shapeVars.end()
                || ( it->second.kind != SHAPE_KIND::CIRCLE && it->second.kind != SHAPE_KIND::ARC ) )
        {
            return false;
        }

        aCircle.center = pointAt( it->second.startX );
        aCircle.rad = &m_params[it->second.radius];
        return true;
    };

    // An ellipse for ellipse-target constraints; an elliptical arc is accepted too (its center,
    // focus and minor radius are shared with the Ellipse base).
    auto ellipseFor = [&]( const CONSTRAINT_MEMBER& aMember, GCS::Ellipse& aEllipse ) -> bool
    {
        auto it = m_shapeVars.find( aMember.m_item );

        if( it == m_shapeVars.end()
            || ( it->second.kind != SHAPE_KIND::ELLIPSE && it->second.kind != SHAPE_KIND::ELLIPSE_ARC ) )
        {
            return false;
        }

        aEllipse.center = pointAt( it->second.startX );
        aEllipse.focus1 = pointAt( it->second.focusX );
        aEllipse.radmin = &m_params[it->second.radius];
        return true;
    };

    // Push a driving constant (length/radius), normalized from IU, returning its stable index.
    auto pushConstant = [&]( double aIU ) -> int
    {
        int idx = pushParam( aIU * m_invScale );
        fixedParams.insert( idx );
        return idx;
    };

    int tag = 1;

    for( PCB_CONSTRAINT* constraint : aConstraints )
    {
        const std::vector<CONSTRAINT_MEMBER>& members = constraint->GetMembers();
        bool                                  mapped = false;

        switch( constraint->GetConstraintType() )
        {
        case PCB_CONSTRAINT_TYPE::PARALLEL:
        {
            GCS::Line l1, l2;

            if( members.size() == 2 && lineFor( members[0], l1 ) && lineFor( members[1], l2 ) )
            {
                m_gcs->addConstraintParallel( l1, l2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::HORIZONTAL:
        {
            GCS::Line l;

            if( members.size() == 1 && lineFor( members[0], l ) )
            {
                m_gcs->addConstraintHorizontal( l, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::VERTICAL:
        {
            GCS::Line l;

            if( members.size() == 1 && lineFor( members[0], l ) )
            {
                m_gcs->addConstraintVertical( l, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::COINCIDENT:
        {
            int a = members.size() == 2 ? anchorParamIndex( members[0] ) : -1;
            int b = members.size() == 2 ? anchorParamIndex( members[1] ) : -1;

            if( a >= 0 && b >= 0 )
            {
                GCS::Point p1 = pointAt( a );
                GCS::Point p2 = pointAt( b );
                m_gcs->addConstraintP2PCoincident( p1, p2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::FIXED_POSITION:
        {
            int a = members.size() == 1 ? anchorParamIndex( members[0] ) : -1;

            if( a >= 0 )
            {
                fixedParams.insert( a );
                fixedParams.insert( a + 1 );
                mapped = true;   // enforced by omission from the unknowns, not a solver constraint
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::PERPENDICULAR:
        {
            GCS::Line l1, l2;

            if( members.size() == 2 && lineFor( members[0], l1 ) && lineFor( members[1], l2 ) )
            {
                m_gcs->addConstraintPerpendicular( l1, l2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:
        {
            GCS::Line l1, l2;

            if( members.size() == 2 && lineFor( members[0], l1 ) && lineFor( members[1], l2 ) )
            {
                m_gcs->addConstraintEqualLength( l1, l2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:
        {
            int          p = members.size() == 2 ? anchorParamIndex( members[0] ) : -1;
            GCS::Line    l;
            GCS::Circle  circ;
            GCS::Ellipse ell;

            if( p >= 0 && lineFor( members[1], l ) )
            {
                GCS::Point point = pointAt( p );
                m_gcs->addConstraintPointOnLine( point, l, tag );
                mapped = true;
            }
            else if( p >= 0 && circleFor( members[1], circ ) )
            {
                // A circle or arc target keeps the point on its circumference.
                GCS::Point point = pointAt( p );
                m_gcs->addConstraintPointOnCircle( point, circ, tag );
                mapped = true;
            }
            else if( p >= 0 && ellipseFor( members[1], ell ) )
            {
                GCS::Point point = pointAt( p );
                m_gcs->addConstraintPointOnEllipse( point, ell, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::MIDPOINT:
        {
            int       p = members.size() == 2 ? anchorParamIndex( members[0] ) : -1;
            GCS::Line seg;

            // The point is the midpoint of the segment: its endpoints are symmetric about it.
            if( p >= 0 && lineFor( members[1], seg ) )
            {
                GCS::Point mid = pointAt( p );
                m_gcs->addConstraintP2PSymmetric( seg.p1, seg.p2, mid, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::COLLINEAR:
        {
            GCS::Line l1, l2;

            // Both endpoints of the second segment lie on the first's supporting line.
            if( members.size() == 2 && lineFor( members[0], l1 ) && lineFor( members[1], l2 ) )
            {
                m_gcs->addConstraintPointOnLine( l2.p1, l1, tag );
                m_gcs->addConstraintPointOnLine( l2.p2, l1, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::SYMMETRIC:
        {
            int       a = members.size() == 3 ? anchorParamIndex( members[0] ) : -1;
            int       b = members.size() == 3 ? anchorParamIndex( members[1] ) : -1;
            GCS::Line axis;

            if( a >= 0 && b >= 0 && lineFor( members[2], axis ) )
            {
                GCS::Point pa = pointAt( a );
                GCS::Point pb = pointAt( b );
                m_gcs->addConstraintP2PSymmetric( pa, pb, axis, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:
        {
            GCS::Line l;

            if( members.size() == 1 && constraint->HasValue() && lineFor( members[0], l ) )
            {
                int len = pushConstant( *constraint->GetValue() );
                m_gcs->addConstraintP2PDistance( l.p1, l.p2, &m_params[len], tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
        {
            GCS::Circle c;

            if( members.size() == 1 && constraint->HasValue() && circleFor( members[0], c ) )
            {
                int rad = pushConstant( *constraint->GetValue() );
                m_gcs->addConstraintCircleRadius( c, &m_params[rad], tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:
        {
            GCS::Circle c1, c2;

            if( members.size() == 2 && circleFor( members[0], c1 ) && circleFor( members[1], c2 ) )
            {
                m_gcs->addConstraintEqualRadius( c1, c2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::CONCENTRIC:
        {
            // Any center-bearing shape can be concentric, ellipses included.
            auto centerOf = [&]( const CONSTRAINT_MEMBER& aMember, GCS::Point& aOut ) -> bool
            {
                GCS::Circle  c;
                GCS::Ellipse e;

                if( circleFor( aMember, c ) )
                {
                    aOut = c.center;
                    return true;
                }

                if( ellipseFor( aMember, e ) )
                {
                    aOut = e.center;
                    return true;
                }

                return false;
            };

            GCS::Point p1, p2;

            if( members.size() == 2 && centerOf( members[0], p1 ) && centerOf( members[1], p2 ) )
            {
                m_gcs->addConstraintP2PCoincident( p1, p2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION:
        {
            GCS::Line l1, l2;

            if( members.size() == 2 && constraint->HasValue() && lineFor( members[0], l1 )
                    && lineFor( members[1], l2 ) )
            {
                // planegcs angles are in radians; the constraint stores degrees.
                int angle = pushParam( *constraint->GetValue() * M_PI / 180.0 );
                fixedParams.insert( angle );
                m_gcs->addConstraintL2LAngle( l1, l2, &m_params[angle], tag, constraint->IsDriving() );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::TANGENT:
        {
            if( members.size() != 2 )
                break;

            GCS::Line    l;
            GCS::Circle  c1, c2;
            GCS::Ellipse ell;

            // The members may come in either order.
            int lineIdx = lineFor( members[0], l ) ? 0 : ( lineFor( members[1], l ) ? 1 : -1 );

            if( lineIdx >= 0 )
            {
                const CONSTRAINT_MEMBER& other = members[lineIdx == 0 ? 1 : 0];

                if( circleFor( other, c1 ) )
                {
                    // Keep the circle on the side of the line it is on now.
                    double dx = *l.p2.x - *l.p1.x;
                    double dy = *l.p2.y - *l.p1.y;
                    double cross = dx * ( *c1.center.y - *l.p1.y ) - dy * ( *c1.center.x - *l.p1.x );

                    m_gcs->addConstraintTangent( l, c1, cross > 0.0, tag );
                    mapped = true;
                }
                else if( ellipseFor( other, ell ) )
                {
                    m_gcs->addConstraintTangent( l, ell, tag );
                    mapped = true;
                }
            }
            else if( circleFor( members[0], c1 ) && circleFor( members[1], c2 ) )
            {
                m_gcs->addConstraintTangent( c1, c2, tag );
                mapped = true;
            }

            break;
        }

        default:
            break;   // UNDEFINED and point-anchored families handled elsewhere
        }

        // An unmappable constraint (wrong member count/kind for its type) is skipped so it cannot
        // disable solving for the whole connected cluster; the remaining constraints still solve.
        // It is recorded so the caller can flag it as errored rather than dropping it silently.
        if( !mapped )
        {
            m_unmapped.push_back( constraint->m_Uuid );
            continue;
        }

        m_tagToConstraint[tag] = constraint->m_Uuid;
        tag++;
    }

    // Everything not grounded or held as a driving constant is an unknown the solver may move.
    GCS::VEC_pD unknowns;

    for( int i = 0; i < static_cast<int>( m_params.size() ); ++i )
    {
        if( !fixedParams.count( i ) )
            unknowns.push_back( &m_params[i] );
    }

    m_gcs->declareUnknowns( unknowns );
    m_gcs->initSolution();
    m_gcs->maxIter = MAX_SOLVE_ITERATIONS;

    m_built = true;
    return true;
}


bool BOARD_CONSTRAINT_ADAPTER::Solve( bool aStabilize )
{
    if( !m_built )
        return false;

    if( aStabilize )
        holdFreeSegmentLengths();

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    if( aStabilize )
        m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );

    return ret == GCS::Success || ret == GCS::Converged;
}


bool BOARD_CONSTRAINT_ADAPTER::SolveAfterResize( const KIID& aResizedShape )
{
    if( !m_built )
        return false;

    for( auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.radius < 0 )
            continue;

        int         target = pushParam( m_params[vars.radius] );
        GCS::Circle c;
        c.center = GCS::Point{ &m_params[vars.startX], &m_params[vars.startX + 1] };
        c.rad = &m_params[vars.radius];

        if( kiid == aResizedShape )
        {
            // The user set this radius, so hold it hard. Pin the centre yielding so the shape moves
            // only if a locked neighbour leaves no other way to stay tangent.
            m_gcs->addConstraintCircleRadius( c, &m_params[target], RESIZE_RADIUS_TAG, true );

            int cx = pushParam( m_params[vars.startX] );
            int cy = pushParam( m_params[vars.startX + 1] );
            m_gcs->addConstraintCoordinateX( c.center, &m_params[cx], GCS::DefaultTemporaryConstraint );
            m_gcs->addConstraintCoordinateY( c.center, &m_params[cy], GCS::DefaultTemporaryConstraint );
        }
        else
        {
            // Neighbours keep their size unless a real radius constraint says otherwise.
            m_gcs->addConstraintCircleRadius( c, &m_params[target], GCS::DefaultTemporaryConstraint );
        }
    }

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );

    return ret == GCS::Success || ret == GCS::Converged;
}


bool BOARD_CONSTRAINT_ADAPTER::Solve( const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor, bool aStabilize )
{
    if( !m_built )
        return false;

    int anchorX = anchorParamIndex( aDragged );

    if( anchorX < 0 )
        return false;

    // Reuse fixed backing slots for the cursor target so repeated drag solves (warm-started from
    // the previous solution still in m_params) never grow the backing store.
    if( m_dragTargetX < 0 )
    {
        m_dragTargetX = pushParam( 0.0 );
        m_dragTargetY = pushParam( 0.0 );
    }

    m_params[m_dragTargetX] = normalizeX( aCursor.x );
    m_params[m_dragTargetY] = normalizeY( aCursor.y );

    GCS::Point anchor = GCS::Point{ &m_params[anchorX], &m_params[anchorX + 1] };

    // A temporary (negatively tagged) pin yields to the real constraints when over-constrained.
    m_gcs->addConstraintCoordinateX( anchor, &m_params[m_dragTargetX], GCS::DefaultTemporaryConstraint );
    m_gcs->addConstraintCoordinateY( anchor, &m_params[m_dragTargetY], GCS::DefaultTemporaryConstraint );

    if( aStabilize )
        holdFreeSegmentLengths();

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );

    return ret == GCS::Success || ret == GCS::Converged;
}


void BOARD_CONSTRAINT_ADAPTER::holdFreeSegmentLengths()
{
    // A yielding length hold keeps an angle constraint from collapsing a free segment to a point.
    for( auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind != SHAPE_KIND::SEGMENT || vars.shape->IsLocked() )
            continue;

        GCS::Point p1{ &m_params[vars.startX], &m_params[vars.startX + 1] };
        GCS::Point p2{ &m_params[vars.endX], &m_params[vars.endX + 1] };
        double     dx = m_params[vars.endX] - m_params[vars.startX];
        double     dy = m_params[vars.endX + 1] - m_params[vars.startX + 1];
        int        len = pushParam( std::hypot( dx, dy ) );
        m_gcs->addConstraintP2PDistance( p1, p2, &m_params[len], GCS::DefaultTemporaryConstraint );
    }
}


std::vector<PCB_SHAPE*> BOARD_CONSTRAINT_ADAPTER::Apply( const std::function<void( PCB_SHAPE* )>& aBeforeWrite )
{
    std::vector<PCB_SHAPE*> changed;

    if( !m_built )
        return changed;

    for( auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind == SHAPE_KIND::CIRCLE )
        {
            VECTOR2I center( KiROUND( denormalizeX( m_params[vars.startX] ) ),
                             KiROUND( denormalizeY( m_params[vars.startX + 1] ) ) );
            int      radius = KiROUND( m_params[vars.radius] * m_scale );

            if( center == vars.shape->GetCenter() && radius == vars.shape->GetRadius() )
                continue;

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            vars.shape->SetCenter( center );
            vars.shape->SetRadius( radius );
            changed.push_back( vars.shape );
            continue;
        }

        if( vars.kind == SHAPE_KIND::ARC )
        {
            VECTOR2I center( KiROUND( denormalizeX( m_params[vars.startX] ) ),
                             KiROUND( denormalizeY( m_params[vars.startX + 1] ) ) );
            VECTOR2I start( KiROUND( denormalizeX( m_params[vars.arcStartX] ) ),
                            KiROUND( denormalizeY( m_params[vars.arcStartX + 1] ) ) );
            VECTOR2I end( KiROUND( denormalizeX( m_params[vars.arcEndX] ) ),
                          KiROUND( denormalizeY( m_params[vars.arcEndX + 1] ) ) );

            if( start == vars.shape->GetStart() && end == vars.shape->GetEnd()
                    && center == vars.shape->GetCenter() )
            {
                continue;
            }

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            // Reconstruct the arc from its solved centre and endpoints: the mid lies on the solved
            // circle at the bisector of the swept angles.  The bisector is taken on the side that
            // keeps the original winding, so a clockwise arc does not flip to counter-clockwise.
            double rad = m_params[vars.radius] * m_scale;
            double sa = std::atan2( start.y - center.y, start.x - center.x );
            double ea = std::atan2( end.y - center.y, end.x - center.x );

            if( vars.shape->IsClockwiseArc() )
            {
                if( ea > sa )
                    ea -= 2.0 * M_PI;
            }
            else if( ea < sa )
            {
                ea += 2.0 * M_PI;
            }

            double   midAngle = 0.5 * ( sa + ea );
            VECTOR2I mid( KiROUND( center.x + rad * std::cos( midAngle ) ),
                          KiROUND( center.y + rad * std::sin( midAngle ) ) );

            vars.shape->SetArcGeometry( start, mid, end );
            changed.push_back( vars.shape );
            continue;
        }

        if( vars.kind == SHAPE_KIND::ELLIPSE || vars.kind == SHAPE_KIND::ELLIPSE_ARC )
        {
            VECTOR2I center( KiROUND( denormalizeX( m_params[vars.startX] ) ),
                             KiROUND( denormalizeY( m_params[vars.startX + 1] ) ) );

            // Recover major radius and rotation from the solved focus.
            double fx = ( m_params[vars.focusX] - m_params[vars.startX] ) * m_scale;
            double fy = ( m_params[vars.focusX + 1] - m_params[vars.startX + 1] ) * m_scale;
            double focal = std::hypot( fx, fy );
            double minor = m_params[vars.radius] * m_scale;
            double major = std::sqrt( focal * focal + minor * minor );

            // A focal distance below 1 IU means a circle-degenerate ellipse with no defined
            // rotation, so keep the shape's current one.
            EDA_ANGLE rotation =
                    focal > 1.0 ? EDA_ANGLE( std::atan2( fy, fx ), RADIANS_T ) : vars.shape->GetEllipseRotation();

            EDA_ANGLE startAngle = vars.shape->GetEllipseStartAngle();
            EDA_ANGLE endAngle = vars.shape->GetEllipseEndAngle();

            if( vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            {
                startAngle = EDA_ANGLE( m_params[vars.startAngle], RADIANS_T );
                endAngle = EDA_ANGLE( m_params[vars.endAngle], RADIANS_T );
            }

            auto sameAngle = []( const EDA_ANGLE& aA, const EDA_ANGLE& aB )
            {
                return std::abs( ( aA - aB ).Normalize180().AsDegrees() ) < 1e-6;
            };

            if( center == vars.shape->GetEllipseCenter() && KiROUND( major ) == vars.shape->GetEllipseMajorRadius()
                && KiROUND( minor ) == vars.shape->GetEllipseMinorRadius()
                && sameAngle( rotation, vars.shape->GetEllipseRotation() )
                && sameAngle( startAngle, vars.shape->GetEllipseStartAngle() )
                && sameAngle( endAngle, vars.shape->GetEllipseEndAngle() ) )
            {
                continue;
            }

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            vars.shape->SetEllipseCenter( center );
            vars.shape->SetEllipseRotation( rotation );
            vars.shape->SetEllipseMajorRadius( KiROUND( major ) );
            vars.shape->SetEllipseMinorRadius( KiROUND( minor ) );

            if( vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            {
                vars.shape->SetEllipseStartAngle( startAngle );
                vars.shape->SetEllipseEndAngle( endAngle );
            }

            changed.push_back( vars.shape );
            continue;
        }

        VECTOR2I start( KiROUND( denormalizeX( m_params[vars.startX] ) ),
                        KiROUND( denormalizeY( m_params[vars.startX + 1] ) ) );
        VECTOR2I end( KiROUND( denormalizeX( m_params[vars.endX] ) ),
                      KiROUND( denormalizeY( m_params[vars.endX + 1] ) ) );

        if( start == vars.shape->GetStart() && end == vars.shape->GetEnd() )
            continue;

        // Never write a segment the solver shrank to a point. A 1 um result is a collapse, not
        // intent, and a zero-length line vanishes from the canvas.
        const double collapseFloor = 1000.0;
        double       newLen = ( end - start ).EuclideanNorm();
        double       curLen = ( vars.shape->GetEnd() - vars.shape->GetStart() ).EuclideanNorm();

        if( newLen < collapseFloor && curLen >= collapseFloor )
            continue;

        if( aBeforeWrite )
            aBeforeWrite( vars.shape );

        vars.shape->SetStart( start );
        vars.shape->SetEnd( end );
        changed.push_back( vars.shape );
    }

    return changed;
}


CONSTRAINT_DIAGNOSIS BOARD_CONSTRAINT_ADAPTER::Diagnose()
{
    CONSTRAINT_DIAGNOSIS diag;

    if( !m_built )
        return diag;

    m_gcs->diagnose();
    diag.freeDof = m_gcs->dofsNumber();

    GCS::VEC_I conflictingTags;
    GCS::VEC_I redundantTags;
    m_gcs->getConflicting( conflictingTags );
    m_gcs->getRedundant( redundantTags );

    auto tagsToKiids = [&]( const GCS::VEC_I& aTags, std::vector<KIID>& aOut )
    {
        for( int t : aTags )
        {
            auto it = m_tagToConstraint.find( t );

            if( it != m_tagToConstraint.end() )
                aOut.push_back( it->second );
        }
    };

    tagsToKiids( conflictingTags, diag.conflicting );
    tagsToKiids( redundantTags, diag.redundant );

    // Also flag any constraint the geometry does not satisfy. The rank analysis can miss it.
    const double residualTol = 1e-3;

    for( const auto& [tag, kiid] : m_tagToConstraint )
    {
        double err = m_gcs->calculateConstraintErrorByTag( tag );

        // A nan is a degenerate system, not a conflict. A real contradiction leaves a finite error.
        if( !std::isfinite( err ) || std::abs( err ) <= residualTol )
            continue;

        if( std::find( diag.conflicting.begin(), diag.conflicting.end(), kiid ) == diag.conflicting.end() )
            diag.conflicting.push_back( kiid );
    }

    // .solved reflects the last Solve(), which Diagnose() does not run; the caller sets it.
    return diag;
}


CONSTRAINT_DIAGNOSIS SolveCluster( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor,
                                   std::vector<PCB_SHAPE*>*                 aModified,
                                   const std::function<void( PCB_SHAPE* )>& aBeforeModify, bool aIncludeDragged,
                                   bool aStabilize )
{
    CONSTRAINT_DIAGNOSIS diag;

    if( !aBoard )
        return diag;

    // The cluster is the connected component of the shape<->constraint graph containing the
    // dragged shape, since a constraint links the shapes of all its members.
    auto shapeToConstraints = buildShapeConstraintMap( aBoard, collectAllConstraints( aBoard ) );

    std::unordered_set<KIID>     clusterShapes;
    std::vector<PCB_CONSTRAINT*> clusterConstraints;
    collectConstraintCluster( shapeToConstraints, aDragged.m_item, clusterShapes, clusterConstraints );

    std::vector<PCB_SHAPE*> shapes = resolveClusterShapes( aBoard, clusterShapes );

    if( shapes.empty() || clusterConstraints.empty() )
        return diag;

    BOARD_CONSTRAINT_ADAPTER adapter;

    if( !adapter.Build( shapes, clusterConstraints ) )
        return diag;

    bool solved = adapter.Solve( aDragged, aCursor, aStabilize );

    if( !solved )
        return diag;   // leave geometry untouched on a failed/diverged solve

    PCB_SHAPE* draggedShape = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( aDragged.m_item, true ) );

    // Stage each neighbor (not the dragged shape, which the caller stages itself) just before
    // its geometry is written, so the whole re-derivation is one undoable transaction.
    std::vector<PCB_SHAPE*> changed = adapter.Apply(
            [&]( PCB_SHAPE* aShape )
            {
                if( ( aIncludeDragged || aShape != draggedShape ) && aBeforeModify )
                    aBeforeModify( aShape );
            } );

    diag = adapter.Diagnose();
    diag.solved = true;

    if( aModified )
    {
        for( PCB_SHAPE* shape : changed )
        {
            if( aIncludeDragged || shape != draggedShape )
                aModified->push_back( shape );
        }
    }

    return diag;
}


wxString ConstraintStateSummary( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag, bool* aOverConstrained )
{
    bool over = !aDiag.errored.empty() || !aDiag.conflicting.empty();

    if( aOverConstrained )
        *aOverConstrained = over;

    if( !aDiag.errored.empty() )
        return _( "Geometric constraints: error (a constraint has invalid or missing members)" );

    if( !aDiag.conflicting.empty() )
        return _( "Geometric constraints: over-constrained" );

    if( aDiag.shapeStates.empty() )
        return _( "No geometric constraints" );

    if( aDiag.totalFreeDof == 0 )
        return _( "Geometric constraints: fully constrained" );

    return wxString::Format( _( "Geometric constraints: under-constrained "
                                "(%d degrees of freedom)" ),
                             aDiag.totalFreeDof );
}


CONSTRAINT_DIAGNOSIS ApplyConstraintImmediately( BOARD* aBoard, const PCB_CONSTRAINT* aConstraint,
                                                 std::vector<PCB_SHAPE*>* aModified,
                                                 const std::function<void( PCB_SHAPE* )>& aBeforeModify )
{
    if( !aBoard || !aConstraint || aConstraint->GetMembers().empty() )
        return {};

    // Pin the first member's anchor where it is; the solver moves the rest of the cluster (and the
    // pinned shape's own free geometry) to meet the new relation.  A WHOLE member is pinned at the
    // shape's first concrete anchor -- START for a segment/arc, CENTER for a circle.
    CONSTRAINT_MEMBER pin = aConstraint->GetMembers().front();

    if( pin.m_anchor == CONSTRAINT_ANCHOR::WHOLE )
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( pin.m_item, true ) );

        if( !shape )
            return {};

        std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( shape );

        if( anchors.empty() )
            return {};

        pin.m_anchor = anchors.front().anchor;
    }

    std::optional<VECTOR2I> pos = ConstraintAnchorPosition( aBoard, pin );

    if( !pos )
        return {};

    // The pinned shape itself can move (e.g. a fixed-length segment's far end), and the caller does
    // not stage it separately, so report it too.
    return SolveCluster( aBoard, pin, *pos, aModified, aBeforeModify, /* aIncludeDragged */ true,
                         /* aStabilize */ true );
}


void ReSolveShapeClusters( BOARD* aBoard, const std::vector<PCB_SHAPE*>& aShapes, std::vector<PCB_SHAPE*>* aModified,
                           const std::function<void( PCB_SHAPE* )>& aBeforeModify )
{
    if( !aBoard )
        return;

    auto shapeToConstraints = buildShapeConstraintMap( aBoard, collectAllConstraints( aBoard ) );

    // Solve each affected cluster once, no matter how many of its shapes were edited.
    std::set<KIID> visited;

    for( PCB_SHAPE* shape : aShapes )
    {
        if( !shape || visited.count( shape->m_Uuid ) || !shapeToConstraints.count( shape->m_Uuid ) )
            continue;

        std::unordered_set<KIID>     clusterShapes;
        std::vector<PCB_CONSTRAINT*> clusterConstraints;
        collectConstraintCluster( shapeToConstraints, shape->m_Uuid, clusterShapes, clusterConstraints, &visited );

        std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( shape );

        if( anchors.empty() )
            continue;

        SolveCluster( aBoard, { shape->m_Uuid, anchors.front().anchor }, anchors.front().pos, aModified, aBeforeModify,
                      true );
    }
}


void ReSolveAfterShapeResize( BOARD* aBoard, PCB_SHAPE* aShape, std::vector<PCB_SHAPE*>* aModified,
                              const std::function<void( PCB_SHAPE* )>& aBeforeModify )
{
    if( !aBoard || !aShape )
        return;

    auto shapeToConstraints = buildShapeConstraintMap( aBoard, collectAllConstraints( aBoard ) );

    if( !shapeToConstraints.count( aShape->m_Uuid ) )
        return;

    std::unordered_set<KIID>     clusterShapes;
    std::vector<PCB_CONSTRAINT*> clusterConstraints;
    collectConstraintCluster( shapeToConstraints, aShape->m_Uuid, clusterShapes, clusterConstraints );

    std::vector<PCB_SHAPE*> shapes = resolveClusterShapes( aBoard, clusterShapes );

    if( shapes.empty() || clusterConstraints.empty() )
        return;

    BOARD_CONSTRAINT_ADAPTER adapter;

    if( !adapter.Build( shapes, clusterConstraints ) || !adapter.SolveAfterResize( aShape->m_Uuid ) )
        return;

    std::vector<PCB_SHAPE*> changed = adapter.Apply(
            [&]( PCB_SHAPE* aChanged )
            {
                if( aBeforeModify )
                    aBeforeModify( aChanged );
            } );

    if( aModified )
    {
        for( PCB_SHAPE* shape : changed )
            aModified->push_back( shape );
    }
}


BOARD_CONSTRAINT_DIAGNOSTICS DiagnoseBoardConstraints( BOARD* aBoard )
{
    BOARD_CONSTRAINT_DIAGNOSTICS result;

    if( !aBoard )
        return result;

    std::vector<PCB_CONSTRAINT*> boardConstraints = collectAllConstraints( aBoard );

    std::unordered_map<KIID, std::vector<PCB_CONSTRAINT*>> shapeToConstraints =
            buildShapeConstraintMap( aBoard, boardConstraints, &result.errored );

    std::set<KIID> visitedShapes;

    for( const auto& [seedShape, seedConstraints] : shapeToConstraints )
    {
        if( visitedShapes.count( seedShape ) )
            continue;

        std::unordered_set<KIID>     clusterShapes;
        std::vector<PCB_CONSTRAINT*> clusterConstraints;
        collectConstraintCluster( shapeToConstraints, seedShape, clusterShapes, clusterConstraints,
                                  &visitedShapes );

        std::vector<PCB_SHAPE*> shapes = resolveClusterShapes( aBoard, clusterShapes );

        BOARD_CONSTRAINT_ADAPTER adapter;

        if( shapes.empty() || clusterConstraints.empty() || !adapter.Build( shapes, clusterConstraints ) )
            continue;

        // A constraint Build() could not map (e.g. a shape was changed to an incompatible kind) is
        // not enforced; flag it so the user sees it is broken rather than silently ignored.
        result.errored.insert( result.errored.end(), adapter.UnmappedConstraints().begin(),
                               adapter.UnmappedConstraints().end() );

        // Solve first so the residual check sees a real contradiction, not an unsolved constraint.
        // The board is not touched.
        adapter.Solve( true );

        CONSTRAINT_DIAGNOSIS diag = adapter.Diagnose();

        CONSTRAINT_STATE state = CONSTRAINT_STATE::WELL_CONSTRAINED;

        if( diag.IsOverConstrained() )
            state = CONSTRAINT_STATE::OVER_CONSTRAINED;
        else if( diag.IsUnderConstrained() )
            state = CONSTRAINT_STATE::UNDER_CONSTRAINED;

        for( PCB_SHAPE* shape : shapes )
            result.shapeStates[shape->m_Uuid] = state;

        if( diag.freeDof > 0 )
            result.totalFreeDof += diag.freeDof;

        result.conflicting.insert( result.conflicting.end(), diag.conflicting.begin(),
                                   diag.conflicting.end() );
        result.redundant.insert( result.redundant.end(), diag.redundant.begin(),
                                 diag.redundant.end() );
    }

    // A constraint with a dangling member is also unmappable, so it can be flagged by both the
    // adjacency scan and Build; report each errored constraint once.
    std::sort( result.errored.begin(), result.errored.end() );
    result.errored.erase( std::unique( result.errored.begin(), result.errored.end() ),
                          result.errored.end() );

    return result;
}
