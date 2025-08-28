/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 jean-pierre.charras
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

#include <algorithm>    // std::max
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <kiid.h>

#include <build_version.h>
#include <locale_io.h>
#include <macros.h>
#include <potracelib.h>
#include <reporter.h>
#include <fmt/format.h>
#include <wx/translation.h>

#include "bitmap2component.h"


/* free a potrace bitmap */
static void bm_free( potrace_bitmap_t* bm )
{
    if( bm )
        free( bm->map );

    free( bm );
}


static void BezierToPolyline( std::vector <potrace_dpoint_t>& aCornersBuffer,
                              potrace_dpoint_t p1, potrace_dpoint_t p2,
                              potrace_dpoint_t p3, potrace_dpoint_t p4 );


BITMAPCONV_INFO::BITMAPCONV_INFO( std::string& aData, REPORTER& aReporter ):
    m_Data( aData ),
    m_reporter( aReporter )
{
    m_Format = POSTSCRIPT_FMT;
    m_PixmapWidth  = 0;
    m_PixmapHeight = 0;
    m_ScaleX  = 1.0;
    m_ScaleY  = 1.0;
    m_Paths   = nullptr;
    m_CmpName = "LOGO";
}


int BITMAPCONV_INFO::ConvertBitmap( potrace_bitmap_t* aPotrace_bitmap, OUTPUT_FMT_ID aFormat,
                                    int aDpi_X, int aDpi_Y, const wxString& aLayer )
{
    potrace_param_t* param;
    potrace_state_t* st;

    // set tracing parameters, starting from defaults
    param = potrace_param_default();

    if( !param )
    {
        m_reporter.Report( fmt::format( "Error allocating parameters: {}\n", strerror( errno ) ),
                           RPT_SEVERITY_ERROR );
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
            potrace_state_free( st );

        potrace_param_free( param );

        m_reporter.Report( fmt::format( "Error tracing bitmap: {}\n", strerror( errno ) ),
                           RPT_SEVERITY_ERROR );
        return 1;
    }

    m_PixmapWidth  = aPotrace_bitmap->w;
    m_PixmapHeight = aPotrace_bitmap->h;     // the bitmap size in pixels
    m_Paths        = st->plist;
    m_Format       = aFormat;

    switch( aFormat )
    {
    case DRAWING_SHEET_FMT:
        m_ScaleX = PL_IU_PER_MM * 25.4 / aDpi_X;  // the conversion scale from PPI to micron
        m_ScaleY = PL_IU_PER_MM * 25.4 / aDpi_Y;  // Y axis is top to bottom
        createOutputData();
        break;

    case POSTSCRIPT_FMT:
        m_ScaleX = 1.0;                // the conversion scale
        m_ScaleY = m_ScaleX;
        createOutputData();
        break;

    case SYMBOL_FMT:
    case SYMBOL_PASTE_FMT:
        m_ScaleX = SCH_IU_PER_MM * 25.4 / aDpi_X;   // the conversion scale from PPI to eeschema iu
        m_ScaleY = -SCH_IU_PER_MM * 25.4 / aDpi_Y;  // Y axis is bottom to Top for components in libs
        createOutputData();
        break;

    case FOOTPRINT_FMT:
        m_ScaleX = PCB_IU_PER_MM * 25.4 / aDpi_X;   // the conversion scale from PPI to IU
        m_ScaleY = PCB_IU_PER_MM * 25.4 / aDpi_Y;   // Y axis is top to bottom in Footprint Editor
        createOutputData( aLayer );
        break;
    }

    bm_free( aPotrace_bitmap );
    potrace_state_free( st );
    potrace_param_free( param );

    return 0;
}


