/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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
#include <math.h>

// For some unknown reasons, polygon.hpp shoul be included first
#include "boost/polygon/polygon.hpp"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

#include "potracelib.h"
#include "auxiliary.h"


#ifndef max
    #define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )
#endif
#ifndef min
    #define min( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#endif

// Define some types used here from boost::polygon
namespace bpl = boost::polygon;         // bpl = boost polygon library
using namespace bpl::operators;         // +, -, =, ...

typedef int                                coordinate_type;

typedef bpl::polygon_data<coordinate_type> KPolygon;        // define a basic polygon
typedef std::vector<KPolygon>              KPolygonSet;     // define a set of polygons

typedef bpl::point_data<coordinate_type>   KPolyPoint;      // define a corner of a polygon

enum output_format {
    POSTSCRIPT_FMT = 1,
    PCBNEW_FMT,
    EESCHEMA_FMT
};
/* free a potrace bitmap */
static void bm_free( potrace_bitmap_t* bm )
{
    if( bm != NULL )
    {
        free( bm->map );
    }
    free( bm );
}


/* Helper class to handle useful info to convert a bitmap image to
 *  a polygonal object description
 */
class BITMAPCONV_INFO
{
public:
    enum output_format m_Format;    // File format
    int m_PixmapWidth;
    int m_PixmapHeight;             // the bitmap size in pixels
    double             m_ScaleX;
    double             m_ScaleY;    // the conversion scale
    potrace_path_t*    m_Paths;     // the list of paths, from potrace (list of lines and bezier curves)
    FILE* m_Outfile;                // File to create
    const char * m_CmpName;                 // The string used as cmp/footprint name

public:
    BITMAPCONV_INFO();

    /**
     * Function CreateOutputFile
     * Creates the output file specified by m_Outfile,
     * depending on file format given by m_Format
     */
    void CreateOutputFile();


private:
    /**
     * Function OuputFileHeader
     * write to file the header depending on file format
     */
    void OuputFileHeader();

    /**
     * Function OuputFileEnd
     * write to file the last strings depending on file format
     */
    void OuputFileEnd();


    /**
     * Function OuputOnePolygon
     * write one polygon to output file.
     * Polygon coordinates are expected scaled by the polugon extraction function
     */
    void OuputOnePolygon( KPolygon & aPolygon );

};

static void BezierToPolyline( std::vector <potrace_dpoint_t>& aCornersBuffer,
                              potrace_dpoint_t                p1,
                              potrace_dpoint_t                p2,
                              potrace_dpoint_t                p3,
                              potrace_dpoint_t                p4 );


BITMAPCONV_INFO::BITMAPCONV_INFO()
{
    m_Format = POSTSCRIPT_FMT;
    m_PixmapWidth  = 0;
    m_PixmapHeight = 0;
    m_ScaleX  = 1.0;
    m_ScaleY  = 1.0;
    m_Paths   = NULL;
    m_Outfile = NULL;
    m_CmpName = "LOGO";
}


