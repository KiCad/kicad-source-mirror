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
#include <iterator>
#include <ranges>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <wx/debug.h>

#include <board.h>
#include <footprint.h>
#include <geometry/seg.h>
#include <math/util.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>

#include <constraints/constraint_builder.h>

#include <GCS.h>


// IU are nanometres; one normalized unit is one millimetre (1e6 IU).  Squaring raw IU in the
// residuals (~1e16) is badly conditioned, so the cluster is solved in this millimetre frame.
static constexpr double IU_PER_NORM_UNIT = 1e6;

// Bound the solver so an interactive drag can never stall the UI thread.
static constexpr int MAX_SOLVE_ITERATIONS = 100;

// Shares the drag and stabilize temporary subsystem kept far weaker so it only fills the null space
// and never fights a real drive or cursor pin
static constexpr double STAY_PUT_WEIGHT = 1e-4;



// Every constraint owned by the board or by any of its footprints.  In the footprint editor the
// "board" is a footprint holder, so footprint-scoped constraints must be gathered too.
static std::vector<PCB_CONSTRAINT*> collectAllConstraints( BOARD* aBoard )
{
    std::vector<PCB_CONSTRAINT*> all( aBoard->Constraints().begin(), aBoard->Constraints().end() );

    for( FOOTPRINT* footprint : aBoard->Footprints() )
        all.insert( all.end(), footprint->Constraints().begin(), footprint->Constraints().end() );

    return all;
}


bool BoardHasConstraints( BOARD* aBoard )
{
    if( !aBoard )
        return false;

    if( !aBoard->Constraints().empty() )
        return true;

    return std::ranges::any_of( aBoard->Footprints(),
                                []( const FOOTPRINT* aFootprint )
                                { return !aFootprint->Constraints().empty(); } );
}


bool ConstraintItemIsLocked( const BOARD_ITEM* aItem )
{
    if( !aItem )
        return false;

    if( aItem->IsLocked() )
        return true;

    // FOOTPRINT::IsLocked() reports the raw lock bit, so mirror BOARD_ITEM::IsLocked()'s
    // footprint-editor exemption before consulting the parent.
    const BOARD* board = aItem->GetBoard();

    if( !board || board->GetBoardUse() == BOARD_USE::FPHOLDER )
        return false;

    const FOOTPRINT* parent = aItem->GetParentFootprint();

    return parent && parent->IsLocked();
}


// Adjacency from a member KIID to the constraints touching it.  When @p aErrored is given, any
// constraint that cannot be satisfied -- it has no members, or a member that does not resolve to a
// constrainable item (a shape or dimension) -- is recorded there (error state), since such a
// constraint never reaches the solver's per-cluster mapping where the remaining error cases are caught.
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

            // Members must resolve to a constrainable item (shape or dimension); a deleted or
            // wrong-typed item is an error.
            if( aErrored && !ResolveConstrainableItem( aBoard, member.m_item ) )
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
// Resolve a set of KIIDs to the live items of type T among them, dropping other kinds and deleted
// items.
template <typename T>
static std::vector<T*> resolveClusterItems( BOARD* aBoard, const std::unordered_set<KIID>& aIds )
{
    std::vector<T*> items;

    for( const KIID& id : aIds )
    {
        if( T* item = dynamic_cast<T*>( aBoard->ResolveItem( id, true ) ) )
            items.push_back( item );
    }

    return items;
}


static std::vector<PCB_SHAPE*> resolveClusterShapes( BOARD* aBoard, const std::unordered_set<KIID>& aIds )
{
    return resolveClusterItems<PCB_SHAPE>( aBoard, aIds );
}


static std::vector<PCB_DIMENSION_BASE*> resolveClusterDimensions( BOARD* aBoard,
                                                                  const std::unordered_set<KIID>& aIds )
{
    return resolveClusterItems<PCB_DIMENSION_BASE>( aBoard, aIds );
}


// The stored angular value is the undirected corner angle [0, 180]; planegcs drives the directed
// angle from l1's direction (p1->p2) to l2's.  Return the directed target (radians) that realizes
// the corner for the present geometry.
//
// The corner rays run from the shared vertex outward, so the vertex's endpoint parity fixes how the
// corner maps to the directed angle: corner = |Normalize180( theta + (vB - vA) * pi )|, where vA/vB
// are the vertex endpoint indices (0 = p1, 1 = p2).  Only two directed targets realize the corner
// for that parity, { alpha, -alpha } shifted by (vB - vA) * pi; the one nearest the current directed
// angle keeps the configuration rather than flipping to the mirror.  Considering the other parity's
// targets too (the naive four-candidate set) would let an obtuse->acute value edit pick the stale
// complement and no-op the solve.  Not shared with the arc-sweep mapping, which is mod 2*pi.
// A zero-length line has no direction, so its angle equation is singular; skip mapping it.
static bool isDegenerateLine( const GCS::Line& aLine )
{
    return std::hypot( *aLine.p2.x - *aLine.p1.x, *aLine.p2.y - *aLine.p1.y ) < 1e-9;
}


static double directedAngleForCorner( const GCS::Line& aL1, const GCS::Line& aL2, double aCornerDeg )
{
    const double ax[2] = { *aL1.p1.x, *aL1.p2.x };
    const double ay[2] = { *aL1.p1.y, *aL1.p2.y };
    const double bx[2] = { *aL2.p1.x, *aL2.p2.x };
    const double by[2] = { *aL2.p1.y, *aL2.p2.y };

    int    vA = 0, vB = 0;
    double best = std::numeric_limits<double>::max();

    for( int i = 0; i < 2; ++i )
    {
        for( int j = 0; j < 2; ++j )
        {
            double dist = std::hypot( ax[i] - bx[j], ay[i] - by[j] );

            if( dist < best )
            {
                best = dist;
                vA = i;
                vB = j;
            }
        }
    }

    double d1x = ax[1] - ax[0], d1y = ay[1] - ay[0];
    double d2x = bx[1] - bx[0], d2y = by[1] - by[0];
    double theta = std::atan2( d1x * d2y - d1y * d2x, d1x * d2x + d1y * d2y );

    // Fold to [0, pi] so a value from an out-of-range file, API, or an earlier signed-angle board
    // still maps to a well-defined corner.
    double alpha = std::abs( std::remainder( aCornerDeg * M_PI / 180.0, 2.0 * M_PI ) );
    double shift = ( vB - vA ) * M_PI;
    double c1 = alpha - shift;
    double c2 = -alpha - shift;

    double d1 = std::abs( std::remainder( theta - c1, 2.0 * M_PI ) );
    double d2 = std::abs( std::remainder( theta - c2, 2.0 * M_PI ) );

    return d1 <= d2 ? c1 : c2;
}


// The stored value is the unsigned swept angle; the solver constrains the endpoints' polar
// separation endAngle - startAngle.  KiCad arcs are canonically positive-sweep (SetArcGeometry
// swaps the ends so IsClockwiseArc is always false), so the target is +alpha.  Return its
// representative (mod 2*pi) nearest the current separation, so the solver only rotates an endpoint
// rather than jumping the param a full turn.  This is the arc's own mod-2*pi mapping, deliberately
// not the line-angle mod-pi one.
static double arcSweepTarget( double aStartAngle, double aEndAngle, double aSweepDeg )
{
    double current = aEndAngle - aStartAngle;
    double alpha = std::abs( aSweepDeg ) * M_PI / 180.0;

    return current - std::remainder( current - alpha, 2.0 * M_PI );
}


BOARD_CONSTRAINT_ADAPTER::BOARD_CONSTRAINT_ADAPTER() :
        m_gcs( std::make_unique<GCS::System>() )
{
}


BOARD_CONSTRAINT_ADAPTER::~BOARD_CONSTRAINT_ADAPTER()
{
    // GCS::System frees the Constraint objects it owns; m_params outlives it (member order).
    m_gcs->clear();
}


int BOARD_CONSTRAINT_ADAPTER::pushParam( double aValue )
{
    // Pin and hold targets pushed here never reclaimed since the deque cannot shrink while GCS holds
    // pointers into it and growth stays bounded since every caller builds a fresh adapter per solve
    m_params.push_back( aValue );
    return static_cast<int>( m_params.size() ) - 1;
}


void BOARD_CONSTRAINT_ADAPTER::recordReferenceValue( PCB_CONSTRAINT* aConstraint )
{
    if( aConstraint && !aConstraint->IsDriving() )
        m_referenceConstraints.push_back( aConstraint );
}


void BOARD_CONSTRAINT_ADAPTER::ApplyReferenceValues( const std::function<void( BOARD_ITEM* )>& aBeforeWrite )
{
    if( !m_built )
        return;

    for( PCB_CONSTRAINT* constraint : m_referenceConstraints )
    {
        const std::vector<CONSTRAINT_MEMBER>& members = constraint->GetMembers();

        if( members.empty() )
            continue;

        auto it = m_shapeVars.find( members.front().m_item );

        if( it == m_shapeVars.end() )
            continue;

        const PCB_SHAPE*      shape = it->second.shape;
        std::optional<double> value;
        double                tol = 1.0;   // IU for a length/radius; degrees for an angle

        // Measure from the shape geometry Apply() just wrote, not the solver param, so a settled
        // solve re-measures the same rounded value and never churns undo.
        switch( constraint->GetConstraintType() )
        {
        case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:

            // A two-point form has no owning segment so re-measure the distance between its two
            // member anchors from the same solved params Apply rounded from
            if( members.size() == 2 )
            {
                auto anchorPos = [&]( const CONSTRAINT_MEMBER& aMember ) -> std::optional<VECTOR2I>
                {
                    ANCHOR_PARAMS anchor = anchorParams( aMember );

                    if( !anchor.IsValid() )
                        return std::nullopt;

                    return VECTOR2I( KiROUND( denormalizeX( m_params[anchor.x] ) ),
                                     KiROUND( denormalizeY( m_params[anchor.y] ) ) );
                };

                std::optional<VECTOR2I> pa = anchorPos( members[0] );
                std::optional<VECTOR2I> pb = anchorPos( members[1] );

                if( pa && pb )
                {
                    // An orthogonal dimension measures one axis so re-measure that axis component
                    // an aligned or radial two-point form measures the euclidean distance instead
                    if( PCB_DIM_ORTHOGONAL* ortho = orthogonalDimensionForMembers( members ) )
                    {
                        bool horizontal = ortho->GetOrientation() == PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL;
                        value = horizontal ? std::abs( pb->x - pa->x ) : std::abs( pb->y - pa->y );
                    }
                    else
                    {
                        value = ( *pb - *pa ).EuclideanNorm();
                    }
                }
            }
            else
            {
                value = ( shape->GetEnd() - shape->GetStart() ).EuclideanNorm();
            }

            break;

        case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
            value = shape->GetRadius();
            break;

        case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION:
        {
            if( members.size() != 2 )
                break;

            auto other = m_shapeVars.find( members[1].m_item );

            if( other == m_shapeVars.end() )
                break;

            const PCB_SHAPE* shapeB = other->second.shape;
            value = MeasureCornerAngle( SEG( shape->GetStart(), shape->GetEnd() ),
                                        SEG( shapeB->GetStart(), shapeB->GetEnd() ) ).AsDegrees();
            tol = 1e-3;
            break;
        }

        case PCB_CONSTRAINT_TYPE::ARC_ANGLE:
            value = shape->GetArcAngle().AsDegrees();
            tol = 1e-3;
            break;

        default:
            break;
        }

        if( !value )
            continue;

        std::optional<double> current = constraint->GetValue();

        if( current && std::abs( *value - *current ) <= tol )
            continue;

        if( aBeforeWrite )
            aBeforeWrite( constraint );

        constraint->SetValue( value );
    }
}


