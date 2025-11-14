/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/kicad_algo.h>
#include <geometry/geometry_utils.h>
#include <geometry/seg.h>               // for SEG
#include <geometry/shape_arc.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>


std::ostream& operator<<( std::ostream& aStream, const SHAPE_ARC& aArc )
{
    aStream << "Arc( P0=" << aArc.GetP0() << " P1=" << aArc.GetP1() << " Mid=" << aArc.GetArcMid()
            << " Width=" << aArc.GetWidth() << " )";
    return aStream;
}


SHAPE_ARC::SHAPE_ARC( const VECTOR2I& aArcCenter, const VECTOR2I& aArcStartPoint,
                      const EDA_ANGLE& aCenterAngle, int aWidth ) :
        SHAPE( SH_ARC ),
        m_width( aWidth )
{
    m_start = aArcStartPoint;

    VECTOR2D mid = aArcStartPoint;
    VECTOR2D end = aArcStartPoint;
    VECTOR2D center = aArcCenter;

    RotatePoint( mid, center, -aCenterAngle / 2.0 );
    RotatePoint( end, center, -aCenterAngle );

    m_mid = KiROUND( mid );
    m_end = KiROUND( end );

    update_values();
}


SHAPE_ARC::SHAPE_ARC( const VECTOR2I& aArcStart, const VECTOR2I& aArcMid,
                      const VECTOR2I& aArcEnd, int aWidth ) :
        SHAPE( SH_ARC ),
        m_start( aArcStart ),
        m_mid( aArcMid ),
        m_end( aArcEnd ),
        m_width( aWidth )
{
    update_values();
}


SHAPE_ARC::SHAPE_ARC( const SEG& aSegmentA, const SEG& aSegmentB, int aRadius, int aWidth ) :
        SHAPE( SH_ARC )
{
    m_width = aWidth;

    /*
     * Construct an arc that is tangent to two segments with a given radius.
     *
     *               p
     *                A
     *             A   \
     *            /     \
     *           /  . .  \ segB
     *          /.       .\
     *   segA  /     c     \
     *        /             B
     *       /
     *      /
     *     B
     *
     *
     * segA is the fist segment (with its points A and B)
     * segB is the second segment (with its points A and B)
     * p is the point at which segA and segB would intersect if they were projected
     * c is the centre of the arc to be constructed
     * rad is the radius of the arc to be constructed
     *
     * We can create two vectors, between point p and segA /segB
     *    pToA = p - segA.B   //< note that segA.A would also be valid as it is colinear
     *    pToB = p - segB.B   //< note that segB.A would also be valid as it is colinear
     *
     * Let the angle formed by segA and segB be called 'alpha':
     *   alpha = angle( pToA ) - angle( pToB )
     *
     * The distance PC can be computed as
     *   distPC = rad / abs( sin( alpha / 2 ) )
     *
     * The polar angle of the vector PC can be computed as:
     *   anglePC = angle( pToA ) + alpha / 2
     *
     * Therefore:
     *    C.x = P.x + distPC*cos( anglePC )
     *    C.y = P.y + distPC*sin( anglePC )
     */

    OPT_VECTOR2I p = aSegmentA.Intersect( aSegmentB, true, true );

    if( !p || aSegmentA.Length() == 0 || aSegmentB.Length() == 0 )
    {
        // Catch bugs in debug
        wxASSERT_MSG( false, "The input segments do not intersect or one is zero length." );

        // Make a 180 degree arc around aSegmentA in case we end up here in release
        m_start = aSegmentA.A;
        m_end   = aSegmentA.B;
        m_mid   = m_start;

        VECTOR2I arcCenter = aSegmentA.Center();
        RotatePoint( m_mid, arcCenter, ANGLE_90 ); // mid point at 90 degrees
    }
    else
    {
        VECTOR2I pToA = aSegmentA.B - *p;
        VECTOR2I pToB = aSegmentB.B - *p;

        if( pToA.EuclideanNorm() == 0 )
            pToA = aSegmentA.A - *p;

        if( pToB.EuclideanNorm() == 0 )
            pToB = aSegmentB.A - *p;

        EDA_ANGLE pToAangle( pToA );
        EDA_ANGLE pToBangle( pToB );

        EDA_ANGLE alpha = ( pToAangle - pToBangle ).Normalize180();

        double    distPC = (double) aRadius / abs( sin( alpha.AsRadians() / 2 ) );
        EDA_ANGLE angPC  = pToAangle - alpha / 2;
        VECTOR2I  arcCenter;

        arcCenter.x = p->x + KiROUND( distPC * angPC.Cos() );
        arcCenter.y = p->y + KiROUND( distPC * angPC.Sin() );

        // The end points of the arc are the orthogonal projected lines from the line segments
        // to the center of the arc
        m_start = aSegmentA.LineProject( arcCenter );
        m_end = aSegmentB.LineProject( arcCenter );

        //The mid point is rotated start point around center, half the angle of the arc.
        VECTOR2I startVector = m_start - arcCenter;
        VECTOR2I endVector   = m_end - arcCenter;

        EDA_ANGLE startAngle( startVector );
        EDA_ANGLE endAngle( endVector );
        EDA_ANGLE midPointRotAngle = ( startAngle - endAngle ).Normalize180() / 2;

        m_mid = m_start;
        RotatePoint( m_mid, arcCenter, midPointRotAngle );
    }

    update_values();
}