int bitmap2component( potrace_bitmap_t* aPotrace_bitmap, FILE* aOutfile, int aFormat )
{
    potrace_param_t* param;
    potrace_state_t* st;


    // set tracing parameters, starting from defaults
    param = potrace_param_default();
    if( !param )
    {
        fprintf( stderr, "Error allocating parameters: %s\n", strerror( errno ) );
        return 1;
    }
    param->turdsize = 0;

    /* convert the bitmap to curves */
    st = potrace_trace( param, aPotrace_bitmap );
    if( !st || st->status != POTRACE_STATUS_OK )
    {
        fprintf( stderr, "Error tracing bitmap: %s\n", strerror( errno ) );
        return 1;
    }

    BITMAPCONV_INFO info;
    info.m_PixmapWidth  = aPotrace_bitmap->w;
    info.m_PixmapHeight = aPotrace_bitmap->h;     // the bitmap size in pixels
    info.m_Paths   = st->plist;
    info.m_Outfile = aOutfile;

    switch( aFormat )
    {
    case 2:
        info.m_Format = POSTSCRIPT_FMT;
        info.m_ScaleX = info.m_ScaleY = 1.0;        // the conversion scale
        // output vector data, e.g. as a rudimentary EPS file (mainly for tests)
        info.CreateOutputFile();
        break;

    case 1:
        info.m_Format = EESCHEMA_FMT;
        info.m_ScaleX = 1000.0 / 300;       // the conversion scale
        info.m_ScaleY = -info.m_ScaleX;     // Y axis is bottom to Top for components in libs
        info.CreateOutputFile();
        break;

    case 0:
        info.m_Format = PCBNEW_FMT;
        info.m_ScaleX = 10000.0 / 300;          // the conversion scale
        info.m_ScaleY = info.m_ScaleX;          // Y axis is top to bottom in modedit
        info.CreateOutputFile();
        break;

    default:
        break;
    }


    bm_free( aPotrace_bitmap );
    potrace_state_free( st );
    potrace_param_free( param );

    return 0;
}


void BITMAPCONV_INFO::OuputFileHeader()
{
    int Ypos = (int) ( m_PixmapHeight / 2 * m_ScaleY );
    int fieldSize;             // fields text size = 60 mils

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        /* output vector data, e.g. as a rudimentary EPS file */
        fprintf( m_Outfile, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
        fprintf( m_Outfile, "%%%%BoundingBox: 0 0 %d %d\n",
                 m_PixmapWidth, m_PixmapHeight );
        fprintf( m_Outfile, "gsave\n" );
        break;

    case PCBNEW_FMT:
        #define FIELD_LAYER 21
        fieldSize = 600;             // fields text size = 60 mils
        Ypos += fieldSize / 2;
        fprintf( m_Outfile, "PCBNEW-LibModule-V1\n" );
        fprintf( m_Outfile, "$INDEX\n%s\n$EndINDEX\n", m_CmpName );

        fprintf( m_Outfile, "#\n# %s\n", m_CmpName );
        fprintf( m_Outfile, "# pixmap w = %d, h = %d\n#\n",
                 m_PixmapWidth, m_PixmapHeight );
        fprintf( m_Outfile, "$MODULE %s\n", m_CmpName );
        fprintf( m_Outfile, "Po 0 0 0 15 00000000 00000000 ~~\n" );
        fprintf( m_Outfile, "Li %s\n", m_CmpName );
        fprintf( m_Outfile, "T0 0 %d %d %d 0 %d N I %d \"G***\"\n",
                 Ypos, fieldSize, fieldSize, fieldSize / 5, FIELD_LAYER );
        fprintf( m_Outfile, "T1 0 %d %d %d 0 %d N I %d \"%s\"\n",
                 -Ypos, fieldSize, fieldSize, fieldSize / 5, FIELD_LAYER, m_CmpName );
        break;

    case EESCHEMA_FMT:
        fprintf( m_Outfile, "EESchema-LIBRARY Version 2.3\n" );
        fprintf( m_Outfile, "#\n# %s\n", m_CmpName );
        fprintf( m_Outfile, "# pixmap size w = %d, h = %d\n#\n",
                 m_PixmapWidth, m_PixmapHeight );

        // print reference and value
        fieldSize = 60;             // fields text size = 60 mils
        Ypos += fieldSize / 2;
        fprintf( m_Outfile, "DEF %s G 0 40 Y Y 1 F N\n", m_CmpName );
        fprintf( m_Outfile, "F0 \"#G\" 0 %d %d H I C CNN\n", Ypos, fieldSize );
        fprintf( m_Outfile, "F1 \"%s\" 0 %d %d H I C CNN\n", m_CmpName, -Ypos, fieldSize );
        fprintf( m_Outfile, "DRAW\n" );
        break;
    }
}


