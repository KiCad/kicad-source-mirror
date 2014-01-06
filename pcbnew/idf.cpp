/**
 * file: idf.cpp
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013  Cirilo Bernardo
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

// TODO: Consider using different precision formats for THOU vs MM output
// Keep in mind that THOU cannot represent MM very well although MM can
// represent 1 THOU with 4 decimal places. For modern manufacturing we
// are interested in a resolution of about 0.1 THOU.

#include <list>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <wx/filename.h>
#include <macros.h>
#include <idf.h>
#include <build_version.h>

// differences in angle smaller than MIN_ANG are considered equal
#define MIN_ANG     (0.01)
// minimum drill diameter (nanometers) - 10000 is a 0.01mm drill
#define IDF_MIN_DIA ( 10000.0 )

// minimum board thickness; this is about 0.012mm (0.5 mils)
// which is about the thickness of a single kapton layer typically
// used in a flexible design.
#define IDF_MIN_BRD_THICKNESS (12000)

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


IDF_DRILL_DATA::IDF_DRILL_DATA( double aDrillDia, double aPosX, double aPosY,
        IDF3::KEY_PLATING aPlating,
        const std::string aRefDes,
        const std::string aHoleType,
        IDF3::KEY_OWNER aOwner )
{
    if( aDrillDia < 0.3 )
        dia = 0.3;
    else
        dia = aDrillDia;

    x = aPosX;
    y = aPosY;
    plating = aPlating;

    if( !aRefDes.compare( "BOARD" ) )
    {
        kref = IDF3::BOARD;
    }
    else if( aRefDes.empty() || !aRefDes.compare( "NOREFDES" ) )
    {
        kref = IDF3::NOREFDES;
    }
    else if( !aRefDes.compare( "PANEL" ) )
    {
        kref = IDF3::PANEL;
    }
    else
    {
        kref = IDF3::REFDES;
        refdes = aRefDes;
    }

    if( !aHoleType.compare( "PIN" ) )
    {
        khole = IDF3::PIN;
    }
    else if( !aHoleType.compare( "VIA" ) )
    {
        khole = IDF3::VIA;
    }
    else if( aHoleType.empty() || !aHoleType.compare( "MTG" ) )
    {
        khole = IDF3::MTG;
    }
    else if( !aHoleType.compare( "TOOL" ) )
    {
        khole = IDF3::TOOL;
    }
    else
    {
        khole = IDF3::OTHER;
        holetype = aHoleType;
    }

    owner = aOwner;
}    // IDF_DRILL_DATA::IDF_DRILL_DATA( ... )


bool IDF_DRILL_DATA::Write( FILE* aLayoutFile )
{
    // TODO: check stream integrity and return 'false' as appropriate

    if( !aLayoutFile )
        return false;

    std::string holestr;
    std::string refstr;
    std::string ownstr;
    std::string pltstr;

    switch( khole )
    {
    case IDF3::PIN:
        holestr = "PIN";
        break;

    case IDF3::VIA:
        holestr = "VIA";
        break;

    case IDF3::TOOL:
        holestr = "TOOL";
        break;

    case IDF3::OTHER:
        holestr = "\"" + holetype + "\"";
        break;

    default:
        holestr = "MTG";
        break;
    }

    switch( kref )
    {
    case IDF3::BOARD:
        refstr = "BOARD";
        break;

    case IDF3::PANEL:
        refstr = "PANEL";
        break;

    case IDF3::REFDES:
        refstr = "\"" + refdes + "\"";
        break;

    default:
        refstr = "NOREFDES";
        break;
    }

    if( plating == IDF3::PTH )
        pltstr = "PTH";
    else
        pltstr = "NPTH";

    switch( owner )
    {
    case IDF3::MCAD:
        ownstr = "MCAD";
        break;

    case IDF3::ECAD:
        ownstr = "ECAD";
        break;

    default:
        ownstr = "UNOWNED";
    }

    fprintf( aLayoutFile, "%.3f %.5f %.5f %s %s %s %s\n",
            dia, x, y, pltstr.c_str(), refstr.c_str(), holestr.c_str(), ownstr.c_str() );

    return true;
}    // IDF_DRILL_DATA::Write( aLayoutFile )


IDF_BOARD::IDF_BOARD()
{
    outlineIndex = 0;
    scale = 1e-6;
    boardThickness = 1.6;       // default to 1.6mm thick boards

    useThou = false;            // by default we want mm output
    hasBrdOutlineHdr = false;

    layoutFile = NULL;
    libFile = NULL;
}


IDF_BOARD::~IDF_BOARD()
{
    Finish();
}


bool IDF_BOARD::Setup( wxString aBoardName,
        wxString aFullFileName,
        bool aUseThou,
        int aBoardThickness )
{
    if( aBoardThickness < IDF_MIN_BRD_THICKNESS )
        return false;

    if( aUseThou )
    {
        useThou = true;
        scale = 1e-3 / 25.4;
    }
    else
    {
        useThou = false;
        scale = 1e-6;
    }

    boardThickness = aBoardThickness * scale;

    wxFileName brdname( aBoardName );
    wxFileName idfname( aFullFileName );

    // open the layout file
    idfname.SetExt( wxT( "emn" ) );
    layoutFile = wxFopen( aFullFileName, wxT( "wt" ) );

    if( layoutFile == NULL )
        return false;

    // open the library file
    idfname.SetExt( wxT( "emp" ) );
    libFile = wxFopen( idfname.GetFullPath(), wxT( "wt" ) );

    if( libFile == NULL )
    {
        fclose( layoutFile );
        layoutFile = NULL;
        return false;
    }

    wxDateTime tdate( time( NULL ) );

    fprintf( layoutFile, ".HEADER\n"
                         "BOARD_FILE 3.0 \"Created by KiCad %s\""
                         " %.4u/%.2u/%.2u.%.2u:%.2u:%.2u 1\n"
                         "\"%s\" %s\n"
                         ".END_HEADER\n\n",
            TO_UTF8( GetBuildVersion() ),
            tdate.GetYear(), tdate.GetMonth() + 1, tdate.GetDay(),
            tdate.GetHour(), tdate.GetMinute(), tdate.GetSecond(),
            TO_UTF8( brdname.GetFullName() ), useThou ? "THOU" : "MM" );

    fprintf( libFile, ".HEADER\n"
                      "BOARD_FILE 3.0 \"Created by KiCad %s\" %.4d/%.2d/%.2d.%.2d:%.2d:%.2d 1\n"
                      ".END_HEADER\n\n",
            TO_UTF8( GetBuildVersion() ),
            tdate.GetYear(), tdate.GetMonth() + 1, tdate.GetDay(),
            tdate.GetHour(), tdate.GetMinute(), tdate.GetSecond() );

    return true;
}


bool IDF_BOARD::Finish( void )
{
    // Steps to finalize the board and library files:
    // 1. (emp) finalize the library file
    // 2. (emn) close the BOARD_OUTLINE section
    // 3. (emn) write out the DRILLED_HOLES section
    // 4. (emn) write out the COMPONENT_PLACEMENT section

    // TODO:
    // idfLib.Finish();
    if( libFile != NULL )
    {
        fclose( libFile );
        libFile = NULL;
    }

    if( layoutFile == NULL )
        return false;

    // Finalize the board outline section
    fprintf( layoutFile, ".END_BOARD_OUTLINE\n\n" );

    // Write out the drill section
    if( WriteDrills() )
    {
        fclose( layoutFile );
        layoutFile = NULL;
        return false;
    }

    // TODO: Write out the component placement section
    // IDF3::export_placement();

    fclose( layoutFile );
    layoutFile = NULL;

    return true;
}


bool IDF_BOARD::AddOutline( IDF_OUTLINE& aOutline )
{
    if( !layoutFile )
        return false;

    // TODO: check the stream integrity

    std::list<IDF_SEGMENT*>::iterator bo;
    std::list<IDF_SEGMENT*>::iterator eo;

    if( !hasBrdOutlineHdr )
    {
        fprintf( layoutFile, ".BOARD_OUTLINE ECAD\n%.5f\n", boardThickness );
        hasBrdOutlineHdr = true;
    }

    if( aOutline.size() == 1 )
    {
        if( !aOutline.front()->IsCircle() )
            return false;                   // this is a bad outline

        // NOTE: a circle always has an angle of 360, never -360,
        // otherwise SolidWorks chokes on the file.
        fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                aOutline.front()->startPoint.x, aOutline.front()->startPoint.y );
        fprintf( layoutFile, "%d %.5f %.5f 360\n", outlineIndex,
                aOutline.front()->endPoint.x, aOutline.front()->endPoint.y );

        ++outlineIndex;
        return true;
    }

    // ensure that the very last point is the same as the very first point
    aOutline.back()-> endPoint = aOutline.front()->startPoint;

    // check if we must reverse things
    if( ( aOutline.IsCCW() && ( outlineIndex > 0 ) )
        || ( ( !aOutline.IsCCW() ) && ( outlineIndex == 0 ) ) )
    {
        eo  = aOutline.begin();
        bo  = aOutline.end();
        --bo;

        // for the first item we write out both points
        if( aOutline.front()->angle < MIN_ANG && aOutline.front()->angle > -MIN_ANG )
        {
            fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                    aOutline.front()->endPoint.x, aOutline.front()->endPoint.y );
            fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                    aOutline.front()->startPoint.x, aOutline.front()->startPoint.y );
        }
        else
        {
            fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                    aOutline.front()->endPoint.x, aOutline.front()->endPoint.y );
            fprintf( layoutFile, "%d %.5f %.5f %.5f\n", outlineIndex,
                    aOutline.front()->startPoint.x, aOutline.front()->startPoint.y,
                    -aOutline.front()->angle );
        }

        // for all other segments we only write out the start point
        while( bo != eo )
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                        (*bo)->startPoint.x, (*bo)->startPoint.y );
            }
            else
            {
                fprintf( layoutFile, "%d %.5f %.5f %.5f\n", outlineIndex,
                        (*bo)->startPoint.x, (*bo)->startPoint.y, -(*bo)->angle );
            }

            --bo;
        }
    }
    else
    {
        bo  = aOutline.begin();
        eo  = aOutline.end();

        // for the first item we write out both points
        if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
        {
            fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                    (*bo)->startPoint.x, (*bo)->startPoint.y );
            fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                    (*bo)->endPoint.x, (*bo)->endPoint.y );
        }
        else
        {
            fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                    (*bo)->startPoint.x, (*bo)->startPoint.y );
            fprintf( layoutFile, "%d %.5f %.5f %.5f\n", outlineIndex,
                    (*bo)->endPoint.x, (*bo)->endPoint.y, (*bo)->angle );
        }

        ++bo;

        // for all other segments we only write out the last point
        while( bo != eo )
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                fprintf( layoutFile, "%d %.5f %.5f 0\n", outlineIndex,
                        (*bo)->endPoint.x, (*bo)->endPoint.y );
            }
            else
            {
                fprintf( layoutFile, "%d %.5f %.5f %.5f\n", outlineIndex,
                        (*bo)->endPoint.x, (*bo)->endPoint.y, (*bo)->angle );
            }

            ++bo;
        }
    }

    ++outlineIndex;

    return true;
}


bool IDF_BOARD::AddDrill( double dia, double x, double y,
        IDF3::KEY_PLATING plating,
        const std::string refdes,
        const std::string holeType,
        IDF3::KEY_OWNER owner )
{
    if( dia < IDF_MIN_DIA * scale )
        return false;

    IDF_DRILL_DATA* dp = new IDF_DRILL_DATA( dia, x, y, plating, refdes, holeType, owner );
    drills.push_back( dp );

    return true;
}


bool IDF_BOARD::AddSlot( double aWidth, double aLength, double aOrientation,
        double aX, double aY )
{
    if( aWidth < IDF_MIN_DIA * scale )
        return false;

    if( aLength < IDF_MIN_DIA * scale )
        return false;

    IDF_POINT c[2];     // centers
    IDF_POINT pt[4];

    double a1 = aOrientation / 180.0 * M_PI;
    double a2 = a1 + M_PI2;
    double d1 = aLength / 2.0;
    double d2 = aWidth / 2.0;
    double sa1 = sin( a1 );
    double ca1 = cos( a1 );
    double dsa2 = d2 * sin( a2 );
    double dca2 = d2 * cos( a2 );

    c[0].x  = aX + d1 * ca1;
    c[0].y  = aY + d1 * sa1;

    c[1].x  = aX - d1 * ca1;
    c[1].y  = aY - d1 * sa1;

    pt[0].x = c[0].x - dca2;
    pt[0].y = c[0].y - dsa2;

    pt[1].x = c[1].x - dca2;
    pt[1].y = c[1].y - dsa2;

    pt[2].x = c[1].x + dca2;
    pt[2].y = c[1].y + dsa2;

    pt[3].x = c[0].x + dca2;
    pt[3].y = c[0].y + dsa2;

    IDF_OUTLINE outline;

    // first straight run
    IDF_SEGMENT* seg = new IDF_SEGMENT( pt[0], pt[1] );
    outline.push( seg );
    // first 180 degree cap
    seg = new IDF_SEGMENT( c[1], pt[1], -180.0, true );
    outline.push( seg );
    // final straight run
    seg = new IDF_SEGMENT( pt[2], pt[3] );
    outline.push( seg );
    // final 180 degree cap
    seg = new IDF_SEGMENT( c[0], pt[3], -180.0, true );
    outline.push( seg );

    return AddOutline( outline );
}


bool IDF_BOARD::WriteDrills( void )
{
    if( !layoutFile )
        return false;

    // TODO: check the stream integrity and return false as appropriate
    if( drills.empty() )
        return true;

    fprintf( layoutFile, ".DRILLED_HOLES\n" );

    std::list<struct IDF_DRILL_DATA*>::iterator ds  = drills.begin();
    std::list<struct IDF_DRILL_DATA*>::iterator de  = drills.end();

    while( ds != de )
    {
        if( !(*ds)->Write( layoutFile ) )
            return false;

        ++ds;
    }

    fprintf( layoutFile, ".END_DRILLED_HOLES\n" );

    return true;
}


double IDF_BOARD::GetScale( void )
{
    return scale;
}


void IDF_BOARD::SetOffset( double x, double y )
{
    offsetX = x;
    offsetY = y;
}


void IDF_BOARD::GetOffset( double& x, double& y )
{
    x = offsetX;
    y = offsetY;
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


bool IDF_LIB::WriteLib( FILE* aLibFile )
{
    if( !aLibFile )
        return false;

    // TODO: check stream integrity and return false as appropriate

    // TODO: export models

    return true;
}


bool IDF_LIB::WriteBrd( FILE* aLayoutFile )
{
    if( !aLayoutFile )
        return false;

    // TODO: check stream integrity and return false as appropriate

    // TODO: write out the board placement information

    return true;
}