SHAPE_ARC::SHAPE_ARC( const SHAPE_ARC& aOther ) :
        SHAPE( SH_ARC )
{
    m_start = aOther.m_start;
    m_end = aOther.m_end;
    m_mid = aOther.m_mid;
    m_width = aOther.m_width;
    m_bbox = aOther.m_bbox;
    m_center = aOther.m_center;
    m_radius = aOther.m_radius;
}


SHAPE_ARC::SHAPE_ARC( const SHAPE_ARC& aOther, int aWidth ) :
        SHAPE_ARC( aOther )
{
    m_width = aWidth;
}


SHAPE_ARC& SHAPE_ARC::ConstructFromStartEndAngle( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                                  const EDA_ANGLE& aAngle, double aWidth )
{
    m_start = aStart;
    m_mid   = aStart;
    m_end   = aEnd;
    m_width = aWidth;

    VECTOR2I center( CalcArcCenter( aStart, aEnd, aAngle ) );

    RotatePoint( m_mid, center, -aAngle / 2.0 );

    update_values();

    return *this;
}


SHAPE_ARC& SHAPE_ARC::ConstructFromStartEndCenter( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                                   const VECTOR2I& aCenter, bool aClockwise,
                                                   double aWidth )
{
    VECTOR2I startLine = aStart - aCenter;
    VECTOR2I endLine = aEnd - aCenter;

    EDA_ANGLE startAngle( startLine );
    EDA_ANGLE endAngle( endLine );

    startAngle.Normalize();
    endAngle.Normalize();

    EDA_ANGLE angle = endAngle - startAngle;

    if( aClockwise )
        angle = angle.Normalize() - ANGLE_360;
    else
        angle = angle.Normalize();

    m_start = aStart;
    m_end = aEnd;
    m_mid = aStart;

    RotatePoint( m_mid, aCenter, -angle / 2.0 );

    update_values();

    return *this;
}


bool SHAPE_ARC::IsEffectiveLine() const
{
    SEG v1 = SEG( m_start, m_mid );
    SEG v2 = SEG( m_mid, m_end );

    return v1.ApproxCollinear( v2 ) && (v1.B - v1.A).Dot(v2.B - v2.A) > 0;
}


