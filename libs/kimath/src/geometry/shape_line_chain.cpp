/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <limits>
#include <math.h>            // for hypot
#include <map>
#include <string>            // for basic_string

#include <clipper2/clipper.h>
#include <core/kicad_algo.h> // for alg::run_on_pair
#include <geometry/circle.h>
#include <geometry/seg.h>    // for SEG, OPT_VECTOR2I
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <math/box2.h>       // for BOX2I
#include <math/util.h>       // for rescale
#include <math/vector2d.h>   // for VECTOR2, VECTOR2I
#include <math/box2_minmax.h>
#include <trigo.h>           // for RotatePoint

class SHAPE;

const ssize_t                     SHAPE_LINE_CHAIN::SHAPE_IS_PT = -1;
const std::pair<ssize_t, ssize_t> SHAPE_LINE_CHAIN::SHAPES_ARE_PT = { SHAPE_IS_PT, SHAPE_IS_PT };


int getArcPolygonizationMaxError()
{
    // This polyline will only be used for display.  The native arc is still used for output.
    // We therefore want to use a higher definition than the typical maxError.
    return SHAPE_ARC::DefaultAccuracyForPCB() / 5;
}


SHAPE_LINE_CHAIN::SHAPE_LINE_CHAIN( const std::vector<int>& aV) :
        SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
        m_accuracy( 0 ),
        m_closed( false ),
        m_width( 0 )
{
    for(size_t i = 0; i < aV.size(); i+= 2 )
    {
        Append( aV[i], aV[i+1] );
    }
}


SHAPE_LINE_CHAIN::SHAPE_LINE_CHAIN( const std::vector<VECTOR2I>& aV, bool aClosed ) :
        SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
        m_accuracy( 0 ),
        m_closed( false ),
        m_width( 0 )
{
    m_points = aV;
    m_shapes = std::vector<std::pair<ssize_t, ssize_t>>( aV.size(), SHAPES_ARE_PT );
    SetClosed( aClosed );
}


SHAPE_LINE_CHAIN::SHAPE_LINE_CHAIN( const SHAPE_ARC& aArc, bool aClosed, std::optional<int> aMaxError ) :
        SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
        m_accuracy( 0 ),
        m_closed( false ),
        m_width( aArc.GetWidth() )
{
    if( aMaxError.has_value() )
        Append( aArc, aMaxError.value() );
    else
        Append( aArc );

    SetClosed( aClosed );
}


SHAPE_LINE_CHAIN::SHAPE_LINE_CHAIN( const Clipper2Lib::Path64&          aPath,
                                    const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                                    const std::vector<SHAPE_ARC>&       aArcBuffer ) :
        SHAPE_LINE_CHAIN_BASE( SH_LINE_CHAIN ),
        m_accuracy( 0 ),
        m_closed( true ),
        m_width( 0 )
{
    std::map<ssize_t, ssize_t> loadedArcs;
    m_points.reserve( aPath.size() );
    m_shapes.reserve( aPath.size() );

    auto loadArc =
        [&]( ssize_t aArcIndex ) -> ssize_t
        {
            if( aArcIndex == SHAPE_IS_PT )
            {
                return SHAPE_IS_PT;
            }
            else if( loadedArcs.count( aArcIndex ) == 0 )
            {
                loadedArcs.insert( { aArcIndex, m_arcs.size() } );
                m_arcs.push_back( aArcBuffer.at( aArcIndex ) );
            }

            return loadedArcs.at( aArcIndex );
        };

    for( size_t ii = 0; ii < aPath.size(); ++ii )
    {
        Append( aPath[ii].x, aPath[ii].y );

        // Add arc info (if exists)
        int idx_z = aPath[ii].z;

        if( idx_z < 0 || idx_z >= (int)aZValueBuffer.size() )
            continue;

        m_shapes[ii].first = loadArc( aZValueBuffer[idx_z].m_FirstArcIdx );
        m_shapes[ii].second = loadArc( aZValueBuffer[idx_z].m_SecondArcIdx );
    }

    // Clipper shouldn't return duplicate contiguous points. if it did, these would be
    // removed during Append() and we would have different number of shapes to points
    wxASSERT( m_shapes.size() == m_points.size() );

    // Clipper might mess up the rotation of the indices such that an arc can be split between
    // the end point and wrap around to the start point. Lets fix the indices up now
    fixIndicesRotation();
}


Clipper2Lib::Path64 SHAPE_LINE_CHAIN::convertToClipper2( bool aRequiredOrientation,
                                                     std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                                                     std::vector<SHAPE_ARC>& aArcBuffer ) const
{
    Clipper2Lib::Path64 c_path;
    SHAPE_LINE_CHAIN input;
    bool             orientation = Area( false ) >= 0;
    ssize_t          shape_offset = aArcBuffer.size();

    if( orientation != aRequiredOrientation )
        input = Reverse();
    else
        input = *this;

    int pointCount = input.PointCount();
    c_path.reserve( pointCount );

    for( int i = 0; i < pointCount; i++ )
    {
        const VECTOR2I& vertex = input.CPoint( i );

        CLIPPER_Z_VALUE z_value( input.m_shapes[i], shape_offset );
        size_t          z_value_ptr = aZValueBuffer.size();
        aZValueBuffer.push_back( z_value );

        c_path.emplace_back( vertex.x, vertex.y, z_value_ptr );
    }

    aArcBuffer.insert( aArcBuffer.end(), input.m_arcs.begin(), input.m_arcs.end() );

    return c_path;
}


void SHAPE_LINE_CHAIN::fixIndicesRotation()
{
    wxCHECK( m_shapes.size() == m_points.size(), /*void*/ );

    if( m_shapes.size() <= 1 )
        return;

    size_t rotations = 0;

    while( ArcIndex( 0 ) != SHAPE_IS_PT
        && !IsArcStart( 0 ) )
    {
        // Rotate right
        std::rotate( m_points.rbegin(), m_points.rbegin() + 1, m_points.rend() );
        std::rotate( m_shapes.rbegin(), m_shapes.rbegin() + 1, m_shapes.rend() );

        // Sanity check - avoid infinite loops  (NB: wxCHECK is not thread-safe)
        if( rotations++ > m_shapes.size() )
            return;
    }
}


void SHAPE_LINE_CHAIN::mergeFirstLastPointIfNeeded()
{
    if( m_closed )
    {
        if( m_points.size() > 1 && m_points.front() == m_points.back() )
        {
            if( ArcIndex( m_shapes.size() - 1 ) != SHAPE_IS_PT )
            {
                m_shapes.front().second = m_shapes.front().first;
                m_shapes.front().first = ArcIndex( m_shapes.size() - 1 ) ;
            }

            m_points.pop_back();
            m_shapes.pop_back();

            fixIndicesRotation();
        }
    }
    else
    {
        if( m_points.size() > 1 && IsSharedPt( 0 ) )
        {
            // Create a duplicate point at the end
            m_points.push_back( m_points.front() );
            m_shapes.push_back( { m_shapes.front().first, SHAPE_IS_PT } );
            m_shapes.front().first = m_shapes.front().second;
            m_shapes.front().second = SHAPE_IS_PT;
        }
    }
}


void SHAPE_LINE_CHAIN::convertArc( ssize_t aArcIndex )
{
    if( aArcIndex < 0 )
        aArcIndex += m_arcs.size();

    if( aArcIndex >= static_cast<ssize_t>( m_arcs.size() ) )
        return;

    // Clear the shapes references
    for( auto& sh : m_shapes )
    {
        alg::run_on_pair( sh,
            [&]( ssize_t& aShapeIndex )
            {
                if( aShapeIndex == aArcIndex )
                    aShapeIndex = SHAPE_IS_PT;

                if( aShapeIndex > aArcIndex )
                    --aShapeIndex;
            } );

        if( sh.second != SHAPE_IS_PT && sh.first == SHAPE_IS_PT )
            std::swap( sh.first, sh.second );
    }

    m_arcs.erase( m_arcs.begin() + aArcIndex );
}


void SHAPE_LINE_CHAIN::amendArc( size_t aArcIndex, const VECTOR2I& aNewStart,
                                 const VECTOR2I& aNewEnd )
{
    wxCHECK_MSG( aArcIndex <  m_arcs.size(), /* void */,
                 wxT( "Invalid arc index requested." ) );

    SHAPE_ARC& theArc = m_arcs[aArcIndex];

    // Try to preseve the centre of the original arc
    SHAPE_ARC newArc;
    newArc.ConstructFromStartEndCenter( aNewStart, aNewEnd, theArc.GetCenter(),
                                        theArc.IsClockwise() );

    m_arcs[aArcIndex] = newArc;
}


void SHAPE_LINE_CHAIN::splitArc( ssize_t aPtIndex, bool aCoincident )
{
    if( aPtIndex < 0 )
        aPtIndex += m_shapes.size();

    if( !IsSharedPt( aPtIndex ) && IsArcStart( aPtIndex ) )
        return; // Nothing to do

    if( !IsPtOnArc( aPtIndex ) )
        return; // Nothing to do

    wxCHECK_MSG( aPtIndex < static_cast<ssize_t>( m_shapes.size() ), /* void */,
                 wxT( "Invalid point index requested." ) );

    if( IsSharedPt( aPtIndex ) || IsArcEnd( aPtIndex ) )
    {
        if( aCoincident || aPtIndex == 0 )
            return; // nothing to do

        ssize_t firstArcIndex = m_shapes[aPtIndex].first;

        const VECTOR2I& newStart = m_arcs[firstArcIndex].GetP0(); // don't amend the start
        const VECTOR2I& newEnd   = m_points[aPtIndex - 1];
        amendArc( firstArcIndex, newStart, newEnd );

        if( IsSharedPt( aPtIndex ) )
        {
            m_shapes[aPtIndex].first  = m_shapes[aPtIndex].second;
            m_shapes[aPtIndex].second = SHAPE_IS_PT;
        }
        else
        {
            m_shapes[aPtIndex] = SHAPES_ARE_PT;
        }

        return;
    }

    ssize_t    currArcIdx = ArcIndex( aPtIndex );
    SHAPE_ARC& currentArc = m_arcs[currArcIdx];

    SHAPE_ARC newArc1;
    SHAPE_ARC newArc2;

    VECTOR2I arc1End = ( aCoincident ) ? m_points[aPtIndex] : m_points[aPtIndex - 1];
    VECTOR2I arc2Start = m_points[aPtIndex];

    newArc1.ConstructFromStartEndCenter( currentArc.GetP0(), arc1End, currentArc.GetCenter(),
                                         currentArc.IsClockwise() );

    newArc2.ConstructFromStartEndCenter( arc2Start, currentArc.GetP1(), currentArc.GetCenter(),
                                         currentArc.IsClockwise() );

    if( !aCoincident && ArcIndex( aPtIndex - 1 ) != currArcIdx )
    {
        //Ignore newArc1 as it has zero points
        m_arcs[currArcIdx] = newArc2;
    }
    else
    {
        m_arcs[currArcIdx] = newArc1;
        m_arcs.insert( m_arcs.begin() + currArcIdx + 1, newArc2 );

        if( aCoincident )
        {
            m_shapes[aPtIndex].second = currArcIdx + 1;
            aPtIndex++;
        }

        // Only change the arc indices for the second half of the point range
        for( int i = aPtIndex; i < PointCount(); i++ )
        {
            alg::run_on_pair( m_shapes[i], [&]( ssize_t& aIndex ) {
                if( aIndex != SHAPE_IS_PT )
                    aIndex++;
            } );
        }
    }
}