BOARD_CONSTRAINT_ADAPTER::ANCHOR_PARAMS
BOARD_CONSTRAINT_ADAPTER::anchorParams( const CONSTRAINT_MEMBER& aMember ) const
{
    auto it = m_shapeVars.find( aMember.m_item );

    if( it == m_shapeVars.end() )
        return {};

    const SHAPE_VARS& vars = it->second;

    // Segment bezier and dimension point-pair store endpoints in startX and endX while a circle or
    // closed ellipse parks its centre in startX instead so those kinds must never alias endpoints
    bool hasEndpoints = vars.kind == SHAPE_KIND::SEGMENT || vars.kind == SHAPE_KIND::BEZIER
                        || vars.kind == SHAPE_KIND::POINT_PAIR;

    // Consecutive x and y pair for these kinds
    auto pairAt = []( int aXIndex ) -> ANCHOR_PARAMS
    {
        return aXIndex >= 0 ? ANCHOR_PARAMS{ aXIndex, aXIndex + 1 } : ANCHOR_PARAMS();
    };

    // A rect exposes only its four indexed corners each an alias over a mixed start and end pair so
    // START END and CENTER never resolve indices 0 to 3 follow the canonical TL TR BR BL order
    if( vars.kind == SHAPE_KIND::RECT )
    {
        if( aMember.m_anchor != CONSTRAINT_ANCHOR::VERTEX || aMember.m_index < 0 || aMember.m_index > 3 )
            return {};

        int leftX = vars.startIsLeft ? vars.startX : vars.endX;
        int rightX = vars.startIsLeft ? vars.endX : vars.startX;
        int topY = ( vars.startIsTop ? vars.startX : vars.endX ) + 1;
        int botY = ( vars.startIsTop ? vars.endX : vars.startX ) + 1;

        switch( aMember.m_index )
        {
        case 0: return { leftX, topY };
        case 1: return { rightX, topY };
        case 2: return { rightX, botY };
        default: return { leftX, botY };
        }
    }

    // A polygon exposes only its indexed outline-0 vertices each a consecutive pair at
    // startX plus 2 times index a stale index from a shrunk outline resolves to nothing
    if( vars.kind == SHAPE_KIND::POLYGON )
    {
        if( aMember.m_anchor != CONSTRAINT_ANCHOR::VERTEX || aMember.m_index < 0
            || aMember.m_index >= vars.vertexCount )
        {
            return {};
        }

        return pairAt( vars.startX + 2 * aMember.m_index );
    }

    switch( aMember.m_anchor )
    {
    case CONSTRAINT_ANCHOR::START:
        if( vars.kind == SHAPE_KIND::ARC || vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            return pairAt( vars.arcStartX );

        return hasEndpoints ? pairAt( vars.startX ) : ANCHOR_PARAMS();
    case CONSTRAINT_ANCHOR::END:
        if( vars.kind == SHAPE_KIND::ARC || vars.kind == SHAPE_KIND::ELLIPSE_ARC )
            return pairAt( vars.arcEndX );

        return hasEndpoints ? pairAt( vars.endX ) : ANCHOR_PARAMS();
    case CONSTRAINT_ANCHOR::CENTER:
        return hasEndpoints ? ANCHOR_PARAMS() : pairAt( vars.startX );
    default:
        return {};
    }
}


PCB_DIM_ORTHOGONAL* BOARD_CONSTRAINT_ADAPTER::orthogonalDimensionForMembers(
        const std::vector<CONSTRAINT_MEMBER>& aMembers ) const
{
    if( aMembers.size() != 2 || aMembers[0].m_item != aMembers[1].m_item )
        return nullptr;

    const CONSTRAINT_ANCHOR a0 = aMembers[0].m_anchor;
    const CONSTRAINT_ANCHOR a1 = aMembers[1].m_anchor;

    const bool startEnd = ( a0 == CONSTRAINT_ANCHOR::START && a1 == CONSTRAINT_ANCHOR::END )
                          || ( a0 == CONSTRAINT_ANCHOR::END && a1 == CONSTRAINT_ANCHOR::START );

    if( !startEnd )
        return nullptr;

    auto it = m_shapeVars.find( aMembers[0].m_item );

    if( it == m_shapeVars.end() || it->second.kind != SHAPE_KIND::POINT_PAIR || !it->second.dimension
            || it->second.dimension->Type() != PCB_DIM_ORTHOGONAL_T )
    {
        return nullptr;
    }

    return static_cast<PCB_DIM_ORTHOGONAL*>( it->second.dimension );
}


bool BOARD_CONSTRAINT_ADAPTER::Build( const std::vector<PCB_SHAPE*>&          aShapes,
                                      const std::vector<PCB_CONSTRAINT*>&     aConstraints,
                                      const std::set<KIID>*                   aFixedShapes,
                                      const std::vector<PCB_DIMENSION_BASE*>& aDimensions )
{
    m_params.clear();
    m_shapeVars.clear();
    m_tagToConstraint.clear();
    m_nonDrivingTags.clear();
    m_tagMembers.clear();
    m_referenceConstraints.clear();
    m_unmapped.clear();
    m_angleConstrainedShapes.clear();
    m_gcs->clear();
    m_built = false;

    if( aShapes.empty() && aDimensions.empty() )
        return false;

    // Centre on the first item's start point so normalized coordinates stay small for a board far
    // from the origin.
    VECTOR2I origin = !aShapes.empty() ? aShapes.front()->GetStart() : aDimensions.front()->GetStart();
    m_originX = origin.x;
    m_originY = origin.y;
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
        else if( shape->GetShape() == SHAPE_T::RECTANGLE )
        {
            // Only the two stored corners are params the four VERTEX corners alias mixed pairs of
            // them through anchorParams so rectness can never be violated corner roles frozen here
            vars.kind = SHAPE_KIND::RECT;
            vars.startIsLeft = shape->GetStart().x <= shape->GetEnd().x;
            vars.startIsTop = shape->GetStart().y <= shape->GetEnd().y;
            vars.startX = pushParam( normalizeX( shape->GetStart().x ) );
            pushParam( normalizeY( shape->GetStart().y ) );
            vars.endX = pushParam( normalizeX( shape->GetEnd().x ) );
            pushParam( normalizeY( shape->GetEnd().y ) );
        }
        else if( shape->GetShape() == SHAPE_T::POLY )
        {
            // Only a single hole-free arc-free outline is modeled since write-back rebuilds one
            // outline and would destroy anything else so other polys are skipped and read unmapped
            if( !ConstraintPolygonIsModelable( shape ) )
                continue;

            const SHAPE_LINE_CHAIN& outline = shape->GetPolyShape().COutline( 0 );

            vars.kind = SHAPE_KIND::POLYGON;
            vars.vertexCount = outline.PointCount();
            vars.startX = pushParam( normalizeX( outline.CPoint( 0 ).x ) );
            pushParam( normalizeY( outline.CPoint( 0 ).y ) );

            for( int i = 1; i < outline.PointCount(); ++i )
            {
                pushParam( normalizeX( outline.CPoint( i ).x ) );
                pushParam( normalizeY( outline.CPoint( i ).y ) );
            }
        }
        else if( shape->GetShape() == SHAPE_T::BEZIER )
        {
            // Only the endpoints participate in the solve the control handles are not solver
            // variables and follow their adjacent endpoint in Apply
            vars.kind = SHAPE_KIND::BEZIER;
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

        // Freeze a locked or caller-pinned shape's whole span so the solver moves only the rest of
        // the cluster.
        if( ConstraintItemIsLocked( shape ) || ( aFixedShapes && aFixedShapes->count( shape->m_Uuid ) ) )
            lockedRanges.emplace_back( firstParam, static_cast<int>( m_params.size() ) );

        m_shapeVars[shape->m_Uuid] = vars;
    }

    // Each dimension contributes its two feature points, so a coincident constraint can pull the
    // dimension along with the shape it is bound to.
    for( PCB_DIMENSION_BASE* dimension : aDimensions )
    {
        SHAPE_VARS vars;
        vars.kind = SHAPE_KIND::POINT_PAIR;
        vars.dimension = dimension;
        vars.startX = pushParam( normalizeX( dimension->GetStart().x ) );
        pushParam( normalizeY( dimension->GetStart().y ) );

        // Only aligned/orthogonal/radial dimensions have a second feature point; a leader or centre
        // mark's end is a control point, so it is never a bindable anchor (endX stays -1).
        switch( dimension->Type() )
        {
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_RADIAL_T:
            vars.endX = pushParam( normalizeX( dimension->GetEnd().x ) );
            pushParam( normalizeY( dimension->GetEnd().y ) );
            break;

        default:
            break;
        }

        m_shapeVars[dimension->m_Uuid] = vars;
    }

    // Params the solver may not change.  Grounded points, driving constants (lengths, radii) and
    // locked-shape params go here; everything else stays an unknown.
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

    // Anchor x and y need not be consecutive since a rect corner aliases mixed start end params
    // pointAt stays for SHAPE_VARS internal consecutive pairs
    auto pointFor = [&]( const ANCHOR_PARAMS& aParams ) -> GCS::Point
    {
        return GCS::Point{ &m_params[aParams.x], &m_params[aParams.y] };
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
            // One whole segment aligns its own two endpoints while two point anchors align the pair
            // the user picked so a corner can be leveled without a segment between the points
            if( members.size() == 1 )
            {
                GCS::Line l;

                if( lineFor( members[0], l ) )
                {
                    m_gcs->addConstraintHorizontal( l, tag );
                    mapped = true;
                }
            }
            else if( members.size() == 2 )
            {
                ANCHOR_PARAMS a = anchorParams( members[0] );
                ANCHOR_PARAMS b = anchorParams( members[1] );

                if( a.IsValid() && b.IsValid() )
                {
                    GCS::Point p1 = pointFor( a );
                    GCS::Point p2 = pointFor( b );
                    m_gcs->addConstraintHorizontal( p1, p2, tag );
                    mapped = true;
                }
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::VERTICAL:
        {
            if( members.size() == 1 )
            {
                GCS::Line l;

                if( lineFor( members[0], l ) )
                {
                    m_gcs->addConstraintVertical( l, tag );
                    mapped = true;
                }
            }
            else if( members.size() == 2 )
            {
                ANCHOR_PARAMS a = anchorParams( members[0] );
                ANCHOR_PARAMS b = anchorParams( members[1] );

                if( a.IsValid() && b.IsValid() )
                {
                    GCS::Point p1 = pointFor( a );
                    GCS::Point p2 = pointFor( b );
                    m_gcs->addConstraintVertical( p1, p2, tag );
                    mapped = true;
                }
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::COINCIDENT:
        {
            ANCHOR_PARAMS a = members.size() == 2 ? anchorParams( members[0] ) : ANCHOR_PARAMS();
            ANCHOR_PARAMS b = members.size() == 2 ? anchorParams( members[1] ) : ANCHOR_PARAMS();

            if( a.IsValid() && b.IsValid() )
            {
                GCS::Point p1 = pointFor( a );
                GCS::Point p2 = pointFor( b );
                m_gcs->addConstraintP2PCoincident( p1, p2, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::FIXED_POSITION:
        {
            ANCHOR_PARAMS a = members.size() == 1 ? anchorParams( members[0] ) : ANCHOR_PARAMS();

            if( a.IsValid() )
            {
                fixedParams.insert( a.x );
                fixedParams.insert( a.y );
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
            ANCHOR_PARAMS p = members.size() == 2 ? anchorParams( members[0] ) : ANCHOR_PARAMS();
            GCS::Line     l;
            GCS::Circle   circ;
            GCS::Ellipse  ell;

            if( p.IsValid() && lineFor( members[1], l ) )
            {
                GCS::Point point = pointFor( p );
                m_gcs->addConstraintPointOnLine( point, l, tag );
                mapped = true;
            }
            else if( p.IsValid() && circleFor( members[1], circ ) )
            {
                // A circle or arc target keeps the point on its circumference.
                GCS::Point point = pointFor( p );
                m_gcs->addConstraintPointOnCircle( point, circ, tag );
                mapped = true;
            }
            else if( p.IsValid() && ellipseFor( members[1], ell ) )
            {
                GCS::Point point = pointFor( p );
                m_gcs->addConstraintPointOnEllipse( point, ell, tag );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::MIDPOINT:
        {
            ANCHOR_PARAMS p = members.size() == 2 ? anchorParams( members[0] ) : ANCHOR_PARAMS();
            GCS::Line     seg;

            // A midpoint constraint is the segment's endpoints being symmetric about the point.
            if( p.IsValid() && lineFor( members[1], seg ) )
            {
                GCS::Point mid = pointFor( p );
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
            ANCHOR_PARAMS a = members.size() == 3 ? anchorParams( members[0] ) : ANCHOR_PARAMS();
            ANCHOR_PARAMS b = members.size() == 3 ? anchorParams( members[1] ) : ANCHOR_PARAMS();
            GCS::Line     axis;

            if( a.IsValid() && b.IsValid() && lineFor( members[2], axis ) )
            {
                GCS::Point pa = pointFor( a );
                GCS::Point pb = pointFor( b );
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
                m_gcs->addConstraintP2PDistance( l.p1, l.p2, &m_params[len], tag, constraint->IsDriving() );
                recordReferenceValue( constraint );

                if( constraint->IsDriving() )
                {
                    if( auto it = m_shapeVars.find( members[0].m_item ); it != m_shapeVars.end() )
                        it->second.fixedLengthParam = len;
                }

                mapped = true;
            }
            // Two point anchors fix the distance between them with no segment between so a driving
            // aligned dimension can drive its own endpoints and the geometry coincident with them
            else if( members.size() == 2 && constraint->HasValue() )
            {
                ANCHOR_PARAMS a = anchorParams( members[0] );
                ANCHOR_PARAMS b = anchorParams( members[1] );

                if( a.IsValid() && b.IsValid() )
                {
                    PCB_DIM_ORTHOGONAL* ortho = orthogonalDimensionForMembers( members );

                    if( ortho )
                    {
                        // An orthogonal dimension measures one axis so the driving length fixes that
                        // axis and matches updateGeometry sign convention keeping the current side
                        bool   horiz = ortho->GetOrientation() == PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL;
                        int    aAxis = horiz ? a.x : a.y;
                        int    bAxis = horiz ? b.x : b.y;
                        double gap = m_params[bAxis] - m_params[aAxis];
                        double sign = gap >= 0.0 ? 1.0 : -1.0;
                        int    len = pushConstant( sign * *constraint->GetValue() );

                        m_gcs->addConstraintDifference( &m_params[aAxis], &m_params[bAxis],
                                                        &m_params[len], tag, constraint->IsDriving() );
                    }
                    else
                    {
                        GCS::Point pa = pointFor( a );
                        GCS::Point pb = pointFor( b );
                        int        len = pushConstant( *constraint->GetValue() );
                        m_gcs->addConstraintP2PDistance( pa, pb, &m_params[len], tag,
                                                         constraint->IsDriving() );
                    }

                    recordReferenceValue( constraint );
                    mapped = true;
                }
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
        {
            GCS::Circle c;

            if( members.size() == 1 && constraint->HasValue() && circleFor( members[0], c ) )
            {
                int rad = pushConstant( *constraint->GetValue() );
                m_gcs->addConstraintCircleRadius( c, &m_params[rad], tag, constraint->IsDriving() );
                recordReferenceValue( constraint );
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
                    && lineFor( members[1], l2 ) && !isDegenerateLine( l1 ) && !isDegenerateLine( l2 ) )
            {
                int angle = pushParam( directedAngleForCorner( l1, l2, *constraint->GetValue() ) );
                fixedParams.insert( angle );
                m_gcs->addConstraintL2LAngle( l1, l2, &m_params[angle], tag, constraint->IsDriving() );
                recordReferenceValue( constraint );
                mapped = true;
            }

            break;
        }

        case PCB_CONSTRAINT_TYPE::ARC_ANGLE:
        {
            auto vit = members.size() == 1 ? m_shapeVars.find( members[0].m_item ) : m_shapeVars.end();

            // A value outside (0, 360) is a degenerate sweep (from a corrupt file or the API); leave
            // it unmapped so it reads as errored rather than merging the endpoints.
            bool validSweep = constraint->HasValue() && *constraint->GetValue() > 0.0
                              && *constraint->GetValue() < 360.0;

            if( vit != m_shapeVars.end() && vit->second.kind == SHAPE_KIND::ARC && validSweep )
            {
                const SHAPE_VARS& vars = vit->second;
                double target = arcSweepTarget( m_params[vars.startAngle], m_params[vars.endAngle],
                                                *constraint->GetValue() );
                int    tgt = pushParam( target );
                fixedParams.insert( tgt );
                m_gcs->addConstraintDifference( &m_params[vars.startAngle], &m_params[vars.endAngle],
                                                &m_params[tgt], tag, constraint->IsDriving() );
                recordReferenceValue( constraint );
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

        if( !constraint->IsDriving() )
            m_nonDrivingTags.insert( tag );

        for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
            m_tagMembers[tag].push_back( member.m_item );

        // Remember shapes a direction or angle constraint could shrink to a point so the stabilize
        // solve length-holds just those and leaves dragged-along neighbours free to grow
        switch( constraint->GetConstraintType() )
        {
        case PCB_CONSTRAINT_TYPE::HORIZONTAL:
        case PCB_CONSTRAINT_TYPE::VERTICAL:

            // The two-point form aligns loose anchors not the owning segments lengths so only the
            // whole-segment form marks its member for the length-hold stabilization
            if( constraint->GetMembers().size() != 1 )
                break;

            [[fallthrough]];

        case PCB_CONSTRAINT_TYPE::PARALLEL:
        case PCB_CONSTRAINT_TYPE::PERPENDICULAR:
        case PCB_CONSTRAINT_TYPE::COLLINEAR:
        case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION:
        case PCB_CONSTRAINT_TYPE::ARC_ANGLE:
            for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
                m_angleConstrainedShapes.insert( member.m_item );

            break;

        default:
            break;
        }

        tag++;
    }

    // Reserve the hold tags just past the mapped constraints, so they can never collide with a
    // real constraint's tag however large the cluster grows.
    m_lengthHoldTag = tag;
    m_resizeRadiusTag = tag + 1;

    // Ground each dimension endpoint no mapped constraint bound, so an attached dimension adds zero
    // free DOF and only its bound point can move (through the shape it is coincident with).  A locked
    // dimension is grounded whole, like a locked shape.
    std::set<KIID> unmappedConstraints( m_unmapped.begin(), m_unmapped.end() );
    std::set<int>  referencedDimParams;

    for( PCB_CONSTRAINT* constraint : aConstraints )
    {
        if( unmappedConstraints.contains( constraint->m_Uuid ) )
            continue;   // an unmapped constraint enforces nothing, so it references no param

        for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
        {
            auto it = m_shapeVars.find( member.m_item );

            if( it != m_shapeVars.end() && it->second.kind == SHAPE_KIND::POINT_PAIR )
            {
                if( ANCHOR_PARAMS anchor = anchorParams( member ); anchor.IsValid() )
                    referencedDimParams.insert( anchor.x );
            }
        }
    }

    for( const auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind != SHAPE_KIND::POINT_PAIR )
            continue;

        bool locked = ConstraintItemIsLocked( vars.dimension );

        for( int pointX : { vars.startX, vars.endX } )
        {
            if( pointX >= 0 && ( locked || !referencedDimParams.contains( pointX ) ) )
            {
                fixedParams.insert( pointX );
                fixedParams.insert( pointX + 1 );
            }
        }
    }

    // Everything not grounded or held as a driving constant is an unknown the solver may move.
    GCS::VEC_pD unknowns;

    for( int i = 0; i < static_cast<int>( m_params.size() ); ++i )
    {
        if( !fixedParams.contains( i ) )
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

    // A hard hold here (no drag pin) so a contradiction cannot hide by collapsing a segment or arc.
    if( aStabilize )
    {
        holdFreeSegmentLengths( m_lengthHoldTag, m_angleConstrainedShapes );
        holdFreeArcRadii( m_lengthHoldTag, m_angleConstrainedShapes );
    }

    // With no shape singled out as edited hold every shape where it sits for a minimal-movement solve
    // these soft pins live in the null space of the hard constraints so the diagnosis stays untouched
    pinUneditedShapes( {}, GCS::DefaultTemporaryConstraint );

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    bool solved = solveSucceeded( ret );

    m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );

    if( aStabilize )
        m_gcs->clearByTag( m_lengthHoldTag );

    return solved;
}


bool BOARD_CONSTRAINT_ADAPTER::SolveAfterResize( const KIID& aResizedShape )
{
    if( !m_built )
        return false;

    for( const auto& [kiid, vars] : m_shapeVars )
    {
        // Preserve every curve's free radius while re-solving the resized cluster.  This covers
        // ellipses too; their focus-to-center offset params are fixed at Build, so pinning the
        // minor radius pins the whole shape (focal distance and rotation cannot drift).
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
            m_gcs->addConstraintCircleRadius( c, &m_params[target], m_resizeRadiusTag, true );

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

    // Hold every neighbour where it sits so a resize translates only the shapes a constraint forces
    // the resized shape's own centre is pinned above so exclude it here
    pinUneditedShapes( { aResizedShape }, GCS::DefaultTemporaryConstraint );

    // The radius loop above holds nothing of a polygon so a resized polygon needs its own
    // minimal-movement vertex pins here
    holdPolygonVertices( { aResizedShape }, GCS::DefaultTemporaryConstraint );

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    bool solved = solveSucceeded( ret );

    m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );
    m_gcs->clearByTag( m_resizeRadiusTag );

    return solved;
}


bool BOARD_CONSTRAINT_ADAPTER::Solve( const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor, bool aStabilize,
                                     const std::set<KIID>& aEdited,
                                     const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged )
{
    if( !m_built )
        return false;

    ANCHOR_PARAMS params = anchorParams( aDragged );

    if( !params.IsValid() )
        return false;

    // Reuse fixed backing slots for the cursor target so repeated drag solves (warm-started from
    // the previous solution still in m_params) never grow the backing store.
    if( m_dragTargetX < 0 )
    {
        m_dragTargetX = pushParam( 0.0 );
        m_dragTargetY = pushParam( 0.0 );
    }

    double targetX = normalizeX( aCursor.x );
    double targetY = normalizeY( aCursor.y );

    // For an arc endpoint, project the target onto the arc's current circle so the cursor pin agrees
    // with the centre + radius holds pinDraggedShapeRest adds, regardless of whether the caller
    // already projected.  Without this an off-circle target and the holds fight and the arc drifts.
    if( auto it = m_shapeVars.find( aDragged.m_item );
        !aStabilize && it != m_shapeVars.end() && it->second.kind == SHAPE_KIND::ARC
        && ( aDragged.m_anchor == CONSTRAINT_ANCHOR::START || aDragged.m_anchor == CONSTRAINT_ANCHOR::END ) )
    {
        const SHAPE_VARS& vars = it->second;
        double            dx = targetX - m_params[vars.startX];
        double            dy = targetY - m_params[vars.startX + 1];
        double            len = std::hypot( dx, dy );

        if( len > 1e-9 )
        {
            double radius = m_params[vars.radius];
            targetX = m_params[vars.startX] + dx * radius / len;
            targetY = m_params[vars.startX + 1] + dy * radius / len;
        }
    }

    if( auto it = m_shapeVars.find( aDragged.m_item );
        !aStabilize && it != m_shapeVars.end() && it->second.kind == SHAPE_KIND::SEGMENT
        && it->second.fixedLengthParam >= 0
        && ( aDragged.m_anchor == CONSTRAINT_ANCHOR::START || aDragged.m_anchor == CONSTRAINT_ANCHOR::END ) )
    {
        const SHAPE_VARS& vars = it->second;
        int               farX = aDragged.m_anchor == CONSTRAINT_ANCHOR::START ? vars.endX : vars.startX;
        double            segLen = m_params[vars.fixedLengthParam];
        double            dx = targetX - m_params[farX];
        double            dy = targetY - m_params[farX + 1];
        double            len = std::hypot( dx, dy );

        if( len > 1e-9 && segLen > 1e-9 )
        {
            targetX = m_params[farX] + dx * segLen / len;
            targetY = m_params[farX + 1] + dy * segLen / len;
        }
    }

    m_params[m_dragTargetX] = targetX;
    m_params[m_dragTargetY] = targetY;

    GCS::Point anchor = GCS::Point{ &m_params[params.x], &m_params[params.y] };

    // A temporary negatively tagged pin yields to the real constraints when over-constrained and
    // keeps the default weight far above the stay-put pins so a hard-linked neighbour follows it
    m_gcs->addConstraintCoordinateX( anchor, &m_params[m_dragTargetX], GCS::DefaultTemporaryConstraint );
    m_gcs->addConstraintCoordinateY( anchor, &m_params[m_dragTargetY], GCS::DefaultTemporaryConstraint );

    // The co-dragged anchor gets a plain pin at the same weight since only polygon edge drags carry
    // one the rest-hold exclusion below shares the same validity check so it is never left unheld
    const CONSTRAINT_MEMBER* coDragged = nullptr;

    if( aCoDragged )
    {
        ANCHOR_PARAMS coParams = anchorParams( aCoDragged->first );

        if( coParams.IsValid() )
        {
            coDragged = &aCoDragged->first;

            if( m_coDragTargetX < 0 )
            {
                m_coDragTargetX = pushParam( 0.0 );
                m_coDragTargetY = pushParam( 0.0 );
            }

            m_params[m_coDragTargetX] = normalizeX( aCoDragged->second.x );
            m_params[m_coDragTargetY] = normalizeY( aCoDragged->second.y );

            GCS::Point coAnchor = GCS::Point{ &m_params[coParams.x], &m_params[coParams.y] };

            m_gcs->addConstraintCoordinateX( coAnchor, &m_params[m_coDragTargetX], GCS::DefaultTemporaryConstraint );
            m_gcs->addConstraintCoordinateY( coAnchor, &m_params[m_coDragTargetY], GCS::DefaultTemporaryConstraint );
        }
    }

    // Only while live-dragging one handle.  The settle/apply paths (aStabilize) instead let the
    // pinned shape's rest move to meet a newly applied relation, e.g. a fixed-length shrink.
    if( !aStabilize )
        pinDraggedShapeRest( aDragged, GCS::DefaultTemporaryConstraint, coDragged );

    // Protect only shapes a direction or angle constraint could collapse a merely dragged-along
    // neighbour keeps its own stay-put pins instead
    if( aStabilize )
    {
        holdFreeSegmentLengths( GCS::DefaultTemporaryConstraint, m_angleConstrainedShapes );
        holdFreeArcRadii( GCS::DefaultTemporaryConstraint, m_angleConstrainedShapes );
    }

    // Hold every other cluster shape where it sits so only edited shapes and whatever a hard
    // constraint forces actually move every genuinely edited shape is excluded here
    std::set<KIID> editedShapes = aEdited;
    editedShapes.insert( aDragged.m_item );

    if( coDragged )
        editedShapes.insert( coDragged->m_item );

    pinUneditedShapes( editedShapes, GCS::DefaultTemporaryConstraint );

    // An edited polygon is excluded above but its unbound vertices still need minimal-movement pins
    // a live-dragged polygon is left out too since pinDraggedShapeRest already holds its other vertices
    std::set<KIID> heldPolygons = editedShapes;

    if( !aStabilize )
        heldPolygons.erase( aDragged.m_item );

    holdPolygonVertices( heldPolygons, GCS::DefaultTemporaryConstraint );

    m_gcs->initSolution();
    int ret = m_gcs->solve();
    m_gcs->applySolution();

    bool solved = solveSucceeded( ret );

    m_gcs->clearByTag( GCS::DefaultTemporaryConstraint );

    return solved;
}


bool BOARD_CONSTRAINT_ADAPTER::solveSucceeded( int aSolveResult )
{
    if( aSolveResult == GCS::Success || aSolveResult == GCS::Converged )
        return true;

    // planegcs bases Success only on the hard subsystem residual but the soft stay-put subsystem
    // perturbs the SQP trajectory so a valid solve often still reports Failed judge the hard
    // constraints directly instead a genuine contradiction still leaves a large or non-finite residual
    const double residualTol = 1e-3;

    // Tag-0 residual is the structural curve rules for arc ellipse and focus which must hold for
    // coherent geometry a NaN means no tag-0 constraint exists for a segment-only cluster not a failure
    double structuralErr = m_gcs->calculateConstraintErrorByTag( 0 );

    if( std::isfinite( structuralErr ) && std::abs( structuralErr ) > residualTol )
        return false;

    for( const auto& [tag, kiid] : m_tagToConstraint )
    {
        // A reference non-driving constraint only measures so its residual is never a failure
        if( m_nonDrivingTags.contains( tag ) )
            continue;

        double err = m_gcs->calculateConstraintErrorByTag( tag );

        if( !std::isfinite( err ) || std::abs( err ) > residualTol )
            return false;
    }

    return true;
}


void BOARD_CONSTRAINT_ADAPTER::softPinPoint( const ANCHOR_PARAMS& aPoint, int aTag, std::optional<double> aWeight )
{
    // A zero weight would neutralize the pin instead of tiering it
    wxASSERT( !aWeight || *aWeight > 0 );

    if( !aPoint.IsValid() )
        return;

    int        pinX = pushParam( m_params[aPoint.x] );
    int        pinY = pushParam( m_params[aPoint.y] );
    GCS::Point point{ &m_params[aPoint.x], &m_params[aPoint.y] };

    int cx = m_gcs->addConstraintCoordinateX( point, &m_params[pinX], aTag );
    int cy = m_gcs->addConstraintCoordinateY( point, &m_params[pinY], aTag );

    if( aWeight )
    {
        m_gcs->rescaleConstraint( cx, *aWeight );
        m_gcs->rescaleConstraint( cy, *aWeight );
    }
}


void BOARD_CONSTRAINT_ADAPTER::softPinPoint( int aPointX, int aTag, std::optional<double> aWeight )
{
    if( aPointX >= 0 )
        softPinPoint( ANCHOR_PARAMS{ aPointX, aPointX + 1 }, aTag, aWeight );
}


void BOARD_CONSTRAINT_ADAPTER::pinDraggedShapeRest( const CONSTRAINT_MEMBER& aDragged, int aTag,
                                                    const CONSTRAINT_MEMBER* aCoDragged )
{
    auto it = m_shapeVars.find( aDragged.m_item );

    if( it == m_shapeVars.end() )
        return;

    const SHAPE_VARS& vars = it->second;

    // The pins are temporary and keep the default weight far above the stay-put pins so they hold
    // the dragged shape rest against a neighbour weaker stay-put pin while a real constraint still wins
    if( vars.kind == SHAPE_KIND::SEGMENT || vars.kind == SHAPE_KIND::BEZIER )
    {
        if( aDragged.m_anchor == CONSTRAINT_ANCHOR::START )
            softPinPoint( vars.endX, aTag );
        else if( aDragged.m_anchor == CONSTRAINT_ANCHOR::END )
            softPinPoint( vars.startX, aTag );
    }
    else if( vars.kind == SHAPE_KIND::ARC )
    {
        if( aDragged.m_anchor == CONSTRAINT_ANCHOR::START || aDragged.m_anchor == CONSTRAINT_ANCHOR::END )
        {
            // Hold the circle (centre + radius) and the far endpoint, so only the dragged endpoint
            // sweeps along the arc instead of the whole arc drifting or ballooning.
            softPinPoint( vars.startX, aTag );
            holdArcRadius( vars, aTag );
            softPinPoint( aDragged.m_anchor == CONSTRAINT_ANCHOR::START ? vars.arcEndX : vars.arcStartX, aTag );
        }
        else if( aDragged.m_anchor == CONSTRAINT_ANCHOR::CENTER )
        {
            // Hold both endpoints, so dragging the centre changes the radius but keeps the ends.
            softPinPoint( vars.arcStartX, aTag );
            softPinPoint( vars.arcEndX, aTag );
        }
    }
    else if( vars.kind == SHAPE_KIND::RECT && aDragged.m_anchor == CONSTRAINT_ANCHOR::VERTEX )
    {
        // Hold the diagonally opposite corner so grabbing a corner handle resizes the rectangle
        // about it instead of the whole shape drifting
        softPinPoint( anchorParams( CONSTRAINT_MEMBER( aDragged.m_item, CONSTRAINT_ANCHOR::VERTEX,
                                                       ( aDragged.m_index + 2 ) % 4 ) ),
                      aTag );
    }
    else if( vars.kind == SHAPE_KIND::POLYGON && aDragged.m_anchor == CONSTRAINT_ANCHOR::VERTEX )
    {
        // Hold every other vertex so grabbing one handle moves only its own vertices an edge drag
        // names its second vertex through the co-dragged member whose pin must not be fought here
        for( int i = 0; i < vars.vertexCount; ++i )
        {
            if( i == aDragged.m_index )
                continue;

            if( aCoDragged && aCoDragged->m_item == aDragged.m_item && i == aCoDragged->m_index )
                continue;

            softPinPoint( vars.startX + 2 * i, aTag );
        }
    }
}


void BOARD_CONSTRAINT_ADAPTER::pinUneditedShapes( const std::set<KIID>& aEdited, int aTag )
{
    // The stay-put pins are the weakest tier so the drive pins and hard constraints all win while a
    // neighbour with slack still holds exactly since nothing else acts on its coordinates

    // Hold a curve radius scalar so a circle or ellipse with slack keeps its size
    auto holdRadius = [&]( const SHAPE_VARS& aVars )
    {
        GCS::Circle circle;
        circle.center = GCS::Point{ &m_params[aVars.startX], &m_params[aVars.startX + 1] };
        circle.rad = &m_params[aVars.radius];

        int rad = pushParam( m_params[aVars.radius] );
        m_gcs->rescaleConstraint( m_gcs->addConstraintCircleRadius( circle, &m_params[rad], aTag ), STAY_PUT_WEIGHT );
    };

    for( const auto& [kiid, vars] : m_shapeVars )
    {
        if( aEdited.contains( kiid ) )
            continue;

        // A locked shape or dimension is already frozen at Build so a soft pin would only add
        // redundant work
        if( ConstraintItemIsLocked( vars.shape ) || ConstraintItemIsLocked( vars.dimension ) )
            continue;

        switch( vars.kind )
        {
        case SHAPE_KIND::SEGMENT:
        case SHAPE_KIND::BEZIER:
        case SHAPE_KIND::POINT_PAIR:
        case SHAPE_KIND::RECT:
            // Pinning both stored points fixes the whole shape a segment entirely and a rect four
            // corners with them since the corners alias these params
            softPinPoint( vars.startX, aTag, STAY_PUT_WEIGHT );
            softPinPoint( vars.endX, aTag, STAY_PUT_WEIGHT );
            break;

        case SHAPE_KIND::CIRCLE:
        case SHAPE_KIND::ELLIPSE:
            // A circle or closed ellipse has no endpoints so hold the centre and the radius scalar
            // the ellipse focus-to-centre offset is fixed at Build so this pins the whole shape
            softPinPoint( vars.startX, aTag, STAY_PUT_WEIGHT );
            holdRadius( vars );
            break;

        case SHAPE_KIND::ARC:
        case SHAPE_KIND::ELLIPSE_ARC:
            softPinPoint( vars.startX, aTag, STAY_PUT_WEIGHT );
            softPinPoint( vars.arcStartX, aTag, STAY_PUT_WEIGHT );
            softPinPoint( vars.arcEndX, aTag, STAY_PUT_WEIGHT );
            holdRadius( vars );
            break;

        case SHAPE_KIND::POLYGON:
            // Every vertex bound ones included the pins live in the null space of the hard
            // constraints so a driving length still moves the vertices it binds
            for( int i = 0; i < vars.vertexCount; ++i )
                softPinPoint( vars.startX + 2 * i, aTag, STAY_PUT_WEIGHT );

            break;
        }
    }
}


void BOARD_CONSTRAINT_ADAPTER::holdPolygonVertices( const std::set<KIID>& aShapes, int aTag )
{
    for( const auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind != SHAPE_KIND::POLYGON || ConstraintItemIsLocked( vars.shape ) )
            continue;

        if( !aShapes.contains( kiid ) )
            continue;

        for( int i = 0; i < vars.vertexCount; ++i )
            softPinPoint( vars.startX + 2 * i, aTag, STAY_PUT_WEIGHT );
    }
}


void BOARD_CONSTRAINT_ADAPTER::holdFreeSegmentLengths( int aTag, const std::set<KIID>& aShapes )
{
    // Hold each named free segment length so an angle constraint cannot collapse it to a point
    for( const auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind != SHAPE_KIND::SEGMENT || ConstraintItemIsLocked( vars.shape ) )
            continue;

        if( !aShapes.contains( kiid ) )
            continue;

        GCS::Point p1{ &m_params[vars.startX], &m_params[vars.startX + 1] };
        GCS::Point p2{ &m_params[vars.endX], &m_params[vars.endX + 1] };
        double     dx = m_params[vars.endX] - m_params[vars.startX];
        double     dy = m_params[vars.endX + 1] - m_params[vars.startX + 1];
        int        len = pushParam( std::hypot( dx, dy ) );
        m_gcs->addConstraintP2PDistance( p1, p2, &m_params[len], aTag );
    }
}


void BOARD_CONSTRAINT_ADAPTER::holdArcRadius( const SHAPE_VARS& aVars, int aTag )
{
    GCS::Circle circle;
    circle.center = GCS::Point{ &m_params[aVars.startX], &m_params[aVars.startX + 1] };
    circle.rad = &m_params[aVars.radius];

    int rad = pushParam( m_params[aVars.radius] );
    m_gcs->addConstraintCircleRadius( circle, &m_params[rad], aTag );
}


void BOARD_CONSTRAINT_ADAPTER::holdFreeArcRadii( int aTag, const std::set<KIID>& aShapes )
{
    // Hold each named free arc radius so an ARC_ANGLE change rotates an endpoint instead of the
    // solver collapsing the arc to a point a real FIXED_RADIUS still wins since this hold is temporary
    for( const auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind != SHAPE_KIND::ARC || ConstraintItemIsLocked( vars.shape ) )
            continue;

        if( !aShapes.contains( kiid ) )
            continue;

        holdArcRadius( vars, aTag );
    }
}


std::vector<PCB_SHAPE*> BOARD_CONSTRAINT_ADAPTER::Apply( const std::function<void( BOARD_ITEM* )>& aBeforeWrite )
{
    std::vector<PCB_SHAPE*> changed;

    if( !m_built )
        return changed;

    // The solved point at param index aX (its y is the next slot), back in IU.
    auto pointAt = [&]( int aX )
    {
        return VECTOR2I( KiROUND( denormalizeX( m_params[aX] ) ), KiROUND( denormalizeY( m_params[aX + 1] ) ) );
    };

    for( const auto& [kiid, vars] : m_shapeVars )
    {
        if( vars.kind == SHAPE_KIND::POINT_PAIR )
        {
            VECTOR2I start = pointAt( vars.startX );

            // A leader or centre mark has no bindable end (endX == -1); leave its end untouched.
            VECTOR2I end = vars.endX >= 0 ? pointAt( vars.endX ) : vars.dimension->GetEnd();

            if( start == vars.dimension->GetStart() && end == vars.dimension->GetEnd() )
                continue;

            if( aBeforeWrite )
                aBeforeWrite( vars.dimension );

            vars.dimension->SetStart( start );
            vars.dimension->SetEnd( end );
            vars.dimension->Update();   // SetStart/SetEnd alone do not re-derive the crossbar and text
            continue;
        }

        if( vars.kind == SHAPE_KIND::CIRCLE )
        {
            VECTOR2I center = pointAt( vars.startX );
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
            // A sub-micron radius is a collapse, not intent, so leave the arc as it was rather than
            // write a degenerate ring the diagnostics would not catch.
            if( m_params[vars.radius] * m_scale < 1000.0 && vars.shape->GetRadius() >= 1000 )
                continue;

            VECTOR2I center = pointAt( vars.startX );
            VECTOR2I start = pointAt( vars.arcStartX );
            VECTOR2I end = pointAt( vars.arcEndX );

            if( start == vars.shape->GetStart() && end == vars.shape->GetEnd()
                    && center == vars.shape->GetCenter() )
            {
                continue;
            }

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            // The mid lies on the solved circle at the bisector of the swept angles.  Take the
            // bisector on the side that keeps the original winding so the arc does not flip.
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
            VECTOR2I center = pointAt( vars.startX );

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

        if( vars.kind == SHAPE_KIND::BEZIER )
        {
            VECTOR2I start = pointAt( vars.startX );
            VECTOR2I end = pointAt( vars.endX );

            if( start == vars.shape->GetStart() && end == vars.shape->GetEnd() )
                continue;

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            // Translate each control handle by its adjacent endpoint delta so the curve shape rides
            // along instead of shearing when an endpoint moves
            VECTOR2I startDelta = start - vars.shape->GetStart();
            VECTOR2I endDelta = end - vars.shape->GetEnd();

            vars.shape->SetBezierC1( vars.shape->GetBezierC1() + startDelta );
            vars.shape->SetBezierC2( vars.shape->GetBezierC2() + endDelta );
            vars.shape->SetStart( start );
            vars.shape->SetEnd( end );
            vars.shape->RebuildBezierToSegmentsPointsList();
            changed.push_back( vars.shape );
            continue;
        }

        if( vars.kind == SHAPE_KIND::POLYGON )
        {
            const SHAPE_POLY_SET& polySet = vars.shape->GetPolyShape();

            // A vertex-count mismatch means an external edit changed the outline CPoint wraps indices
            // so a stale count would silently resurrect dropped vertices skip the write instead
            if( polySet.OutlineCount() != 1 || polySet.HoleCount( 0 ) != 0
                    || polySet.COutline( 0 ).PointCount() != vars.vertexCount )
            {
                continue;
            }

            std::vector<VECTOR2I> points;
            points.reserve( vars.vertexCount );

            for( int i = 0; i < vars.vertexCount; ++i )
                points.push_back( pointAt( vars.startX + 2 * i ) );

            const SHAPE_LINE_CHAIN& outline = polySet.COutline( 0 );
            bool                    moved = false;

            for( int i = 0; i < vars.vertexCount; ++i )
            {
                if( points[i] != outline.CPoint( i ) )
                {
                    moved = true;
                    break;
                }
            }

            if( !moved )
                continue;

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            // SetPolyPoints rebuilds the poly wholesale dropping cached derived geometry with it
            // ingestion only admits single-outline hole-free polys so nothing else is lost here
            vars.shape->SetPolyPoints( points );
            changed.push_back( vars.shape );
            continue;
        }

        if( vars.kind == SHAPE_KIND::RECT )
        {
            VECTOR2I start = pointAt( vars.startX );
            VECTOR2I end = pointAt( vars.endX );

            if( start == vars.shape->GetStart() && end == vars.shape->GetEnd() )
                continue;

            // A sub-micron side is a collapse not intent and a zero-area rect vanishes from the
            // canvas so drop the write using the segment guard floor applied per axis
            const int collapseFloor = 1000;
            int       newWidth = std::abs( end.x - start.x );
            int       newHeight = std::abs( end.y - start.y );
            int       curWidth = std::abs( vars.shape->GetEnd().x - vars.shape->GetStart().x );
            int       curHeight = std::abs( vars.shape->GetEnd().y - vars.shape->GetStart().y );

            if( ( newWidth < collapseFloor && curWidth >= collapseFloor )
                    || ( newHeight < collapseFloor && curHeight >= collapseFloor ) )
            {
                continue;
            }

            if( aBeforeWrite )
                aBeforeWrite( vars.shape );

            // SetStart and SetEnd do not re-clamp the stored corner radius so re-apply it through the
            // setter whose half-shorter-side clamp keeps a shrunk rect radius in range
            int cornerRadius = vars.shape->GetCornerRadius();

            vars.shape->SetStart( start );
            vars.shape->SetEnd( end );

            if( cornerRadius > 0 )
                vars.shape->SetCornerRadius( cornerRadius );

            changed.push_back( vars.shape );
            continue;
        }

        VECTOR2I start = pointAt( vars.startX );
        VECTOR2I end = pointAt( vars.endX );

        if( start == vars.shape->GetStart() && end == vars.shape->GetEnd() )
            continue;

        // A sub-micron result is a collapse, not intent, and a zero-length line vanishes from the
        // canvas, so drop it.
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

    auto flagConflict = [&]( const KIID& aKiid )
    {
        if( std::ranges::find( diag.conflicting, aKiid ) == diag.conflicting.end() )
            diag.conflicting.push_back( aKiid );
    };

    // Also flag any constraint the geometry does not satisfy. The rank analysis can miss it.
    const double residualTol = 1e-3;

    for( const auto& [tag, kiid] : m_tagToConstraint )
    {
        // A reference constraint only measures; its stored value drifting from the geometry is
        // never a contradiction.
        if( m_nonDrivingTags.contains( tag ) )
            continue;

        double err = m_gcs->calculateConstraintErrorByTag( tag );

        // A nan is a degenerate system, not a conflict. A real contradiction leaves a finite error.
        if( !std::isfinite( err ) || std::abs( err ) <= residualTol )
            continue;

        flagConflict( kiid );
    }

    // The solve collapsing a segment to a point (e.g. horizontal and vertical at once) satisfies the
    // constraints with zero residual, so flag the constraints incident on the collapsed segments.
    const double   normFloor = 1e-3;
    std::set<KIID> collapsedShapes;

    for( const auto& [k, vars] : m_shapeVars )
    {
        if( vars.kind != SHAPE_KIND::SEGMENT )
            continue;

        double solvedLen = std::hypot( m_params[vars.endX] - m_params[vars.startX],
                                       m_params[vars.endX + 1] - m_params[vars.startX + 1] );
        double origLen = ( vars.shape->GetEnd() - vars.shape->GetStart() ).EuclideanNorm() * m_invScale;

        if( origLen > normFloor && solvedLen < normFloor )
            collapsedShapes.insert( k );
    }

    if( !collapsedShapes.empty() )
    {
        bool attributed = false;

        for( const auto& [tag, kiid] : m_tagToConstraint )
        {
            // A reference measurement never causes a collapse.
            if( m_nonDrivingTags.contains( tag ) )
                continue;

            auto members = m_tagMembers.find( tag );

            if( members == m_tagMembers.end() )
                continue;

            bool incident = std::any_of( members->second.begin(), members->second.end(),
                                         [&]( const KIID& aMember )
                                         {
                                             return collapsedShapes.contains( aMember );
                                         } );

            if( incident )
            {
                flagConflict( kiid );
                attributed = true;
            }
        }

        // A collapse no mapped constraint touches should not happen; keep the whole-cluster flag
        // as a fallback so the contradiction is never silently dropped.
        if( !attributed )
        {
            for( const auto& [tag, kiid] : m_tagToConstraint )
            {
                if( !m_nonDrivingTags.contains( tag ) )
                    flagConflict( kiid );
            }
        }
    }

    // .solved reflects the last Solve(), which Diagnose() does not run; the caller sets it.
    return diag;
}


CONSTRAINT_DIAGNOSIS SolveCluster( BOARD* aBoard, const CONSTRAINT_MEMBER& aDragged, const VECTOR2I& aCursor,
                                   std::vector<PCB_SHAPE*>*                  aModified,
                                   const std::function<void( BOARD_ITEM* )>& aBeforeModify, bool aIncludeDragged,
                                   bool aStabilize, const std::set<KIID>& aEdited,
                                   const std::optional<std::pair<CONSTRAINT_MEMBER, VECTOR2I>>& aCoDragged )
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

    std::vector<PCB_SHAPE*>          shapes = resolveClusterShapes( aBoard, clusterShapes );
    std::vector<PCB_DIMENSION_BASE*> dimensions = resolveClusterDimensions( aBoard, clusterShapes );

    if( ( shapes.empty() && dimensions.empty() ) || clusterConstraints.empty() )
        return diag;

    BOARD_CONSTRAINT_ADAPTER adapter;

    if( !adapter.Build( shapes, clusterConstraints, nullptr, dimensions ) )
        return diag;

    bool solved = adapter.Solve( aDragged, aCursor, aStabilize, aEdited, aCoDragged );

    if( !solved )
        return diag;   // leave geometry untouched on a failed/diverged solve

    PCB_SHAPE* draggedShape = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( aDragged.m_item, true ) );

    // Stage each neighbor (not the dragged shape, which the caller stages itself) just before
    // its geometry is written, so the whole re-derivation is one undoable transaction.
    std::vector<PCB_SHAPE*> changed = adapter.Apply(
            [&]( BOARD_ITEM* aItem )
            {
                if( ( aIncludeDragged || aItem != draggedShape ) && aBeforeModify )
                    aBeforeModify( aItem );
            } );

    adapter.ApplyReferenceValues( aBeforeModify );

    diag = adapter.Diagnose();
    diag.solved = true;

    if( aModified )
    {
        std::ranges::copy_if( changed, std::back_inserter( *aModified ),
                              [&]( const PCB_SHAPE* aShape )
                              { return aIncludeDragged || aShape != draggedShape; } );
    }

    return diag;
}


CONSTRAINT_DIAGNOSIS ApplyConstraintImmediately( BOARD* aBoard, const PCB_CONSTRAINT* aConstraint,
                                                 std::vector<PCB_SHAPE*>*                  aModified,
                                                 const std::function<void( BOARD_ITEM* )>& aBeforeModify )
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
        pin.m_index = anchors.front().index;
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
                           const std::function<void( BOARD_ITEM* )>& aBeforeModify )
{
    if( !aBoard )
        return;

    auto shapeToConstraints = buildShapeConstraintMap( aBoard, collectAllConstraints( aBoard ) );

    // Solve each affected cluster once, no matter how many of its shapes were edited.
    std::set<KIID> visited;

    for( PCB_SHAPE* shape : aShapes )
    {
        if( !shape || visited.contains( shape->m_Uuid ) || !shapeToConstraints.contains( shape->m_Uuid ) )
            continue;

        std::unordered_set<KIID>     clusterShapes;
        std::vector<PCB_CONSTRAINT*> clusterConstraints;
        collectConstraintCluster( shapeToConstraints, shape->m_Uuid, clusterShapes, clusterConstraints, &visited );

        std::vector<CONSTRAINT_ANCHOR_POINT> anchors = ConstraintShapeAnchors( shape );

        if( anchors.empty() )
            continue;

        // Every edited shape that fell into this cluster must stay free only the untouched
        // neighbours get stay-put pins solving from a single seed would pin the others back
        std::set<KIID> edited;

        for( PCB_SHAPE* other : aShapes )
        {
            if( other && clusterShapes.contains( other->m_Uuid ) )
                edited.insert( other->m_Uuid );
        }

        SolveCluster( aBoard, { shape->m_Uuid, anchors.front().anchor, anchors.front().index },
                      anchors.front().pos, aModified, aBeforeModify,
                      /* aIncludeDragged */ true, /* aStabilize */ false, edited );
    }
}


void ReSolveShapeClustersHoldingEdited( BOARD* aBoard, const std::vector<PCB_SHAPE*>& aEditedShapes,
                                        std::vector<PCB_SHAPE*>* aModified,
                                        const std::function<void( BOARD_ITEM* )>& aBeforeModify )
{
    if( !aBoard )
        return;

    auto           shapeToConstraints = buildShapeConstraintMap( aBoard, collectAllConstraints( aBoard ) );
    std::set<KIID> visited;

    for( PCB_SHAPE* seed : aEditedShapes )
    {
        if( !seed || visited.contains( seed->m_Uuid ) || !shapeToConstraints.contains( seed->m_Uuid ) )
            continue;

        std::unordered_set<KIID>     clusterShapes;
        std::vector<PCB_CONSTRAINT*> clusterConstraints;
        collectConstraintCluster( shapeToConstraints, seed->m_Uuid, clusterShapes, clusterConstraints, &visited );

        std::vector<PCB_SHAPE*>          shapes = resolveClusterShapes( aBoard, clusterShapes );
        std::vector<PCB_DIMENSION_BASE*> dimensions = resolveClusterDimensions( aBoard, clusterShapes );

        if( shapes.empty() || clusterConstraints.empty() )
            continue;

        // A properties edit is authoritative since the user typed exact values so every edited shape
        // is held fixed across all its DOF and only constrained neighbours move to satisfy relations
        std::set<KIID> fixed;

        for( PCB_SHAPE* edited : aEditedShapes )
        {
            if( edited && clusterShapes.contains( edited->m_Uuid ) )
                fixed.insert( edited->m_Uuid );
        }

        BOARD_CONSTRAINT_ADAPTER adapter;

        if( !adapter.Build( shapes, clusterConstraints, &fixed, dimensions ) || !adapter.Solve( true ) )
            continue;

        std::vector<PCB_SHAPE*> changed = adapter.Apply( aBeforeModify );

        // Driven reference values re-measure against the held geometry so a dimension bound to an
        // edited shape reads its new size instead of the stale one
        adapter.ApplyReferenceValues( aBeforeModify );

        if( aModified )
            aModified->insert( aModified->end(), changed.begin(), changed.end() );
    }
}


void ReSolveAfterShapeResize( BOARD* aBoard, PCB_SHAPE* aShape, std::vector<PCB_SHAPE*>* aModified,
                              const std::function<void( BOARD_ITEM* )>& aBeforeModify )
{
    if( !aBoard || !aShape )
        return;

    auto shapeToConstraints = buildShapeConstraintMap( aBoard, collectAllConstraints( aBoard ) );

    if( !shapeToConstraints.contains( aShape->m_Uuid ) )
        return;

    std::unordered_set<KIID>     clusterShapes;
    std::vector<PCB_CONSTRAINT*> clusterConstraints;
    collectConstraintCluster( shapeToConstraints, aShape->m_Uuid, clusterShapes, clusterConstraints );

    std::vector<PCB_SHAPE*>          shapes = resolveClusterShapes( aBoard, clusterShapes );
    std::vector<PCB_DIMENSION_BASE*> dimensions = resolveClusterDimensions( aBoard, clusterShapes );

    if( shapes.empty() || clusterConstraints.empty() )
        return;

    BOARD_CONSTRAINT_ADAPTER adapter;

    if( !adapter.Build( shapes, clusterConstraints, nullptr, dimensions )
        || !adapter.SolveAfterResize( aShape->m_Uuid ) )
    {
        return;
    }

    std::vector<PCB_SHAPE*> changed = adapter.Apply(
            [&]( BOARD_ITEM* aChanged )
            {
                if( aBeforeModify )
                    aBeforeModify( aChanged );
            } );

    adapter.ApplyReferenceValues( aBeforeModify );

    if( aModified )
        aModified->insert( aModified->end(), changed.begin(), changed.end() );
}


// Diagnose one cluster in isolation an unbuildable cluster with no resolvable members or no
// constraints contributes nothing so its result stays empty
static CLUSTER_DIAGNOSIS diagnoseSingleCluster( BOARD*                              aBoard,
                                                const std::unordered_set<KIID>&     aClusterShapes,
                                                const std::vector<PCB_CONSTRAINT*>& aClusterConstraints )
{
    CLUSTER_DIAGNOSIS result;

    std::vector<PCB_SHAPE*>          shapes = resolveClusterShapes( aBoard, aClusterShapes );
    std::vector<PCB_DIMENSION_BASE*> dimensions = resolveClusterDimensions( aBoard, aClusterShapes );

    BOARD_CONSTRAINT_ADAPTER adapter;

    if( ( shapes.empty() && dimensions.empty() ) || aClusterConstraints.empty()
        || !adapter.Build( shapes, aClusterConstraints, nullptr, dimensions ) )
    {
        return result;
    }

    // A constraint Build() could not map (e.g. a shape was changed to an incompatible kind) is not
    // enforced; flag it so the user sees it is broken rather than silently ignored.
    result.erroredUnmapped = adapter.UnmappedConstraints();

    // Solve first so the residual check sees a real contradiction not an unsolved constraint a
    // contradictory cluster deliberately diverges here since the stabilize holds forbid the escape
    adapter.Solve( true );

    CONSTRAINT_DIAGNOSIS diag = adapter.Diagnose();

    if( diag.IsOverConstrained() )
        result.state = CONSTRAINT_STATE::OVER_CONSTRAINED;
    else if( diag.IsUnderConstrained() )
        result.state = CONSTRAINT_STATE::UNDER_CONSTRAINED;
    else
        result.state = CONSTRAINT_STATE::WELL_CONSTRAINED;

    for( PCB_SHAPE* shape : shapes )
        result.shapeIds.push_back( shape->m_Uuid );

    // Bound dimensions join their cluster's verdict so the overlay can mark them; a free dimension
    // never reaches a cluster and so keeps no state.
    for( PCB_DIMENSION_BASE* dimension : dimensions )
        result.dimensionIds.push_back( dimension->m_Uuid );

    if( diag.freeDof > 0 )
        result.freeDof = diag.freeDof;

    result.conflicting = diag.conflicting;
    result.redundant = diag.redundant;

    return result;
}


// Fold one cluster's verdict into the board-wide result an empty unbuildable cluster leaves
// aResult untouched
static void assembleClusterInto( BOARD_CONSTRAINT_DIAGNOSTICS& aResult, const CLUSTER_DIAGNOSIS& aCluster )
{
    for( const KIID& id : aCluster.shapeIds )
        aResult.shapeStates[id] = aCluster.state;

    for( const KIID& id : aCluster.dimensionIds )
        aResult.shapeStates[id] = aCluster.state;

    aResult.totalFreeDof += aCluster.freeDof;

    aResult.conflicting.insert( aResult.conflicting.end(), aCluster.conflicting.begin(),
                                aCluster.conflicting.end() );
    aResult.redundant.insert( aResult.redundant.end(), aCluster.redundant.begin(),
                              aCluster.redundant.end() );
    aResult.errored.insert( aResult.errored.end(), aCluster.erroredUnmapped.begin(),
                            aCluster.erroredUnmapped.end() );
}


// A constraint with a dangling member is unmappable so it can be flagged by both the adjacency
// scan and Build report each errored constraint once here
static void dedupErrored( BOARD_CONSTRAINT_DIAGNOSTICS& aResult )
{
    std::sort( aResult.errored.begin(), aResult.errored.end() );
    aResult.errored.erase( std::unique( aResult.errored.begin(), aResult.errored.end() ),
                           aResult.errored.end() );
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
        if( visitedShapes.contains( seedShape ) )
            continue;

        std::unordered_set<KIID>     clusterShapes;
        std::vector<PCB_CONSTRAINT*> clusterConstraints;
        collectConstraintCluster( shapeToConstraints, seedShape, clusterShapes, clusterConstraints,
                                  &visitedShapes );

        assembleClusterInto( result,
                             diagnoseSingleCluster( aBoard, clusterShapes, clusterConstraints ) );
    }

    dedupErrored( result );

    return result;
}


// Boost-style mix so a change in any hashed field changes the cluster hash
static void hashCombine( std::size_t& aSeed, std::size_t aValue )
{
    aSeed ^= aValue + 0x9e3779b97f4a7c15ULL + ( aSeed << 6 ) + ( aSeed >> 2 );
}


static void hashInt( std::size_t& aSeed, long long aValue )
{
    hashCombine( aSeed, static_cast<std::size_t>( aValue ) );
}


static void hashDouble( std::size_t& aSeed, double aValue )
{
    // Hash the exact bit pattern so any coordinate or value change invalidates the cache folding
    // both 32-bit halves so a double change is never lost where size_t is only 32 bits wide
    std::uint64_t bits = 0;
    static_assert( sizeof( bits ) == sizeof( aValue ) );
    std::memcpy( &bits, &aValue, sizeof( bits ) );
    hashCombine( aSeed, static_cast<std::size_t>( bits & 0xFFFFFFFFULL ) );
    hashCombine( aSeed, static_cast<std::size_t>( bits >> 32 ) );
}


static void hashPoint( std::size_t& aSeed, const VECTOR2I& aPoint )
{
    hashInt( aSeed, aPoint.x );
    hashInt( aSeed, aPoint.y );
}


static void hashKiid( std::size_t& aSeed, const KIID& aId )
{
    hashCombine( aSeed, std::hash<KIID>{}( aId ) );
}


// Hash every solve input a PCB_SHAPE contributes dispatched by kind so no getter is called on a
// shape type it asserts for since GetRadius and GetCenter are unimplemented for the wrong kinds
static void hashShape( std::size_t& aSeed, const PCB_SHAPE* aShape )
{
    hashInt( aSeed, static_cast<int>( aShape->GetShape() ) );

    // Build freezes a locked shape including via its parent footprint so the lock state changes
    // the solve even when the geometry does not
    hashInt( aSeed, ConstraintItemIsLocked( aShape ) ? 1 : 0 );

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECTANGLE:
        hashPoint( aSeed, aShape->GetStart() );
        hashPoint( aSeed, aShape->GetEnd() );
        break;

    case SHAPE_T::BEZIER:
        hashPoint( aSeed, aShape->GetStart() );
        hashPoint( aSeed, aShape->GetEnd() );
        hashPoint( aSeed, aShape->GetBezierC1() );
        hashPoint( aSeed, aShape->GetBezierC2() );
        break;

    case SHAPE_T::CIRCLE:
        hashPoint( aSeed, aShape->GetCenter() );
        hashInt( aSeed, aShape->GetRadius() );
        break;

    case SHAPE_T::POLY:
    {
        const SHAPE_POLY_SET& poly = aShape->GetPolyShape();

        // The outline hole and arc counts gate ingestion so any of them changing must invalidate
        // even when no vertex moved
        hashInt( aSeed, poly.OutlineCount() );

        if( poly.OutlineCount() > 0 )
        {
            hashInt( aSeed, poly.HoleCount( 0 ) );

            const SHAPE_LINE_CHAIN& outline = poly.COutline( 0 );

            hashInt( aSeed, static_cast<long long>( outline.ArcCount() ) );

            for( int i = 0; i < outline.PointCount(); ++i )
                hashPoint( aSeed, outline.CPoint( i ) );
        }

        break;
    }

    case SHAPE_T::ARC:
        hashPoint( aSeed, aShape->GetStart() );
        hashPoint( aSeed, aShape->GetEnd() );
        hashPoint( aSeed, aShape->GetCenter() );
        hashInt( aSeed, aShape->GetRadius() );
        break;

    case SHAPE_T::ELLIPSE:
    case SHAPE_T::ELLIPSE_ARC:
        hashPoint( aSeed, aShape->GetEllipseCenter() );
        hashInt( aSeed, aShape->GetEllipseMajorRadius() );
        hashInt( aSeed, aShape->GetEllipseMinorRadius() );
        hashDouble( aSeed, aShape->GetEllipseRotation().AsRadians() );

        if( aShape->GetShape() == SHAPE_T::ELLIPSE_ARC )
        {
            hashPoint( aSeed, aShape->GetStart() );
            hashPoint( aSeed, aShape->GetEnd() );
            hashDouble( aSeed, aShape->GetEllipseStartAngle().AsRadians() );
            hashDouble( aSeed, aShape->GetEllipseEndAngle().AsRadians() );
        }

        break;

    default:
        // Build reads the front shape start for the cluster normalization origin even when the kind
        // is unmapped so a change there can still shift the solve hash it here too
        hashPoint( aSeed, aShape->GetStart() );
        break;
    }
}


static void hashDimension( std::size_t& aSeed, const PCB_DIMENSION_BASE* aDimension )
{
    hashInt( aSeed, static_cast<int>( aDimension->Type() ) );
    hashPoint( aSeed, aDimension->GetStart() );
    hashPoint( aSeed, aDimension->GetEnd() );

    // An orthogonal dimension driving length fixes the axis its orientation selects so a
    // horizontal to vertical flip changes the solve without moving the endpoints
    if( aDimension->Type() == PCB_DIM_ORTHOGONAL_T )
    {
        hashInt( aSeed, static_cast<int>(
                                static_cast<const PCB_DIM_ORTHOGONAL*>( aDimension )->GetOrientation() ) );
    }
}


static void hashConstraint( std::size_t& aSeed, const PCB_CONSTRAINT* aConstraint )
{
    hashKiid( aSeed, aConstraint->m_Uuid );
    hashInt( aSeed, static_cast<int>( aConstraint->GetConstraintType() ) );
    hashInt( aSeed, aConstraint->IsDriving() ? 1 : 0 );
    hashInt( aSeed, aConstraint->HasValue() ? 1 : 0 );

    if( aConstraint->HasValue() )
        hashDouble( aSeed, *aConstraint->GetValue() );

    for( const CONSTRAINT_MEMBER& member : aConstraint->GetMembers() )
    {
        hashKiid( aSeed, member.m_item );
        hashInt( aSeed, static_cast<int>( member.m_anchor ) );
        hashInt( aSeed, member.m_index );
    }
}


// A hash over every input diagnoseSingleCluster would solve on so any geometry value or membership
// change invalidates the cached result err toward hashing more since a missed field leaves it stale
static std::size_t hashCluster( BOARD* aBoard, const std::unordered_set<KIID>& aClusterShapes,
                                const std::vector<PCB_CONSTRAINT*>& aClusterConstraints )
{
    std::size_t seed = 0;

    std::vector<KIID> ids( aClusterShapes.begin(), aClusterShapes.end() );
    std::sort( ids.begin(), ids.end() );

    for( const KIID& id : ids )
    {
        BOARD_ITEM* item = aBoard->ResolveItem( id, true );

        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
            hashShape( seed, shape );
        else if( PCB_DIMENSION_BASE* dimension = dynamic_cast<PCB_DIMENSION_BASE*>( item ) )
            hashDimension( seed, dimension );
    }

    std::vector<PCB_CONSTRAINT*> constraints = aClusterConstraints;
    std::sort( constraints.begin(), constraints.end(),
               []( const PCB_CONSTRAINT* aLhs, const PCB_CONSTRAINT* aRhs )
               { return aLhs->m_Uuid < aRhs->m_Uuid; } );

    for( const PCB_CONSTRAINT* constraint : constraints )
        hashConstraint( seed, constraint );

    return seed;
}


void BOARD_CONSTRAINT_DIAGNOSER::Clear()
{
    m_cache.clear();
}


BOARD_CONSTRAINT_DIAGNOSTICS BOARD_CONSTRAINT_DIAGNOSER::Diagnose( BOARD* aBoard )
{
    BOARD_CONSTRAINT_DIAGNOSTICS result;

    if( !aBoard )
    {
        m_cache.clear();
        return result;
    }

    std::vector<PCB_CONSTRAINT*> boardConstraints = collectAllConstraints( aBoard );

    // The map-scan errored dangling members is recomputed in full every call so it can never go
    // stale however aggressively the per-cluster diagnoses are cached
    std::unordered_map<KIID, std::vector<PCB_CONSTRAINT*>> shapeToConstraints =
            buildShapeConstraintMap( aBoard, boardConstraints, &result.errored );

    std::set<KIID>              visitedShapes;
    std::set<std::vector<KIID>> seenKeys;

    for( const auto& [seedShape, seedConstraints] : shapeToConstraints )
    {
        if( visitedShapes.contains( seedShape ) )
            continue;

        std::unordered_set<KIID>     clusterShapes;
        std::vector<PCB_CONSTRAINT*> clusterConstraints;
        collectConstraintCluster( shapeToConstraints, seedShape, clusterShapes, clusterConstraints,
                                  &visitedShapes );

        // Clusters partition the board constraints so the sorted constraint-id set uniquely
        // identifies a cluster and stays put across a geometry or value edit the hash catches those
        std::vector<KIID> key;

        for( const PCB_CONSTRAINT* constraint : clusterConstraints )
            key.push_back( constraint->m_Uuid );

        std::sort( key.begin(), key.end() );

        std::size_t hash = hashCluster( aBoard, clusterShapes, clusterConstraints );
        auto        it = m_cache.find( key );

        if( it == m_cache.end() || it->second.hash != hash )
        {
            CLUSTER_DIAGNOSIS cluster =
                    diagnoseSingleCluster( aBoard, clusterShapes, clusterConstraints );
            m_solveCount++;
            it = m_cache.insert_or_assign( key, CACHE_ENTRY{ hash, std::move( cluster ) } ).first;
        }

        assembleClusterInto( result, it->second.result );
        seenKeys.insert( key );
    }

    // Drop cache entries for clusters no longer present so a stale result can never leak into a
    // later pass that happens to rebuild the same key
    for( auto it = m_cache.begin(); it != m_cache.end(); )
    {
        if( seenKeys.contains( it->first ) )
            ++it;
        else
            it = m_cache.erase( it );
    }

    dedupErrored( result );

    return result;
}
