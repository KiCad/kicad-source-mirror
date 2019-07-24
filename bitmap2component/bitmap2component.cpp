/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 jean-pierre.charras
 * Copyright (C) 1992-2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>    // std::max
#include <cmath>
#include <string>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <common.h>
#include <layers_id_colors_and_visibility.h>

#include <potracelib.h>

#include "bitmap2component.h"

// Unit conversion. Coord unit from potrace is mm
#define MM2MICRON 1e3       // For pl_editor
#define MM2NANOMETER 1e6    // For pcbew

/* free a potrace bitmap */
static void bm_free( potrace_bitmap_t* bm )
{
    if( bm != NULL )
    {
        free( bm->map );
    }
    free( bm );
}


static void BezierToPolyline( std::vector <potrace_dpoint_t>& aCornersBuffer,
                              potrace_dpoint_t                p1,
                              potrace_dpoint_t                p2,
                              potrace_dpoint_t                p3,
                              potrace_dpoint_t                p4 );


BITMAPCONV_INFO::BITMAPCONV_INFO( std::string& aData ):
    m_Data( aData )
{
    m_Format = POSTSCRIPT_FMT;
    m_PixmapWidth  = 0;
    m_PixmapHeight = 0;
    m_ScaleX  = 1.0;
    m_ScaleY  = 1.0;
    m_Paths   = NULL;
    m_CmpName = "LOGO";
}


int BITMAPCONV_INFO::ConvertBitmap( potrace_bitmap_t* aPotrace_bitmap,
                      OUTPUT_FMT_ID aFormat, int aDpi_X, int aDpi_Y,
                      BMP2CMP_MOD_LAYER aModLayer )
{
    potrace_param_t* param;
    potrace_state_t* st;

    // set tracing parameters, starting from defaults
    param = potrace_param_default();

    if( !param )
    {
        char msg[256];
        sprintf( msg, "Error allocating parameters: %s\n", strerror( errno ) );
        m_errors += msg;
        return 1;
    }

    // For parameters: see http://potrace.sourceforge.net/potracelib.pdf
    param->turdsize = 0;        // area (in pixels) of largest path to be ignored.
                                // Potrace default is 2
    param->opttolerance = 0.2;  // curve optimization tolerance. Potrace default is 0.2

    /* convert the bitmap to curves */
    st = potrace_trace( param, aPotrace_bitmap );

    if( !st || st->status != POTRACE_STATUS_OK )
    {
        if( st )
        {
            potrace_state_free( st );
        }
        potrace_param_free( param );

        char msg[256];
        sprintf( msg, "Error tracing bitmap: %s\n", strerror( errno ) );
        m_errors += msg;
        return 1;
    }

    m_PixmapWidth  = aPotrace_bitmap->w;
    m_PixmapHeight = aPotrace_bitmap->h;     // the bitmap size in pixels
    m_Paths   = st->plist;

    switch( aFormat )
    {
    case KICAD_LOGO:
        m_Format = KICAD_LOGO;
        m_ScaleX = MM2MICRON * 25.4 / aDpi_X;  // the conversion scale from PPI to micron
        m_ScaleY = MM2MICRON * 25.4 / aDpi_Y;  // Y axis is top to bottom
        createOutputData();
        break;

    case POSTSCRIPT_FMT:
        m_Format = POSTSCRIPT_FMT;
        m_ScaleX = 1.0;                // the conversion scale
        m_ScaleY = m_ScaleX;
        // output vector data, e.g. as a rudimentary EPS file (mainly for tests)
        createOutputData();
        break;

    case EESCHEMA_FMT:
        m_Format = EESCHEMA_FMT;
        m_ScaleX = 1000.0 / aDpi_X;       // the conversion scale from PPI to UI (mil)
        m_ScaleY = -1000.0 / aDpi_Y;      // Y axis is bottom to Top for components in libs
        createOutputData();
        break;

    case PCBNEW_KICAD_MOD:
        m_Format = PCBNEW_KICAD_MOD;
        m_ScaleX = MM2NANOMETER * 25.4 / aDpi_X;   // the conversion scale from PPI to UI
        m_ScaleY = MM2NANOMETER * 25.4 / aDpi_Y;   // Y axis is top to bottom in modedit
        createOutputData( aModLayer );
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


void BITMAPCONV_INFO::outputDataHeader(  const char * aBrdLayerName )
{
    int Ypos = (int) ( m_PixmapHeight / 2 * m_ScaleY );
    int fieldSize;             // fields text size = 60 mils
    char strbuf[1024];

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        /* output vector data, e.g. as a rudimentary EPS file */
        m_Data += "%%!PS-Adobe-3.0 EPSF-3.0\n";
        sprintf( strbuf, "%%%%BoundingBox: 0 0 %d %d\n", m_PixmapWidth, m_PixmapHeight );
        m_Data += strbuf;
        m_Data += "gsave\n";
        break;

    case PCBNEW_KICAD_MOD:
        // fields text size = 1.5 mm
        // fields text thickness = 1.5 / 5 = 0.3mm
        sprintf( strbuf, "(module %s (layer F.Cu)\n  (at 0 0)\n", m_CmpName.c_str() );
        m_Data += strbuf;
        sprintf( strbuf, " (fp_text reference \"G***\" (at 0 0) (layer %s)\n"
            "  (effects (font (thickness 0.3)))\n  )\n", aBrdLayerName );
        m_Data += strbuf;
        sprintf( strbuf, "  (fp_text value \"%s\" (at 0.75 0) (layer %s) hide\n"
            "  (effects (font (thickness 0.3)))\n  )\n", m_CmpName.c_str(), aBrdLayerName );
        m_Data += strbuf;
        break;

    case KICAD_LOGO:
        m_Data += "(polygon (pos 0 0 rbcorner) (rotate 0) (linewidth 0.01)\n";
        break;

    case EESCHEMA_FMT:
        sprintf( strbuf, "EESchema-LIBRARY Version 2.3\n" );
        m_Data += strbuf;
        sprintf( strbuf, "#\n# %s\n", m_CmpName.c_str() );
        m_Data += strbuf;
        sprintf( strbuf, "# pixmap size w = %d, h = %d\n#\n",
                 m_PixmapWidth, m_PixmapHeight );
        m_Data += strbuf;

        // print reference and value
        fieldSize = 50;             // fields text size = 50 mils
        Ypos += fieldSize / 2;
        sprintf( strbuf, "DEF %s G 0 40 Y Y 1 F N\n", m_CmpName.c_str() );
        m_Data += strbuf;
        sprintf( strbuf, "F0 \"#G\" 0 %d %d H I C CNN\n", Ypos, fieldSize );
        m_Data += strbuf;
        sprintf( strbuf, "F1 \"%s\" 0 %d %d H I C CNN\n", m_CmpName.c_str(), -Ypos, fieldSize );
        m_Data += strbuf;
        m_Data += "DRAW\n";
        break;
    }
}


void BITMAPCONV_INFO::outputDataEnd()
{
    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        m_Data += "grestore\n";
        m_Data += "%%EOF\n";
        break;

    case PCBNEW_KICAD_MOD:
        m_Data += ")\n";
        break;

    case KICAD_LOGO:
        m_Data += ")\n";
        break;

    case EESCHEMA_FMT:
        m_Data += "ENDDRAW\n";
        m_Data += "ENDDEF\n";
        break;
    }
}