bool SHAPE_LINE_CHAIN_BASE::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                                     VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aP, aClearance ) )
    {
        if( aLocation )
            *aLocation = aP;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I nearest;

    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        const SEG& s = GetSegment( i );
        VECTOR2I pn = s.NearestPoint( aP );
        SEG::ecoord dist_sq = ( pn - aP ).SquaredEuclideanNorm();

        if( dist_sq < closest_dist_sq )
        {
            nearest = pn;
            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    return false;
}


bool SHAPE_LINE_CHAIN::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                                VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aP, aClearance ) )
    {
        if( aLocation )
            *aLocation = aP;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I    nearest;

    // Collide line segments
    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        if( IsArcSegment( i ) )
            continue;

        const SEG&  s = GetSegment( i );
        VECTOR2I    pn = s.NearestPoint( aP );
        SEG::ecoord dist_sq = ( pn - aP ).SquaredEuclideanNorm();

        if( dist_sq < closest_dist_sq )
        {
            nearest = pn;
            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    // Collide arc segments
    for( size_t i = 0; i < ArcCount(); i++ )
    {
        const SHAPE_ARC& arc = Arc( i );

        // The arcs in the chain should have zero width
        wxASSERT_MSG( arc.GetWidth() == 0, wxT( "Invalid arc width - should be zero" ) );

        if( arc.Collide( aP, aClearance, aActual, aLocation ) )
            return true;
    }

    return false;
}


void SHAPE_LINE_CHAIN::Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter )
{
    for( VECTOR2I& pt : m_points )
        RotatePoint( pt, aCenter, aAngle );

    for( SHAPE_ARC& arc : m_arcs )
        arc.Rotate( aAngle, aCenter );
}


bool SHAPE_LINE_CHAIN::ClosestSegmentsFast( const SHAPE_LINE_CHAIN& aOther, VECTOR2I& aPt0,
                                            VECTOR2I& aPt1 ) const
{
    const std::vector<VECTOR2I>& myPts = m_points;
    const std::vector<VECTOR2I>& otherPts = aOther.m_points;

    const int c_maxBoxes = 100;
    const int c_minPtsPerBox = 20;

    int myPointsPerBox = std::max( c_minPtsPerBox, int( myPts.size() / c_maxBoxes ) + 1 );
    int otherPointsPerBox = std::max( c_minPtsPerBox, int( otherPts.size() / c_maxBoxes ) + 1 );

    int myNumBoxes = ( myPts.size() + myPointsPerBox - 1 ) / myPointsPerBox;
    int otherNumBoxes = ( otherPts.size() + otherPointsPerBox - 1 ) / otherPointsPerBox;

    struct BOX
    {
        BOX2I_MINMAX bbox;
        VECTOR2I     center;
        int          radius;
        bool         valid = false;
    };

    std::vector<BOX> myBoxes( myNumBoxes );
    std::vector<BOX> otherBoxes( otherNumBoxes );

    // Calculate bounding boxes
    for( size_t i = 0; i < myPts.size(); i++ )
    {
        const VECTOR2I pt = myPts[i];
        BOX&           box = myBoxes[i / myPointsPerBox];

        if( box.valid )
        {
            box.bbox.Merge( pt );
        }
        else
        {
            box.bbox = BOX2I_MINMAX( pt );
            box.valid = true;
        }
    }

    for( size_t i = 0; i < otherPts.size(); i++ )
    {
        const VECTOR2I pt = otherPts[i];
        BOX&           box = otherBoxes[i / otherPointsPerBox];

        if( box.valid )
        {
            box.bbox.Merge( pt );
        }
        else
        {
            box.bbox = BOX2I_MINMAX( pt );
            box.valid = true;
        }
    }

    // Store centers and radiuses
    for( BOX& box : myBoxes )
    {
        box.center = box.bbox.GetCenter();
        box.radius = int( box.bbox.GetDiameter() / 2 );
    }

    for( BOX& box : otherBoxes )
    {
        box.center = box.bbox.GetCenter();
        box.radius = int( box.bbox.GetDiameter() / 2 );
    }

    // Find closest pairs
    struct DIST_PAIR
    {
        DIST_PAIR( int64_t aDistSq, size_t aIdA, size_t aIdB ) :
                dist( aDistSq ), idA( aIdA ), idB( aIdB )
        {
        }

        int64_t dist;
        size_t  idA;
        size_t  idB;
    };

    std::vector<DIST_PAIR> pairsToTest;

    for( size_t ia = 0; ia < myBoxes.size(); ia++ )
    {
        for( size_t ib = 0; ib < otherBoxes.size(); ib++ )
        {
            const BOX& ca = myBoxes[ia];
            const BOX& cb = otherBoxes[ib];

            if( !ca.valid || !cb.valid )
                continue;

            VECTOR2L pA( ca.center );
            VECTOR2L pB( cb.center );

            int64_t dist = ( pB - pA ).EuclideanNorm();

            dist -= ca.radius;
            dist -= cb.radius;

            pairsToTest.emplace_back( dist, ia, ib );
        }
    }

    std::sort( pairsToTest.begin(), pairsToTest.end(),
               []( const DIST_PAIR& a, const DIST_PAIR& b )
               {
                   return a.dist < b.dist;
               } );

    const int c_polyPairsLimit = 5;

    // Find closest segments in tested pairs
    int64_t total_closest_dist_sq = VECTOR2I::ECOORD_MAX;

    for( size_t pairId = 0; pairId < pairsToTest.size() && pairId < c_polyPairsLimit; pairId++ )
    {
        const DIST_PAIR& pair = pairsToTest[pairId];

        VECTOR2I ptA;
        VECTOR2I ptB;
        int64_t  dist_sq;

        size_t myStartId = pair.idA * myPointsPerBox;
        size_t myEndId = myStartId + myPointsPerBox;

        if( myEndId > myPts.size() )
            myEndId = myPts.size();

        VECTOR2I myPrevPt = myPts[myStartId == 0 ? myPts.size() - 1 : myStartId - 1];

        size_t otherStartId = pair.idB * otherPointsPerBox;
        size_t otherEndId = otherStartId + otherPointsPerBox;

        if( otherEndId > otherPts.size() )
            otherEndId = otherPts.size();

        VECTOR2I otherPrevPt = otherPts[otherStartId == 0 ? otherPts.size() - 1 : otherStartId - 1];

        if( ClosestSegments( myPrevPt, myPts.begin() + myStartId, myPts.begin() + myEndId,
                             otherPrevPt, otherPts.begin() + otherStartId,
                             otherPts.begin() + otherEndId, ptA, ptB, dist_sq ) )
        {
            if( dist_sq < total_closest_dist_sq )
            {
                total_closest_dist_sq = dist_sq;
                aPt0 = ptA;
                aPt1 = ptB;
            }
        }
    }

    return total_closest_dist_sq != VECTOR2I::ECOORD_MAX;
}


bool SHAPE_LINE_CHAIN::ClosestSegments( const VECTOR2I& aMyPrevPt, const point_citer& aMyStart,
                                        const point_citer& aMyEnd, const VECTOR2I& aOtherPrevPt,
                                        const point_citer& aOtherStart,
                                        const point_citer& aOtherEnd, VECTOR2I& aPt0,
                                        VECTOR2I& aPt1, int64_t& aDistSq )
{
    if( aMyStart == aMyEnd )
        return false;

    if( aOtherStart == aOtherEnd )
        return false;

    int64_t  closest_dist_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I lastPtA = aMyPrevPt;

    for( point_citer itA = aMyStart; itA != aMyEnd; itA++ )
    {
        const VECTOR2I& ptA = *itA;
        VECTOR2I        lastPtB = aOtherPrevPt;

        for( point_citer itB = aOtherStart; itB != aOtherEnd; itB++ )
        {
            const VECTOR2I& ptB = *itB;

            SEG segA( lastPtA, ptA );
            SEG segB( lastPtB, ptB );

            VECTOR2I nearestA, nearestB;

            int64_t dist_sq;

            if( segA.NearestPoints( segB, nearestA, nearestB, dist_sq ) )
            {
                if( dist_sq < closest_dist_sq )
                {
                    closest_dist_sq = dist_sq;
                    aPt0 = nearestA;
                    aPt1 = nearestB;
                }
            }

            lastPtB = ptB;
        }

        lastPtA = ptA;
    }

    aDistSq = closest_dist_sq;
    return closest_dist_sq != VECTOR2I::ECOORD_MAX;
}


bool SHAPE_LINE_CHAIN::ClosestPoints( const point_citer& aMyStart, const point_citer& aMyEnd,
                                      const point_citer& aOtherStart, const point_citer& aOtherEnd,
                                      VECTOR2I& aPt0, VECTOR2I& aPt1, int64_t& aDistSq )
{
    int64_t closest_dist_sq = VECTOR2I::ECOORD_MAX;

    for( point_citer itA = aMyStart; itA != aMyEnd; itA++ )
    {
        const VECTOR2I& ptA = *itA;

        for( point_citer itB = aOtherStart; itB != aOtherEnd; itB++ )
        {
            const VECTOR2I& ptB = *itB;

            ecoord dx = (ecoord) ptB.x - ptA.x;
            ecoord dy = (ecoord) ptB.y - ptA.y;

            SEG::ecoord dist_sq = dx * dx + dy * dy;

            if( dist_sq < closest_dist_sq )
            {
                closest_dist_sq = dist_sq;
                aPt0 = ptA;
                aPt1 = ptB;
            }
        }
    }

    aDistSq = closest_dist_sq;
    return closest_dist_sq != VECTOR2I::ECOORD_MAX;
}


bool SHAPE_LINE_CHAIN::ClosestPoints( const SHAPE_LINE_CHAIN& aOther, VECTOR2I& aPt0,
                                      VECTOR2I& aPt1 ) const
{
    ecoord dist_sq;

    return ClosestPoints( m_points.cbegin(), m_points.cend(), aOther.m_points.cbegin(),
                          aOther.m_points.cend(), aPt0, aPt1, dist_sq );
}


bool SHAPE_LINE_CHAIN_BASE::Collide( const SEG& aSeg, int aClearance, int* aActual,
                                     VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aSeg.A ) )
    {
        if( aLocation )
            *aLocation = aSeg.A;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I nearest;

    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        const SEG& s = GetSegment( i );
        SEG::ecoord dist_sq = s.SquaredDistance( aSeg );

        if( dist_sq < closest_dist_sq )
        {
            if( aLocation )
                nearest = s.NearestPoint( aSeg );

            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0)
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    return false;
}


