/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  Cirilo Bernardo
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
#include <libdxfrw.h>
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


bool DXF2IDF::ReadDxf( const std::string aFile )
{
    dxfRW* reader = new dxfRW( aFile.c_str() );

    if( !reader )
        return false;

    bool success = reader->read( this, true );

    delete reader;
    return success;
}


void DXF2IDF::addLine( const DRW_Line& data )
{
    IDF_POINT p1, p2;

    p1.x = data.basePoint.x * m_scale;
    p1.y = data.basePoint.y * m_scale;
    p2.x = data.secPoint.x * m_scale;
    p2.y = data.secPoint.y * m_scale;

    insertLine( p1, p2 );
    return;
}


void DXF2IDF::addCircle( const DRW_Circle& data )
{
    IDF_POINT p1, p2;

    p1.x = data.basePoint.x * m_scale;
    p1.y = data.basePoint.y * m_scale;

    p2.x = p1.x + data.radious * m_scale;
    p2.y = p1.y;

    IDF_SEGMENT* seg = new IDF_SEGMENT( p1, p2, 360, true );

    if( seg )
        lines.push_back( seg );

    return;
}


void DXF2IDF::addArc( const DRW_Arc& data )
{
    IDF_POINT p1, p2;

    p1.x = data.basePoint.x * m_scale;
    p1.y = data.basePoint.y * m_scale;

    // note: DXF circles always run CCW
    double ea = data.endangle;

    while( ea < data.staangle )
        ea += M_PI;

    p2.x = p1.x + cos( data.staangle ) * data.radious * m_scale;
    p2.y = p1.y + sin( data.staangle ) * data.radious * m_scale;

    double angle = ( ea - data.staangle ) * 180.0 / M_PI;

    IDF_SEGMENT* seg = new IDF_SEGMENT( p1, p2, angle, true );

    if( seg )
        lines.push_back( seg );

    return;
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
        std::cerr << "* DXF2IDF::WriteOutline(): no valid outline in file\n";
        return false;
    }

    if( !lines.empty() )
    {
        std::cerr << "* DXF2IDF::WriteOutline(): WARNING: more than 1 outline in file\n";
        std::cerr << "*                          Only the first outline will be used\n";
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
            std::cerr << "* DXF2IDF::WriteOutline(): bad outline\n";
            return false;
        }

            // NOTE: a circle always has an angle of 360, never -360,
            // otherwise SolidWorks chokes on the file.
            if( isInch )
            {
                fprintf( aFile, "%c %d %d 0\n", loopDir,
                         (int) (1000 * outline.front()->startPoint.x),
                         (int) (1000 * outline.front()->startPoint.y) );
                fprintf( aFile, "%c %d %d 360\n", loopDir,
                         (int) (1000 * outline.front()->endPoint.x),
                         (int) (1000 * outline.front()->endPoint.y) );
            }
            else
            {
                fprintf( aFile, "%c %.3f %.3f 0\n", loopDir,
                         outline.front()->startPoint.x, outline.front()->startPoint.y );
                fprintf( aFile, "%c %.3f %.3f 360\n", loopDir,
                         outline.front()->endPoint.x, outline.front()->endPoint.y );
            }

        return true;
    }

    // ensure that the very last point is the same as the very first point
    outline.back()-> endPoint = outline.front()->startPoint;

    bo  = outline.begin();
    eo  = outline.end();

    // for the first item we write out both points
    if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
    {
        if( isInch )
        {
            fprintf( aFile, "%c %d %d 0\n", loopDir,
                     (int) (1000 * (*bo)->startPoint.x),
                     (int) (1000 * (*bo)->startPoint.y) );
            fprintf( aFile, "%c %d %d 0\n", loopDir,
                     (int) (1000 * (*bo)->endPoint.x),
                     (int) (1000 * (*bo)->endPoint.y) );
        }
        else
        {
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir,
                     (*bo)->startPoint.x, (*bo)->startPoint.y );
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir,
                     (*bo)->endPoint.x, (*bo)->endPoint.y );
        }
    }
    else
    {
        if( isInch )
        {
            fprintf( aFile, "%c %d %d 0\n", loopDir,
                     (int) (1000 * (*bo)->startPoint.x),
                     (int) (1000 * (*bo)->startPoint.y) );
            fprintf( aFile, "%c %d %d %.2f\n", loopDir,
                     (int) (1000 * (*bo)->endPoint.x),
                     (int) (1000 * (*bo)->endPoint.y),
                     (*bo)->angle );
        }
        else
        {
            fprintf( aFile, "%c %.3f %.3f 0\n", loopDir,
                     (*bo)->startPoint.x, (*bo)->startPoint.y );
            fprintf( aFile, "%c %.3f %.3f %.2f\n", loopDir,
                     (*bo)->endPoint.x, (*bo)->endPoint.y, (*bo)->angle );
        }
    }

    ++bo;

    // for all other segments we only write out the last point
    while( bo != eo )
    {
        if( isInch )
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                fprintf( aFile, "%c %d %d 0\n", loopDir,
                         (int) (1000 * (*bo)->endPoint.x),
                         (int) (1000 * (*bo)->endPoint.y) );
            }
            else
            {
                fprintf( aFile, "%c %d %d %.2f\n", loopDir,
                         (int) (1000 * (*bo)->endPoint.x),
                         (int) (1000 * (*bo)->endPoint.y),
                         (*bo)->angle );
            }
        }
        else
        {
            if( (*bo)->angle < MIN_ANG && (*bo)->angle > -MIN_ANG )
            {
                fprintf( aFile, "%c %.5f %.5f 0\n", loopDir,
                         (*bo)->endPoint.x, (*bo)->endPoint.y );
            }
            else
            {
                fprintf( aFile, "%c %.5f %.5f %.2f\n", loopDir,
                         (*bo)->endPoint.x, (*bo)->endPoint.y, (*bo)->angle );
            }
        }

        ++bo;
    }

    return true;
}