void BITMAPCONV_INFO::ouputOnePolygon( SHAPE_LINE_CHAIN & aPolygon, const char* aBrdLayerName )
{
    // write one polygon to output file.
    // coordinates are expected in target unit.
    int ii, jj;
    VECTOR2I currpoint;
    char strbuf[1024];

    int   offsetX = (int)( m_PixmapWidth / 2 * m_ScaleX );
    int   offsetY = (int)( m_PixmapHeight / 2 * m_ScaleY );

    const VECTOR2I startpoint = aPolygon.CPoint( 0 );

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        offsetY = (int)( m_PixmapHeight * m_ScaleY );
        sprintf( strbuf, "newpath\n%d %d moveto\n",
                 startpoint.x, offsetY - startpoint.y );
        m_Data += strbuf;
        jj = 0;
        for( ii = 1; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            sprintf( strbuf, " %d %d lineto",
                     currpoint.x, offsetY - currpoint.y );
            m_Data += strbuf;

            if( jj++ > 6 )
            {
                jj = 0;
                m_Data += "\n";
            }
        }

        m_Data += "\nclosepath fill\n";
        break;

    case PCBNEW_KICAD_MOD:
    {
        double width = 0.0;         // outline thickness in mm: no thickness
        m_Data += "  (fp_poly (pts";

        jj = 0;
        for( ii = 0; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            sprintf( strbuf, " (xy %f %f)",
                    ( currpoint.x - offsetX ) / MM2NANOMETER,
                    ( currpoint.y - offsetY ) / MM2NANOMETER );
            m_Data += strbuf;

            if( jj++ > 6 )
            {
                jj = 0;
                m_Data += "\n    ";
            }
        }
        // No need to close polygon
        m_Data += " )";
        sprintf( strbuf, "(layer %s) (width  %f)\n  )\n", aBrdLayerName, width );
        m_Data += strbuf;
    }
    break;

    case KICAD_LOGO:
        m_Data += "  (pts";
        // Internal units = micron, file unit = mm
        jj = 0;
        for( ii = 0; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            sprintf( strbuf, " (xy %.3f %.3f)",
                    ( currpoint.x - offsetX ) / MM2MICRON,
                    ( currpoint.y - offsetY ) / MM2MICRON );
            m_Data += strbuf;

            if( jj++ > 4 )
            {
                jj = 0;
                m_Data += "\n    ";
            }
        }
        // Close polygon
        sprintf( strbuf, " (xy %.3f %.3f) )\n",
                 ( startpoint.x - offsetX ) / MM2MICRON,
                 ( startpoint.y - offsetY ) / MM2MICRON );
        m_Data += strbuf;
        break;

    case EESCHEMA_FMT:
        // The polygon outline thickness is fixed here to 1 mil, the minimal
        // value in Eeschema (0 means use default thickness for graphics)
        #define EE_LINE_THICKNESS 1
        sprintf( strbuf, "P %d 0 0 %d",
                 (int) aPolygon.PointCount() + 1, EE_LINE_THICKNESS );
        m_Data += strbuf;
        for( ii = 0; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            sprintf( strbuf, " %d %d",
                     currpoint.x - offsetX, currpoint.y - offsetY );
            m_Data += strbuf;
        }

        // Close polygon
        sprintf( strbuf, " %d %d",
                 startpoint.x - offsetX, startpoint.y - offsetY );
        m_Data += strbuf;

        m_Data += " F\n";
        break;
    }
}