bool SHAPE_ARC::Collide( const SEG& aSeg, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    VECTOR2I center = GetCenter();
    double   radius = VECTOR2D( center - m_start ).EuclideanNorm();
    SHAPE_CIRCLE circle( center, radius );
    ecoord   clearance_sq = SEG::Square( aClearance );

    // Circle or at least an arc with less space remaining than the clearance
    if( GetCentralAngle().AsDegrees() > 180.0
        && ( m_start - m_end ).SquaredEuclideanNorm() < clearance_sq )
    {
        ecoord   a_dist_sq = ( aSeg.A - center ).SquaredEuclideanNorm();
        ecoord   b_dist_sq = ( aSeg.B - center ).SquaredEuclideanNorm();
        ecoord   radius_sq = SEG::Square( radius - aClearance );

        if( a_dist_sq < radius_sq && b_dist_sq < radius_sq )
            return false;


        return circle.Collide( aSeg, aClearance, aActual, aLocation );
    }

    // Possible points of the collision are:
    // 1. Intersetion of the segment with the full circle
    // 2. Closest point on the segment to the center of the circle
    // 3. Closest point on the segment to the end points of the arc
    // 4. End points of the segment

    std::vector<VECTOR2I> candidatePts = circle.GetCircle().Intersect( aSeg );

    candidatePts.push_back( aSeg.NearestPoint( center ) );
    candidatePts.push_back( aSeg.NearestPoint( m_start ) );
    candidatePts.push_back( aSeg.NearestPoint( m_end ) );
    candidatePts.push_back( aSeg.A );
    candidatePts.push_back( aSeg.B );

    bool any_collides = false;

    for( const VECTOR2I& candidate : candidatePts )
    {
        bool collides = Collide( candidate, aClearance, aActual, aLocation );
        any_collides |= collides;

        if( collides && ( !aActual || *aActual == 0 ) )
            return true;
    }

    return any_collides;
}


int SHAPE_ARC::IntersectLine( const SEG& aSeg, std::vector<VECTOR2I>* aIpsBuffer ) const
{
    if( aSeg.A == aSeg.B )      // One point does not define a line....
        return 0;

    CIRCLE circ( GetCenter(), GetRadius() );

    std::vector<VECTOR2I> intersections = circ.IntersectLine( aSeg );

    const size_t originalSize = aIpsBuffer->size();

    for( const VECTOR2I& intersection : intersections )
    {
        if( sliceContainsPoint( intersection ) )
            aIpsBuffer->push_back( intersection );
    }

    return aIpsBuffer->size() - originalSize;
}


int SHAPE_ARC::Intersect( const CIRCLE& aCircle, std::vector<VECTOR2I>* aIpsBuffer ) const
{
    CIRCLE thiscirc( GetCenter(), GetRadius() );

    std::vector<VECTOR2I> intersections = thiscirc.Intersect( aCircle );

    const size_t originalSize = aIpsBuffer->size();

    for( const VECTOR2I& intersection : intersections )
    {
        if( sliceContainsPoint( intersection ) )
            aIpsBuffer->push_back( intersection );
    }

    return aIpsBuffer->size() - originalSize;
}


int SHAPE_ARC::Intersect( const SHAPE_ARC& aArc, std::vector<VECTOR2I>* aIpsBuffer ) const
{
    CIRCLE thiscirc( GetCenter(), GetRadius() );
    CIRCLE othercirc( aArc.GetCenter(), aArc.GetRadius() );

    std::vector<VECTOR2I> intersections = thiscirc.Intersect( othercirc );

    const size_t originalSize = aIpsBuffer->size();

    for( const VECTOR2I& intersection : intersections )
    {
        if( sliceContainsPoint( intersection ) && aArc.sliceContainsPoint( intersection ) )
            aIpsBuffer->push_back( intersection );
    }

    return aIpsBuffer->size() - originalSize;
}


