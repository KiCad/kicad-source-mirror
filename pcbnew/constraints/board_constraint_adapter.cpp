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
        if( vars.kind == SHAPE_KIND::ARC )
            return vars.arcStartX;

        // A circle has no endpoints; its startX holds the centre, so never alias them.
        return vars.kind == SHAPE_KIND::CIRCLE ? -1 : vars.startX;
    case CONSTRAINT_ANCHOR::END:
        if( vars.kind == SHAPE_KIND::ARC )
            return vars.arcEndX;

        return vars.kind == SHAPE_KIND::CIRCLE ? -1 : vars.endX;
    case CONSTRAINT_ANCHOR::CENTER:
        return ( vars.kind == SHAPE_KIND::CIRCLE || vars.kind == SHAPE_KIND::ARC ) ? vars.startX : -1;
    default:
        return -1;
    }
}


bool BOARD_CONSTRAINT_ADAPTER::Build( const std::vector<PCB_SHAPE*>& aShapes,
                                      const std::vector<PCB_CONSTRAINT*>& aConstraints )
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
        else
        {
            continue;   // other shapes are not mapped
        }

        // A locked shape is an immovable reference: record its parameter span so the solver moves
        // only the unlocked geometry sharing the cluster.
        if( shape->IsLocked() )
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
            int       p = members.size() == 2 ? anchorParamIndex( members[0] ) : -1;
            GCS::Line l;

            if( p >= 0 && lineFor( members[1], l ) )
            {
                GCS::Point point = pointAt( p );
                m_gcs->addConstraintPointOnLine( point, l, tag );
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
            GCS::Circle c1, c2;

            if( members.size() == 2 && circleFor( members[0], c1 ) && circleFor( members[1], c2 ) )
            {
                m_gcs->addConstraintP2PCoincident( c1.center, c2.center, tag );
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


bool BOARD_CONSTRAINT_ADAPTER::Solve()
{
    if( !m_built )
        return false;

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    return ret == GCS::Success || ret == GCS::Converged;
}


bool BOARD_CONSTRAINT_ADAPTER::Solve( const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor )
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

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );

    return ret == GCS::Success || ret == GCS::Converged;
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

        VECTOR2I start( KiROUND( denormalizeX( m_params[vars.startX] ) ),
                        KiROUND( denormalizeY( m_params[vars.startX + 1] ) ) );
        VECTOR2I end( KiROUND( denormalizeX( m_params[vars.endX] ) ),
                      KiROUND( denormalizeY( m_params[vars.endX + 1] ) ) );

        if( start == vars.shape->GetStart() && end == vars.shape->GetEnd() )
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

    // .solved reflects the last Solve(), which Diagnose() does not run; the caller sets it.
    return diag;
}


CONSTRAINT_DIAGNOSIS SolveCluster( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged,
                                   const VECTOR2I& aCursor, std::vector<PCB_SHAPE*>* aModified,
                                   const std::function<void( PCB_SHAPE* )>& aBeforeModify,
                                   bool aIncludeDragged )
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

    bool solved = adapter.Solve( aDragged, aCursor );

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
    return SolveCluster( aBoard, pin, *pos, aModified, aBeforeModify, /* aIncludeDragged */ true );
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