void BITMAPCONV_INFO::outputDataHeader( const wxString& aBrdLayerName )
{
    double Ypos = ( m_PixmapHeight / 2.0 * m_ScaleY );    // fields Y position in mm
    double fieldSize;             // fields text size in mm

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        m_Data += "%!PS-Adobe-3.0 EPSF-3.0\n";
        m_Data += fmt::format( "%%BoundingBox: 0 0 {} {}\n", m_PixmapWidth, m_PixmapHeight );
        m_Data += "gsave\n";
        break;

    case FOOTPRINT_FMT:
        // fields text size = 1.5 mm
        // fields text thickness = 1.5 / 5 = 0.3mm
        m_Data += fmt::format( "(footprint \"{}\" (version 20221018) (generator \"bitmap2component\") (generator_version \"{}\")\n"
                               "  (layer \"F.Cu\")\n",
                               m_CmpName.c_str(),
                               GetMajorMinorVersion().ToStdString() );

        m_Data += fmt::format( "  (attr board_only exclude_from_pos_files exclude_from_bom)\n" );
        m_Data += fmt::format( "  (fp_text reference \"G***\" (at 0 0) (layer \"{}\")\n"
                               "      (effects (font (size 1.5 1.5) (thickness 0.3)))\n"
                               "    (uuid {})\n  )\n",
                               aBrdLayerName.ToStdString().c_str(),
                               KIID().AsString().ToStdString().c_str() );

        m_Data += fmt::format( "  (fp_text value \"{}\" (at 0.75 0) (layer \"{}\") hide\n"
                               "      (effects (font (size 1.5 1.5) (thickness 0.3)))\n"
                               "    (uuid {})\n  )\n",
                               m_CmpName.c_str(),
                               aBrdLayerName.ToStdString().c_str(),
                               KIID().AsString().ToStdString().c_str() );
        break;

    case DRAWING_SHEET_FMT:
        m_Data += fmt::format( "(kicad_wks (version 20220228) (generator \"bitmap2component\") (generator_version \"{}\")\n",
                               GetMajorMinorVersion().ToStdString() );
        m_Data += "  (setup (textsize 1.5 1.5)(linewidth 0.15)(textlinewidth 0.15)\n";
        m_Data += "  (left_margin 10)(right_margin 10)(top_margin 10)(bottom_margin 10))\n";
        m_Data += "  (polygon (name \"\") (pos 0 0) (linewidth 0.01)\n";
        break;

    case SYMBOL_FMT:
        m_Data += fmt::format( "(kicad_symbol_lib (version 20220914) (generator \"bitmap2component\") (generator_version \"{}\")\n",
                               GetMajorMinorVersion().ToStdString() );
        KI_FALLTHROUGH;

    case SYMBOL_PASTE_FMT:
        fieldSize = 1.27;             // fields text size in mm (= 50 mils)
        Ypos /= SCH_IU_PER_MM;
        Ypos += fieldSize / 2;
        m_Data += fmt::format( "  (symbol \"{}\" (pin_names (offset 1.016)) (in_bom yes) (on_board yes)\n",
                               m_CmpName.c_str() );

        m_Data += fmt::format( "    (property \"Reference\" \"#G\" (at 0 {:g} 0)\n"
                               "      (effects (font (size {:g} {:g})) hide)\n    )\n",
                               -Ypos,
                               fieldSize,
                               fieldSize );

        m_Data += fmt::format( "    (property \"Value\" \"{}\" (at 0 {:g} 0)\n"
                               "      (effects (font (size {:g} {:g})) hide)\n    )\n",
                               m_CmpName.c_str(),
                               Ypos,
                               fieldSize,
                               fieldSize );

        m_Data += fmt::format( "    (property \"Footprint\" \"\" (at 0 0 0)\n"
                               "      (effects (font (size {:g} {:g})) hide)\n    )\n",
                               fieldSize,
                               fieldSize );

        m_Data += fmt::format( "    (property \"Datasheet\" \"\" (at 0 0 0)\n"
                               "      (effects (font (size {:g} {:g})) hide)\n    )\n",
                               fieldSize,
                               fieldSize );

        m_Data += fmt::format( "    (symbol \"{}_0_0\"\n", m_CmpName.c_str() );
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

    case FOOTPRINT_FMT:
        m_Data += ")\n";
        break;

    case DRAWING_SHEET_FMT:
        m_Data += "  )\n)\n";
        break;

    case SYMBOL_PASTE_FMT:
        m_Data += "    )\n";        // end symbol_0_0
        m_Data += "  )\n";          // end symbol
        break;

    case SYMBOL_FMT:
        m_Data += "    )\n";        // end symbol_0_0
        m_Data += "  )\n";          // end symbol
        m_Data += ")\n";            // end lib
        break;
    }
}


