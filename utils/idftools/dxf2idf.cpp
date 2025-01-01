/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cstdio>
#include <iostream>
#include <dxf2idf.h>

// differences in angle smaller than MIN_ANG are considered equal
#define MIN_ANG     (0.01)

// min and max bulge bracketing min. arc before transition to line segment
// and max. arc limit
// MIN_BULGE = 0.002 ~0.45 degrees
// MAX_BULGE = 2000  ~89.97 degrees
#define MIN_BULGE 0.002
#define MAX_BULGE 2000.0


DXF2IDF::~DXF2IDF()
{
    while( !lines.empty() )
    {
#ifdef DEBUG_IDF
        IDF3::printSeg( lines.back() );
#endif
        delete lines.back();
        lines.pop_back();
    }
}


bool DXF2IDF::ReadDxf( const std::string& aFile )
{
    DL_Dxf dxf_reader;
    bool success = true;

    if( !dxf_reader.in( aFile, this ) )  // if file open failed
        success = false;

    return success;
}


void DXF2IDF::addLine( const DL_LineData& aData )
{
    IDF_POINT p1, p2;

    p1.x = aData.x1 * m_scale;
    p1.y = aData.y1 * m_scale;
    p2.x = aData.x2 * m_scale;
    p2.y = aData.y2 * m_scale;

    insertLine( p1, p2 );
}


void DXF2IDF::addCircle( const DL_CircleData& aData )
{
    IDF_POINT p1, p2;

    p1.x = aData.cx * m_scale;
    p1.y = aData.cy * m_scale;

    p2.x = p1.x + aData.radius * m_scale;
    p2.y = p1.y;

    IDF_SEGMENT* seg = new IDF_SEGMENT( p1, p2, 360, true );

    if( seg )
        lines.push_back( seg );
}


void DXF2IDF::addArc( const DL_ArcData& aData )
{
    IDF_POINT p1, p2;

    p1.x = aData.cx * m_scale;
    p1.y = aData.cy * m_scale;

    // note: DXF circles always run CCW
    double ea = aData.angle2;

    while( ea < aData.angle1 )
        ea += M_PI;

    p2.x = p1.x + cos( aData.angle1 ) * aData.radius * m_scale;
    p2.y = p1.y + sin( aData.angle1 ) * aData.radius * m_scale;

    double angle = ( ea - aData.angle1 ) * 180.0 / M_PI;

    IDF_SEGMENT* seg = new IDF_SEGMENT( p1, p2, angle, true );

    if( seg )
        lines.push_back( seg );
}


bool DXF2IDF::WriteOutline( FILE* aFile, bool isInch )
{
    if( lines.empty() )
    {
        std::cerr << "* DXF2IDF: empty outline\n";
        return false;
    }

    // 1. find lowest X value
    // 2. string an outline together
    // 3. emit warnings if more than 1 outline
    IDF_OUTLINE outline;

    IDF3::GetOutline( lines, outline );

    if( outline.empty() )
    {
        return false;
    }

    char loopDir = '1';

    if( outline.IsCCW() )
        loopDir = '0';

    std::list<IDF_SEGMENT*>::iterator bo;
    std::list<IDF_SEGMENT*>::iterator eo;

    if( outline.size() == 1 )
    {
        if( !outline.front()->IsCircle() )
        {
            return false;
        }

        // NOTE: a circle always has an angle of 360, never -360,
        // otherwise SolidWorks chokes on the file.
        if( isInch )
        {
            fprintf( aFile, "%c %d %d 0\n", loopDir, (int) ( 1000 * outline.front()->startPoint.x ),
                     (int) ( 1000 * outline.front()->startPoint.y ) );
            fprintf( aFile, "%c %d %d 360\n", loopDir, (int) ( 1000 * outline.front()->endPoint.x ),
                     (int) ( 1000 * outline.front()->endPoint.y ) );
        }
        else
        {
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir, outline.front()->startPoint.x,
                     outline.front()->startPoint.y );
            fprintf( aFile, "%c %.3f %.3f 360\n", loopDir, outline.front()->endPoint.x,
                     outline.front()->endPoint.y );
        }

        return true;
    }

    // ensure that the very last point is the same as the very first point
    outline.back()-> endPoint = outline.front()->startPoint;

    bo  = outline.begin();
    eo  = outline.end();

    // for the first item we write out both points
    if( ( *bo )->angle < MIN_ANG && ( *bo )->angle > -MIN_ANG )
    {
        if( isInch )
        {
            fprintf( aFile, "%c %d %d 0\n", loopDir, (int) ( 1000 * ( *bo )->startPoint.x ),
                     (int) ( 1000 * ( *bo )->startPoint.y ) );
            fprintf( aFile, "%c %d %d 0\n", loopDir, (int) ( 1000 * ( *bo )->endPoint.x ),
                     (int) ( 1000 * ( *bo )->endPoint.y ) );
        }
        else
        {
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir, ( *bo )->startPoint.x,
                     ( *bo )->startPoint.y );
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir, ( *bo )->endPoint.x, ( *bo )->endPoint.y );
        }
    }
    else
    {
        if( isInch )
        {
            fprintf( aFile, "%c %d %d 0\n", loopDir, (int) ( 1000 * ( *bo )->startPoint.x ),
                     (int) ( 1000 * ( *bo )->startPoint.y ) );
            fprintf( aFile, "%c %d %d %.2f\n", loopDir, (int) ( 1000 * ( *bo )->endPoint.x ),
                     (int) ( 1000 * ( *bo )->endPoint.y ), ( *bo )->angle );
        }
        else
        {
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir, ( *bo )->startPoint.x,
                     ( *bo )->startPoint.y );
            fprintf( aFile, "%c %.3f %.3f %.2f\n", loopDir, ( *bo )->endPoint.x,
                     ( *bo )->endPoint.y, ( *bo )->angle );
        }
    }

    ++bo;

    // for all other segments we only write out the last point
    while( bo != eo )
    {
        if( isInch )
        {
            if( ( *bo )->angle < MIN_ANG && ( *bo )->angle > -MIN_ANG )
            {
                fprintf( aFile, "%c %d %d 0\n", loopDir, (int) ( 1000 * ( *bo )->endPoint.x ),
                         (int) ( 1000 * ( *bo )->endPoint.y ) );
            }
            else
            {
                fprintf( aFile, "%c %d %d %.2f\n", loopDir, (int) ( 1000 * ( *bo )->endPoint.x ),
                         (int) ( 1000 * ( *bo )->endPoint.y ), ( *bo )->angle );
            }
        }
        else
        {
            if( ( *bo )->angle < MIN_ANG && ( *bo )->angle > -MIN_ANG )
            {
                fprintf( aFile, "%c %.5f %.5f 0\n", loopDir, ( *bo )->endPoint.x,
                         ( *bo )->endPoint.y );
            }
            else
            {
                fprintf( aFile, "%c %.5f %.5f %.2f\n", loopDir, ( *bo )->endPoint.x,
                         ( *bo )->endPoint.y, ( *bo )->angle );
            }
        }

        ++bo;
    }

    return true;
}

