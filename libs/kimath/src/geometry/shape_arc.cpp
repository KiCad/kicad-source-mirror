/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/circle.h>
#include <geometry/geometry_utils.h>
#include <geometry/seg.h>               // for SEG
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
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

    m_mid = VECTOR2I( KiROUND( mid.x ), KiROUND( mid.y ) );
    m_end = VECTOR2I( KiROUND( end.x ), KiROUND( end.y ) );

    update_bbox();
}


SHAPE_ARC::SHAPE_ARC( const VECTOR2I& aArcStart, const VECTOR2I& aArcMid,
                      const VECTOR2I& aArcEnd, int aWidth ) :
        SHAPE( SH_ARC ),
        m_start( aArcStart ),
        m_mid( aArcMid ),
        m_end( aArcEnd ),
        m_width( aWidth )
{
    update_bbox();
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

    update_bbox();
}


SHAPE_ARC::SHAPE_ARC( const SHAPE_ARC& aOther )
    : SHAPE( SH_ARC )
{
    m_start = aOther.m_start;
    m_end = aOther.m_end;
    m_mid = aOther.m_mid;
    m_width = aOther.m_width;
    m_bbox = aOther.m_bbox;
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

    update_bbox();

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

    update_bbox();

    return *this;
}


bool SHAPE_ARC::Collide( const SEG& aSeg, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    if( aSeg.A == aSeg.B )
        return Collide( aSeg.A, aClearance, aActual, aLocation );

    VECTOR2I center = GetCenter();
    CIRCLE   circle( center, GetRadius() );

    // Possible points of the collision are:
    // 1. Intersetion of the segment with the full circle
    // 2. Closest point on the segment to the center of the circle
    // 3. Closest point on the segment to the end points of the arc
    // 4. End points of the segment

    std::vector<VECTOR2I> candidatePts = circle.Intersect( aSeg );

    candidatePts.push_back( aSeg.NearestPoint( center ) );
    candidatePts.push_back( aSeg.NearestPoint( m_start ) );
    candidatePts.push_back( aSeg.NearestPoint( m_end ) );
    candidatePts.push_back( aSeg.A );
    candidatePts.push_back( aSeg.B );

    for( const VECTOR2I& candidate : candidatePts )
    {
        if( Collide( candidate, aClearance, aActual, aLocation ) )
            return true;
    }

    return false;
}


int SHAPE_ARC::IntersectLine( const SEG& aSeg, std::vector<VECTOR2I>* aIpsBuffer ) const
{
    if( aSeg.A == aSeg.B )      // One point does not define a line....
        return 0;

    CIRCLE circ( GetCenter(), GetRadius() );

    std::vector<VECTOR2I> intersections = circ.IntersectLine( aSeg );

    size_t originalSize = aIpsBuffer->size();

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

    size_t originalSize = aIpsBuffer->size();

    for( const VECTOR2I& intersection : intersections )
    {
        if( sliceContainsPoint( intersection ) && aArc.sliceContainsPoint( intersection ) )
            aIpsBuffer->push_back( intersection );
    }

    return aIpsBuffer->size() - originalSize;
}


