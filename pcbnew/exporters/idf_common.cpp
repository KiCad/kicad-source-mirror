/**
 * file: idf_common.cpp
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014  Cirilo Bernardo
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

#include <list>
#include <string>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <richio.h>
#include <idf_common.h>
#include <build_version.h>

#ifdef DEBUG_IDF
void IDF3::PrintSeg( IDF_SEGMENT* aSegment )
{
    if( aSegment->IsCircle() )
    {
        fprintf(stdout, "printSeg(): CIRCLE: C(%.3f, %.3f) P(%.3f, %.3f) rad. %.3f\n",
                aSegment->startPoint.x, aSegment->startPoint.y,
                aSegment->endPoint.x, aSegment->endPoint.y,
                aSegment->radius );
        return;
    }

    if( aSegment->angle < -MIN_ANG || aSegment->angle > MIN_ANG )
    {
        fprintf(stdout, "printSeg(): ARC: p1(%.3f, %.3f) p2(%.3f, %.3f) ang. %.3f\n",
                aSegment->startPoint.x, aSegment->startPoint.y,
                aSegment->endPoint.x, aSegment->endPoint.y,
                aSegment->angle );
        return;
    }

    fprintf(stdout, "printSeg(): LINE: p1(%.3f, %.3f) p2(%.3f, %.3f)\n",
            aSegment->startPoint.x, aSegment->startPoint.y,
            aSegment->endPoint.x, aSegment->endPoint.y );

    return;
}
#endif


bool IDF_POINT::Matches( const IDF_POINT& aPoint, double aRadius )
{
    double dx = x - aPoint.x;
    double dy = y - aPoint.y;

    double d2 = dx * dx + dy * dy;

    if( d2 <= aRadius * aRadius )
        return true;

    return false;
}


double IDF_POINT::CalcDistance( const IDF_POINT& aPoint ) const
{
    double dx   = aPoint.x - x;
    double dy   = aPoint.y - y;
    double dist = sqrt( dx * dx + dy * dy );

    return dist;
}


double IDF3::CalcAngleRad( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint )
{
    return atan2( aEndPoint.y - aStartPoint.y, aEndPoint.x - aStartPoint.x );
}


double IDF3::CalcAngleDeg( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint )
{
    double ang = CalcAngleRad( aStartPoint, aEndPoint );

    // round to thousandths of a degree
    int iang = int (ang / M_PI * 1800000.0);

    ang = iang / 10000.0;

    return ang;
}


void IDF3::GetOutline( std::list<IDF_SEGMENT*>& aLines,
                       IDF_OUTLINE& aOutline )
{
    aOutline.Clear();

    // NOTE: To tell if the point order is CCW or CW,
    // sum all:  (endPoint.X[n] - startPoint.X[n])*(endPoint[n] + startPoint.Y[n])
    // If the result is >0, the direction is CW, otherwise
    // it is CCW. Note that the result cannot be 0 unless
    // we have a bounded area of 0.

    // First we find the segment with the leftmost point
    std::list<IDF_SEGMENT*>::iterator bl    = aLines.begin();
    std::list<IDF_SEGMENT*>::iterator el    = aLines.end();
    std::list<IDF_SEGMENT*>::iterator idx   = bl++;       // iterator for the object with minX

    double minx = (*idx)->GetMinX();
    double curx;

    while( bl != el )
    {
        curx = (*bl)->GetMinX();

        if( curx < minx )
        {
            minx = curx;
            idx = bl;
        }

        ++bl;
    }

    aOutline.push( *idx );
#ifdef DEBUG_IDF
    PrintSeg( *idx );
#endif
    aLines.erase( idx );

    // If the item is a circle then we're done
    if( aOutline.front()->IsCircle() )
        return;

    // Assemble the loop
    bool complete = false;  // set if loop is complete
    bool matched;           // set if a segment's end point was matched

    while( !complete )
    {
        matched = false;
        bl  = aLines.begin();
        el  = aLines.end();

        while( bl != el && !matched )
        {
            if( (*bl)->MatchesStart( aOutline.back()->endPoint ) )
            {
                if( (*bl)->IsCircle() )
                {
                    // a circle on the perimeter is pathological but we just ignore it
                    ++bl;
                }
                else
                {
                    matched = true;
#ifdef DEBUG_IDF
                    PrintSeg( *bl );
#endif
                    aOutline.push( *bl );
                    aLines.erase( bl );
                }

                continue;
            }

            ++bl;
        }

        if( !matched )
        {
            // attempt to match the end points
            bl  = aLines.begin();
            el  = aLines.end();

            while( bl != el && !matched )
            {
                if( (*bl)->MatchesEnd( aOutline.back()->endPoint ) )
                {
                    if( (*bl)->IsCircle() )
                    {
                        // a circle on the perimeter is pathological but we just ignore it
                        ++bl;
                    }
                    else
                    {
                        matched = true;
                        (*bl)->SwapEnds();
#ifdef DEBUG_IDF
                        printSeg( *bl );
#endif
                        aOutline.push( *bl );
                        aLines.erase( bl );
                    }

                    continue;
                }

                ++bl;
            }
        }

        if( !matched )
        {
            // still no match - attempt to close the loop
            if( (aOutline.size() > 1) || ( aOutline.front()->angle < -MIN_ANG )
                || ( aOutline.front()->angle > MIN_ANG ) )
            {
                // close the loop
                IDF_SEGMENT* seg = new IDF_SEGMENT( aOutline.back()->endPoint,
                                                    aOutline.front()->startPoint );

                if( seg )
                {
                    complete = true;
#ifdef DEBUG_IDF
                    printSeg( seg );
#endif
                    aOutline.push( seg );
                    break;
                }
            }

            // the outline is bad; drop the segments
            aOutline.Clear();

            return;
        }

        // check if the loop is complete
        if( aOutline.front()->MatchesStart( aOutline.back()->endPoint ) )
        {
            complete = true;
            break;
        }
    }
}


IDF_SEGMENT::IDF_SEGMENT()
{
    angle = 0.0;
    offsetAngle = 0.0;
    radius = 0.0;
}


IDF_SEGMENT::IDF_SEGMENT( const IDF_POINT& aStartPoint, const IDF_POINT& aEndPoint )
{
    angle = 0.0;
    offsetAngle = 0.0;
    radius = 0.0;
    startPoint = aStartPoint;
    endPoint = aEndPoint;
}


IDF_SEGMENT::IDF_SEGMENT( const IDF_POINT& aStartPoint,
                          const IDF_POINT& aEndPoint,
                          double aAngle,
                          bool aFromKicad )
{
    double diff = abs( aAngle ) - 360.0;

    if( ( diff < MIN_ANG
        && diff > -MIN_ANG ) || ( aAngle < MIN_ANG && aAngle > -MIN_ANG ) || (!aFromKicad) )
    {
        angle = 0.0;
        startPoint = aStartPoint;
        endPoint = aEndPoint;

        if( diff < MIN_ANG && diff > -MIN_ANG )
        {
            angle = 360.0;
            center = aStartPoint;
            offsetAngle = 0.0;
            radius = aStartPoint.CalcDistance( aEndPoint );
        }
        else if( aAngle < MIN_ANG && aAngle > -MIN_ANG )
        {
            CalcCenterAndRadius();
        }

        return;
    }

    // we need to convert from the KiCad arc convention
    angle = aAngle;

    center = aStartPoint;

    offsetAngle = IDF3::CalcAngleDeg( aStartPoint, aEndPoint );

    radius = aStartPoint.CalcDistance( aEndPoint );

    startPoint = aEndPoint;

    double ang = offsetAngle + aAngle;
    ang = (ang / 180.0) * M_PI;

    endPoint.x  = ( radius * cos( ang ) ) + center.x;
    endPoint.y  = ( radius * sin( ang ) ) + center.y;
}


bool IDF_SEGMENT::MatchesStart( const IDF_POINT& aPoint, double aRadius )
{
    return startPoint.Matches( aPoint, aRadius );
}


bool IDF_SEGMENT::MatchesEnd( const IDF_POINT& aPoint, double aRadius )
{
    return endPoint.Matches( aPoint, aRadius );
}


void IDF_SEGMENT::CalcCenterAndRadius( void )
{
    // NOTE:  this routine does not check if the points are the same
    // or too close to be sensible in a production setting.

    double offAng = IDF3::CalcAngleRad( startPoint, endPoint );
    double d = startPoint.CalcDistance( endPoint ) / 2.0;
    double xm   = ( startPoint.x + endPoint.x ) * 0.5;
    double ym   = ( startPoint.y + endPoint.y ) * 0.5;

    radius = d / sin( angle * M_PI / 180.0 );

    if( radius < 0.0 )
    {
        radius = -radius;
    }

    // calculate the height of the triangle with base d and hypotenuse r
    double dh2 = radius * radius - d * d;

    if( dh2 < 0 )
    {
        // this should only ever happen due to rounding errors when r == d
        dh2 = 0;
    }

    double h = sqrt( dh2 );

    if( angle > 0.0 )
        offAng += M_PI2;
    else
        offAng -= M_PI2;

    if( ( angle > M_PI ) || ( angle < -M_PI ) )
        offAng += M_PI;

    center.x = h * cos( offAng ) + xm;
    center.y = h * sin( offAng ) + ym;

    offsetAngle = IDF3::CalcAngleDeg( center, startPoint );
}


bool IDF_SEGMENT::IsCircle( void )
{
    double diff = abs( angle ) - 360.0;

    if( ( diff < MIN_ANG ) && ( diff > -MIN_ANG ) )
        return true;

    return false;
}


double IDF_SEGMENT::GetMinX( void )
{
    if( angle == 0.0 )
        return std::min( startPoint.x, endPoint.x );

    // Calculate the leftmost point of the circle or arc

    if( IsCircle() )
    {
        // if only everything were this easy
        return center.x - radius;
    }

    // cases:
    // 1. CCW arc: if offset + included angle >= 180 deg then
    // MinX = center.x - radius, otherwise MinX is the
    // same as for the case of a line.
    // 2. CW arc: if offset + included angle <= -180 deg then
    // MinX = center.x - radius, otherwise MinX is the
    // same as for the case of a line.

    if( angle > 0 )
    {
        // CCW case
        if( ( offsetAngle + angle ) >= 180.0 )
        {
            return center.x - radius;
        }
        else
        {
            return std::min( startPoint.x, endPoint.x );
        }
    }

    // CW case
    if( ( offsetAngle + angle ) <= -180.0 )
    {
        return center.x - radius;
    }

    return std::min( startPoint.x, endPoint.x );
}


void IDF_SEGMENT::SwapEnds( void )
{
    if( IsCircle() )
    {
        // reverse the direction
        angle = -angle;
        return;
    }

    IDF_POINT tmp = startPoint;
    startPoint = endPoint;
    endPoint = tmp;

    if( ( angle < MIN_ANG ) && ( angle > -MIN_ANG ) )
        return;         // nothing more to do

        // change the direction of the arc
        angle = -angle;
    // calculate the new offset angle
    offsetAngle = IDF3::CalcAngleDeg( center, startPoint );
}


void IDF_OUTLINE::push( IDF_SEGMENT* item )
{
    if( !outline.empty() )
    {
        if( item->IsCircle() )
        {
            // not allowed
            wxString msg = wxT( "INVALID GEOMETRY: a circle is being added to a non-empty outline" );
            THROW_IO_ERROR( msg );
        }
        else
        {
            if( outline.back()->IsCircle() )
            {
                // we can't add lines to a circle
                wxString msg = wxT( "INVALID GEOMETRY: a line is being added to a circular outline" );
                THROW_IO_ERROR( msg );
            }
            else if( !item->MatchesStart( outline.back()->endPoint ) )
            {
                // startPoint[N] != endPoint[N -1]
                wxString msg = wxT( "INVALID GEOMETRY: disjoint segments" );
                THROW_IO_ERROR( msg );
            }
        }
    }

    outline.push_back( item );
    dir += ( outline.back()->endPoint.x - outline.back()->startPoint.x )
    * ( outline.back()->endPoint.y + outline.back()->startPoint.y );
}