void BITMAPCONV_INFO::createOutputData( BMP2CMP_MOD_LAYER aModLayer )
{
    std::vector <potrace_dpoint_t> cornersBuffer;

    // polyset_areas is a set of polygon to draw
    SHAPE_POLY_SET polyset_areas;

    // polyset_holes is the set of holes inside polyset_areas outlines
    SHAPE_POLY_SET polyset_holes;

    potrace_dpoint_t( *c )[3];

    LOCALE_IO toggle;   // Temporary switch the locale to standard C to r/w floats

    // The layer name has meaning only for .kicad_mod files.
    // For these files the header creates 2 invisible texts: value and ref
    // (needed but not usefull) on silk screen layer
    outputDataHeader( getBrdLayerName( MOD_LYR_FSILKS ) );

    bool main_outline = true;

    /* draw each as a polygon with no hole.
     * Bezier curves are approximated by a polyline
     */
    potrace_path_t* paths = m_Paths;    // the list of paths

    if(!m_Paths)
    {
        m_errors += "No path in black and white image: no outline created\n";
    }

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
            polyset_areas.NewOutline();
            for( unsigned int i = 0; i < cornersBuffer.size(); i++ )
            {
                polyset_areas.Append( int( cornersBuffer[i].x * m_ScaleX ),
                                      int( cornersBuffer[i].y * m_ScaleY ) );
            }
        }
        else
        {
            // Add current hole in polyset_holes
            polyset_holes.NewOutline();
            for( unsigned int i = 0; i < cornersBuffer.size(); i++ )
            {
                polyset_holes.Append( int( cornersBuffer[i].x * m_ScaleX ),
                                      int( cornersBuffer[i].y * m_ScaleY ) );
            }
        }

        cornersBuffer.clear();

        /* at the end of a group of a positive path and its negative children, fill.
         */
        if( paths->next == NULL || paths->next->sign == '+' )
        {
            polyset_areas.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
            polyset_holes.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
            polyset_areas.BooleanSubtract( polyset_holes, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            // Ensure there are no self intersecting polygons
            polyset_areas.NormalizeAreaOutlines();

            // Convert polygon with holes to a unique polygon
            polyset_areas.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

            // Output current resulting polygon(s)
            for( int ii = 0; ii < polyset_areas.OutlineCount(); ii++ )
            {
                SHAPE_LINE_CHAIN& poly = polyset_areas.Outline( ii );
                ouputOnePolygon(poly, getBrdLayerName( aModLayer ) );
            }

            polyset_areas.RemoveAllContours();
            polyset_holes.RemoveAllContours();
            main_outline = true;
        }
        paths = paths->next;
    }

    outputDataEnd();
}

// a helper function to calculate a square value
inline double square( double x )
{
    return x*x;
}

// a helper function to calculate a cube value
inline double cube( double x )
{
    return x*x*x;
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
    dd0     = square( p1.x - 2 * p2.x + p3.x ) + square( p1.y - 2 * p2.y + p3.y );
    dd1     = square( p2.x - 2 * p3.x + p4.x ) + square( p2.y - 2 * p3.y + p4.y );
    dd      = 6 * sqrt( std::max( dd0, dd1 ) );
    e2      = 8 * delta <= dd ? 8 * delta / dd : 1;
    epsilon = sqrt( e2 ); /* necessary interval size */

    for( t = epsilon; t<1; t += epsilon )
    {
        potrace_dpoint_t intermediate_point;
        intermediate_point.x = p1.x * cube( 1 - t ) +
                               3* p2.x* square( 1 - t ) * t +
                               3 * p3.x * (1 - t) * square( t ) +
                               p4.x* cube( t );

        intermediate_point.y = p1.y * cube( 1 - t ) +
                               3* p2.y* square( 1 - t ) * t +
                               3 * p3.y * (1 - t) * square( t ) + p4.y* cube( t );

        aCornersBuffer.push_back( intermediate_point );
    }

    aCornersBuffer.push_back( p4 );
}