void SHAPE_ARC::update_values()
{
    m_center = CalcArcCenter( m_start, m_mid, m_end );
    m_radius = std::sqrt( ( VECTOR2D( m_start ) - m_center ).SquaredEuclideanNorm() );

    std::vector<VECTOR2I> points;
    // Put start and end points in the point list
    points.push_back( m_start );
    points.push_back( m_end );

    EDA_ANGLE start_angle = GetStartAngle();
    EDA_ANGLE end_angle = start_angle + GetCentralAngle();

    // we always count quadrants clockwise (increasing angle)
    if( start_angle > end_angle )
        std::swap( start_angle, end_angle );

    int quad_angle_start = std::ceil( start_angle.AsDegrees() / 90.0 );
    int quad_angle_end = std::floor( end_angle.AsDegrees() / 90.0 );

    // very large radius means the arc is similar to a segment
    // so do not try to add more points, center cannot be handled
    // Very large is here > INT_MAX/2
    if( m_radius < (double)INT_MAX/2.0 )
    {
        const int radius = KiROUND( m_radius );

        // count through quadrants included in arc
        for( int quad_angle = quad_angle_start; quad_angle <= quad_angle_end; ++quad_angle )
        {
            VECTOR2I quad_pt = m_center;

            switch( quad_angle % 4 )
            {
            case 0:          quad_pt += { radius, 0 };  break;
            case 1: case -3: quad_pt += { 0, radius };  break;
            case 2: case -2: quad_pt += { -radius, 0 }; break;
            case 3: case -1: quad_pt += { 0, -radius }; break;
            default:
                assert( false );
            }

            points.push_back( quad_pt );
        }
    }

    m_bbox.Compute( points );
}


const BOX2I SHAPE_ARC::BBox( int aClearance ) const
{
    BOX2I bbox( m_bbox );

    if( m_width != 0 )
        bbox.Inflate( KiROUND( m_width / 2.0 ) + 1 );

    if( aClearance != 0 )
        bbox.Inflate( aClearance );

    return bbox;
}


VECTOR2I SHAPE_ARC::NearestPoint( const VECTOR2I& aP ) const
{
    const static int s_epsilon = 8;

    CIRCLE   fullCircle( GetCenter(), GetRadius() );
    VECTOR2I nearestPt = fullCircle.NearestPoint( aP );

    if( ( nearestPt - m_start ).SquaredEuclideanNorm() <= s_epsilon )
        return m_start;

    if( ( nearestPt - m_end ).SquaredEuclideanNorm() <= s_epsilon )
        return m_end;

    if( sliceContainsPoint( nearestPt ) )
        return nearestPt;

    if( ( aP - m_start ).SquaredEuclideanNorm() <= ( aP - m_end ).SquaredEuclideanNorm() )
        return m_start;
    else
        return m_end;
}


bool SHAPE_ARC::NearestPoints( const SHAPE_CIRCLE& aCircle, VECTOR2I& aPtA, VECTOR2I& aPtB,
                               int64_t& aDistSq ) const
{
    if( GetCenter() == aCircle.GetCenter() && GetRadius() == aCircle.GetRadius() )
    {
        aPtA = aPtB = GetP0();
        aDistSq = 0;
        return true;
    }

    aDistSq = std::numeric_limits<int64_t>::max();

    CIRCLE circle1( GetCenter(), GetRadius() );
    CIRCLE circle2( aCircle.GetCircle() );
    std::vector<VECTOR2I> intersections = circle1.Intersect( circle2 );

    for( const VECTOR2I& pt : intersections )
    {
        if( sliceContainsPoint( pt ) )
        {
            aPtA = aPtB = pt;
            aDistSq = 0;
            return true;
        }
    }

    std::vector<VECTOR2I> pts = { m_start, m_end, circle1.NearestPoint( aCircle.GetCenter() ) };

    for( const VECTOR2I& pt : pts )
    {
        if( sliceContainsPoint( pt ) )
        {
            VECTOR2I nearestPt2 = circle2.NearestPoint( pt );
            int64_t distSq = pt.SquaredDistance( nearestPt2 );

            if( distSq < aDistSq )
            {
                aDistSq = distSq;
                aPtA = pt;
                aPtB = nearestPt2;
            }
        }
    }

    // Adjust point A by half the arc width towards point B
    VECTOR2I dir = ( aPtB - aPtA ).Resize( GetWidth() / 2 );
    aPtA += dir;

    if( aDistSq < SEG::Square( GetWidth() / 2 ) )
        aDistSq = 0;
    else
        aDistSq = aPtA.SquaredDistance( aPtB );

    return true;
}