void BITMAPCONV_INFO::OuputFileEnd()
{
    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        fprintf( m_Outfile, "grestore\n" );
        fprintf( m_Outfile, "%%EOF\n" );
        break;

    case PCBNEW_FMT:
        fprintf( m_Outfile, "$EndMODULE %s\n", m_CmpName );
        fprintf( m_Outfile, "$EndLIBRARY\n" );
        break;

    case EESCHEMA_FMT:
        fprintf( m_Outfile, "ENDDRAW\n" );
        fprintf( m_Outfile, "ENDDEF\n" );
        break;
    }
}

/**
 * Function OuputOnePolygon
 * write one polygon to output file.
 * Polygon coordinates are expected scaled by the polugon extraction function
 */
void BITMAPCONV_INFO::OuputOnePolygon( KPolygon & aPolygon )
{
    unsigned ii;
    KPolyPoint currpoint;

    int   offsetX = (int)( m_PixmapWidth / 2 * m_ScaleX );
    int   offsetY = (int)( m_PixmapHeight / 2 * m_ScaleY );

    KPolyPoint startpoint = *aPolygon.begin();

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        fprintf( m_Outfile, "%d %d moveto\n",
                 startpoint.x(), startpoint.y() );
        for( ii = 1; ii < aPolygon.size(); ii++ )
        {
            currpoint = *(aPolygon.begin() + ii);
            fprintf( m_Outfile, "%d %d lineto\n",
                     currpoint.x(), currpoint.y() );
        }

        fprintf( m_Outfile, "0 setgray fill\n" );
        break;

    case PCBNEW_FMT:
    {
        #define SILKSCREEN_N_FRONT 21
        int layer = SILKSCREEN_N_FRONT;
        int width = 1;
        fprintf( m_Outfile, "DP %d %d %d %d %d %d %d\n",
                 0, 0, 0, 0,
                 (int) aPolygon.size() + 1, width, layer );

        for( ii = 0; ii < aPolygon.size(); ii++ )
        {
            currpoint = *( aPolygon.begin() + ii );
            fprintf( m_Outfile, "Dl %d %d\n",
                    currpoint.x() - offsetX, currpoint.y() - offsetY );
        }

        // Close polygon
        fprintf( m_Outfile, "Dl %d %d\n",
                 startpoint.x() - offsetX, startpoint.y() - offsetY );
    }
    break;

    case EESCHEMA_FMT:
        fprintf( m_Outfile, "P %d 0 0 1", (int) aPolygon.size() + 1 );
        for( ii = 0; ii < aPolygon.size(); ii++ )
        {
            currpoint = *(aPolygon.begin() + ii);
            fprintf( m_Outfile, " %d %d",
                     currpoint.x() - offsetX, currpoint.y() - offsetY );
        }

        // Close polygon
        fprintf( m_Outfile, " %d %d",
                 startpoint.x() - offsetX, startpoint.y() - offsetY );

        fprintf( m_Outfile, " F\n" );
        break;
    }
}


