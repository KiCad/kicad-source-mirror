/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jean-pierre.charras
 * Copyright (C) 1992-2013 Kicad Developers, see change_log.txt for contributors.
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
#include <cmath>
#include <algorithm>    // std::max

// For some unknown reasons, polygon.hpp should be included first
#include <boost/polygon/polygon.hpp>

#include <wx/wx.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <layers_id_colors_and_visibility.h>
#include <potracelib.h>
#include <auxiliary.h>

#include <bitmap2component.h>


// Define some types used here from boost::polygon
namespace bpl = boost::polygon;         // bpl = boost polygon library
using namespace bpl::operators;         // +, -, =, ...

typedef int                                coordinate_type;

typedef bpl::polygon_data<coordinate_type> KPolygon;        // define a basic polygon
typedef std::vector<KPolygon>              KPolygonSet;     // define a set of polygons

typedef bpl::point_data<coordinate_type>   KPolyPoint;      // define a corner of a polygon

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
    enum OUTPUT_FMT_ID m_Format; // File format
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
    void CreateOutputFile( BMP2CMP_MOD_LAYER aModLayer = (BMP2CMP_MOD_LAYER) 0 );


private:
    /**
     * Function OuputFileHeader
     * write to file the header depending on file format
     */
    void OuputFileHeader(  const char * aBrdLayerName );

    /**
     * Function OuputFileEnd
     * write to file the last strings depending on file format
     */
    void OuputFileEnd();


    /**
     * @return the board layer name depending on the board layer selected
     * @param aChoice = the choice (MOD_LYR_FSILKS to MOD_LYR_FINAL)
     */
    const char * getBrdLayerName( BMP2CMP_MOD_LAYER aChoice );

    /**
     * Function OuputOnePolygon
     * write one polygon to output file.
     * Polygon coordinates are expected scaled by the polugon extraction function
     */
    void OuputOnePolygon( KPolygon & aPolygon, const char * aBrdLayerName );

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


int bitmap2component( potrace_bitmap_t* aPotrace_bitmap, FILE* aOutfile,
                      OUTPUT_FMT_ID aFormat, int aDpi_X, int aDpi_Y,
                      BMP2CMP_MOD_LAYER aModLayer )
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
    case KICAD_LOGO:
        info.m_Format = KICAD_LOGO;
        info.m_ScaleX = 1e3 * 25.4 / aDpi_X;       // the conversion scale from PPI to micro
        info.m_ScaleY = 1e3 * 25.4 / aDpi_Y;       // Y axis is top to bottom
        info.CreateOutputFile();
        break;

    case POSTSCRIPT_FMT:
        info.m_Format = POSTSCRIPT_FMT;
        info.m_ScaleX = 1.0;                // the conversion scale
        info.m_ScaleY = info.m_ScaleX;
        // output vector data, e.g. as a rudimentary EPS file (mainly for tests)
        info.CreateOutputFile();
        break;

    case EESCHEMA_FMT:
        info.m_Format = EESCHEMA_FMT;
        info.m_ScaleX = 1000.0 / aDpi_X;       // the conversion scale from PPI to UI
        info.m_ScaleY = -1000.0 / aDpi_Y;      // Y axis is bottom to Top for components in libs
        info.CreateOutputFile();
        break;

    case PCBNEW_KICAD_MOD:
        info.m_Format = PCBNEW_KICAD_MOD;
        info.m_ScaleX = 1e6 * 25.4 / aDpi_X;       // the conversion scale from PPI to UI
        info.m_ScaleY = 1e6 * 25.4 / aDpi_Y;       // Y axis is top to bottom in modedit
        info.CreateOutputFile( aModLayer );
        break;

    default:
        break;
    }


    bm_free( aPotrace_bitmap );
    potrace_state_free( st );
    potrace_param_free( param );

    return 0;
}

const char* BITMAPCONV_INFO::getBrdLayerName( BMP2CMP_MOD_LAYER aChoice )
{
    const char * layerName = "F.SilkS";

    switch( aChoice )
    {
    case MOD_LYR_FSOLDERMASK:
        layerName = "F.Mask";
        break;

    case MOD_LYR_ECO1:
        layerName = "Eco1.User";
        break;

    case MOD_LYR_ECO2:
        layerName = "Eco2.User";
        break;

    case MOD_LYR_FSILKS:
    default:    // case MOD_LYR_FSILKS only unless there is a bug
        break;
    }

    return layerName;
}