bool SHAPE_ARC::NearestPoints( const SEG& aSeg, VECTOR2I& aPtA, VECTOR2I& aPtB,
                               int64_t& aDistSq ) const
{
    aDistSq = std::numeric_limits<int64_t>::max();
    CIRCLE circle( GetCenter(), GetRadius() );

    // First check for intersections on the circle
    std::vector<VECTOR2I> intersections = circle.Intersect( aSeg );

    for( const VECTOR2I& pt : intersections )
    {
        if( sliceContainsPoint( pt ) )
        {
            aPtA = aPtB = pt;
            aDistSq = 0;
            return true;
        }
    }

    // Check the endpoints of the segment against the nearest point on the arc
    for( const VECTOR2I& pt : { aSeg.A, aSeg.B } )
    {
        if( sliceContainsPoint( pt ) )
        {
            VECTOR2I nearestPt = circle.NearestPoint( pt );
            int64_t distSq = pt.SquaredDistance( nearestPt );

            if( distSq < aDistSq )
            {
                aDistSq = distSq;
                aPtA = nearestPt;
                aPtB = pt;
            }
        }
    }

    // Check the endpoints of the arc against the nearest point on the segment
    for( const VECTOR2I& pt : { m_start, m_end } )
    {
        VECTOR2I nearestPt = aSeg.NearestPoint( pt );
        int64_t distSq = pt.SquaredDistance( nearestPt );

        if( distSq < aDistSq )
        {
            aDistSq = distSq;
            aPtA = pt;
            aPtB = nearestPt;
        }
    }

    // Check the closest points on the segment to the circle (for segments outside the arc)
    VECTOR2I segNearestPt = aSeg.NearestPoint( GetCenter() );

    if( sliceContainsPoint( segNearestPt ) )
    {
        VECTOR2I circleNearestPt = circle.NearestPoint( segNearestPt );
        int64_t distSq = segNearestPt.SquaredDistance( circleNearestPt );

        if( distSq < aDistSq )
        {
            aDistSq = distSq;
            aPtA = segNearestPt;
            aPtB = circleNearestPt;
        }
    }

    // Adjust point A by half the arc width towards point B
    VECTOR2I dir = ( aPtB - aPtA ).Resize( GetWidth() / 2 );
    aPtA += dir;

    if( aDistSq < SEG::Square( GetWidth() / 2 ) )
        aDistSq = 0;
    else
        aDistSq = aPtA.SquaredDistance( aPtB );

    return true;
}


bool SHAPE_ARC::NearestPoints( const SHAPE_RECT& aRect, VECTOR2I& aPtA, VECTOR2I& aPtB,
                               int64_t& aDistSq ) const
{
    aDistSq = std::numeric_limits<int64_t>::max();

    SHAPE_LINE_CHAIN lineChain( aRect.Outline() );

    // Reverse the output points to match the rect_outline/arc order
    lineChain.NearestPoints( this, aPtB, aPtA );
    aDistSq = aPtA.SquaredDistance( aPtB );
    return true;
}


