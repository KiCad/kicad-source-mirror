/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <snap/snap_resolver.h>

#include "snap_manifold.h"

#include <mmh3_hash.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <tuple>


namespace
{
constexpr double LINE_TOLERANCE_IU = 2.0;
constexpr double RANK_EPSILON = 1e-12;


const char* statusName( SNAP_RESULT_STATUS aStatus )
{
    switch( aStatus )
    {
    case SNAP_RESULT_STATUS::SUCCESS: return "SUCCESS";
    case SNAP_RESULT_STATUS::BASE_CONFLICT: return "BASE_CONFLICT";
    case SNAP_RESULT_STATUS::INCOMPATIBLE: return "INCOMPATIBLE";
    case SNAP_RESULT_STATUS::NONCONVERGENT: return "NONCONVERGENT";
    case SNAP_RESULT_STATUS::INVALID_GEOMETRY: return "INVALID_GEOMETRY";
    case SNAP_RESULT_STATUS::BUDGET_EXHAUSTED: return "BUDGET_EXHAUSTED";
    }

    return "UNKNOWN";
}


const char* relationName( SNAP_RELATION aRelation )
{
    switch( aRelation )
    {
    case SNAP_RELATION::COINCIDENCE: return "COINCIDENCE";
    case SNAP_RELATION::X_COORDINATE: return "X_COORDINATE";
    case SNAP_RELATION::Y_COORDINATE: return "Y_COORDINATE";
    case SNAP_RELATION::POINT_ON_LINE: return "POINT_ON_LINE";
    case SNAP_RELATION::POINT_ON_RAY: return "POINT_ON_RAY";
    case SNAP_RELATION::POINT_ON_SEGMENT: return "POINT_ON_SEGMENT";
    case SNAP_RELATION::POINT_ON_CIRCLE: return "POINT_ON_CIRCLE";
    case SNAP_RELATION::POINT_ON_ARC: return "POINT_ON_ARC";
    case SNAP_RELATION::ANGLE: return "ANGLE";
    case SNAP_RELATION::TANGENT: return "TANGENT";
    case SNAP_RELATION::NORMAL: return "NORMAL";
    case SNAP_RELATION::BBOX_ALIGNMENT: return "BBOX_ALIGNMENT";
    case SNAP_RELATION::BBOX_EQUAL_GAP: return "BBOX_EQUAL_GAP";
    case SNAP_RELATION::GRID_X: return "GRID_X";
    case SNAP_RELATION::GRID_Y: return "GRID_Y";
    }

    return "UNKNOWN";
}


const char* referenceKindName( SNAP_REFERENCE_KIND aKind )
{
    switch( aKind )
    {
    case SNAP_REFERENCE_KIND::NONE: return "NONE";
    case SNAP_REFERENCE_KIND::BOUNDS_FEATURE: return "BOUNDS_FEATURE";
    case SNAP_REFERENCE_KIND::ANCHOR_POINT: return "ANCHOR_POINT";
    }

    return "UNKNOWN";
}


std::string stableIdString( const SNAP_STABLE_ID& aId )
{
    std::ostringstream stream;
    stream << static_cast<int>( aId.kind ) << ':';

    for( uint8_t byte : aId.target )
        stream << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<int>( byte );

    stream << std::dec << ':' << aId.featureIndex << ':' << aId.solutionBranch;
    return stream.str();
}


SNAP_TARGET_ID hashTarget( MMH3_HASH& aHash )
{
    HASH_128       digest = aHash.digest();
    SNAP_TARGET_ID bytes;
    std::copy( std::begin( digest.Value8 ), std::end( digest.Value8 ), bytes.begin() );
    return bytes;
}


SNAP_TARGET_ID idFingerprint( const SNAP_STABLE_ID& aId )
{
    MMH3_HASH hash( 0x53494446 );
    hash.addData( aId.target.data(), aId.target.size() );
    hash.add( static_cast<int32_t>( aId.kind ) );
    hash.add( aId.featureIndex );
    hash.add( aId.solutionBranch );
    return hashTarget( hash );
}


SNAP_TARGET_ID pairTarget( const SNAP_TARGET_ID& aFirst, const SNAP_TARGET_ID& aSecond )
{
    MMH3_HASH hash( 0x53494450 );
    hash.addData( aFirst.data(), aFirst.size() );
    hash.addData( aSecond.data(), aSecond.size() );
    return hashTarget( hash );
}


std::optional<bool> layoutAxis( const SNAP_CANDIDATE& aCandidate )
{
    if( aCandidate.subtype != SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT )
        return std::nullopt;

    return std::abs( aCandidate.direction.x ) >= std::abs( aCandidate.direction.y );
}


struct EQUATION
{
    double a;
    double b;
    double c;
    bool   exact;
};


VECTOR2I nearestOnManifold( const INTERSECTABLE_GEOM& aGeometry, const VECTOR2I& aPoint )
{
    return std::visit(
            [&]( const auto& aShape )
            {
                return aShape.NearestPoint( aPoint );
            },
            aGeometry );
}


std::vector<VECTOR2I> manifoldIntersections( const INTERSECTABLE_GEOM& aFirst, const INTERSECTABLE_GEOM& aSecond,
                                             const VECTOR2I& aSource )
{
    std::vector<VECTOR2I> result;
    const CIRCLE*         circle = std::get_if<CIRCLE>( &aFirst );
    const LINE*           line = std::get_if<LINE>( &aSecond );

    if( !circle || !line )
    {
        circle = std::get_if<CIRCLE>( &aSecond );
        line = std::get_if<LINE>( &aFirst );
    }

    if( circle && line )
    {
        // CIRCLE::IntersectLine uses a 4 IU tangent tolerance, wider than snap exactness permits.
        const SEG& segment = line->GetContainedSeg();
        VECTOR2D   origin( segment.A );
        VECTOR2D   direction( segment.B - segment.A );
        VECTOR2D   center( circle->Center );
        double     divisor = direction.SquaredEuclideanNorm();
        double     parameter = ( center - origin ).Dot( direction ) / divisor;
        VECTOR2D   projection = origin + direction * parameter;
        double     perpendicularSquared = ( projection - center ).SquaredEuclideanNorm();
        double     radiusSquared = static_cast<double>( circle->Radius ) * circle->Radius;

        if( perpendicularSquared <= radiusSquared )
        {
            double   offset = std::sqrt( std::max( 0.0, radiusSquared - perpendicularSquared ) / divisor );
            VECTOR2D first = projection + direction * offset;
            VECTOR2D second = projection - direction * offset;
            result.emplace_back( KiROUND( first.x ), KiROUND( first.y ) );
            result.emplace_back( KiROUND( second.x ), KiROUND( second.y ) );
        }
    }
    else
    {
        std::visit( INTERSECTION_VISITOR( aSecond, result ), aFirst );
    }

    std::sort( result.begin(), result.end(),
               [&]( const VECTOR2I& aLeft, const VECTOR2I& aRight )
               {
                   return std::tuple( aLeft.SquaredDistance( aSource ), aLeft.x, aLeft.y )
                          < std::tuple( aRight.SquaredDistance( aSource ), aRight.x, aRight.y );
               } );
    result.erase( std::unique( result.begin(), result.end() ), result.end() );
    return result;
}


LINE equationLine( const EQUATION& aEquation )
{
    VECTOR2D origin;

    if( std::abs( aEquation.a ) > std::abs( aEquation.b ) )
        origin = VECTOR2D( aEquation.c / aEquation.a, 0.0 );
    else
        origin = VECTOR2D( 0.0, aEquation.c / aEquation.b );

    VECTOR2D direction( aEquation.b, -aEquation.a );
    double   length = direction.EuclideanNorm();
    direction = direction * ( 1000000.0 / length );

    VECTOR2I integerOrigin( KiROUND( origin.x ), KiROUND( origin.y ) );
    VECTOR2I integerEnd( KiROUND( origin.x + direction.x ), KiROUND( origin.y + direction.y ) );
    return LINE( integerOrigin, integerEnd );
}


int subtypeRank( SNAP_CANDIDATE_SUBTYPE aSubtype )
{
    switch( aSubtype )
    {
    case SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR: return 0;
    case SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT:
    case SNAP_CANDIDATE_SUBTYPE::INTERSECTION: return 1;
    case SNAP_CANDIDATE_SUBTYPE::TANGENT_NORMAL: return 2;
    case SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT: return 3;
    case SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD: return 4;
    case SNAP_CANDIDATE_SUBTYPE::ACTIVE_EXTENSION: return 5;
    case SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH: return 0;
    case SNAP_CANDIDATE_SUBTYPE::GRID_AXIS: return 0;
    case SNAP_CANDIDATE_SUBTYPE::CURSOR: return 0;
    }

    return 0;
}


std::vector<EQUATION> equations( const std::vector<SNAP_CANDIDATE>& aCandidates )
{
    std::vector<EQUATION> result;

    for( const SNAP_CANDIDATE& candidate : aCandidates )
    {
        switch( candidate.relation )
        {
        case SNAP_RELATION::COINCIDENCE:
        case SNAP_RELATION::TANGENT:
        case SNAP_RELATION::NORMAL:
            result.push_back( { 1.0, 0.0, candidate.origin.x, true } );
            result.push_back( { 0.0, 1.0, candidate.origin.y, true } );
            break;

        case SNAP_RELATION::X_COORDINATE:
        case SNAP_RELATION::GRID_X: result.push_back( { 1.0, 0.0, candidate.origin.x, true } ); break;

        case SNAP_RELATION::Y_COORDINATE:
        case SNAP_RELATION::GRID_Y: result.push_back( { 0.0, 1.0, candidate.origin.y, true } ); break;

        case SNAP_RELATION::BBOX_ALIGNMENT:
        case SNAP_RELATION::BBOX_EQUAL_GAP:
            if( candidate.direction.x != 0.0 )
                result.push_back( { 1.0, 0.0, candidate.origin.x, true } );
            else if( candidate.direction.y != 0.0 )
                result.push_back( { 0.0, 1.0, candidate.origin.y, true } );
            break;

        case SNAP_RELATION::POINT_ON_LINE:
        case SNAP_RELATION::POINT_ON_RAY:
        case SNAP_RELATION::POINT_ON_SEGMENT:
        case SNAP_RELATION::ANGLE:
        {
            double a = -candidate.direction.y;
            double b = candidate.direction.x;
            result.push_back( { a, b, a * candidate.origin.x + b * candidate.origin.y, false } );
            break;
        }

        default: break;
        }
    }

    return result;
}


bool solve( const SNAP_SOURCE_CONTEXT& aContext, const std::vector<SNAP_CANDIDATE>& aCandidates, VECTOR2I& aPosition,
            int& aRemainingDof, std::vector<double>& aResiduals )
{
    std::vector<EQUATION>                  constraints = equations( aCandidates );
    std::vector<const INTERSECTABLE_GEOM*> nonlinear;
    std::optional<EQUATION>                first;
    std::optional<EQUATION>                second;

    for( const SNAP_CANDIDATE& candidate : aCandidates )
    {
        if( candidate.manifold
            && ( std::holds_alternative<CIRCLE>( *candidate.manifold )
                 || std::holds_alternative<SHAPE_ARC>( *candidate.manifold ) ) )
        {
            nonlinear.push_back( &*candidate.manifold );
        }
    }

    for( const EQUATION& equation : constraints )
    {
        double norm = std::hypot( equation.a, equation.b );

        if( norm <= RANK_EPSILON )
            return false;

        if( !first )
        {
            first = equation;
            continue;
        }

        double determinant = first->a * equation.b - equation.a * first->b;

        if( std::abs( determinant ) > RANK_EPSILON )
        {
            second = equation;
            break;
        }
    }

    double x = aContext.sourcePoint.x;
    double y = aContext.sourcePoint.y;

    if( first && second )
    {
        double determinant = first->a * second->b - second->a * first->b;
        x = ( first->c * second->b - second->c * first->b ) / determinant;
        y = ( first->a * second->c - second->a * first->c ) / determinant;
        aRemainingDof = 0;
    }
    else if( first )
    {
        double divisor = first->a * first->a + first->b * first->b;
        double delta = ( first->c - first->a * x - first->b * y ) / divisor;
        x += delta * first->a;
        y += delta * first->b;
        aRemainingDof = 1;
    }
    else
    {
        aRemainingDof = 2;
    }

    if( !second && !nonlinear.empty() )
    {
        std::vector<VECTOR2I> points;

        if( first )
        {
            INTERSECTABLE_GEOM line = equationLine( *first );
            points = manifoldIntersections( line, *nonlinear.front(), aContext.sourcePoint );
        }
        else if( nonlinear.size() >= 2 )
        {
            points = manifoldIntersections( *nonlinear[0], *nonlinear[1], aContext.sourcePoint );
        }
        else
        {
            points.push_back( nearestOnManifold( *nonlinear.front(), aContext.sourcePoint ) );
        }

        if( points.empty() )
            return false;

        x = points.front().x;
        y = points.front().y;
        aRemainingDof = first || nonlinear.size() >= 2 ? 0 : 1;
    }

    aPosition = VECTOR2I( KiROUND( x ), KiROUND( y ) );
    aResiduals.clear();

    for( const EQUATION& equation : constraints )
    {
        double residual = std::abs( equation.a * aPosition.x + equation.b * aPosition.y - equation.c )
                          / std::hypot( equation.a, equation.b );
        aResiduals.push_back( residual );

        if( equation.exact )
        {
            if( residual != 0.0 )
                return false;
        }
        else if( residual > LINE_TOLERANCE_IU )
        {
            return false;
        }
    }

    for( const SNAP_CANDIDATE& candidate : aCandidates )
    {
        if( candidate.manifold && snapManifoldDistance( *candidate.manifold, aPosition ) > LINE_TOLERANCE_IU )
        {
            return false;
        }

        if( !candidate.finite )
            continue;

        VECTOR2D offset( aPosition.x - candidate.origin.x, aPosition.y - candidate.origin.y );
        double   divisor = candidate.direction.SquaredEuclideanNorm();

        if( divisor <= RANK_EPSILON )
            return false;

        double parameter = offset.Dot( candidate.direction ) / divisor;

        if( parameter < candidate.domainStart || parameter > candidate.domainEnd )
            return false;
    }

    return true;
}
} // namespace