bool SHAPE_LINE_CHAIN::Collide( const SEG& aSeg, int aClearance, int* aActual,
                                VECTOR2I* aLocation ) const
{
    if( IsClosed() && PointInside( aSeg.A ) )
    {
        if( aLocation )
            *aLocation = aSeg.A;

        if( aActual )
            *aActual = 0;

        return true;
    }

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    SEG::ecoord clearance_sq = SEG::Square( aClearance );
    VECTOR2I    nearest;

    // Collide line segments
    for( size_t i = 0; i < GetSegmentCount(); i++ )
    {
        if( IsArcSegment( i ) )
            continue;

        const SEG&  s = GetSegment( i );
        SEG::ecoord dist_sq = s.SquaredDistance( aSeg );

        if( dist_sq < closest_dist_sq )
        {
            if( aLocation )
                nearest = s.NearestPoint( aSeg );

            closest_dist_sq = dist_sq;

            if( closest_dist_sq == 0 )
                break;

            // If we're not looking for aActual then any collision will do
            if( closest_dist_sq < clearance_sq && !aActual )
                break;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < clearance_sq )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        return true;
    }

    int         dist = std::numeric_limits<int>::max();
    SEG::ecoord closest_dist = sqrt( closest_dist_sq );

    // Collide arc segments
    for( size_t i = 0; i < ArcCount(); i++ )
    {
        const SHAPE_ARC& arc = Arc( i );
        VECTOR2I         pos;

        // The arcs in the chain should have zero width
        wxASSERT_MSG( arc.GetWidth() == 0, wxT( "Invalid arc width - should be zero" ) );

        if( arc.Collide( aSeg, aClearance, aActual || aLocation ? &dist : nullptr,
                         aLocation ? &pos : nullptr ) )
        {
            if( !aActual )
                return true;

            if( dist < closest_dist )
            {
                closest_dist = dist;
                nearest = pos;
            }
        }
    }

    if( closest_dist == 0 || closest_dist < aClearance )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = closest_dist;

        return true;
    }

    return false;
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Reverse() const
{
    SHAPE_LINE_CHAIN a( *this );

    reverse( a.m_points.begin(), a.m_points.end() );
    reverse( a.m_shapes.begin(), a.m_shapes.end() );
    reverse( a.m_arcs.begin(), a.m_arcs.end() );

    for( auto& sh : a.m_shapes )
    {
        if( sh != SHAPES_ARE_PT )
        {
            alg::run_on_pair( sh,
                [&]( ssize_t& aShapeIndex )
                {
                    if( aShapeIndex != SHAPE_IS_PT )
                        aShapeIndex = a.m_arcs.size() - aShapeIndex - 1;
                } );

            if( sh.second != SHAPE_IS_PT )
            {
                // If the second element is populated, the first one should be too!
                assert( sh.first != SHAPE_IS_PT );

                // Switch round first and second in shared points, as part of reversing the chain
                std::swap( sh.first, sh.second );
            }
        }
    }

    for( SHAPE_ARC& arc : a.m_arcs )
        arc.Reverse();

    a.m_closed = m_closed;

    return a;
}


void SHAPE_LINE_CHAIN::ClearArcs()
{
    for( ssize_t arcIndex = m_arcs.size() - 1; arcIndex >= 0; --arcIndex )
        convertArc( arcIndex );
}


long long int SHAPE_LINE_CHAIN::Length() const
{
    long long int l = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        // Only include segments that aren't part of arc shapes
        if( !IsArcSegment(i) )
            l += CSegment( i ).Length();
    }

    for( size_t i = 0; i < ArcCount(); i++ )
        l += CArcs()[i].GetLength();

    return l;
}


void SHAPE_LINE_CHAIN::Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection )
{
    for( auto& pt : m_points )
    {
        if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
            pt.x = -pt.x + 2 * aRef.x;
        else
            pt.y = -pt.y + 2 * aRef.y;
    }

    for( auto& arc : m_arcs )
        arc.Mirror( aRef, aFlipDirection );
}


void SHAPE_LINE_CHAIN::Mirror( const SEG& axis )
{
    for( auto& pt : m_points )
        pt = axis.ReflectPoint( pt );

    for( auto& arc : m_arcs )
        arc.Mirror( axis );
}