void SHAPE_ARC::update_bbox()
{
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

    VECTOR2I center = GetCenter();
    const int radius = KiROUND( GetRadius() );

    // count through quadrants included in arc
    for( int quad_angle = quad_angle_start; quad_angle <= quad_angle_end; ++quad_angle )
    {
        VECTOR2I  quad_pt = center;

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

    m_bbox.Compute( points );
}


const BOX2I SHAPE_ARC::BBox( int aClearance ) const
{
    BOX2I bbox( m_bbox );

    if( aClearance != 0 )
        bbox.Inflate( aClearance );

    return bbox;
}


bool SHAPE_ARC::IsClockwise() const
{
    return GetCentralAngle() < ANGLE_0;
}


bool SHAPE_ARC::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                         VECTOR2I* aLocation ) const
{
    int minDist = aClearance + m_width / 2;
    auto bbox = BBox( minDist );

    // Fast check using bounding box:
    if( !bbox.Contains( aP ) )
        return false;

    CIRCLE   fullCircle( GetCenter(), GetRadius() );
    VECTOR2I nearestPt = fullCircle.NearestPoint( aP );

    int dist = ( nearestPt - aP ).EuclideanNorm();

    // If not a 360 degree arc, need to use arc angles to decide if point collides
    if( m_start != m_end )
    {
        bool   ccw = GetCentralAngle() > ANGLE_0;
        EDA_ANGLE angleToPt( aP - fullCircle.Center ); // Angle from center to the point
        EDA_ANGLE rotatedPtAngle = ( angleToPt.Normalize() - GetStartAngle() ).Normalize();
        EDA_ANGLE rotatedEndAngle = ( GetEndAngle() - GetStartAngle() ).Normalize();

        if( ( ccw && rotatedPtAngle > rotatedEndAngle )
            || ( !ccw && rotatedPtAngle < rotatedEndAngle ) )
        {
            int distStartpt = ( aP - m_start ).EuclideanNorm();
            int distEndpt = ( aP - m_end ).EuclideanNorm();
            dist = std::min( distStartpt, distEndpt );
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
    EDA_ANGLE angle( m_start - GetCenter() );
    return angle.Normalize();
}


EDA_ANGLE SHAPE_ARC::GetEndAngle() const
{
    EDA_ANGLE angle( m_end - GetCenter() );
    return angle.Normalize();
}


VECTOR2I SHAPE_ARC::GetCenter() const
{
    return CalcArcCenter( m_start, m_mid, m_end );
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

    VECTOR2I  center = GetCenter();
    EDA_ANGLE angle1 = EDA_ANGLE( m_mid - center ) - EDA_ANGLE( m_start - center );
    EDA_ANGLE angle2 = EDA_ANGLE( m_end - center ) - EDA_ANGLE( m_mid - center );

    return angle1.Normalize180() + angle2.Normalize180();
}


double SHAPE_ARC::GetRadius() const
{
    return ( m_start - GetCenter() ).EuclideanNorm();
}


const SHAPE_LINE_CHAIN SHAPE_ARC::ConvertToPolyline( double aAccuracy,
                                                     double* aEffectiveAccuracy ) const
{
    SHAPE_LINE_CHAIN rv;
    double    r    = GetRadius();
    EDA_ANGLE sa   = GetStartAngle();
    VECTOR2I  c    = GetCenter();
    EDA_ANGLE ca   = GetCentralAngle();

    int n;

    // To calculate the arc to segment count, use the external radius instead of the radius.
    // for a arc with small radius and large width, the difference can be significant
    double external_radius = r+(m_width/2);
    double effectiveAccuracy;

    if( external_radius < aAccuracy/2 )     // Should be a very rare case
    {
        // In this case, the arc is approximated by one segment, with a effective error
        // between -aAccuracy/2 and +aAccuracy/2, as expected.
        n = 0;
        effectiveAccuracy = external_radius;
    }
    else
    {
        n = GetArcToSegmentCount( external_radius, aAccuracy, ca );

        // Recalculate the effective error of approximation, that can be < aAccuracy
        int seg360 = n * 360.0 / fabs( ca.AsDegrees() );
        effectiveAccuracy = CircleToEndSegmentDeltaRadius( external_radius, seg360 );
    }

    // Split the error on either side of the arc.  Since we want the start and end points
    // to be exactly on the arc, the first and last segments need to be shorter to stay within
    // the error band (since segments normally start 1/2 the error band outside the arc).
    r += effectiveAccuracy / 2;
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

    if( aEffectiveAccuracy )
        *aEffectiveAccuracy = effectiveAccuracy;

    return rv;
}


void SHAPE_ARC::Move( const VECTOR2I& aVector )
{
    m_start += aVector;
    m_end += aVector;
    m_mid += aVector;
    update_bbox();
}


void SHAPE_ARC::Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter )
{
    RotatePoint( m_start, aCenter, aAngle );
    RotatePoint( m_end, aCenter, aAngle );
    RotatePoint( m_mid, aCenter, aAngle );

    update_bbox();
}


void SHAPE_ARC::Mirror( bool aX, bool aY, const VECTOR2I& aVector )
{
    if( aX )
    {
        m_start.x = -m_start.x + 2 * aVector.x;
        m_end.x = -m_end.x + 2 * aVector.x;
        m_mid.x = -m_mid.x + 2 * aVector.x;
    }

    if( aY )
    {
        m_start.y = -m_start.y + 2 * aVector.y;
        m_end.y = -m_end.y + 2 * aVector.y;
        m_mid.y = -m_mid.y + 2 * aVector.y;
    }

    update_bbox();
}


void SHAPE_ARC::Mirror( const SEG& axis )
{
    m_start = axis.ReflectPoint( m_start );
    m_end = axis.ReflectPoint( m_end );
    m_mid = axis.ReflectPoint( m_mid );

    update_bbox();
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
    VECTOR2I  center = GetCenter();
    EDA_ANGLE phi( p - center );
    EDA_ANGLE ca = GetCentralAngle();
    EDA_ANGLE sa = GetStartAngle();
    EDA_ANGLE ea;

    if( ca >= ANGLE_0 )
    {
        ea = sa + ca;
    }
    else
    {
        ea = sa;
        sa += ca;
    }

    return alg::within_wrapped_range( phi.AsDegrees(), sa.AsDegrees(), ea.AsDegrees(), 360.0 );
}