void DXF2IDF::addHeader( const DRW_Header* data )
{
    std::map<std::string, DRW_Variant*>::const_iterator it;

    for( it = data->vars.begin(); it != data->vars.end(); ++it )
    {
        std::string key = ( (*it).first ).c_str();

        if( key == "$INSUNITS" )
        {
            DRW_Variant* var = (*it).second;

            switch( var->content.i )
            {
                case 1:     // inches
                    m_scale = 25.4;
                    break;

                case 2:     // feet
                    m_scale = 304.8;
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
                    // 4: mm
                    // 3: miles
                    // 7: kilometers
                    // 15: decameters
                    // 16: hectometers
                    // 17: gigameters
                    // 18: AU
                    // 19: lightyears
                    // 20: parsecs
                    break;
            }
        }
    }
}


void DXF2IDF::addLWPolyline(const DRW_LWPolyline& data )
{
    IDF_POINT poly_start;
    IDF_POINT seg_start;
    IDF_POINT seg_end;
    double bulge = 0.0;

    if( !data.vertlist.empty() )
    {
        DRW_Vertex2D* vertex = data.vertlist[0];
        seg_start.x = vertex->x * m_scale;
        seg_start.y = vertex->y * m_scale;
        poly_start = seg_start;
        bulge = vertex->bulge;
    }

    for( size_t i = 1; i < data.vertlist.size(); ++i )
    {
        DRW_Vertex2D* vertex = data.vertlist[i];
        seg_end.x = vertex->x * m_scale;
        seg_end.y = vertex->y * m_scale;

        if( std::abs( bulge ) < MIN_BULGE )
            insertLine( seg_start, seg_end );
        else
            insertArc( seg_start, seg_end, bulge );

        seg_start = seg_end;
        bulge = vertex->bulge;
    }

    // Polyline flags bit 0 indicates closed (1) or open (0) polyline
    if( data.flags & 1 )
    {
        if( std::abs( bulge ) < MIN_BULGE )
            insertLine( seg_start, poly_start );
        else
            insertArc( seg_start, poly_start, bulge );
    }

    return;
}


void DXF2IDF::addPolyline(const DRW_Polyline& data )
{
    IDF_POINT poly_start;
    IDF_POINT seg_start;
    IDF_POINT seg_end;

    if( !data.vertlist.empty() )
    {
        DRW_Vertex* vertex = data.vertlist[0];
        seg_start.x = vertex->basePoint.x * m_scale;
        seg_start.y = vertex->basePoint.y * m_scale;
        poly_start = seg_start;
    }

    for( size_t i = 1; i < data.vertlist.size(); ++i )
    {
        DRW_Vertex* vertex = data.vertlist[i];
        seg_end.x = vertex->basePoint.x * m_scale;
        seg_end.y = vertex->basePoint.y * m_scale;
        insertLine( seg_start, seg_end );
        seg_start = seg_end;
    }

    // Polyline flags bit 0 indicates closed (1) or open (0) polyline
    if( data.flags & 1 )
        insertLine( seg_start, poly_start );

    return;
}


void DXF2IDF::insertLine( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd )
{
    IDF_SEGMENT* seg = new IDF_SEGMENT( aSegStart, aSegEnd );

    if( seg )
        lines.push_back( seg );

    return;
}


void DXF2IDF::insertArc( const IDF_POINT& aSegStart, const IDF_POINT& aSegEnd,
    double aBulge )
{
    if( aBulge < -MAX_BULGE )
        aBulge = -MAX_BULGE;
    else if( aBulge > MAX_BULGE )
        aBulge = MAX_BULGE;

    double ang = 720.0 * atan( aBulge ) / M_PI;

    IDF_SEGMENT* seg = new IDF_SEGMENT( aSegStart, aSegEnd, ang, false );

    if( seg )
        lines.push_back( seg );

    return;
}