std::ostream& operator<<( std::ostream& aStream, SNAP_RESULT_STATUS aStatus )
{
    return aStream << statusName( aStatus );
}


bool SNAP_STABLE_ID::operator<( const SNAP_STABLE_ID& aOther ) const
{
    return std::tie( kind, target, featureIndex, solutionBranch )
           < std::tie( aOther.kind, aOther.target, aOther.featureIndex, aOther.solutionBranch );
}


SNAP_STABLE_ID MakeDerivedSnapId( SNAP_ID_KIND aKind, const SNAP_STABLE_ID& aSource, int aFeatureIndex,
                                  int aSolutionBranch )
{
    return { aKind, idFingerprint( aSource ), aFeatureIndex, aSolutionBranch };
}


SNAP_STABLE_ID MakeIntersectionSnapId( const SNAP_STABLE_ID& aFirst, const SNAP_STABLE_ID& aSecond,
                                       int aSolutionBranch )
{
    const SNAP_STABLE_ID* first = &aFirst;
    const SNAP_STABLE_ID* second = &aSecond;

    if( *second < *first )
        std::swap( first, second );

    return { SNAP_ID_KIND::INTERSECTION, pairTarget( idFingerprint( *first ), idFingerprint( *second ) ), 0,
             aSolutionBranch };
}