void DXF2IDF::setVariableInt( const std::string& key, int value, int code )
{
    // Called for every int variable in the DXF file (e.g. "$INSUNITS").

    if( key == "$INSUNITS" )    // Drawing units
    {
        switch( value )
        {
        case 1:     // inches
            m_scale = 25.4;
            break;

        case 2:     // feet
            m_scale = 304.8;
            break;

        case 4:     // mm
            m_scale = 1.0;
            break;

        case 5:     // centimeters
            m_scale = 10.0;
            break;

        case 6:     // meters
            m_scale = 1000.0;
            break;

        case 8:     // microinches
            m_scale = 2.54e-5;
            break;

        case 9:     // mils
            m_scale = 0.0254;
            break;

        case 10:    // yards
            m_scale = 914.4;
            break;

        case 11:    // Angstroms
            m_scale = 1.0e-7;
            break;

        case 12:    // nanometers
            m_scale = 1.0e-6;
            break;

        case 13:    // micrometers
            m_scale = 1.0e-3;
            break;

        case 14:    // decimeters
            m_scale = 100.0;
            break;

        default:
            // use the default of 1.0 for:
            // 0: Unspecified Units
            // 3: miles
            // 7: kilometers
            // 15: decameters
            // 16: hectometers
            // 17: gigameters
            // 18: AU
            // 19: lightyears
            // 20: parsecs
            m_scale = 1.0;
            break;
        }
    }
}


void DXF2IDF::addPolyline(const DL_PolylineData& aData )
{
    // Convert DXF Polylines into a series of Lines and Arcs.
    // A Polyline (as opposed to a LWPolyline) may be a 3D line or
    // even a 3D Mesh. The only type of Polyline which is guaranteed
    // to import correctly is a 2D Polyline in X and Y, which is what
    // we assume of all Polylines. The width used is the width of the Polyline.
    // per-vertex line widths, if present, are ignored.
    m_entityParseStatus = 1;
    m_entity_flags = aData.flags;
    m_entityType = DL_ENTITY_POLYLINE;
}


void DXF2IDF::addVertex( const DL_VertexData& aData )
{
    if( m_entityParseStatus == 0 )
        return;     // Error

    if( m_entityParseStatus == 1 )    // This is the first vertex of an entity
    {
        m_lastCoordinate.x = aData.x * m_scale;
        m_lastCoordinate.y = aData.y * m_scale;
        m_polylineStart = m_lastCoordinate;
        m_bulgeVertex = aData.bulge;
        m_entityParseStatus = 2;
        return;
    }

    IDF_POINT seg_end;

    seg_end.x = aData.x * m_scale;
    seg_end.y = aData.y * m_scale;
    insertLine( m_lastCoordinate, seg_end );
    m_lastCoordinate = seg_end;
}


void DXF2IDF::endEntity()
{
    if( m_entityType == DL_ENTITY_POLYLINE )
    {
        // Polyline flags bit 0 indicates closed (1) or open (0) polyline
        if( m_entity_flags & 1 )
        {
            if( std::abs( m_bulgeVertex ) < MIN_BULGE )
                insertLine( m_lastCoordinate, m_polylineStart );
            else
                insertArc( m_lastCoordinate, m_polylineStart, m_bulgeVertex );
        }
    }

    m_entityType = 0 ;
    m_entity_flags = 0;
    m_entityParseStatus = 0;
}


void DXF2IDF::insertLine( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd )
{
    IDF_SEGMENT* seg = new IDF_SEGMENT( aSegStart, aSegEnd );

    if( seg )
        lines.push_back( seg );
}


void DXF2IDF::insertArc( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd, double aBulge )
{
    if( aBulge < -MAX_BULGE )
        aBulge = -MAX_BULGE;
    else if( aBulge > MAX_BULGE )
        aBulge = MAX_BULGE;

    double ang = 720.0 * atan( aBulge ) / M_PI;

    IDF_SEGMENT* seg = new IDF_SEGMENT( aSegStart, aSegEnd, ang, false );

    if( seg )
        lines.push_back( seg );
}