void BITMAPCONV_INFO::CreateOutputFile()
{
    KPolyPoint currpoint;

    std::vector <potrace_dpoint_t> cornersBuffer;

    // This KPolygonSet polyset_areas is a complex polygon to draw
    // and can be complex depending on holes inside this polygon
    KPolygonSet polyset_areas;

    // This KPolygonSet polyset_holes is the set of holes inside polyset_areas
    KPolygonSet polyset_holes;

    potrace_dpoint_t( *c )[3];
    OuputFileHeader();

    bool main_outline = true;

    /* draw each as a polygon with no hole.
     * Bezier curves are approximated by a polyline
     */
    potrace_path_t* paths = m_Paths;    // the list of paths
    while( paths != NULL )
    {
        int cnt  = paths->curve.n;
        int* tag = paths->curve.tag;
        c = paths->curve.c;
        potrace_dpoint_t startpoint = c[cnt - 1][2];
        for( int i = 0; i < cnt; i++ )
        {
            switch( tag[i] )
            {
            case POTRACE_CORNER:
                cornersBuffer.push_back( c[i][1] );
                cornersBuffer.push_back( c[i][2] );
                startpoint = c[i][2];
                break;

            case POTRACE_CURVETO:
                BezierToPolyline( cornersBuffer, startpoint, c[i][0], c[i][1], c[i][2] );
                startpoint = c[i][2];
                break;
            }
        }

        // Store current path
        if( main_outline )
        {
            main_outline = false;

            // build the current main polygon
            std::vector<KPolyPoint> cornerslist; // a simple boost polygon
            for( unsigned int i = 0; i < cornersBuffer.size(); i++ )
            {
                currpoint.x( (coordinate_type) (cornersBuffer[i].x * m_ScaleX) );
                currpoint.y( (coordinate_type) (cornersBuffer[i].y * m_ScaleY) );
                cornerslist.push_back( currpoint );
            }

            KPolygon poly;
            bpl::set_points( poly, cornerslist.begin(), cornerslist.end() );
            polyset_areas.push_back( poly );
        }
        else
        {
            // Add current hole in polyset_holes
            std::vector<KPolyPoint> cornerslist; // a simple boost polygon
            for( unsigned int i = 0; i < cornersBuffer.size(); i++ )
            {
                currpoint.x( (coordinate_type) (cornersBuffer[i].x * m_ScaleX) );
                currpoint.y( (coordinate_type) (cornersBuffer[i].y * m_ScaleY) );
                cornerslist.push_back( currpoint );
            }

            KPolygon poly;
            bpl::set_points( poly, cornerslist.begin(), cornerslist.end() );
            polyset_holes.push_back( poly );
        }
        cornersBuffer.clear();

        /* at the end of a group of a positive path and its negative children, fill.
         */
        if( paths->next == NULL || paths->next->sign == '+' )
        {
            // Substract holes to main polygon:
            polyset_areas -= polyset_holes;

            // Output current resulting polygon(s)
            for( unsigned ii = 0; ii < polyset_areas.size(); ii++ )
            {
                KPolygon& poly = polyset_areas[ii];
                OuputOnePolygon(poly );
            }

            polyset_areas.clear();
            polyset_holes.clear();
            main_outline = true;
        }
        paths = paths->next;
    }

    OuputFileEnd();
}


/* render a Bezier curve. */
void BezierToPolyline( std::vector <potrace_dpoint_t>& aCornersBuffer,
                       potrace_dpoint_t                p1,
                       potrace_dpoint_t                p2,
                       potrace_dpoint_t                p3,
                       potrace_dpoint_t                p4 )
{
    double dd0, dd1, dd, delta, e2, epsilon, t;

    // p1 = starting point

    /* we approximate the curve by small line segments. The interval
     *  size, epsilon, is determined on the fly so that the distance
     *  between the true curve and its approximation does not exceed the
     *  desired accuracy delta. */

    delta = 0.25; /* desired accuracy, in pixels */

    /* let dd = maximal value of 2nd derivative over curve - this must
     *  occur at an endpoint. */
    dd0     = sq( p1.x - 2 * p2.x + p3.x ) + sq( p1.y - 2 * p2.y + p3.y );
    dd1     = sq( p2.x - 2 * p3.x + p4.x ) + sq( p2.y - 2 * p3.y + p4.y );
    dd      = 6 * sqrt( max( dd0, dd1 ) );
    e2      = 8 * delta <= dd ? 8 * delta / dd : 1;
    epsilon = sqrt( e2 ); /* necessary interval size */

    for( t = epsilon; t<1; t += epsilon )
    {
        potrace_dpoint_t intermediate_point;
        intermediate_point.x = p1.x * cu( 1 - t ) +
                               3* p2.x* sq( 1 - t ) * t +
                               3 * p3.x * (1 - t) * sq( t ) +
                               p4.x* cu( t );

        intermediate_point.y = p1.y * cu( 1 - t ) +
                               3* p2.y* sq( 1 - t ) * t +
                               3 * p3.y * (1 - t) * sq( t ) + p4.y* cu( t );

        aCornersBuffer.push_back( intermediate_point );
    }

    aCornersBuffer.push_back( p4 );
}