SNAP_STABLE_ID MakePointSnapId( SNAP_ID_KIND aKind, const VECTOR2I& aPoint, int aFeatureIndex )
{
    MMH3_HASH hash( 0x534E4150 );
    hash.add( static_cast<int32_t>( aKind ) );
    hash.add( aPoint.x );
    hash.add( aPoint.y );
    hash.add( aFeatureIndex );
    return { aKind, hashTarget( hash ), aFeatureIndex, 0 };
}


SNAP_STABLE_ID MakeCompositeSnapId( SNAP_ID_KIND aKind, const std::vector<SNAP_TARGET_ID>& aTargets, int aFeatureIndex )
{
    std::vector<SNAP_TARGET_ID> targets = aTargets;
    std::sort( targets.begin(), targets.end() );

    MMH3_HASH hash( 0x53494443 );
    hash.add( static_cast<int32_t>( aKind ) );
    hash.add( static_cast<uint32_t>( targets.size() ) );

    for( const SNAP_TARGET_ID& target : targets )
        hash.addData( target.data(), target.size() );

    hash.add( aFeatureIndex );
    return { aKind, hashTarget( hash ), aFeatureIndex, 0 };
}


SNAP_CANDIDATE SNAP_CANDIDATE::Point( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                      const VECTOR2I& aPoint, double aResidual )
{
    SNAP_CANDIDATE candidate;
    candidate.id = std::move( aId );
    candidate.priority = aPriority;
    candidate.subtype = aSubtype;
    candidate.relation = SNAP_RELATION::COINCIDENCE;
    candidate.origin = VECTOR2D( aPoint );
    candidate.normalizedScreenResidual = aResidual;
    candidate.consumedDof = 2;
    return candidate;
}