bool SHAPE_ARC::NearestPoints( const SHAPE_ARC& aArc, VECTOR2I& aPtA, VECTOR2I& aPtB,
                               int64_t& aDistSq ) const
{
    auto adjustForArcWidths =
            [&]()
            {
                // Adjust point A by half the arc-width towards point B
                VECTOR2I dir = ( aPtB - aPtA ).Resize( GetWidth() / 2 );
                aPtA += dir;

                // Adjust point B by half the other arc-width towards point A
                dir = ( aPtA - aPtB ).Resize( aArc.GetWidth() / 2 );
                aPtB += dir;

                if( aDistSq < SEG::Square( GetWidth() / 2 + aArc.GetWidth() / 2 ) )
                    aDistSq = 0;
                else
                    aDistSq = aPtA.SquaredDistance( aPtB );
            };

    aDistSq = std::numeric_limits<int64_t>::max();

    VECTOR2I center1 = GetCenter();
    VECTOR2I center2 = aArc.GetCenter();

    // Centers aren't exact, so center_dist_sq won't be exact either
    int64_t center_dist_sq = center1.SquaredDistance( center2 );
    int64_t center_epsilon = KiROUND( std::min( m_radius, aArc.GetRadius() ) / 1000 );
    bool    colocated = center_dist_sq < center_epsilon * center_epsilon;

    // Start by checking endpoints
    std::vector<VECTOR2I> pts1 = { m_start, m_end };
    std::vector<VECTOR2I> pts2 = { aArc.GetP0(), aArc.GetP1() };

    for( const VECTOR2I& pt1 : pts1 )
    {
        for( const VECTOR2I& pt2 : pts2 )
        {
            int64_t distSq = pt1.SquaredDistance( pt2 );

            if( distSq < aDistSq )
            {
                aDistSq = distSq;
                aPtA = pt1;
                aPtB = pt2;

                if( aDistSq == 0 )
                    return true;
            }
        }
    }

    for( const VECTOR2I& pt : pts1 )
    {
        if( aArc.sliceContainsPoint( pt ) )
        {
            CIRCLE circle( center2, aArc.GetRadius() );
            aPtA = pt;
            aPtB = circle.NearestPoint( pt );
            aDistSq = aPtA.SquaredDistance( aPtB );

            if( colocated || aDistSq == 0 )
            {
                if( aDistSq != 0 )
                    adjustForArcWidths();

                return true;
            }
        }
    }

    for( const VECTOR2I& pt : pts2 )
    {
        if( sliceContainsPoint( pt ) )
        {
            CIRCLE circle( center1, GetRadius() );
            aPtA = circle.NearestPoint( pt );
            aPtB = pt;
            aDistSq = aPtA.SquaredDistance( aPtB );

            if( colocated || aDistSq == 0 )
            {
                if( aDistSq != 0 )
                    adjustForArcWidths();

                return true;
            }
        }
    }

    // The remaining checks are require the arcs to be on non-concentric circles
    if( colocated )
        return true;

    CIRCLE circle1( center1, GetRadius() );
    CIRCLE circle2( center2, aArc.GetRadius() );

    // First check for intersections on the circles
    std::vector<VECTOR2I> intersections = circle1.Intersect( circle2 );

    for( const VECTOR2I& pt : intersections )
    {
        if( sliceContainsPoint( pt ) && aArc.sliceContainsPoint( pt ) )
        {
            aPtA = pt;
            aPtB = pt;
            aDistSq = 0;
            return true;
        }
    }

    // Check for the closest points on the circles
    VECTOR2I pt1 = circle1.NearestPoint( center2 );
    VECTOR2I pt2 = circle2.NearestPoint( center1 );
    bool     pt1InSlice = sliceContainsPoint( pt1 );
    bool     pt2InSlice = aArc.sliceContainsPoint( pt2 );

    if( pt1InSlice && pt2InSlice )
    {
        int64_t distSq = pt1.SquaredDistance( pt2 );

        if( distSq < aDistSq )
        {
            aDistSq = distSq;
            aPtA = pt1;
            aPtB = pt2;
        }

        adjustForArcWidths();
        return true;
    }

    // Check the endpoints of arc 1 against the nearest point on arc 2
    if( pt2InSlice )
    {
        for( const VECTOR2I& pt : pts1 )
        {
            int64_t distSq = pt.SquaredDistance( pt2 );

            if( distSq < aDistSq )
            {
                aDistSq = distSq;
                aPtA = pt;
                aPtB = pt2;
            }
        }
    }

    // Check the endpoints of arc 2 against the nearest point on arc 1
    if( pt1InSlice )
    {
        for( const VECTOR2I& pt : pts2 )
        {
            int64_t distSq = pt.SquaredDistance( pt1 );

            if( distSq < aDistSq )
            {
                aDistSq = distSq;
                aPtA = pt1;
                aPtB = pt;
            }
        }
    }

    adjustForArcWidths();
    return true;
}


