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
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cctype>
#include <strings.h>
#include <appl_wxstruct.h>
#include <wx/file.h>
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


// START: a few routines to help IDF_LIB but which may be of general use in the future
// as IDF support develops

// fetch a line from the given input file and trim the ends
static bool FetchIDFLine( std::ifstream& aModel, std::string& aLine, bool& isComment );

// extract an IDF string and move the index to point to the character after the substring
static bool GetIDFString( const std::string& aLine, std::string& aIDFString,
                          bool& hasQuotes, int& aIndex );

// END: IDF_LIB helper routines

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
    // 1. (emn) close the BOARD_OUTLINE section
    // 2. (emn) write out the DRILLED_HOLES section
    // 3. (emp) finalize the library file
    // 4. (emn) write out the COMPONENT_PLACEMENT section

    if( layoutFile == NULL || libFile == NULL )
        return false;

    // Finalize the board outline section
    fprintf( layoutFile, ".END_BOARD_OUTLINE\n\n" );

    // Write out the drill section
    bool ok = WriteDrills();

    // populate the library (*.emp) file and write the
    // PLACEMENT section
    if( ok )
        ok = IDFLib.WriteFiles( layoutFile, libFile );

    fclose( libFile );
    libFile = NULL;

    fclose( layoutFile );
    layoutFile = NULL;

    return ok;
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