SNAP_CANDIDATE SNAP_CANDIDATE::Line( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                     const VECTOR2I& aOrigin, const VECTOR2D& aDirection, double aResidual )
{
    SNAP_CANDIDATE candidate;
    candidate.id = std::move( aId );
    candidate.priority = aPriority;
    candidate.subtype = aSubtype;
    candidate.relation = aPriority == SNAP_PRIORITY_TIER::ANGLE ? SNAP_RELATION::ANGLE : SNAP_RELATION::POINT_ON_LINE;
    candidate.origin = VECTOR2D( aOrigin );
    candidate.direction = aDirection;
    candidate.normalizedScreenResidual = aResidual;
    candidate.consumedDof = 1;

    return candidate;
}


SNAP_CANDIDATE SNAP_CANDIDATE::AxisX( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                      int aCoordinate, double aResidual )
{
    SNAP_CANDIDATE candidate;
    candidate.id = std::move( aId );
    candidate.priority = aPriority;
    candidate.subtype = aSubtype;
    candidate.relation = aPriority == SNAP_PRIORITY_TIER::GRID ? SNAP_RELATION::GRID_X : SNAP_RELATION::X_COORDINATE;
    candidate.origin = VECTOR2D( aCoordinate, 0.0 );
    candidate.direction = VECTOR2D( 1.0, 0.0 );
    candidate.normalizedScreenResidual = aResidual;
    candidate.consumedDof = 1;
    return candidate;
}