bool SHAPE_ARC::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                         VECTOR2I* aLocation ) const
{
    int minDist = aClearance + m_width / 2;
    auto bbox = BBox( minDist );

    // Fast check using bounding box:
    if( !bbox.Contains( aP ) )
        return false;

    VECTOR2L  center = GetCenter();
    double    radius = VECTOR2D( center - m_start ).EuclideanNorm();
    CIRCLE    fullCircle( center, radius );
    VECTOR2D  nearestPt = fullCircle.NearestPoint( VECTOR2D( aP ) );
    int       dist = KiROUND( nearestPt.Distance( aP ) );
    EDA_ANGLE angleToPt( aP - fullCircle.Center ); // Angle from center to the point

    if( !dist )
    {
        // Be sure to keep the sqrt of the squared distance instead of allowing a EuclideanNorm
        // because this trucates the distance to an integer before subtracting
        dist = KiROUND( radius - sqrt( ( aP - center ).SquaredEuclideanNorm() ) );
        nearestPt = center + VECTOR2I( radius, 0 );
        RotatePoint( nearestPt, center, -angleToPt );
    }

    // If not a 360 degree arc, need to use arc angles to decide if point collides
    if( m_start != m_end )
    {
        bool   ccw = GetCentralAngle() > ANGLE_0;
        EDA_ANGLE rotatedPtAngle = ( angleToPt.Normalize() - GetStartAngle() ).Normalize();
        EDA_ANGLE rotatedEndAngle = ( GetEndAngle() - GetStartAngle() ).Normalize();

        if( ( ccw && rotatedPtAngle > rotatedEndAngle )
            || ( !ccw && rotatedPtAngle < rotatedEndAngle ) )
        {
            int distStartpt = ( aP - m_start ).EuclideanNorm();
            int distEndpt = ( aP - m_end ).EuclideanNorm();

            if( distStartpt < distEndpt )
            {
                dist = distStartpt;
                nearestPt = m_start;
            }
            else
            {
                dist = distEndpt;
                nearestPt = m_end;
            }
        }
    }

    if( dist <= minDist )
    {
        if( aLocation )
            *aLocation = nearestPt;

        if( aActual )
            *aActual = std::max( 0, dist - m_width / 2 );

        return true;
    }

    return false;
}


EDA_ANGLE SHAPE_ARC::GetStartAngle() const
{
    VECTOR2L center = GetCenter();
    EDA_ANGLE angle( m_start - center );
    return angle.Normalize();
}


EDA_ANGLE SHAPE_ARC::GetEndAngle() const
{
    VECTOR2L center = GetCenter();
    EDA_ANGLE angle( m_end - center );
    return angle.Normalize();
}


const VECTOR2I& SHAPE_ARC::GetCenter() const
{
    return m_center;
}


double SHAPE_ARC::GetLength() const
{
    double radius = GetRadius();
    EDA_ANGLE includedAngle = GetCentralAngle();

    return std::abs( radius * includedAngle.AsRadians() );
}


EDA_ANGLE SHAPE_ARC::GetCentralAngle() const
{
    // Arcs with same start and end points can be 0 deg or 360 deg arcs.
    // However, they are expected to be circles.
    // So return 360 degrees as central arc:
    if( m_start == m_end )
        return ANGLE_360;

    VECTOR2L  center = GetCenter();
    EDA_ANGLE angle = EDA_ANGLE( m_end - center ) - EDA_ANGLE( m_start - center );

    // Using only m_start and m_end arc points to calculate the central arc is not enough
    // there are 2 arcs having the same center and end points.
    // Using the middle point is mandatory to know what arc is the right one.
    // IsCCW() uses m_start, m_middle and m_end arc points to know the arc orientation
    if( IsCCW() )
    {
        if( angle < ANGLE_0 )
            angle += ANGLE_360;
    }
    else
    {
        if( angle > ANGLE_0 )
            angle -= ANGLE_360;
    }

    return angle;
}


double SHAPE_ARC::GetRadius() const
{
    return m_radius;
}