void BITMAPCONV_INFO::OuputFileHeader(  const char * aBrdLayerName )
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

    case PCBNEW_KICAD_MOD:
        // fields text size = 1.5 mm
        // fields text thickness = 1.5 / 5 = 0.3mm
        fprintf( m_Outfile, "(module %s (layer F.Cu)\n  (at 0 0)\n",
                 m_CmpName );
        fprintf( m_Outfile, " (fp_text reference \"G***\" (at 0 0) (layer %s) hide\n"
            "  (effects (font (thickness 0.3)))\n  )\n", aBrdLayerName );
        fprintf( m_Outfile, "  (fp_text value \"%s\" (at 0.75 0) (layer %s) hide\n"
            "  (effects (font (thickness 0.3)))\n  )\n", m_CmpName, aBrdLayerName );
        break;

    case KICAD_LOGO:
        fprintf( m_Outfile, "(polygon (pos 0 0 rbcorner) (rotate 0) (linewidth 0.01)\n" );
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

    case PCBNEW_KICAD_MOD:
        fprintf( m_Outfile, ")\n" );
        break;

    case KICAD_LOGO:
        fprintf( m_Outfile, ")\n" );
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
 * Polygon coordinates are expected scaled by the polygon extraction function
 */
void BITMAPCONV_INFO::OuputOnePolygon( KPolygon & aPolygon, const char * aBrdLayerName )
{
    unsigned ii, jj;
    KPolyPoint currpoint;

    int   offsetX = (int)( m_PixmapWidth / 2 * m_ScaleX );
    int   offsetY = (int)( m_PixmapHeight / 2 * m_ScaleY );

    KPolyPoint startpoint = *aPolygon.begin();

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        offsetY = (int)( m_PixmapHeight * m_ScaleY );
        fprintf( m_Outfile, "newpath\n%d %d moveto\n",
                 startpoint.x(), offsetY - startpoint.y() );
        jj = 0;
        for( ii = 1; ii < aPolygon.size(); ii++ )
        {
            currpoint = *(aPolygon.begin() + ii);
            fprintf( m_Outfile, " %d %d lineto",
                     currpoint.x(), offsetY - currpoint.y() );

            if( jj++ > 6 )
            {
                jj = 0;
                fprintf( m_Outfile, ("\n") );
            }
        }

        fprintf( m_Outfile, "\nclosepath fill\n" );
        break;

    case PCBNEW_KICAD_MOD:
    {
        double width = 0.1;
        fprintf( m_Outfile, "  (fp_poly (pts" );

        jj = 0;
        for( ii = 0; ii < aPolygon.size(); ii++ )
        {
            currpoint = *( aPolygon.begin() + ii );
            fprintf( m_Outfile, " (xy %f %f)",
                    (currpoint.x() - offsetX) / 1e6,
                    (currpoint.y() - offsetY) / 1e6 );

            if( jj++ > 6 )
            {
                jj = 0;
                fprintf( m_Outfile, ("\n    ") );
            }
        }
        // Close polygon
        fprintf( m_Outfile, " (xy %f %f) )",
                (startpoint.x() - offsetX) / 1e6, (startpoint.y() - offsetY) / 1e6 );

        fprintf( m_Outfile, "(layer %s) (width  %f)\n  )\n", aBrdLayerName, width );

    }
    break;

    case KICAD_LOGO:
        fprintf( m_Outfile, "  (pts" );
        // Internal units = micron, file unit = mm
        jj = 0;
        for( ii = 0; ii < aPolygon.size(); ii++ )
        {
            currpoint = *( aPolygon.begin() + ii );
            fprintf( m_Outfile, " (xy %.3f %.3f)",
                    (currpoint.x() - offsetX) / 1e3,
                    (currpoint.y() - offsetY) / 1e3 );

            if( jj++ > 4 )
            {
                jj = 0;
                fprintf( m_Outfile, ("\n    ") );
            }
        }
        // Close polygon
        fprintf( m_Outfile, " (xy %.3f %.3f) )\n",
                (startpoint.x() - offsetX) / 1e3, (startpoint.y() - offsetY) / 1e3 );
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


void BITMAPCONV_INFO::CreateOutputFile( BMP2CMP_MOD_LAYER aModLayer )
{
    KPolyPoint currpoint;

    std::vector <potrace_dpoint_t> cornersBuffer;

    // This KPolygonSet polyset_areas is a complex polygon to draw
    // and can be complex depending on holes inside this polygon
    KPolygonSet polyset_areas;

    // This KPolygonSet polyset_holes is the set of holes inside polyset_areas
    KPolygonSet polyset_holes;

    potrace_dpoint_t( *c )[3];

    setlocale( LC_NUMERIC, "C" );    // Switch the locale to standard C

    // The layer name has meaning only for .kicad_mod files.
    // For these files the header creates 2 invisible texts: value and ref
    // (needed but not usefull) on silk screen layer
    OuputFileHeader( getBrdLayerName( MOD_LYR_FSILKS ) );

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
                OuputOnePolygon(poly, getBrdLayerName( aModLayer ) );
            }

            polyset_areas.clear();
            polyset_holes.clear();
            main_outline = true;
        }
        paths = paths->next;
    }

    OuputFileEnd();

    setlocale( LC_NUMERIC, "" );      // revert to the current locale
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