SNAP_CANDIDATE SNAP_CANDIDATE::AxisY( SNAP_STABLE_ID aId, SNAP_PRIORITY_TIER aPriority, SNAP_CANDIDATE_SUBTYPE aSubtype,
                                      int aCoordinate, double aResidual )
{
    SNAP_CANDIDATE candidate;
    candidate.id = std::move( aId );
    candidate.priority = aPriority;
    candidate.subtype = aSubtype;
    candidate.relation = aPriority == SNAP_PRIORITY_TIER::GRID ? SNAP_RELATION::GRID_Y : SNAP_RELATION::Y_COORDINATE;
    candidate.origin = VECTOR2D( 0.0, aCoordinate );
    candidate.direction = VECTOR2D( 0.0, 1.0 );
    candidate.normalizedScreenResidual = aResidual;
    candidate.consumedDof = 1;
    return candidate;
}


bool SNAP_RESULT::Accepted( const SNAP_STABLE_ID& aId ) const
{
    return std::find( accepted.begin(), accepted.end(), aId ) != accepted.end();
}


void SNAP_RESOLVER::AddCandidate( SNAP_CANDIDATE aCandidate )
{
    m_candidates.emplace_back( std::move( aCandidate ) );
}


void SNAP_RESOLVER::Clear()
{
    m_candidates.clear();
}


void SNAP_RESOLVER::SetRetainedCandidate( std::optional<SNAP_STABLE_ID> aId )
{
    m_retainedCandidate = std::move( aId );
}


void SNAP_RESOLVER::SetStickyCandidates( std::vector<SNAP_STABLE_ID> aIds )
{
    m_stickyCandidates = std::move( aIds );
}


void SNAP_RESOLVER::SetRankingHysteresis( double aNormalizedResidual )
{
    m_rankingHysteresis = std::max( 0.0, aNormalizedResidual );
}


bool SNAP_RESOLVER::hasHysteresis( const SNAP_STABLE_ID& aId ) const
{
    if( m_retainedCandidate && aId == *m_retainedCandidate )
        return true;

    return std::find( m_stickyCandidates.begin(), m_stickyCandidates.end(), aId ) != m_stickyCandidates.end();
}


void SNAP_RESOLVER::SetFeasibilityCallback( FEASIBILITY_CALLBACK aCallback )
{
    m_feasibilityCallback = std::move( aCallback );
}


void SNAP_RESOLVER::SetTraceCallback( TRACE_CALLBACK aCallback )
{
    m_traceCallback = std::move( aCallback );
}