void SHAPE_LINE_CHAIN::Replace( int aStartIndex, int aEndIndex, const VECTOR2I& aP )
{
    Remove( aStartIndex, aEndIndex );
    Insert( aStartIndex, aP );
    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Replace( int aStartIndex, int aEndIndex, const SHAPE_LINE_CHAIN& aLine )
{
    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    // We only process lines in order in this house
    wxASSERT( aStartIndex <= aEndIndex );
    wxASSERT( aEndIndex < static_cast<int>( m_points.size() ) );

    SHAPE_LINE_CHAIN newLine = aLine;

    // Zero points to add?
    if( newLine.PointCount() == 0 )
    {
        Remove( aStartIndex, aEndIndex );
        return;
    }

    // Remove coincident points in the new line
    if( newLine.m_points.front() == m_points[aStartIndex] )
    {
        aStartIndex++;
        newLine.Remove( 0 );

        // Zero points to add?
        if( newLine.PointCount() == 0 )
        {
            Remove( aStartIndex, aEndIndex );
            return;
        }
    }

    if( newLine.m_points.back() == m_points[aEndIndex] && aEndIndex > 0 )
    {
        aEndIndex--;
        newLine.Remove( -1 );
    }

    Remove( aStartIndex, aEndIndex );

    // Zero points to add?
    if( newLine.PointCount() == 0 )
        return;

    // The total new arcs index is added to the new arc indices
    size_t prev_arc_count = m_arcs.size();
    std::vector<std::pair<ssize_t, ssize_t>> new_shapes = newLine.m_shapes;

    for( std::pair<ssize_t, ssize_t>& shape_pair : new_shapes )
    {
        alg::run_on_pair( shape_pair,
            [&]( ssize_t& aShape )
            {
                if( aShape != SHAPE_IS_PT )
                    aShape += prev_arc_count;
            } );
    }

    m_shapes.insert( m_shapes.begin() + aStartIndex, new_shapes.begin(), new_shapes.end() );
    m_points.insert( m_points.begin() + aStartIndex, newLine.m_points.begin(),
                     newLine.m_points.end() );
    m_arcs.insert( m_arcs.end(), newLine.m_arcs.begin(), newLine.m_arcs.end() );

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Remove( int aStartIndex, int aEndIndex )
{
    wxCHECK( m_shapes.size() == m_points.size(), /*void*/ );

    // Unwrap the chain first (correctly handling removing arc at
    // end of chain coincident with start)
    bool closedState = IsClosed();
    SetClosed( false );

    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    if( aStartIndex >= PointCount() || aEndIndex >= PointCount() || aStartIndex > aEndIndex)
    {
        SetClosed( closedState );
        return;
    }


    // Split arcs, making arcs coincident
    if( !IsArcStart( aStartIndex ) && IsPtOnArc( aStartIndex ) )
        splitArc( aStartIndex, false );

    if( IsSharedPt( aStartIndex ) ) // Don't delete the shared point
        aStartIndex += 1;

    if( !IsArcEnd( aEndIndex ) && IsPtOnArc( aEndIndex ) && aEndIndex < PointCount() - 1 )
        splitArc( aEndIndex + 1, true );

    if( IsSharedPt( aEndIndex ) ) // Don't delete the shared point
        aEndIndex -= 1;

    if( aStartIndex > aEndIndex )
    {
        SetClosed( closedState );
        return;
    }

    std::set<size_t> extra_arcs;
    auto logArcIdxRemoval = [&]( ssize_t& aShapeIndex )
                            {
                                if( aShapeIndex != SHAPE_IS_PT )
                                    extra_arcs.insert( aShapeIndex );
                            };

    // Remove any overlapping arcs in the point range
    for( int i = aStartIndex; i <= aEndIndex; i++ )
    {
        if( IsSharedPt( i ) )
        {
            if( i == aStartIndex )
            {
                logArcIdxRemoval( m_shapes[i].second ); // Only remove the arc on the second index

                // Ensure that m_shapes has been built correctly.
                assert( i < aEndIndex || m_shapes[i + 1].first == m_shapes[i].second );

                continue;
            }
            else if( i == aEndIndex )
            {
                logArcIdxRemoval( m_shapes[i].first ); // Only remove the arc on the first index

                // Ensure that m_shapes has been built correctly.
                assert( i > aStartIndex || ( IsSharedPt( i - 1 )
                                ? m_shapes[i - 1].second == m_shapes[i].first
                                : m_shapes[i - 1].first == m_shapes[i].first ) );
                continue;
            }
        }
        else
        {
            alg::run_on_pair( m_shapes[i], logArcIdxRemoval );
        }
    }

    for( auto arc : extra_arcs )
        convertArc( arc );

    m_shapes.erase( m_shapes.begin() + aStartIndex, m_shapes.begin() + aEndIndex + 1 );
    m_points.erase( m_points.begin() + aStartIndex, m_points.begin() + aEndIndex + 1 );
    assert( m_shapes.size() == m_points.size() );

    SetClosed( closedState );
}


SEG::ecoord SHAPE_LINE_CHAIN_BASE::SquaredDistance( const VECTOR2I& aP, bool aOutlineOnly ) const
{
    ecoord d = VECTOR2I::ECOORD_MAX;

    if( IsClosed() && PointInside( aP ) && !aOutlineOnly )
        return 0;

    for( size_t s = 0; s < GetSegmentCount(); s++ )
        d = std::min( d, GetSegment( s ).SquaredDistance( aP ) );

    return d;
}


int SHAPE_LINE_CHAIN::Split( const VECTOR2I& aP, bool aExact )
{
    int ii = -1;
    int min_dist = 2;

    int found_index = Find( aP );

    if( found_index >= 0 && aExact )
        return found_index;

    for( int s = 0; s < SegmentCount(); s++ )
    {
        const SEG seg = CSegment( s );
        int dist = seg.Distance( aP );

        // make sure we are not producing a 'slightly concave' primitive. This might happen
        // if aP lies very close to one of already existing points.
        if( dist < min_dist && seg.A != aP && seg.B != aP )
        {
            min_dist = dist;
            if( found_index < 0 )
                ii = s;
            else if( s < found_index )
                ii = s;
        }
    }

    if( ii < 0 )
        ii = found_index;

    if( ii >= 0 )
    {
        // Don't create duplicate points
        if( GetPoint( ii ) == aP )
            return ii;

        size_t newIndex = static_cast<size_t>( ii ) + 1;

        if( IsArcSegment( ii ) )
        {
            m_points.insert( m_points.begin() + newIndex, aP );
            m_shapes.insert( m_shapes.begin() + newIndex, { ArcIndex( ii ), SHAPE_IS_PT } );
            splitArc( newIndex, true ); // Make the inserted point a shared point
        }
        else
        {
            Insert( newIndex, aP );
        }

        return newIndex;
    }

    return -1;
}


int SHAPE_LINE_CHAIN::Find( const VECTOR2I& aP, int aThreshold ) const
{
    for( int s = 0; s < PointCount(); s++ )
    {
        if( aThreshold == 0 )
        {
            if( CPoint( s ) == aP )
                return s;
        }
        else
        {
            if( (CPoint( s ) - aP).EuclideanNorm() <= aThreshold )
                return s;
        }
    }

    return -1;
}


int SHAPE_LINE_CHAIN::FindSegment( const VECTOR2I& aP, int aThreshold ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
    {
        if( CSegment( s ).Distance( aP ) <= aThreshold )
            return s;
    }

    return -1;
}


int SHAPE_LINE_CHAIN::ShapeCount() const
{
    wxCHECK2_MSG( m_points.size() == m_shapes.size(), return 0, "Invalid chain!" );

    if( m_points.size() < 2 )
        return 0;

    int numShapes = 1;

    for( int i = NextShape( 0 ); i != -1; i = NextShape( i ) )
        numShapes++;

    return numShapes;
}


SEG SHAPE_LINE_CHAIN::Segment( int aIndex ) const
{
    int segCount = SegmentCount();

    if( aIndex < 0 )
        aIndex += segCount;

    wxCHECK( aIndex < segCount && aIndex >= 0,
             m_points.size() > 0 ? SEG( m_points.back(), m_points.back() ) : SEG( 0, 0, 0, 0 ) );

    if( aIndex == (int) ( m_points.size() - 1 ) && m_closed )
        return SEG( m_points[aIndex], m_points[0], aIndex );
    else
        return SEG( m_points[aIndex], m_points[aIndex + 1], aIndex );
}


int SHAPE_LINE_CHAIN::NextShape( int aPointIndex ) const
{
    if( aPointIndex < 0 )
        aPointIndex += PointCount();

    if( aPointIndex < 0 )
        return -1;

    int lastIndex = PointCount() - 1;

    // Last point?
    if( aPointIndex >= lastIndex )
        return -1; // we don't want to wrap around

    if( m_shapes[aPointIndex] == SHAPES_ARE_PT )
    {
        if( aPointIndex == lastIndex - 1 )
        {
            if( m_closed )
                return lastIndex;
            else
                return -1;
        }
        else
        {
            return aPointIndex + 1;
        }
    }

    int arcStart = aPointIndex;

    // The second element should only get populated when the point is shared between two shapes.
    // If not a shared point, then the index should always go on the first element.
    wxCHECK2_MSG( m_shapes[aPointIndex].first != SHAPE_IS_PT, return -1, "malformed chain!" );

    ssize_t currentArcIdx = ArcIndex( aPointIndex );

    // Now skip the rest of the arc
    while( aPointIndex < lastIndex && ArcIndex( aPointIndex ) == currentArcIdx )
        aPointIndex += 1;

    bool indexStillOnArc = alg::pair_contains( m_shapes[aPointIndex], currentArcIdx );

    // We want the last vertex of the arc if the initial point was the start of one
    // Well-formed arcs should generate more than one point to travel above
    if( aPointIndex - arcStart > 1 && !indexStillOnArc )
        aPointIndex -= 1;

    if( aPointIndex == lastIndex )
    {
        if( !m_closed || IsArcSegment( aPointIndex ) )
            return -1; //no shape
        else
            return lastIndex; // Segment between last point and the start of the chain
    }

    return aPointIndex;
}


void SHAPE_LINE_CHAIN::SetPoint( int aIndex, const VECTOR2I& aPos )
{
    if( aIndex < 0 )
        aIndex += PointCount();
    else if( aIndex >= PointCount() )
        aIndex -= PointCount();

    m_points[aIndex] = aPos;

    alg::run_on_pair( m_shapes[aIndex],
        [&]( ssize_t& aIdx )
        {
            if( aIdx != SHAPE_IS_PT )
                convertArc( aIdx );
        } );
}


void SHAPE_LINE_CHAIN::RemoveShape( int aPointIndex )
{
    if( aPointIndex < 0 )
        aPointIndex += PointCount();

    if( aPointIndex >= PointCount() || aPointIndex < 0 )
        return; // Invalid index, fail gracefully

    if( m_shapes[aPointIndex] == SHAPES_ARE_PT )
    {
        Remove( aPointIndex );
        return;
    }

    int start  = aPointIndex;
    int end    = aPointIndex;
    int arcIdx = ArcIndex( aPointIndex );

    if( !IsArcStart( start ) )
    {
        // aPointIndex is not a shared point, so iterate backwards to find the start of the arc
        while( start > 0 && ArcIndex( static_cast<ssize_t>( start ) - 1 ) == arcIdx )
            start--;
    }

    if( !IsArcEnd( end ) || start == end )
        end = NextShape( end ); // can be -1 to indicate end of chain

    Remove( start, end );
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Slice( int aStartIndex, int aEndIndex ) const
{
    return Slice( aStartIndex, aEndIndex, getArcPolygonizationMaxError() );
}


const SHAPE_LINE_CHAIN SHAPE_LINE_CHAIN::Slice( int aStartIndex, int aEndIndex, int aMaxError ) const
{
    SHAPE_LINE_CHAIN rv;

    if( aEndIndex < 0 )
        aEndIndex += PointCount();

    if( aStartIndex < 0 )
        aStartIndex += PointCount();

    // Bad programmer checks
    wxCHECK( aStartIndex >= 0, SHAPE_LINE_CHAIN() );
    wxCHECK( aEndIndex >= 0, SHAPE_LINE_CHAIN() );
    wxCHECK( aStartIndex < PointCount(), SHAPE_LINE_CHAIN() );
    wxCHECK( aEndIndex < PointCount(), SHAPE_LINE_CHAIN() );
    wxCHECK( aEndIndex >= aStartIndex, SHAPE_LINE_CHAIN() );

    int numPoints = static_cast<int>( m_points.size() );

    if( IsArcSegment( aStartIndex ) && !IsArcStart( aStartIndex ) )
    {
        // Cutting in middle of an arc, lets split it
        ssize_t          arcToSplitIndex = ArcIndex( aStartIndex );
        const SHAPE_ARC& arcToSplit = Arc( arcToSplitIndex );

        // Copy the points as arc points
        for( size_t i = aStartIndex; i < m_points.size() && arcToSplitIndex == ArcIndex( i ); i++ )
        {
            rv.m_points.push_back( m_points[i] );
            rv.m_shapes.push_back( { rv.m_arcs.size(), SHAPE_IS_PT } );
            rv.m_bbox.Merge( m_points[i] );
        }

        // Create a new arc from the existing one, with different start point.
        SHAPE_ARC newArc;

        VECTOR2I newArcStart = m_points[aStartIndex];

        newArc.ConstructFromStartEndCenter( newArcStart, arcToSplit.GetP1(),
                                            arcToSplit.GetCenter(),
                                            arcToSplit.IsClockwise() );


        rv.m_arcs.push_back( newArc );

        aStartIndex += rv.PointCount();
    }

    for( int i = aStartIndex; i <= aEndIndex && i < numPoints; i = NextShape( i ) )
    {
        if( i == -1 )
            return rv; // NextShape reached the end

        int  nextShape = NextShape( i );
        bool isLastShape = nextShape < 0;

        if( IsArcStart( i ) )
        {
            if(  ( isLastShape && aEndIndex != ( numPoints - 1 ) )
                     || ( nextShape > aEndIndex ) )
            {
                if( i == aEndIndex )
                {
                    // Single point on an arc, just append the single point
                    rv.Append( m_points[i] );
                    return rv;
                }

                // Cutting in middle of an arc, lets split it
                ssize_t          arcIndex = ArcIndex( i );
                const SHAPE_ARC& currentArc = Arc( arcIndex );

                // Copy the points as arc points
                for( ; i <= aEndIndex && i < numPoints; i++ )
                {
                    if( arcIndex != ArcIndex( i ) )
                        break;

                    rv.m_points.push_back( m_points[i] );
                    rv.m_shapes.push_back( { rv.m_arcs.size(), SHAPE_IS_PT } );
                    rv.m_bbox.Merge( m_points[i] );
                }

                // Create a new arc from the existing one, with different end point.
                SHAPE_ARC newArc;

                VECTOR2I newArcEnd = m_points[aEndIndex];

                newArc.ConstructFromStartEndCenter( currentArc.GetP0(), newArcEnd,
                                                    currentArc.GetCenter(),
                                                    currentArc.IsClockwise() );


                rv.m_arcs.push_back( newArc );

                return rv;
            }
            else
            {
                // append the whole arc
                const SHAPE_ARC& currentArc = Arc( ArcIndex( i ) );
                rv.Append( currentArc, aMaxError );
            }

            if( isLastShape )
                return rv;
        }
        else
        {
            wxASSERT_MSG( !IsArcSegment( i ), wxT( "Still on an arc segment, we missed something..." ) );

            if( i == aStartIndex )
                rv.Append( m_points[i] );

            bool nextPointIsArc = isLastShape ? false : IsArcSegment( nextShape );

            if( !nextPointIsArc && i < SegmentCount() && i < aEndIndex )
                rv.Append( GetSegment( i ).B );
        }

    }

    wxASSERT( rv.m_points.size() == rv.m_shapes.size() );

    return rv;
}


void SHAPE_LINE_CHAIN::Append( const SHAPE_LINE_CHAIN& aOtherLine )
{
    assert( m_shapes.size() == m_points.size() );

    if( aOtherLine.PointCount() == 0 )
    {
        return;
    }

    size_t num_arcs = m_arcs.size();
    m_arcs.insert( m_arcs.end(), aOtherLine.m_arcs.begin(), aOtherLine.m_arcs.end() );

    auto fixShapeIndices =
            [&]( const std::pair<ssize_t, ssize_t>& aShapeIndices ) -> std::pair<ssize_t, ssize_t>
            {
                std::pair<ssize_t, ssize_t> retval =  aShapeIndices;

                alg::run_on_pair( retval, [&]( ssize_t& aIndex )
                                          {
                                              if( aIndex != SHAPE_IS_PT )
                                                  aIndex = aIndex + num_arcs;
                                          } );

                return retval;
            };

    if( PointCount() == 0 || aOtherLine.CPoint( 0 ) != CLastPoint() )
    {
        const VECTOR2I p = aOtherLine.CPoint( 0 );
        m_points.push_back( p );
        m_shapes.push_back( fixShapeIndices( aOtherLine.CShapes()[0] ) );
        m_bbox.Merge( p );
    }
    else if( aOtherLine.IsArcSegment( 0 ) )
    {
        // Associate the new arc shape with the last point of this chain
        if( m_shapes.back() == SHAPES_ARE_PT )
            m_shapes.back().first = aOtherLine.CShapes()[0].first + num_arcs;
        else
            m_shapes.back().second = aOtherLine.CShapes()[0].first + num_arcs;
    }


    for( int i = 1; i < aOtherLine.PointCount(); i++ )
    {
        const VECTOR2I p = aOtherLine.CPoint( i );
        m_points.push_back( p );

        ssize_t arcIndex = aOtherLine.ArcIndex( i );

        if( arcIndex != ssize_t( SHAPE_IS_PT ) )
        {
            m_shapes.push_back( fixShapeIndices( aOtherLine.m_shapes[i] ) );
        }
        else
            m_shapes.push_back( SHAPES_ARE_PT );

        m_bbox.Merge( p );
    }

    mergeFirstLastPointIfNeeded();

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Append( const SHAPE_ARC& aArc )
{
    Append( aArc, getArcPolygonizationMaxError() );
}


void SHAPE_LINE_CHAIN::Append( const SHAPE_ARC& aArc, int aMaxError )
{
    SHAPE_LINE_CHAIN chain = aArc.ConvertToPolyline( aMaxError );

    if( chain.PointCount() > 2 )
    {
        chain.m_arcs.push_back( aArc );
        chain.m_arcs.back().SetWidth( 0 );

        for( auto& sh : chain.m_shapes )
            sh.first = 0;
    }

    Append( chain );

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Insert( size_t aVertex, const VECTOR2I& aP )
{
    if( aVertex == m_points.size() )
    {
        Append( aP );
        return;
    }

    wxCHECK( aVertex < m_points.size(), /* void */ );

    if( aVertex > 0 && IsPtOnArc( aVertex ) )
        splitArc( aVertex );

    //@todo need to check we aren't creating duplicate points
    m_points.insert( m_points.begin() + aVertex, aP );
    m_shapes.insert( m_shapes.begin() + aVertex, SHAPES_ARE_PT );

    assert( m_shapes.size() == m_points.size() );
}


void SHAPE_LINE_CHAIN::Insert( size_t aVertex, const SHAPE_ARC& aArc )
{
    Insert( aVertex, aArc, getArcPolygonizationMaxError() );
}


void SHAPE_LINE_CHAIN::Insert( size_t aVertex, const SHAPE_ARC& aArc, int aMaxError )
{
    wxCHECK( aVertex < m_points.size(), /* void */ );

    if( aVertex > 0 && IsPtOnArc( aVertex ) )
        splitArc( aVertex );

    /// Step 1: Find the position for the new arc in the existing arc vector
    ssize_t arc_pos = m_arcs.size();

    for( auto arc_it = m_shapes.rbegin() ;
              arc_it != m_shapes.rend() + aVertex;
              arc_it++ )
    {
        if( *arc_it != SHAPES_ARE_PT )
        {
            arc_pos = std::max( ( *arc_it ).first, ( *arc_it ).second );
            arc_pos++;
        }
    }

    //Increment all arc indices before inserting the new arc
    for( auto& sh : m_shapes )
    {
        alg::run_on_pair( sh,
            [&]( ssize_t& aIndex )
            {
                if( aIndex >= arc_pos )
                    aIndex++;
            } );
    }

    SHAPE_ARC arcCopy( aArc );
    arcCopy.SetWidth( 0 );
    m_arcs.insert( m_arcs.begin() + arc_pos, arcCopy );

    /// Step 2: Add the arc polyline points to the chain
    //@todo need to check we aren't creating duplicate points at start or end
    auto& chain = aArc.ConvertToPolyline( aMaxError );
    m_points.insert( m_points.begin() + aVertex, chain.CPoints().begin(), chain.CPoints().end() );

    /// Step 3: Add the vector of indices to the shape vector
    //@todo need to check we aren't creating duplicate points at start or end
    std::vector<std::pair<ssize_t, ssize_t>> new_points( chain.PointCount(),
                                                         { arc_pos, SHAPE_IS_PT } );

    m_shapes.insert( m_shapes.begin() + aVertex, new_points.begin(), new_points.end() );
    assert( m_shapes.size() == m_points.size() );
}


struct compareOriginDistance
{
    compareOriginDistance( const VECTOR2I& aOrigin ) :
        m_origin( aOrigin ) {};

    bool operator()( const SHAPE_LINE_CHAIN::INTERSECTION& aA,
                     const SHAPE_LINE_CHAIN::INTERSECTION& aB ) const
    {
        return ( m_origin - aA.p ).EuclideanNorm() < ( m_origin - aB.p ).EuclideanNorm();
    }

    VECTOR2I m_origin;
};


int SHAPE_LINE_CHAIN::Intersect( const SEG& aSeg, INTERSECTIONS& aIp ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
    {
        OPT_VECTOR2I p = CSegment( s ).Intersect( aSeg );

        if( p )
        {
            INTERSECTION is;
            is.valid = true;
            is.index_our = s;
            is.index_their = -1;
            is.is_corner_our = is.is_corner_their = false;
            is.p = *p;
            aIp.push_back( is );
        }
    }

    compareOriginDistance comp( aSeg.A );
    sort( aIp.begin(), aIp.end(), comp );

    return aIp.size();
}


bool SHAPE_LINE_CHAIN::Intersects( const SEG& aSeg ) const
{
    for( int s = 0; s < SegmentCount(); s++ )
    {
        if( CSegment( s ).Intersects( aSeg ) )
            return true;
    }

    return false;
}


static inline void addIntersection( SHAPE_LINE_CHAIN::INTERSECTIONS& aIps, int aPc,
                                    const SHAPE_LINE_CHAIN::INTERSECTION& aP )
{
    if( aIps.size() == 0 )
    {
        aIps.push_back( aP );
        return;
    }

    aIps.push_back( aP );
}


int SHAPE_LINE_CHAIN::Intersect( const SHAPE_LINE_CHAIN& aChain, INTERSECTIONS& aIp,
                                 bool aExcludeColinearAndTouching, BOX2I* aChainBBox ) const
{
    BOX2I bb_other = aChainBBox ? *aChainBBox : aChain.BBox();

    for( int s1 = 0; s1 < SegmentCount(); s1++ )
    {
        const SEG& a = CSegment( s1 );
        const BOX2I bb_cur( a.A, a.B - a.A );

        if( !bb_other.Intersects( bb_cur ) )
            continue;

        for( int s2 = 0; s2 < aChain.SegmentCount(); s2++ )
        {
            const SEG& b = aChain.CSegment( s2 );
            INTERSECTION is;

            is.index_our = s1;
            is.index_their = s2;
            is.is_corner_our = false;
            is.is_corner_their = false;
            is.valid = true;

            OPT_VECTOR2I p = a.Intersect( b );

            bool coll = a.Collinear( b );

            if( coll && ! aExcludeColinearAndTouching )
            {
                if( a.Contains( b.A ) )
                {
                    is.p = b.A;
                    is.is_corner_their = true;
                    addIntersection(aIp, PointCount(), is);
                }

                if( a.Contains( b.B ) )
                {
                    is.p = b.B;
                    is.index_their++;
                    is.is_corner_their = true;
                    addIntersection( aIp, PointCount(), is );
                }

                if( b.Contains( a.A ) )
                {
                    is.p = a.A;
                    is.is_corner_our = true;
                    addIntersection( aIp, PointCount(), is );
                }

                if( b.Contains( a.B ) )
                {
                    is.p = a.B;
                    is.index_our++;
                    is.is_corner_our = true;
                    addIntersection( aIp, PointCount(), is );
                }
            }
            else if( p )
            {
                is.p = *p;
                is.is_corner_our = false;
                is.is_corner_their = false;

                if( p == a.A )
                {
                    is.is_corner_our = true;
                }

                if( p == a.B )
                {
                    is.is_corner_our = true;
                    is.index_our++;
                }

                if( p == b.A )
                {
                    is.is_corner_their = true;
                }

                if( p == b.B )
                {
                    is.is_corner_their = true;
                    is.index_their++;
                }

                addIntersection( aIp, PointCount(), is );
            }
        }
    }

    return aIp.size();
}


int SHAPE_LINE_CHAIN::PathLength( const VECTOR2I& aP, int aIndex ) const
{
    int sum = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG seg = CSegment( i );
        bool indexMatch = true;

        if( aIndex >= 0 )
        {
            if( aIndex == SegmentCount() )
            {
                indexMatch = ( i == SegmentCount() - 1 );
            }
            else
            {
                indexMatch = ( i == aIndex );
            }
        }

        if( indexMatch )
        {
            sum += ( aP - seg.A ).EuclideanNorm();
            return sum;
        }
        else
            sum += seg.Length();
    }

    return -1;
}


bool SHAPE_LINE_CHAIN_BASE::PointInside( const VECTOR2I& aPt, int aAccuracy,
                                         bool aUseBBoxCache ) const
{
    /*
     * Don't check the bounding box unless it's cached.  Building it is about the same speed as
     * the rigorous test below and so just slows things down by doing potentially two tests.
     */
    if( aUseBBoxCache && GetCachedBBox() && !GetCachedBBox()->Contains( aPt ) )
        return false;

    if( !IsClosed() || GetPointCount() < 3 )
        return false;

    /*
     * To check for interior points, we draw a line in the positive x direction from
     * the point.  If it intersects an even number of segments, the point is outside the
     * line chain (it had to first enter and then exit).  Otherwise, it is inside the chain.
     *
     * Note: slope might be denormal here in the case of a horizontal line but we require our
     * y to move from above to below the point (or vice versa)
     *
     * Note: we open-code CPoint() here so that we don't end up calculating the size of the
     * vector number-of-points times.  This has a non-trivial impact on zone fill times.
     */
    int  pointCount = GetPointCount();
    bool inside = false;

    for( int i = 0; i < pointCount; )
    {
        const VECTOR2I p1 = GetPoint( i++ );
        const VECTOR2I p2 = GetPoint( i == pointCount ? 0 : i );
        const VECTOR2I diff = p2 - p1;

        if( diff.y == 0 )
            continue;

        const int d = rescale( diff.x, ( aPt.y - p1.y ), diff.y );

        if( ( ( p1.y >= aPt.y ) != ( p2.y >= aPt.y ) ) && ( aPt.x - p1.x < d ) )
            inside = !inside;
    }

    // If accuracy is <= 1 (nm) then we skip the accuracy test for performance.  Otherwise
    // we use "OnEdge(accuracy)" as a proxy for "Inside(accuracy)".
    if( aAccuracy <= 1 )
        return inside;
    else
        return inside || PointOnEdge( aPt, aAccuracy );
}


bool SHAPE_LINE_CHAIN_BASE::PointOnEdge( const VECTOR2I& aPt, int aAccuracy ) const
{
	return EdgeContainingPoint( aPt, aAccuracy ) >= 0;
}


int SHAPE_LINE_CHAIN_BASE::EdgeContainingPoint( const VECTOR2I& aPt, int aAccuracy ) const
{
    const int     threshold = aAccuracy + 1;
    const int64_t thresholdSq = int64_t( threshold ) * threshold;
    const size_t  pointCount = GetPointCount();

    if( !pointCount )
    {
        return -1;
    }
    else if( pointCount == 1 )
    {
        SEG::ecoord distSq = GetPoint( 0 ).SquaredDistance( aPt );
        return distSq <= thresholdSq ? 0 : -1;
    }

    const size_t segCount = GetSegmentCount();

    for( size_t i = 0; i < segCount; i++ )
    {
        const SEG s = GetSegment( i );

        if( s.A == aPt || s.B == aPt )
            return i;

        if( s.SquaredDistance( aPt ) <= thresholdSq )
            return i;
    }

    return -1;
}


bool SHAPE_LINE_CHAIN::CheckClearance( const VECTOR2I& aP, const int aDist ) const
{
    if( !PointCount() )
        return false;
    else if( PointCount() == 1 )
        return m_points[0] == aP;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG s = CSegment( i );

        if( s.A == aP || s.B == aP )
            return true;

        if( s.Distance( aP ) <= aDist )
            return true;
    }

    return false;
}


const std::optional<SHAPE_LINE_CHAIN::INTERSECTION> SHAPE_LINE_CHAIN::SelfIntersecting() const
{
    const size_t     segCount = SegmentCount();
    std::vector<SEG> segments( segCount );

    for( size_t s = 0; s < segCount; s++ )
        segments[s] = CSegment( s );

    for( size_t s1 = 0; s1 < segCount; s1++ )
    {
        const SEG& cs1 = segments[s1];

        for( size_t s2 = s1 + 1; s2 < segCount; s2++ )
        {
            const SEG&     cs2 = segments[s2];
            const VECTOR2I s2a = cs2.A, s2b = cs2.B;

            if( s1 + 1 != s2 && cs1.Contains( s2a ) )
            {
                INTERSECTION is;
                is.index_our = s1;
                is.index_their = s2;
                is.p = s2a;
                return is;
            }
            else if( cs1.Contains( s2b ) &&
                     // for closed polylines, the ending point of the
                     // last segment == starting point of the first segment
                     // this is a normal case, not self intersecting case
                     !( IsClosed() && s1 == 0 && s2 == segCount - 1 ) )
            {
                INTERSECTION is;
                is.index_our = s1;
                is.index_their = s2;
                is.p = s2b;
                return is;
            }
            else
            {
                OPT_VECTOR2I p = cs1.Intersect( cs2, true );

                if( p )
                {
                    INTERSECTION is;
                    is.index_our = s1;
                    is.index_their = s2;
                    is.p = *p;
                    return is;
                }
            }
        }
    }

    return std::optional<SHAPE_LINE_CHAIN::INTERSECTION>();
}


struct SHAPE_KEY
{
    SHAPE_KEY( int aFirstIdx, int aArcIdx, const BOX2I_MINMAX& aBBox ) :
            m_FirstIdx( aFirstIdx ), m_ArcIdx( aArcIdx ), m_BBox( aBBox )
    {
    }

    int          m_FirstIdx;
    int          m_ArcIdx;
    BOX2I_MINMAX m_BBox;
};


const std::optional<SHAPE_LINE_CHAIN::INTERSECTION>
SHAPE_LINE_CHAIN::SelfIntersectingWithArcs() const
{
    auto pointsClose = []( const VECTOR2I& aPt1, const VECTOR2I& aPt2 ) -> bool
    {
        return ( VECTOR2D( aPt1 ) - aPt2 ).SquaredEuclideanNorm() <= 2.0;
    };

    auto collideArcSeg = [&pointsClose]( const SHAPE_ARC& aArc, const SEG& aSeg, int aClearance = 0,
                                         VECTOR2I* aLocation = nullptr )
    {
        VECTOR2I center = aArc.GetCenter();
        CIRCLE   circle( center, aArc.GetRadius() );

        std::vector<VECTOR2I> candidatePts = circle.Intersect( aSeg );

        for( const VECTOR2I& candidate : candidatePts )
        {
            // Skip shared points
            if( aArc.GetP1() == aSeg.A && pointsClose( candidate, aSeg.A ) )
                continue;

            if( aSeg.B == aArc.GetP0() && pointsClose( candidate, aSeg.B ) )
                continue;

            bool collides = aArc.Collide( candidate, aClearance, nullptr, aLocation );

            if( collides )
                return true;
        }

        return false;
    };

    auto collideArcArc = [&pointsClose]( const SHAPE_ARC& aArc1, const SHAPE_ARC& aArc2,
                                         VECTOR2I* aLocation = nullptr )
    {
        std::vector<VECTOR2I> candidatePts;

        aArc1.Intersect( aArc2, &candidatePts );

        for( const VECTOR2I& candidate : candidatePts )
        {
            // Skip shared points
            if( aArc1.GetP1() == aArc2.GetP0() && pointsClose( candidate, aArc1.GetP1() ) )
                continue;

            if( aArc2.GetP1() == aArc1.GetP0() && pointsClose( candidate, aArc2.GetP1() ) )
                continue;

            if( aLocation )
                *aLocation = candidate;

            return true;
        }

        return false;
    };

    auto collideSegSeg = [this]( int s1, int s2, INTERSECTION& is )
    {
        SEG seg1 = CSegment( s1 );
        SEG seg2 = CSegment( s2 );

        const VECTOR2I s2a = seg2.A, s2b = seg2.B;

        if( s1 + 1 != s2 && seg1.Contains( s2a ) )
        {
            is.index_our = s1;
            is.index_their = s2;
            is.p = s2a;
            return true;
        }
        else if( seg1.Contains( s2b ) &&
                 // for closed polylines, the ending point of the
                 // last segment == starting point of the first segment
                 // this is a normal case, not self intersecting case
                 !( IsClosed() && s1 == 0 && s2 == SegmentCount() - 1 ) )
        {
            is.index_our = s1;
            is.index_their = s2;
            is.p = s2b;
            return true;
        }
        else
        {
            OPT_VECTOR2I p = seg1.Intersect( seg2, true );

            if( p )
            {
                is.index_our = s1;
                is.index_their = s2;
                is.p = *p;
                return true;
            }
        }

        return false;
    };

    INTERSECTION is;

    std::vector<SHAPE_KEY> shapeCache;
    for( int si = 0; si != -1; si = NextShape( si ) )
    {
        int arci = ArcIndex( si );

        shapeCache.emplace_back( si, arci,
                                 arci == -1 ? BOX2I_MINMAX( CSegment( si ) )
                                            : BOX2I_MINMAX( Arc( arci ) ) );
    }

    for( size_t sk1 = 0; sk1 < shapeCache.size(); sk1++ )
    {
        for( size_t sk2 = sk1 + 1; sk2 < shapeCache.size(); sk2++ )
        {
            VECTOR2I         loc;
            const SHAPE_KEY& k1 = shapeCache[sk1];
            const SHAPE_KEY& k2 = shapeCache[sk2];

            if( !k1.m_BBox.Intersects( k2.m_BBox ) )
                continue;

            if( k1.m_ArcIdx == -1 && k2.m_ArcIdx == -1 )
            {
                if( collideSegSeg( k1.m_FirstIdx, k2.m_FirstIdx, is ) )
                {
                    return is;
                }
            }
            else if( k1.m_ArcIdx != -1 && k2.m_ArcIdx == -1 )
            {
                if( collideArcSeg( Arc( k1.m_ArcIdx ), CSegment( k2.m_FirstIdx ), 0, &loc ) )
                {
                    is.index_our = k1.m_FirstIdx;
                    is.index_their = k2.m_FirstIdx;
                    is.p = loc;
                    return is;
                }
            }
            else if( k1.m_ArcIdx == -1 && k2.m_ArcIdx != -1 )
            {
                if( collideArcSeg( Arc( k2.m_ArcIdx ), CSegment( k1.m_FirstIdx ), 0, &loc ) )
                {
                    is.index_our = k1.m_FirstIdx;
                    is.index_their = k2.m_FirstIdx;
                    is.p = loc;
                    return is;
                }
            }
            else if( k1.m_ArcIdx != -1 && k2.m_ArcIdx != -1 )
            {
                if( collideArcArc( Arc( k1.m_ArcIdx ), Arc( k2.m_ArcIdx ), &loc ) )
                {
                    is.index_our = k1.m_FirstIdx;
                    is.index_their = k2.m_FirstIdx;
                    is.p = loc;
                    return is;
                }
            }
        }
    }

    return std::optional<SHAPE_LINE_CHAIN::INTERSECTION>();
}


const VECTOR2I SHAPE_LINE_CHAIN::NearestPoint( const VECTOR2I& aP,
                                               bool aAllowInternalShapePoints ) const
{
    if( PointCount() == 0 )
    {
        // The only right answer here is "don't crash".
        return { 0, 0 };
    }

    int min_d = std::numeric_limits<int>::max();
    int nearest = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        int d = CSegment( i ).Distance( aP );

        if( d < min_d )
        {
            min_d = d;
            nearest = i;
        }
    }

    if( !aAllowInternalShapePoints )
    {
        //Snap to arc end points if the closest found segment is part of an arc segment
        if( nearest > 0 && nearest < PointCount() && IsArcSegment( nearest ) )
        {
            VECTOR2I ptToSegStart = CSegment( nearest ).A - aP;
            VECTOR2I ptToSegEnd = CSegment( nearest ).B - aP;

            if( ptToSegStart.EuclideanNorm() > ptToSegEnd.EuclideanNorm() )
                nearest++;

            // Is this the start or end of an arc?  If so, return it directly
            if( IsArcStart( nearest ) || IsArcEnd( nearest ) )
            {
                return m_points[nearest];
            }
            else
            {
                const SHAPE_ARC& nearestArc = Arc( ArcIndex( nearest ) );
                VECTOR2I         ptToArcStart = nearestArc.GetP0() - aP;
                VECTOR2I         ptToArcEnd = nearestArc.GetP1() - aP;

                if( ptToArcStart.EuclideanNorm() > ptToArcEnd.EuclideanNorm() )
                    return nearestArc.GetP1();
                else
                    return nearestArc.GetP0();
            }

        }
    }

    return CSegment( nearest ).NearestPoint( aP );
}


const VECTOR2I SHAPE_LINE_CHAIN::NearestPoint( const SEG& aSeg, int& dist ) const
{
    if( PointCount() == 0 )
    {
        // The only right answer here is "don't crash".
        return { 0, 0 };
    }

    int nearest = 0;

    dist = std::numeric_limits<int>::max();

    for( int i = 0; i < PointCount(); i++ )
    {
        int d = aSeg.LineDistance( CPoint( i ) );

        if( d < dist )
        {
            dist = d;
            nearest = i;
        }
    }

    return CPoint( nearest );
}


int SHAPE_LINE_CHAIN::NearestSegment( const VECTOR2I& aP ) const
{
    int min_d = std::numeric_limits<int>::max();
    int nearest = 0;

    for( int i = 0; i < SegmentCount(); i++ )
    {
        int d = CSegment( i ).Distance( aP );

        if( d < min_d )
        {
            min_d = d;
            nearest = i;
        }
    }

    return nearest;
}


const std::string SHAPE_LINE_CHAIN::Format( bool aCplusPlus ) const
{
    std::stringstream ss;

    ss << "SHAPE_LINE_CHAIN( { ";

    for( int i = 0; i < PointCount(); i++ )
    {
        ss << "VECTOR2I( " << m_points[i].x << ", " << m_points[i].y << ")";

        if( i != PointCount() -1 )
            ss << ", ";
    }

    ss << "}, " << ( m_closed ? "true" : "false" );
    ss << " );";

    return ss.str();

   /* fixme: arcs
    for( size_t i = 0; i < m_arcs.size(); i++ )
        ss << m_arcs[i].GetCenter().x << " " << m_arcs[i].GetCenter().y << " "
        << m_arcs[i].GetP0().x << " " << m_arcs[i].GetP0().y << " "
        << m_arcs[i].GetCentralAngle().AsDegrees();

    return ss.str();*/
}


bool SHAPE_LINE_CHAIN::CompareGeometry( const SHAPE_LINE_CHAIN& aOther,
                                        bool                    aCyclicalCompare,
                                        int aEpsilon ) const
{
    SHAPE_LINE_CHAIN a( *this ), b( aOther );
    a.Simplify();
    b.Simplify();

    if( a.m_points.size() != b.m_points.size() )
        return false;

    if( aCyclicalCompare )
    {
        std::vector<VECTOR2I> aVerts = a.m_points;
        std::vector<VECTOR2I> bVerts = b.m_points;

        auto centroid = []( const std::vector<VECTOR2I>& pts )
        {
            double sx = 0.0, sy = 0.0;
            for( const auto& p : pts )
            {
                sx += p.x;
                sy += p.y;
            }
            return std::pair<double, double>( sx / pts.size(), sy / pts.size() );
        };

        auto aC = centroid( aVerts );
        auto bC = centroid( bVerts );

        auto angleCmp =
                []( const std::pair<double, double>& c, const VECTOR2I& p1, const VECTOR2I& p2 )
        {
            double a1 = atan2( p1.y - c.second, p1.x - c.first );
            double a2 = atan2( p2.y - c.second, p2.x - c.first );
            return a1 < a2;
        };

        // sort by angle around centroid so that cyclic vertex order doesn't matter
        std::sort( aVerts.begin(), aVerts.end(),
                   [&]( const VECTOR2I& p1, const VECTOR2I& p2 )
                   {
                       return angleCmp( aC, p1, p2 );
                   } );

        std::sort( bVerts.begin(), bVerts.end(),
                   [&]( const VECTOR2I& p1, const VECTOR2I& p2 )
                   {
                       return angleCmp( bC, p1, p2 );
                   } );

        for( size_t i = 0; i < aVerts.size(); i++ )
        {
            if( abs( aVerts[i].x - bVerts[i].x ) > aEpsilon
                || abs( aVerts[i].y - bVerts[i].y ) > aEpsilon )
                return false;
        }

    }
    else
    {
        for( int i = 0; i < a.PointCount(); i++ )
        {
            if( abs( a.CPoint( i ).x - b.CPoint( i ).x ) > aEpsilon
                || abs( a.CPoint( i ).y - b.CPoint( i ).y ) > aEpsilon )
                return false;
        }
    }



    return true;
}


bool SHAPE_LINE_CHAIN::Intersects( const SHAPE_LINE_CHAIN& aChain ) const
{
    INTERSECTIONS dummy;
    return Intersect( aChain, dummy ) != 0;
}


SHAPE* SHAPE_LINE_CHAIN::Clone() const
{
    return new SHAPE_LINE_CHAIN( *this );
}


bool SHAPE_LINE_CHAIN::Parse( std::stringstream& aStream )
{
    size_t n_pts;
    size_t n_arcs;

    m_points.clear();
    aStream >> n_pts;

    // Rough sanity check, just make sure the loop bounds aren't absolutely outlandish
    if( n_pts > aStream.str().size() )
        return false;

    aStream >> m_closed;
    aStream >> n_arcs;

    if( n_arcs > aStream.str().size() )
        return false;

    for( size_t i = 0; i < n_pts; i++ )
    {
        int x, y;
        ssize_t ind;
        aStream >> x;
        aStream >> y;
        m_points.emplace_back( x, y );

        aStream >> ind;
        m_shapes.emplace_back( std::make_pair( ind, SHAPE_IS_PT ) );
    }

    for( size_t i = 0; i < n_arcs; i++ )
    {
        VECTOR2I p0, pc;
        double angle;

        aStream >> pc.x;
        aStream >> pc.y;
        aStream >> p0.x;
        aStream >> p0.y;
        aStream >> angle;

        m_arcs.emplace_back( pc, p0, EDA_ANGLE( angle, DEGREES_T ) );
    }

    return true;
}


const VECTOR2I SHAPE_LINE_CHAIN::PointAlong( int aPathLength ) const
{
    int total = 0;

    if( aPathLength == 0 )
        return CPoint( 0 );

    for( int i = 0; i < SegmentCount(); i++ )
    {
        const SEG& s = CSegment( i );
        int l = s.Length();

        if( total + l >= aPathLength )
        {
            VECTOR2I d( s.B - s.A );
            return s.A + d.Resize( aPathLength - total );
        }

        total += l;
    }

    return CLastPoint();
}


double SHAPE_LINE_CHAIN::Area( bool aAbsolute ) const
{
    // see https://www.mathopenref.com/coordpolygonarea2.html

    if( !m_closed )
        return 0.0;

    double area = 0.0;
    int size = m_points.size();

    for( int i = 0, j = size - 1; i < size; ++i )
    {
        area += ( (double) m_points[j].x + m_points[i].x ) *
                ( (double) m_points[j].y - m_points[i].y );
        j = i;
    }

    if( aAbsolute )
        return std::fabs( area * 0.5 ); // The result would be negative if points are anti-clockwise
    else
        return -area * 0.5; // The result would be negative if points are anti-clockwise
}


void SHAPE_LINE_CHAIN::RemoveDuplicatePoints()
{
    std::vector<VECTOR2I> pts_unique;
    std::vector<std::pair<ssize_t, ssize_t>> shapes_unique;

    // Always try to keep at least 2 points otherwise, we're not really a line
    if( PointCount() < 3 )
    {
        return;
    }
    else if( PointCount() == 3 )
    {
        if( m_points[0] == m_points[1] )
            Remove( 1 );

        return;
    }

    int i = 0;

    while( i < PointCount() )
    {
        int j = i + 1;

        // We can eliminate duplicate vertices as long as they are part of the same shape, OR if
        // one of them is part of a shape and one is not.
        while( j < PointCount() && m_points[i] == m_points[j] &&
               ( m_shapes[i] == m_shapes[j] ||
                 m_shapes[i] == SHAPES_ARE_PT ||
                 m_shapes[j] == SHAPES_ARE_PT ) )
        {
            j++;
        }

        std::pair<ssize_t,ssize_t> shapeToKeep = m_shapes[i];

        if( shapeToKeep == SHAPES_ARE_PT )
            shapeToKeep = m_shapes[j - 1];

        assert( shapeToKeep.first < static_cast<int>( m_arcs.size() ) );
        assert( shapeToKeep.second < static_cast<int>( m_arcs.size() ) );

        pts_unique.push_back( CPoint( i ) );
        shapes_unique.push_back( shapeToKeep );

        i = j;
    }

    m_points.clear();
    m_shapes.clear();

    for( size_t ii = 0; ii < pts_unique.size(); ++ii )
    {
        const VECTOR2I p0 = pts_unique[ii];

        m_points.push_back( p0 );
        m_shapes.push_back( shapes_unique[ii] );
    }
}



void SHAPE_LINE_CHAIN::Simplify( int aTolerance )
{
    if( PointCount() < 3 )
        return;

    std::vector<VECTOR2I> new_points;
    std::vector<std::pair<ssize_t, ssize_t>> new_shapes;

    new_points.reserve( m_points.size() );
    new_shapes.reserve( m_shapes.size() );

    for( size_t start_idx = 0; start_idx < m_points.size(); )
    {
        new_points.push_back( m_points[start_idx] );
        new_shapes.push_back( m_shapes[start_idx] );

        // If the line is not closed, we need at least 3 points before simplifying
        if( !m_closed && start_idx == m_points.size() - 2 )
            break;

        // Initialize the end index to be two points ahead of start
        size_t end_idx = ( start_idx + 2 ) % m_points.size();
        bool can_simplify = true;

        while( can_simplify && end_idx != start_idx && ( end_idx > start_idx || m_closed ) )
        {
            // Test all points between start_idx and end_idx
            for( size_t test_idx = ( start_idx + 1 ) % m_points.size();
                 test_idx != end_idx;
                 test_idx = ( test_idx + 1 ) % m_points.size() )
            {
                // Check if all points are regular points (not arcs)
                if( m_shapes[start_idx].first != SHAPE_IS_PT ||
                    m_shapes[test_idx].first != SHAPE_IS_PT ||
                    m_shapes[end_idx].first != SHAPE_IS_PT )
                {
                    can_simplify = false;
                    break;
                }

                // Test if the point is within the allowed error
                if( !TestSegmentHit( m_points[test_idx], m_points[start_idx], m_points[end_idx], aTolerance ) )
                {
                    can_simplify = false;
                    break;
                }
            }

            if( can_simplify )
            {
                // If we can simplify, move end_idx one further
                end_idx = ( end_idx + 1 ) % m_points.size();
            }
        }

        // If we couldn't simplify at all, move to the next point
        if( end_idx == ( start_idx + 2 ) % m_points.size() )
        {
            ++start_idx;
        }
        else
        {
            // Otherwise, jump to the last point we could include in the simplification
            size_t new_start_idx = ( end_idx + m_points.size() - 1 ) % m_points.size();

            // If we looped all the way around, we're done
            if( new_start_idx <= start_idx )
                break;

            start_idx = new_start_idx;
        }
    }

    // If we have only one point, we need to add a second point to make a line
    if( new_points.size() == 1 )
    {
        new_points.push_back( m_points.back() );
        new_shapes.push_back( m_shapes.back() );
    }

    // If we are not closed, then the start and end points of the original line need to
    // be the start and end points of the new line.
    if( !m_closed && m_points.back() != new_points.back() )
    {
        new_points.push_back( m_points.back() );
        new_shapes.push_back( m_shapes.back() );
    }

    m_points.clear();
    m_shapes.clear();
    m_points = std::move( new_points );
    m_shapes = std::move( new_shapes );
}


void SHAPE_LINE_CHAIN::Split( const VECTOR2I& aStart, const VECTOR2I& aEnd, SHAPE_LINE_CHAIN& aPre,
                              SHAPE_LINE_CHAIN& aMid, SHAPE_LINE_CHAIN& aPost ) const
{
    VECTOR2I cp( aEnd );

    VECTOR2I n = NearestPoint( cp, false );
    VECTOR2I m = NearestPoint( aStart, false );

    SHAPE_LINE_CHAIN l( *this );
    l.Split( n, true );
    l.Split( m, true );

    int i_start = l.Find( m );
    int i_end = l.Find( n );

    if( i_start > i_end )
    {
        l = l.Reverse();
        i_start = l.Find( m );
        i_end = l.Find( n );
    }

    aPre = l.Slice( 0, i_start );
    aPost = l.Slice( i_end, -1 );
    aMid = l.Slice( i_start, i_end );
}



SHAPE_LINE_CHAIN& SHAPE_LINE_CHAIN::Simplify2( bool aRemoveColinear )
{
    std::vector<VECTOR2I> pts_unique;
    std::vector<std::pair<ssize_t, ssize_t>> shapes_unique;

    // Always try to keep at least 2 points otherwise, we're not really a line
    if( PointCount() < 3 )
    {
        return *this;
    }
    else if( PointCount() == 3 )
    {
        if( m_points[0] == m_points[1] )
            Remove( 1 );

        return *this;
    }

    int i = 0;
    int np = PointCount();

    // stage 1: eliminate duplicate vertices
    while( i < np )
    {
        int j = i + 1;

        // We can eliminate duplicate vertices as long as they are part of the same shape, OR if
        // one of them is part of a shape and one is not.
        while( j < np && m_points[i] == m_points[j] &&
               ( m_shapes[i] == m_shapes[j] ||
                 m_shapes[i] == SHAPES_ARE_PT ||
                 m_shapes[j] == SHAPES_ARE_PT ) )
        {
            j++;
        }

        std::pair<ssize_t,ssize_t> shapeToKeep = m_shapes[i];

        if( shapeToKeep == SHAPES_ARE_PT )
            shapeToKeep = m_shapes[j - 1];

        assert( shapeToKeep.first < static_cast<int>( m_arcs.size() ) );
        assert( shapeToKeep.second < static_cast<int>( m_arcs.size() ) );

        pts_unique.push_back( CPoint( i ) );
        shapes_unique.push_back( shapeToKeep );

        i = j;
    }

    m_points.clear();
    m_shapes.clear();
    np = pts_unique.size();

    i = 0;

    // stage 2: eliminate colinear segments
    while( i < np - 2 )
    {
        const VECTOR2I p0 = pts_unique[i];
        int n = i;

        if( aRemoveColinear && shapes_unique[i] == SHAPES_ARE_PT
            && shapes_unique[i + 1] == SHAPES_ARE_PT )
        {
            while( n < np - 2
                    && ( SEG( p0, pts_unique[n + 2] ).LineDistance( pts_unique[n + 1] ) <= 1
                      || SEG( p0, pts_unique[n + 2] ).Collinear( SEG( p0, pts_unique[n + 1] ) ) ) )
                n++;
        }

        m_points.push_back( p0 );
        m_shapes.push_back( shapes_unique[i] );

        if( n > i )
            i = n;

        if( n == np - 2 )
        {
            m_points.push_back( pts_unique[np - 1] );
            m_shapes.push_back( shapes_unique[np - 1] );
            return *this;
        }

        i++;
    }

    if( np > 1 )
    {
        m_points.push_back( pts_unique[np - 2] );
        m_shapes.push_back( shapes_unique[np - 2] );
    }

    m_points.push_back( pts_unique[np - 1] );
    m_shapes.push_back( shapes_unique[np - 1] );

    assert( m_points.size() == m_shapes.size() );

    return *this;
}

bool SHAPE_LINE_CHAIN::OffsetLine( int aAmount, CORNER_STRATEGY aCornerStrategy, int aMaxError,
                                   SHAPE_LINE_CHAIN& aLeft, SHAPE_LINE_CHAIN& aRight,
                                   bool aSimplify ) const
{
    if( PointCount() < 2 )
        return false;

    SHAPE_POLY_SET poly;
    poly.OffsetLineChain( *this, aAmount, aCornerStrategy, aMaxError, aSimplify );

    if( poly.OutlineCount() != 1 )
        return false;

    if( poly.COutline( 0 ).PointCount() < 3 )
        return false;

    if( poly.HasHoles() )
        return false;

    SHAPE_LINE_CHAIN outline = poly.COutline( 0 );

    wxASSERT( outline.IsClosed() );

    const VECTOR2I& start = CPoint( 0 );
    const VECTOR2I& end = CLastPoint();

    outline.Split( start, true );
    outline.Split( end, true );

    const int idA = outline.Find( start );
    const int idB = outline.Find( end );

    if( idA == -1 || idB == -1 )
        return false;

    aLeft.Clear();
    aRight.Clear();

    for( int i = idA;; )
    {
        aLeft.Append( outline.CPoint( i ) );

        i = ( i + 1 ) % outline.PointCount();

        if( i == idB )
        {
            aLeft.Append( outline.CPoint( i ) );
            break;
        }

        if( i == idA )
            return false;
    }

    if( aLeft.PointCount() < 2 )
        return false;

    for( int i = idB;; )
    {
        aRight.Append( outline.CPoint( i ) );

        i = ( i + 1 ) % outline.PointCount();

        if( i == idA )
        {
            aRight.Append( outline.CPoint( i ) );
            break;
        }

        if( i == idB )
            return false;
    }

    if( aRight.PointCount() < 2 )
        return false;

    if( aLeft.CPoint( 0 ) != start )
    {
        aLeft = aLeft.Reverse();
        wxASSERT( aLeft.CPoint( 0 ) == start );
    }

    if( aRight.CPoint( 0 ) != start )
    {
        aRight = aRight.Reverse();
        wxASSERT( aRight.CPoint( 0 ) == start );
    }

    SEG base( CPoint( 0 ), CPoint( 1 ) );
    int sideLeft = base.Side( aLeft.CPoint( 1 ) );
    int sideRight = base.Side( aRight.CPoint( 1 ) );

    if( sideLeft == 0 || sideRight == 0 )
        return false;

    if( sideLeft == sideRight )
        return false;

    if( sideLeft > 0 && sideRight < 0 )
        std::swap( aLeft, aRight );

    if( aLeft.PointCount() < 4 )
        return false;

    if( aRight.PointCount() < 4 )
        return false;

    aLeft.Remove( 0 );
    aLeft.Remove( aLeft.PointCount() - 1 );

    aRight.Remove( 0 );
    aRight.Remove( aRight.PointCount() - 1 );

    return true;
}


void SHAPE_LINE_CHAIN::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                                           ERROR_LOC aErrorLoc ) const
{
    aBuffer.AddOutline( *this );
}


SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::POINT_INSIDE_TRACKER( const VECTOR2I& aPoint ) :
    m_point( aPoint ),
    m_finished( false ),
    m_state( 0 ),
    m_count( 0 )
{
}


bool SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::processVertex(
        const VECTOR2I& ip, const VECTOR2I& ipNext )
{
    if( ipNext.y == m_point.y )
    {
        if( ( ipNext.x == m_point.x )
                || ( ip.y == m_point.y && ( ( ipNext.x > m_point.x ) == ( ip.x < m_point.x ) ) ) )
        {
            m_finished = true;
            m_state    = -1;
            return false;
        }
    }

    if( ( ip.y < m_point.y ) != ( ipNext.y < m_point.y ) )
    {
        if( ip.x >= m_point.x )
        {
            if( ipNext.x > m_point.x )
            {
                m_state = 1 - m_state;
            }
            else
            {
                double d = (double) ( ip.x - m_point.x ) * ( ipNext.y - m_point.y )
                           - (double) ( ipNext.x - m_point.x ) * ( ip.y - m_point.y );

                if( !d )
                {
                    m_finished = true;
                    m_state    = -1;
                    return false;
                }

                if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                    m_state = 1 - m_state;
            }
        }
        else
        {
            if( ipNext.x > m_point.x )
            {
                double d = (double) ( ip.x - m_point.x ) * ( ipNext.y - m_point.y )
                           - (double) ( ipNext.x - m_point.x ) * ( ip.y - m_point.y );

                if( !d )
                {
                    m_finished = true;
                    m_state    = -1;
                    return false;
                }

                if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                    m_state = 1 - m_state;
            }
        }
    }

    return true;
}


void SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::AddPolyline( const SHAPE_LINE_CHAIN& aPolyline )
{
    if( !m_count )
    {
        m_lastPoint = aPolyline.CPoint( 0 );
        m_firstPoint = aPolyline.CPoint( 0 );
    }

    m_count += aPolyline.PointCount();

    for( int i = 1; i < aPolyline.PointCount(); i++ )
    {
        auto p = aPolyline.CPoint( i );

        if( !processVertex( m_lastPoint, p ) )
            return;

        m_lastPoint = p;
    }

}


bool SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER::IsInside()
{
    processVertex( m_lastPoint, m_firstPoint );
    return m_state > 0;
}


bool SHAPE_LINE_CHAIN::IsSharedPt( size_t aIndex ) const
{
    return aIndex < m_shapes.size()
           && m_shapes[aIndex].first != SHAPE_IS_PT
           && m_shapes[aIndex].second != SHAPE_IS_PT;
}


bool SHAPE_LINE_CHAIN::IsPtOnArc( size_t aPtIndex ) const
{
    return aPtIndex < m_shapes.size() && m_shapes[aPtIndex] != SHAPES_ARE_PT;
}


bool SHAPE_LINE_CHAIN::IsArcSegment( size_t aSegment ) const
{
    /*
     * A segment is part of an arc except in the special case of two arcs next to each other
     * but without a shared vertex.  Here there is a segment between the end of the first arc
     * and the start of the second arc.
     */
    size_t nextIdx = aSegment + 1;

    if( nextIdx > m_shapes.size() - 1 )
    {
        if( nextIdx == m_shapes.size() && m_closed && IsSharedPt( 0 ) )
            nextIdx = 0; // segment between end point and first point
        else
            return false;
    }

    return ( IsPtOnArc( aSegment )
                 && ( ArcIndex( aSegment ) == m_shapes[nextIdx].first ) );
}


bool SHAPE_LINE_CHAIN::IsArcStart( size_t aIndex ) const
{
    if( !IsArcSegment( aIndex ) ) // also does bound checking
        return false;

    if( IsSharedPt( aIndex ) )
        return true;

    const SHAPE_ARC& arc = Arc( ArcIndex( aIndex ) );

    return arc.GetP0() == m_points[aIndex];
}


bool SHAPE_LINE_CHAIN::IsArcEnd( size_t aIndex ) const
{
    size_t prevIndex = aIndex - 1;

    if( aIndex == 0 )
        prevIndex = m_points.size() - 1;
    else if( aIndex > m_points.size() -1 )
        return false; // invalid index requested

    if( !IsArcSegment( prevIndex ) )
        return false;

    if( IsSharedPt( aIndex ) )
        return true;

    const SHAPE_ARC& arc = Arc( ArcIndex( aIndex ) );

    return arc.GetP1() == m_points[aIndex];
}