void BITMAPCONV_INFO::outputOnePolygon( SHAPE_LINE_CHAIN& aPolygon, const wxString& aBrdLayerName )
{
    // write one polygon to output file.
    // coordinates are expected in target unit.
    int      ii, jj;
    VECTOR2I currpoint;
    int      offsetX = KiROUND( m_PixmapWidth / 2.0 * m_ScaleX );
    int      offsetY = KiROUND( m_PixmapHeight / 2.0 * m_ScaleY );

    const VECTOR2I startpoint = aPolygon.CPoint( 0 );

    switch( m_Format )
    {
    case POSTSCRIPT_FMT:
        offsetY = (int)( m_PixmapHeight * m_ScaleY );
        m_Data += fmt::format( "newpath\n{} {} moveto\n", startpoint.x, offsetY - startpoint.y );
        jj = 0;

        for( ii = 1; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            m_Data += fmt::format( " {} {} lineto", currpoint.x, offsetY - currpoint.y );

            if( jj++ > 6 )
            {
                jj = 0;
                m_Data += "\n";
            }
        }

        m_Data += "\nclosepath fill\n";
        break;

    case FOOTPRINT_FMT:
        m_Data += "  (fp_poly\n    (pts\n";

        jj = 0;

        for( ii = 0; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            m_Data += fmt::format( "      (xy {} {})\n",
                                   ( currpoint.x - offsetX ) / PCB_IU_PER_MM,
                                   ( currpoint.y - offsetY ) / PCB_IU_PER_MM );
        }
        // No need to close polygon

        m_Data += "    )\n\n";
        m_Data += fmt::format( "    (stroke (width {:f}) (type solid)) (fill solid) (layer \"{}\") (uuid {}))\n",
                               0.0,
                               aBrdLayerName.ToStdString().c_str(),
                               KIID().AsString().ToStdString().c_str() );
        break;

    case DRAWING_SHEET_FMT:
        m_Data += "    (pts";

        // Internal units = micron, file unit = mm
        jj = 1;

        for( ii = 0; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            m_Data += fmt::format( " (xy {:.3f} {:.3f})",
                                   ( currpoint.x - offsetX ) / PL_IU_PER_MM,
                                   ( currpoint.y - offsetY ) / PL_IU_PER_MM );

            if( jj++ > 4 )
            {
                jj = 0;
                m_Data += "\n     ";
            }
        }

        // Close polygon
        m_Data += fmt::format( " (xy {:.3f} {:.3f}) )\n",
                              ( startpoint.x - offsetX ) / PL_IU_PER_MM,
                              ( startpoint.y - offsetY ) / PL_IU_PER_MM );
        break;

    case SYMBOL_FMT:
    case SYMBOL_PASTE_FMT:
        // The polygon outline thickness is fixed here to 0.01  ( 0.0 is the default thickness)
#define SCH_LINE_THICKNESS_MM 0.01
        //snprintf( strbuf, sizeof(strbuf), "P %d 0 0 %d", (int) aPolygon.PointCount() + 1, EE_LINE_THICKNESS );
        m_Data += "      (polyline\n        (pts\n";

        for( ii = 0; ii < aPolygon.PointCount(); ii++ )
        {
            currpoint = aPolygon.CPoint( ii );
            m_Data += fmt::format( "          (xy {:f} {:f})\n",
                                   ( currpoint.x - offsetX ) / SCH_IU_PER_MM,
                                   ( currpoint.y - offsetY ) / SCH_IU_PER_MM );
        }

        // Close polygon
        m_Data += fmt::format( "          (xy {:f} {:f})\n",
                               ( startpoint.x - offsetX ) / SCH_IU_PER_MM,
                               ( startpoint.y - offsetY ) / SCH_IU_PER_MM );
        m_Data += "        )\n";        // end pts

        m_Data += fmt::format( "        (stroke (width {:g}) (type default))\n"
                               "        (fill (type outline))\n",
                               SCH_LINE_THICKNESS_MM );
        m_Data += "      )\n"; // end polyline
        break;
    }
}


void BITMAPCONV_INFO::createOutputData( const wxString& aLayer )
{
    std::vector <potrace_dpoint_t> cornersBuffer;

    // polyset_areas is a set of polygon to draw
    SHAPE_POLY_SET polyset_areas;

    // polyset_holes is the set of holes inside polyset_areas outlines
    SHAPE_POLY_SET polyset_holes;

    potrace_dpoint_t( *c )[3];

    // The layer name has meaning only for .kicad_mod files.
    // For these files the header creates 2 invisible texts: value and ref
    // (needed but not useful) on silk screen layer
    outputDataHeader( "F.SilkS" );

    bool main_outline = true;

    /* draw each as a polygon with no hole.
     * Bezier curves are approximated by a polyline
     */
    potrace_path_t* paths = m_Paths;    // the list of paths

    if( !m_Paths )
    {
        m_reporter.Report( _( "No shape in black and white image to convert: no outline created." ),
                           RPT_SEVERITY_ERROR );
    }

    while( paths != nullptr )
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

            for( const potrace_dpoint_s& pt : cornersBuffer )
            {
                polyset_areas.Append( int( pt.x * m_ScaleX ),
                                      int( pt.y * m_ScaleY ) );
            }
        }
        else
        {
            // Add current hole in polyset_holes
            polyset_holes.NewOutline();

            for( const potrace_dpoint_s& pt : cornersBuffer )
            {
                polyset_holes.Append( int( pt.x * m_ScaleX ),
                                      int( pt.y * m_ScaleY ) );
            }
        }

        cornersBuffer.clear();

        // at the end of a group of a positive path and its negative children, fill.
        if( paths->next == nullptr || paths->next->sign == '+' )
        {
            polyset_areas.Simplify();
            polyset_holes.Simplify();
            polyset_areas.BooleanSubtract( polyset_holes );

            // Ensure there are no self intersecting polygons
            if( polyset_areas.NormalizeAreaOutlines() )
            {
                // Convert polygon with holes to a unique polygon
                polyset_areas.Fracture();

                // Output current resulting polygon(s)
                for( int ii = 0; ii < polyset_areas.OutlineCount(); ii++ )
                {
                    SHAPE_LINE_CHAIN& poly = polyset_areas.Outline( ii );
                    outputOnePolygon( poly, aLayer );
                }

                polyset_areas.RemoveAllContours();
                polyset_holes.RemoveAllContours();
                main_outline = true;
            }
        }

        paths = paths->next;
    }

    outputDataEnd();
}

// a helper function to calculate a square value
inline double square( double x )
{
    return x * x;
}


// a helper function to calculate a cube value
inline double cube( double x )
{
    return x * x * x;
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
