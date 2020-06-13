/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <math.h>
#include <vector>

#include <geometry/geometry_utils.h>
#include <geometry/seg.h>               // for SEG
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <math.h>                       // for cos, sin, M_PI, atan2, ceil
#include <math/box2.h>                  // for BOX2I
#include <math/vector2d.h>              // for VECTOR2I, VECTOR2D, VECTOR2
#include <type_traits>                  // for swap


SHAPE_ARC::SHAPE_ARC( const VECTOR2I& aArcCenter, const VECTOR2I& aArcStartPoint,
           double aCenterAngle, int aWidth ) :
    SHAPE( SH_ARC ), m_width( aWidth )
{
    m_start = aArcStartPoint;
    m_mid = aArcStartPoint;
    m_end = aArcStartPoint;

    RotatePoint( m_mid, aArcCenter, -aCenterAngle * 10.0 / 2.0 );
    RotatePoint( m_end, aArcCenter, -aCenterAngle * 10.0 );

    update_bbox();
}


SHAPE_ARC::SHAPE_ARC( const VECTOR2I& aArcStart, const VECTOR2I& aArcMid,
           const VECTOR2I& aArcEnd, int aWidth ) :
    SHAPE( SH_ARC ), m_start( aArcStart ), m_mid( aArcMid ), m_end( aArcEnd ),
    m_width( aWidth )
{
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


bool SHAPE_ARC::Collide( const SEG& aSeg, int aClearance ) const
{
    int  minDist = aClearance + m_width / 2;
    auto center = GetCenter();
    auto centerDist = aSeg.Distance( center );
    auto p1 = GetP1();

    if( centerDist < minDist )
        return true;

    auto ab = (aSeg.B - aSeg.A );
    auto ac = ( center - aSeg.A );

    auto lenAbSq = ab.SquaredEuclideanNorm();

    auto lambda = (double) ac.Dot( ab ) / (double) lenAbSq;


    if( lambda >= 0.0 && lambda <= 1.0 )
    {
        VECTOR2I p;

        p.x = (double) aSeg.A.x * lambda + (double) aSeg.B.x * (1.0 - lambda);
        p.y = (double) aSeg.A.y * lambda + (double) aSeg.B.y * (1.0 - lambda);

        auto p0pdist = ( m_start - p ).EuclideanNorm();

        if( p0pdist < minDist )
            return true;

        auto p1pdist = ( p1 - p ).EuclideanNorm();

        if( p1pdist < minDist )
            return true;
    }

    auto p0dist = aSeg.Distance( m_start );

    if( p0dist > minDist )
        return true;

    auto p1dist = aSeg.Distance( p1 );

    if( p1dist > minDist )
        return false;


    return true;
}


void SHAPE_ARC::update_bbox()
{
    std::vector<VECTOR2I> points;
    // Put start and end points in the point list
    points.push_back( m_start );
    points.push_back( m_end );

    double start_angle = GetStartAngle();
    double end_angle = start_angle + GetCentralAngle();

    // we always count quadrants clockwise (increasing angle)
    if( start_angle > end_angle )
        std::swap( start_angle, end_angle );

    int quad_angle_start = std::ceil( start_angle / 90.0 );
    int quad_angle_end = std::floor( end_angle / 90.0 );

    // count through quadrants included in arc
    for( int quad_angle = quad_angle_start; quad_angle <= quad_angle_end; ++quad_angle )
    {
        const int radius = KiROUND( GetRadius() );
        VECTOR2I  quad_pt = GetCenter();

        switch( quad_angle % 4 )
        {
        case 0: quad_pt += { radius, 0 }; break;
        case 1:
        case -3: quad_pt += { 0, radius }; break;
        case 2:
        case -2: quad_pt += { -radius, 0 }; break;
        case 3:
        case -1: quad_pt += { 0, -radius }; break;
        default: assert( false );
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


bool SHAPE_ARC::Collide( const VECTOR2I& aP, int aClearance ) const
{
    int minDist = aClearance + m_width / 2;
    auto bbox = BBox( minDist );

    if( !bbox.Contains( aP ) )
        return false;

    auto dist =  ( aP - GetCenter() ).EuclideanNorm();

    return dist <= ( GetRadius() + minDist ) && dist >= ( GetRadius() - minDist );
}


double SHAPE_ARC::GetStartAngle() const
{
    VECTOR2D d( m_start - GetCenter() );

    auto ang = 180.0 / M_PI * atan2( d.y, d.x );

    return NormalizeAngleDegrees( ang, 0.0, 360.0 );
}


double SHAPE_ARC::GetEndAngle() const
{
    VECTOR2D d( m_end - GetCenter() );

    auto ang = 180.0 / M_PI * atan2( d.y, d.x );

    return NormalizeAngleDegrees( ang, 0.0, 360.0 );
}


VECTOR2I SHAPE_ARC::GetCenter() const
{
    return GetArcCenter( m_start, m_mid, m_end );
}


double SHAPE_ARC::GetCentralAngle() const
{
    VECTOR2I center = GetCenter();
    VECTOR2I p0 = m_start - center;
    VECTOR2I p1 = m_mid - center;
    VECTOR2I p2 = m_end - center;
    double   angle1 = ArcTangente( p1.y, p1.x ) - ArcTangente( p0.y, p0.x );
    double   angle2 = ArcTangente( p2.y, p2.x ) - ArcTangente( p1.y, p1.x );

    return ( NormalizeAngle180( angle1 ) + NormalizeAngle180( angle2 ) ) / 10.0;
}


double SHAPE_ARC::GetRadius() const
{
    return ( m_start - GetCenter() ).EuclideanNorm();
}


const SHAPE_LINE_CHAIN SHAPE_ARC::ConvertToPolyline( double aAccuracy ) const
{
    SHAPE_LINE_CHAIN rv;
    double r = GetRadius();
    double sa = GetStartAngle();
    auto   c = GetCenter();
    double ca = GetCentralAngle();

    int n;

    if( r == 0.0 )
    {
        n = 0;
    }
    else
    {
        n = GetArcToSegmentCount( r, aAccuracy, ca );
    }

    for( int i = 0; i <= n ; i++ )
    {
        double a = sa;

        if( n != 0 )
            a += ( ca * i ) / n;

        double x = c.x + r * cos( a * M_PI / 180.0 );
        double y = c.y + r * sin( a * M_PI / 180.0 );

        rv.Append( KiROUND( x ), KiROUND( y ) );
    }

    return rv;
}


void SHAPE_ARC::Move( const VECTOR2I& aVector )
{
    m_start += aVector;
    m_end += aVector;
    m_mid += aVector;
    update_bbox();
}


void SHAPE_ARC::Rotate( double aAngle, const VECTOR2I& aCenter )
{
    m_start -= aCenter;
    m_end -= aCenter;
    m_mid -= aCenter;

    m_start.Rotate( aAngle );
    m_end.Rotate( aAngle );
    m_mid.Rotate( aAngle );

    m_start += aCenter;
    m_end += aCenter;
    m_mid += aCenter;
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