void SNAP_RESOLVER::SetClock( CLOCK_CALLBACK aClock )
{
    m_clock = std::move( aClock );
}


void SNAP_RESOLVER::SetDeadline( CLOCK::duration aDeadline )
{
    m_deadline = aDeadline;
}


SNAP_RESULT SNAP_RESOLVER::Resolve( const SNAP_SOURCE_CONTEXT& aContext ) const
{
    struct RANKED
    {
        const SNAP_CANDIDATE* candidate;
        int                   subtypeRank;
        bool                  hysteresis;
        double                effectiveResidual;
    };

    std::vector<RANKED> ranked;
    ranked.reserve( m_candidates.size() );

    for( const SNAP_CANDIDATE& candidate : m_candidates )
    {
        const bool hysteresis = hasHysteresis( candidate.id );
        ranked.push_back(
                { &candidate, subtypeRank( candidate.subtype ), hysteresis,
                  std::max( 0.0, candidate.normalizedScreenResidual - ( hysteresis ? m_rankingHysteresis : 0.0 ) ) } );
    }

    const auto trace = [&]( const std::string& aMessage )
    {
        if( m_traceCallback )
            m_traceCallback( aMessage );
    };

    std::sort( ranked.begin(), ranked.end(),
               []( const RANKED& aLeft, const RANKED& aRight )
               {
                   const SNAP_CANDIDATE& left = *aLeft.candidate;
                   const SNAP_CANDIDATE& right = *aRight.candidate;

                   return std::forward_as_tuple( left.priority, aLeft.subtypeRank, -left.consumedDof,
                                                 left.referenceAffinity, aLeft.effectiveResidual, !aLeft.hysteresis,
                                                 left.id )
                          < std::forward_as_tuple( right.priority, aRight.subtypeRank, -right.consumedDof,
                                                   right.referenceAffinity, aRight.effectiveResidual,
                                                   !aRight.hysteresis, right.id );
               } );

    if( m_traceCallback )
    {
        std::ostringstream stream;
        stream << "resolve source=(" << aContext.sourcePoint.x << ',' << aContext.sourcePoint.y
               << ") candidates=" << ranked.size() << " sticky=" << m_stickyCandidates.size()
               << " reference=" << referenceKindName( aContext.referencePreference.kind )
               << " x-feature=" << aContext.referencePreference.horizontalFeature
               << " y-feature=" << aContext.referencePreference.verticalFeature;
        trace( stream.str() );

        for( size_t i = 0; i < ranked.size(); ++i )
        {
            const SNAP_CANDIDATE& candidate = *ranked[i].candidate;
            stream.str( {} );
            stream.clear();
            stream << "rank index=" << i << " id=" << stableIdString( candidate.id )
                   << " relation=" << relationName( candidate.relation ) << " origin=(" << candidate.origin.x << ','
                   << candidate.origin.y << ')' << " residual=" << candidate.normalizedScreenResidual
                   << " affinity=" << candidate.referenceAffinity << " sticky=" << ranked[i].hysteresis;
            trace( stream.str() );
        }
    }

    SNAP_RESULT result;
    result.position = aContext.sourcePoint;
    std::vector<SNAP_CANDIDATE> acceptedCandidates;
    std::vector<bool>           processed( ranked.size() );
    const CLOCK::time_point     start = m_clock();
    bool                        budgetExhausted = false;
    acceptedCandidates.reserve( 5 );

    if( m_feasibilityCallback )
    {
        result = m_feasibilityCallback( aContext, {} );

        if( result.status != SNAP_RESULT_STATUS::SUCCESS )
        {
            if( m_traceCallback )
            {
                std::ostringstream stream;
                stream << "result status=" << statusName( result.status ) << " position=(" << result.position.x << ','
                       << result.position.y << ") accepted=[]";
                trace( stream.str() );
            }

            return result;
        }
    }

    const auto trialCandidate = [&]( const SNAP_CANDIDATE& aCandidate, SNAP_RESULT& aTrialResult )
    {
        const size_t baseSize = acceptedCandidates.size();
        acceptedCandidates.push_back( aCandidate );
        bool accepted;

        if( m_feasibilityCallback )
        {
            aTrialResult = m_feasibilityCallback( aContext, acceptedCandidates );
            accepted = aTrialResult.status == SNAP_RESULT_STATUS::SUCCESS;
        }
        else
        {
            accepted = solve( aContext, acceptedCandidates, aTrialResult.position, aTrialResult.remainingDof,
                              aTrialResult.quantizedResiduals );
        }

        if( m_traceCallback )
        {
            std::ostringstream stream;
            stream << "trial id=" << stableIdString( aCandidate.id ) << " accepted=" << accepted
                   << " affinity=" << aCandidate.referenceAffinity << " status=" << statusName( aTrialResult.status )
                   << " position=(" << aTrialResult.position.x << ',' << aTrialResult.position.y << ')'
                   << " remaining=" << aTrialResult.remainingDof << " base=" << baseSize;
            trace( stream.str() );
        }

        acceptedCandidates.pop_back();
        return accepted;
    };

    const auto acceptCandidate = [&]( size_t aIndex )
    {
        const SNAP_CANDIDATE& candidate = *ranked[aIndex].candidate;
        SNAP_RESULT           trialResult;

        if( trialCandidate( candidate, trialResult ) )
        {
            acceptedCandidates.push_back( candidate );
            result = std::move( trialResult );
        }

        processed[aIndex] = true;
    };

    for( size_t i = 0; i < ranked.size() && result.remainingDof > 0; ++i )
    {
        if( ranked[i].candidate->priority == SNAP_PRIORITY_TIER::AUTHORED_INTRINSIC )
            acceptCandidate( i );
    }

    std::optional<size_t> acceptedAngle;
    SNAP_RESULT           acceptedAngleResult;

    for( size_t i = 0; i < ranked.size() && result.remainingDof > 0; ++i )
    {
        const SNAP_CANDIDATE& candidate = *ranked[i].candidate;

        if( candidate.priority != SNAP_PRIORITY_TIER::ANGLE )
            continue;

        SNAP_RESULT trialResult;

        if( trialCandidate( candidate, trialResult ) && !acceptedAngle )
        {
            acceptedAngle = i;
            acceptedAngleResult = std::move( trialResult );
        }

        processed[i] = true;
    }

    if( acceptedAngle )
    {
        acceptedCandidates.push_back( *ranked[*acceptedAngle].candidate );
        result = std::move( acceptedAngleResult );
    }

    for( size_t i = 0; i < ranked.size() && result.remainingDof > 0; ++i )
    {
        if( !processed[i] && ranked[i].candidate->subtype == SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR )
        {
            acceptCandidate( i );
            break;
        }
    }

    if( m_retainedCandidate )
    {
        for( size_t i = 0; i < ranked.size() && result.remainingDof > 0; ++i )
        {
            if( !processed[i] && ranked[i].candidate->id == *m_retainedCandidate )
            {
                acceptCandidate( i );
                break;
            }
        }
    }

    for( size_t i = 0; i < ranked.size(); ++i )
    {
        const SNAP_CANDIDATE& candidate = *ranked[i].candidate;

        if( processed[i] )
            continue;

        if( result.remainingDof == 0 )
            break;

        if( m_clock() - start >= m_deadline )
        {
            budgetExhausted = true;
            break;
        }

        const std::optional<bool> axis = layoutAxis( candidate );

        if( axis
            && std::any_of( acceptedCandidates.begin(), acceptedCandidates.end(),
                            [&]( const SNAP_CANDIDATE& aAccepted )
                            {
                                return layoutAxis( aAccepted ) == axis;
                            } ) )
        {
            processed[i] = true;
            continue;
        }

        acceptCandidate( i );

        if( m_clock() - start >= m_deadline && i + 1 != ranked.size() )
        {
            budgetExhausted = true;
            break;
        }
    }

    for( const SNAP_CANDIDATE& candidate : acceptedCandidates )
    {
        result.accepted.push_back( candidate.id );

        result.guides.insert( result.guides.end(), candidate.guides.begin(), candidate.guides.end() );
    }

    if( budgetExhausted )
        result.status = SNAP_RESULT_STATUS::BUDGET_EXHAUSTED;

    if( m_traceCallback )
    {
        std::ostringstream stream;
        stream << "result status=" << statusName( result.status ) << " position=(" << result.position.x << ','
               << result.position.y << ") accepted=[";

        for( size_t i = 0; i < result.accepted.size(); ++i )
        {
            if( i )
                stream << ',';

            stream << stableIdString( result.accepted[i] );
        }

        stream << ']';
        trace( stream.str() );
    }

    return result;
}