bool IDF_BOARD::PlaceComponent( const wxString aComponentFile, const std::string aRefDes,
                     double aXLoc, double aYLoc, double aZLoc,
                     double aRotation, bool isOnTop )
{
    return IDFLib.PlaceComponent( aComponentFile, aRefDes,
                                  aXLoc, aYLoc, aZLoc,
                                  aRotation, isOnTop );
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


IDF_LIB::~IDF_LIB()
{
    while( !components.empty() )
    {
        delete components.back();
        components.pop_back();
    }
}


bool IDF_LIB::writeLib( FILE* aLibFile )
{
    if( !aLibFile )
        return false;

    // TODO: check stream integrity and return false as appropriate

    // export models
    std::list< IDF_COMP* >::const_iterator mbeg = components.begin();
    std::list< IDF_COMP* >::const_iterator mend = components.end();

    while( mbeg != mend )
    {
        if( !(*mbeg)->WriteLib( aLibFile ) )
            return false;
        ++mbeg;
    }

    libWritten = true;
    return true;
}


bool IDF_LIB::writeBrd( FILE* aLayoutFile )
{
    if( !aLayoutFile || !libWritten )
        return false;

    if( components.empty() )
        return true;

    // TODO: check stream integrity and return false as appropriate

    // write out the board placement information
    std::list< IDF_COMP* >::const_iterator mbeg = components.begin();
    std::list< IDF_COMP* >::const_iterator mend = components.end();

    fprintf( aLayoutFile, "\n.PLACEMENT\n" );

    while( mbeg != mend )
    {
        if( !(*mbeg)->WritePlacement( aLayoutFile ) )
            return false;
        ++mbeg;
    }

    fprintf( aLayoutFile, ".END_PLACEMENT\n" );

    return true;
}


bool IDF_LIB::WriteFiles( FILE* aLayoutFile, FILE* aLibFile )
{
    if( !aLayoutFile || !aLibFile )
        return false;

    libWritten = false;
    regOutlines.clear();

    if( !writeLib( aLibFile ) )
        return false;

    return writeBrd( aLayoutFile );
}


bool IDF_LIB::RegisterOutline( const std::string aGeomPartString )
{
    std::set< std::string >::const_iterator it = regOutlines.find( aGeomPartString );

    if( it != regOutlines.end() )
        return true;

    regOutlines.insert( aGeomPartString );

    return false;
}


bool IDF_LIB::PlaceComponent( const wxString aComponentFile, const std::string aRefDes,
                                double aXLoc, double aYLoc, double aZLoc,
                                double aRotation, bool isOnTop )
{
    IDF_COMP* comp = new IDF_COMP( this );

    if( comp == NULL )
    {
        std::cerr << "IDF_LIB: *ERROR* could not allocate memory for a component\n";
        return false;
    }

    components.push_back( comp );

    if( !comp->PlaceComponent( aComponentFile, aRefDes,
                                     aXLoc, aYLoc, aZLoc,
                                     aRotation, isOnTop ) )
    {
        std::cerr << "IDF_LIB: file does not exist (or is symlink):\n";
        std::cerr << "   FILE: " << TO_UTF8( aComponentFile ) << "\n";
        return false;
    }

    return true;
}


IDF_COMP::IDF_COMP( IDF_LIB* aParent )
{
    parent = aParent;
}


bool IDF_COMP::PlaceComponent( const wxString aComponentFile, const std::string aRefDes,
                               double aXLoc, double aYLoc, double aZLoc,
                               double aRotation, bool isOnTop )
{
    componentFile = aComponentFile;
    refdes = aRefDes;

    if( refdes.empty() || !refdes.compare("~") || !refdes.compare("0") )
        refdes = "NOREFDES";

    loc_x = aXLoc;
    loc_y = aYLoc;
    loc_z = aZLoc;
    rotation = aRotation;
    top = isOnTop;

    if( !wxFileName::FileExists( aComponentFile ) )
    {
        wxFileName fn = aComponentFile;
        wxString fname = wxGetApp().FindLibraryPath( fn );

        if( fname.IsEmpty() )
            return false;
        else
            componentFile = fname;
    }

    return true;
}


bool IDF_COMP::WritePlacement( FILE* aLayoutFile )
{
    if( aLayoutFile == NULL )
    {
        std::cerr << "IDF_COMP: *ERROR* WritePlacement() invoked with aLayoutFile = NULL\n";
        return false;
    }

    if( parent == NULL )
    {
        std::cerr << "IDF_COMP: *ERROR* no valid pointer \n";
        return false;
    }

    if( componentFile.empty() )
    {
        std::cerr << "IDF_COMP: *BUG* empty componentFile name in WritePlacement()\n";
        return false;
    }

    if( geometry.empty() && partno.empty() )
    {
        std::cerr << "IDF_COMP: *BUG* geometry and partno strings are empty in WritePlacement()\n";
        return false;
    }

    // TODO: monitor stream integrity and respond accordingly

    // PLACEMENT, RECORD 2:
    fprintf( aLayoutFile, "\"%s\" \"%s\" \"%s\"\n",
             geometry.c_str(), partno.c_str(), refdes.c_str() );

    // PLACEMENT, RECORD 3:
    if( rotation >= -MIN_ANG && rotation <= -MIN_ANG )
    {
        fprintf( aLayoutFile, "%.6f %.6f %.6f 0 %s ECAD\n",
                 loc_x, loc_y, loc_z, top ? "TOP" : "BOTTOM" );
    }
    else
    {
        fprintf( aLayoutFile, "%.6f %.6f %.6f %.3f %s ECAD\n",
                 loc_x, loc_y, loc_z, rotation, top ? "TOP" : "BOTTOM" );
    }

    return true;
}


bool IDF_COMP::WriteLib( FILE* aLibFile )
{
    // 1. parse the file for the .ELECTRICAL or .MECHANICAL section
    //      and extract the Geometry and PartNumber strings
    // 2. Register the name; check if it already exists
    // 3. parse the rest of the file until .END_ELECTRICAL or
    //      .END_MECHANICAL; validate that each entry conforms
    //      to a valid outline
    // 4. write lines to library file
    //
    // NOTE on parsing (the order matters):
    //  + store each line which begins with '#'
    //  + strip blanks from both ends of the line
    //  + drop each blank line
    //  + the first non-blank non-comment line must be
    //      .ELECTRICAL or .MECHANICAL (as per spec, case does not matter)
    //  + the first non-blank line after RECORD 1 must be RECORD 2
    //  + following RECORD 2, only blank lines, valid outline entries,
    //      and .END_{MECHANICAL,ELECTRICAL} are allowed
    //  + only a single outline may be specified; the order may be
    //      CW or CCW.
    //  + all valid lines are stored and written to the library file
    //
    // return: false if we do could not write model data; we may return
    //  true even if we could not read an IDF file for some reason, provided
    //  that the default model was written. In such a case, warnings will be
    //  written to stderr.

    if( aLibFile == NULL )
    {
        std::cerr << "IDF_COMP: *ERROR* WriteLib() invoked with aLibFile = NULL\n";
        return false;
    }

    if( parent == NULL )
    {
        std::cerr << "IDF_COMP: *ERROR* no valid pointer \n";
        return false;
    }

    if( componentFile.empty() )
    {
        std::cerr << "IDF_COMP: *BUG* empty componentFile name in WriteLib()\n";
        return false;
    }

    std::list< std::string > records;
    std::ifstream model;
    std::string fname = TO_UTF8( componentFile );

    model.open( fname.c_str(), std::ios_base::in );

    if( !model.is_open() )
    {
        std::cerr << "* IDF EXPORT: could not open file " << fname << "\n";
        return substituteComponent( aLibFile );
    }

    std::string entryType;  // will be one of ELECTRICAL or MECHANICAL
    std::string endMark;    // will be one of .END_ELECTRICAL or .END_MECHANICAL
    std::string iline;      // the input line
    int state = 1;
    bool isComment;         // true if a line just read in is a comment line
    bool isNewItem = false; // true if the outline is a previously unsaved IDF item

    // some vars for parsing record 3
    int    loopIdx = -1;        // direction of points in outline (0=CW, 1=CCW, -1=no points yet)
    double firstX;
    double firstY;
    bool   lineClosed = false;  // true when outline has been closed; only one outline is permitted

    while( state )
    {
        while( !FetchIDFLine( model, iline, isComment ) && model.good() );

        if( !model.good() )
        {
            // this should not happen; we should at least
            // have encountered the .END_ statement;
            // however, we shall make a concession if the
            // last line is an .END_ statement which had
            // not been correctly terminated
            if( !endMark.empty() && !strncasecmp( iline.c_str(), endMark.c_str(), 15 ) )
            {
                std::cerr << "IDF EXPORT: *WARNING* IDF file is not properly terminated\n";
                std::cerr << "*     FILE: " << fname << "\n";
                records.push_back( endMark );
                break;
            }

            std::cerr << "IDF EXPORT: *ERROR* faulty IDF file\n";
            std::cerr << "*     FILE: " << fname << "\n";
            return substituteComponent( aLibFile );
        }

        switch( state )
        {
            case 1:
                // accept comment lines, .ELECTRICAL, or .MECHANICAL;
                // all others are simply ignored
                if( isComment )
                {
                    records.push_back( iline );
                    break;
                }

                if( !strncasecmp( iline.c_str(), ".electrical", 11 ) )
                {
                    entryType = ".ELECTRICAL";
                    endMark   = ".END_ELECTRICAL";
                    records.push_back( entryType );
                    state = 2;
                    break;
                }

                if( !strncasecmp( iline.c_str(), ".mechanical", 11 ) )
                {
                    entryType = ".MECHANICAL";
                    endMark   = ".END_MECHANICAL";
                    records.push_back( entryType );
                    state = 2;
                    break;
                }

                break;

            case 2:
                // accept only a RECORD 2 compliant line;
                // anything else constitutes a malformed IDF file
                if( isComment )
                {
                    std::cerr << "IDF EXPORT: bad IDF file\n";
                    std::cerr << "*     LINE: " << iline << "\n";
                    std::cerr << "*     FILE: " << fname << "\n";
                    std::cerr << "*   REASON: comment within "
                              << entryType << " section\n";
                    model.close();
                    return substituteComponent( aLibFile );
                }

                if( !parseRec2( iline, isNewItem ) )
                {
                    std::cerr << "IDF EXPORT: bad IDF file\n";
                    std::cerr << "*     LINE: " << iline << "\n";
                    std::cerr << "*     FILE: " << fname << "\n";
                    std::cerr << "*   REASON: expecting RECORD 2 of "
                              << entryType << " section\n";
                    model.close();
                    return substituteComponent( aLibFile );
                }

                if( isNewItem )
                {
                    records.push_back( iline );
                    state = 3;
                }
                else
                {
                    model.close();
                    return true;
                }

                break;

            case 3:
                // accept outline entries or end of section
                if( isComment )
                {
                    std::cerr << "IDF EXPORT: bad IDF file\n";
                    std::cerr << "*     LINE: " << iline << "\n";
                    std::cerr << "*     FILE: " << fname << "\n";
                    std::cerr << "*   REASON: comment within "
                              << entryType << " section\n";
                    model.close();
                    return substituteComponent( aLibFile );
                }

                if( !strncasecmp( iline.c_str(), endMark.c_str(), 15 ) )
                {
                    records.push_back( endMark );
                    state = 0;
                    break;
                }

                if( lineClosed )
                {
                    // there should be no further points
                    std::cerr << "IDF EXPORT: faulty IDF file\n";
                    std::cerr << "*     LINE: " << iline << "\n";
                    std::cerr << "*     FILE: " << fname << "\n";
                    std::cerr << "*   REASON: more than 1 outline in "
                              << entryType << " section\n";
                    model.close();
                    return substituteComponent( aLibFile );
                }

                if( !parseRec3( iline, loopIdx, firstX, firstY, lineClosed ) )
                {
                    std::cerr << "IDF EXPORT: unexpected line in IDF file\n";
                    std::cerr << "*     LINE: " << iline << "\n";
                    std::cerr << "*     FILE: " << fname << "\n";
                    model.close();
                    return substituteComponent( aLibFile );
                }

                records.push_back( iline );
                break;

            default:
                std::cerr << "IDF EXPORT: BUG in " << __FUNCTION__ << ": unexpected state\n";
                model.close();
                return substituteComponent( aLibFile );
                break;
        }   // switch( state )
    }       // while( state )

    model.close();

    if( !lineClosed )
    {
        std::cerr << "IDF EXPORT: component outline not closed\n";
        std::cerr << "*     FILE: " << fname << "\n";
        return substituteComponent( aLibFile );
    }

    std::list< std::string >::iterator lbeg = records.begin();
    std::list< std::string >::iterator lend = records.end();

    // TODO: check stream integrity
    while( lbeg != lend )
    {
        fprintf( aLibFile, "%s\n", lbeg->c_str() );
        ++lbeg;
    }
    fprintf( aLibFile, "\n" );

    return true;
}


bool IDF_COMP::substituteComponent( FILE* aLibFile )
{
    // the component outline does not exist or could not be
    // read; substitute a placeholder

    // TODO: check the stream integrity
    geometry = "NOGEOM";
    partno   = "NOPART";

    if( parent->RegisterOutline( "NOGEOM_NOPART" ) )
        return true;

    fprintf( aLibFile, ".ELECTRICAL\n" );
    fprintf( aLibFile, "\"NOGEOM\" \"NOPART\" MM 5\n" );
    // TODO: for now we shall use a simple cylinder; a more intricate
    // and readily recognized feature (a stylistic X) would be of
    // much greater value.
    fprintf( aLibFile, "0 0 0 0\n" );
    fprintf( aLibFile, "0 2.5 0 360\n" );
    fprintf( aLibFile, ".END_ELECTRICAL\n\n" );

    return true;
}


bool IDF_COMP::parseRec2( const std::string aLine, bool& isNewItem )
{
    // RECORD 2:
    // + "Geometry Name"
    // + "Part Number"
    // + MM or THOU
    // + height (float)

    isNewItem = false;

    int idx = 0;
    bool quoted = false;
    std::string entry;

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2 in model file (no Geometry Name entry)\n";
        return false;
    }

    geometry = entry;

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2 in model file (no Part No. entry)\n";
        return false;
    }

    partno = entry;

    if( geometry.empty() && partno.empty() )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2 in model file\n";
        std::cerr << "          Geometry Name and Part Number are both empty.\n";
        return false;
    }

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2, missing FIELD 3\n";
        return false;
    }

    if( strcasecmp( "MM", entry.c_str() ) && strcasecmp( "THOU", entry.c_str() ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2, invalid FIELD 3 \""
                  << entry << "\"\n";
        return false;
    }

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2, missing FIELD 4\n";
        return false;
    }

    if( quoted )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2, invalid FIELD 4 (quoted)\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    // ensure that we have a valid value
    double val;
    std::stringstream teststr;
    teststr << entry;

    if( !( teststr >> val ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 2, invalid FIELD 4 (must be numeric)\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    teststr.str( "" );
    teststr << geometry << "_" << partno;
    isNewItem = parent->RegisterOutline( teststr.str() );

    return true;
}


bool IDF_COMP::parseRec3( const std::string aLine, int& aLoopIndex,
                          double& aX, double& aY, bool& aClosed )
{
    // RECORD 3:
    // + 0,1 (loop label)
    // + X coord (float)
    // + Y coord (float)
    // + included angle (0 for line, +ang for CCW, -ang for CW, +360 for circle)
    //
    // notes:
    // 1. first entry may not be a circle or arc
    // 2. it would be nice, but not essential, to ensure that the
    //      winding is indeed as specified by the loop label
    //

    double x, y, ang;
    bool ccw    = false;
    bool quoted = false;
    int idx = 0;
    std::string entry;

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, no data\n";
        return false;
    }

    if( quoted )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 1 is quoted\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( entry.compare( "0" ) && entry.compare( "1" ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 1 is invalid (must be 0 or 1)\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( !entry.compare( "0" ) )
        ccw = true;

    if( aLoopIndex == 0 && !ccw )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, LOOP INDEX changed from 0 to 1\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( aLoopIndex == 1 && ccw )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, LOOP INDEX changed from 1 to 0\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 2 does not exist\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( quoted )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 2 is quoted\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    std::stringstream tstr;
    tstr.str( entry );

    if( !(tstr >> x ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, invalid X value in FIELD 2\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 3 does not exist\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( quoted )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 3 is quoted\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    tstr.clear();
    tstr.str( entry );

    if( !(tstr >> y ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, invalid Y value in FIELD 3\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( !GetIDFString( aLine, entry, quoted, idx ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 4 does not exist\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( quoted )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, FIELD 4 is quoted\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    tstr.clear();
    tstr.str( entry );

    if( !(tstr >> ang ) )
    {
        std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, invalid ANGLE value in FIELD 3\n";
        std::cerr << "    LINE: " << aLine << "\n";
        return false;
    }

    if( aLoopIndex == -1 )
    {
        // this is the first point; there are some special checks
        aLoopIndex = ccw ? 0 : 1;
        aX = x;
        aY = y;
        aClosed = false;

        // ensure that the first point is not an arc specification
        if( ang < -MIN_ANG || ang > MIN_ANG )
        {
            std::cerr << "IDF_COMP: *ERROR* invalid RECORD 3, first point has non-zero angle\n";
            std::cerr << "    LINE: " << aLine << "\n";
            return false;
        }
    }
    else
    {
        // does this close the outline?
        if( ang < 0.0 ) ang = -ang;

        ang -= 360.0;

        if( ang > -MIN_ANG && ang < MIN_ANG )
        {
            // this is  a circle; the loop is closed
            aClosed = true;
        }
        else
        {
            x = (aX - x) * (aX - x);
            y = (aY - y) * (aY - y) + x;

            if( y <= 1e-6 )
            {
                // the points are close enough; the loop is closed
                aClosed = true;
            }
        }
    }

    // NOTE:
    // 1. ideally we would ensure that there are no arcs with a radius of 0; this entails
    //    actively calculating the last point as the previous entry could have been an instruction
    //    to create an arc. This check is sacrificed in the interest of speed.
    // 2. a bad outline can be crafted by giving at least one valid segment and then introducing
    //    a circle; such a condition is not checked for here in the interest of speed.
    // 3. a circle specified with an angle of -360 is invalid, but that condition is not
    //    tested here.

    return true;
}


// fetch a line from the given input file and trim the ends
static bool FetchIDFLine( std::ifstream& aModel, std::string& aLine, bool& isComment )
{
    aLine = "";
    std::getline( aModel, aLine );

    isComment = false;

    // A comment begins with a '#' and must be the first character on the line
    if( aLine[0] == '#' )
        isComment = true;


    while( !aLine.empty() && isspace( *aLine.begin() ) )
        aLine.erase( aLine.begin() );

    while( !aLine.empty() && isspace( *aLine.rbegin() ) )
        aLine.erase( --aLine.end() );

    if( aLine.empty() )
        return false;

    return true;
}


// extract an IDF string and move the index to point to the character after the substring
static bool GetIDFString( const std::string& aLine, std::string& aIDFString,
                          bool& hasQuotes, int& aIndex )
{
    // 1. drop all leading spaces
    // 2. if the first character is '"', read until the next '"',
    //    otherwise read until the next space or EOL.

    std::ostringstream ostr;

    int len = aLine.length();
    int idx = aIndex;

    if( idx < 0 || idx >= len )
        return false;

    while( isspace( aLine[idx] ) && idx < len ) ++idx;

    if( idx == len )
    {
        aIndex = idx;
        return false;
    }

    if( aLine[idx] == '"' )
    {
        hasQuotes = true;
        ++idx;
        while( aLine[idx] != '"' && idx < len )
            ostr << aLine[idx++];

        if( idx == len )
        {
            std::cerr << "GetIDFString(): *ERROR*: unterminated quote mark in line:\n";
            std::cerr << "LINE: " << aLine << "\n";
            aIndex = idx;
            return false;
        }

        ++idx;
    }
    else
    {
        hasQuotes = false;

        while( !isspace( aLine[idx] ) && idx < len )
            ostr << aLine[idx++];

    }

    aIDFString = ostr.str();
    aIndex = idx;

    return true;
}