const SHAPE_LINE_CHAIN SHAPE_ARC::ConvertToPolyline( int aMaxError, int* aActualError ) const
{
    SHAPE_LINE_CHAIN rv;
    double    r    = GetRadius();
    EDA_ANGLE sa   = GetStartAngle();
    VECTOR2I  c    = GetCenter();
    EDA_ANGLE ca   = GetCentralAngle();

    SEG    startToEnd( GetP0(), GetP1() );
    double halfMaxError = std::max( 1.0, aMaxError / 2.0 );

    int n;

    // To calculate the arc to segment count, use the external radius instead of the radius.
    // for a arc with small radius and large width, the difference can be significant
    double external_radius = r + ( m_width / 2.0 );
    double effectiveError;

    if( external_radius < halfMaxError
        || startToEnd.Distance( GetArcMid() ) < halfMaxError ) // Should be a very rare case
    {
        // In this case, the arc is approximated by one segment, with a effective error
        // between -aMaxError/2 and +aMaxError/2, as expected.
        n = 0;
        effectiveError = external_radius;
    }
    else
    {
        n = GetArcToSegmentCount( external_radius, aMaxError, ca );

        // Recalculate the effective error of approximation, that can be < aMaxError
        int seg360 = n * 360.0 / fabs( ca.AsDegrees() );
        effectiveError = CircleToEndSegmentDeltaRadius( external_radius, seg360 );
    }

    // Split the error on either side of the arc.  Since we want the start and end points
    // to be exactly on the arc, the first and last segments need to be shorter to stay within
    // the error band (since segments normally start 1/2 the error band outside the arc).
    r += effectiveError / 2;
    n = n * 2;

    rv.Append( m_start );

    for( int i = 1; i < n ; i += 2 )
    {
        EDA_ANGLE a = sa;

        if( n != 0 )
            a += ( ca * i ) / n;

        double x = c.x + r * a.Cos();
        double y = c.y + r * a.Sin();

        rv.Append( KiROUND( x ), KiROUND( y ) );
    }

    rv.Append( m_end );

    if( aActualError )
        *aActualError = KiROUND( effectiveError );

    return rv;
}


void SHAPE_ARC::Move( const VECTOR2I& aVector )
{
    m_start += aVector;
    m_end += aVector;
    m_mid += aVector;
    update_values();
}


void SHAPE_ARC::Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter )
{
    RotatePoint( m_start, aCenter, aAngle );
    RotatePoint( m_end, aCenter, aAngle );
    RotatePoint( m_mid, aCenter, aAngle );

    update_values();
}


void SHAPE_ARC::Mirror( const VECTOR2I& aVector, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        m_start.x = -m_start.x + 2 * aVector.x;
        m_end.x = -m_end.x + 2 * aVector.x;
        m_mid.x = -m_mid.x + 2 * aVector.x;
    }
    else
    {
        m_start.y = -m_start.y + 2 * aVector.y;
        m_end.y = -m_end.y + 2 * aVector.y;
        m_mid.y = -m_mid.y + 2 * aVector.y;
    }

    update_values();
}


void SHAPE_ARC::Mirror( const SEG& axis )
{
    m_start = axis.ReflectPoint( m_start );
    m_end = axis.ReflectPoint( m_end );
    m_mid = axis.ReflectPoint( m_mid );

    update_values();
}


void SHAPE_ARC::Reverse()
{
    std::swap( m_start, m_end );
}


SHAPE_ARC SHAPE_ARC::Reversed() const
{
    return SHAPE_ARC( m_end, m_mid, m_start, m_width );
}


bool SHAPE_ARC::sliceContainsPoint( const VECTOR2I& p ) const
{
    EDA_ANGLE sa = GetStartAngle().Normalize();
    EDA_ANGLE ca = GetCentralAngle();
    EDA_ANGLE ea = sa + ca;

    EDA_ANGLE phi( p - GetCenter() ); // Angle from center to the point
    phi.Normalize();

    if( ca >= ANGLE_0 )
    {
        while( phi < sa )
            phi += ANGLE_360;

        return phi >= sa && phi <= ea;
    }
    else
    {
        while( phi > sa )
            phi -= ANGLE_360;

        return phi <= sa && phi >= ea;
    }
}


void SHAPE_ARC::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError, ERROR_LOC aErrorLoc ) const
{
    TransformArcToPolygon( aBuffer, m_start, m_mid, m_end, m_width, aError, aErrorLoc );
}
