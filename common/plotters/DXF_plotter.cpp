/**
 * @file DXF_plotter.cpp
 * @brief Kicad: specialized plotter for DXF files format
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <ranges>
#include <plotters/plotter_dxf.h>
#include <macros.h>
#include <string_utils.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_rect.h>
#include <trigo.h>
#include <fmt/core.h>

/**
 * Oblique angle for DXF native text
 * (I don't remember if 15 degrees is the ISO value... it looks nice anyway)
 */
static const double DXF_OBLIQUE_ANGLE = 15;

// No support for linewidths in DXF
#define DXF_LINE_WIDTH DO_NOT_SET_LINE_WIDTH

/**
 * The layer/colors palette.
 *
 * The acad/DXF palette is divided in 3 zones:
 *
 *  - The primary colors (1 - 9)
 *  - An HSV zone (10-250, 5 values x 2 saturations x 10 hues
 *  - Greys (251 - 255)

 * There is *no* black... the white does it on paper, usually, and anyway it depends on the
 * plotter configuration, since DXF colors are meant to be logical only (they represent *both*
 * line color and width); later version with plot styles only complicate the matter!
 *
 * As usual, brown and magenta/purple are difficult to place since they are actually variations
 * of other colors.
 */
static const struct
{
    const char *name;
    int color;
} dxf_layer[NBCOLORS] =
{
    { "BLACK",      7 },    // In DXF, color 7 is *both* white and black!
    { "GRAY1",      251 },
    { "GRAY2",      8 },
    { "GRAY3",      9 },
    { "WHITE",      7 },
    { "LYELLOW",    51 },
    { "LORANGE",    41 },
    { "BLUE1",      178 },
    { "GREEN1",     98 },
    { "CYAN1",      138 },
    { "RED1",       18 },
    { "MAGENTA1",   228 },
    { "BROWN1",     58 },
    { "ORANGE1",    34 },
    { "BLUE2",      5 },
    { "GREEN2",     3 },
    { "CYAN2",      4 },
    { "RED2",       1 },
    { "MAGENTA2",   6 },
    { "BROWN2",     54 },
    { "ORANGE2",    42 },
    { "BLUE3",      171 },
    { "GREEN3",     91 },
    { "CYAN3",      131 },
    { "RED3",       11 },
    { "MAGENTA3",   221 },
    { "YELLOW3",    2 },
    { "ORANGE3",    32 },
    { "BLUE4",      5 },
    { "GREEN4",     3 },
    { "CYAN4",      4 },
    { "RED4",       1 },
    { "MAGENTA4",   6 },
    { "YELLOW4",    2 },
    { "ORANGE4",    40 }
};


static const char* getDXFLineType( LINE_STYLE aType )
{
    switch( aType )
    {
    case LINE_STYLE::DEFAULT:
    case LINE_STYLE::SOLID:
        return "CONTINUOUS";
    case LINE_STYLE::DASH:
        return "DASHED";
    case LINE_STYLE::DOT:
        return "DOTTED";
    case LINE_STYLE::DASHDOT:
        return "DASHDOT";
    case LINE_STYLE::DASHDOTDOT:
        return "DIVIDE";
    default:
        wxFAIL_MSG( "Unhandled LINE_STYLE" );
        return "CONTINUOUS";
    }
}


// A helper function to create a color name acceptable in DXF files
// DXF files do not use a RGB definition
static wxString getDXFColorName( const COLOR4D& aColor )
{
    EDA_COLOR_T color = COLOR4D::FindNearestLegacyColor( int( aColor.r * 255 ),
                                                         int( aColor.g * 255 ),
                                                         int( aColor.b * 255 ) );
    wxString cname( dxf_layer[color].name );
    return cname;
}


void DXF_PLOTTER::SetUnits( DXF_UNITS aUnit )
{
    m_plotUnits = aUnit;

    switch( aUnit )
    {
    case DXF_UNITS::MM:
        m_unitScalingFactor = 0.00254;
        m_measurementDirective = 1;
        break;

    case DXF_UNITS::INCH:
    default:
        m_unitScalingFactor = 0.0001;
        m_measurementDirective = 0;
    }
}


// convert aValue to a string, and remove trailing zeros
// In DXF files coordinates need a high precision: at least 9 digits when given
// in inches and 7 digits when in mm.
// So we use 16 digits and remove trailing 0 (if any)
static std::string formatCoord( double aValue )
{
    std::string buf;

    buf = fmt::format( "{:.16f}", aValue );

    // remove trailing zeros
    while( !buf.empty() && buf[buf.size() - 1] == '0' )
    {
        buf.pop_back();
    }

    return buf;
}


void DXF_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                               double aScale, bool aMirror )
{
    m_plotOffset  = aOffset;
    m_plotScale   = aScale;

    /* DXF paper is 'virtual' so there is no need of a paper size.
       Also this way we can handle the aux origin which can be useful
       (for example when aligning to a mechanical drawing) */
    m_paperSize.x = 0;
    m_paperSize.y = 0;

    /* Like paper size DXF units are abstract too. Anyway there is a
     * system variable (MEASUREMENT) which will be set to 0 to indicate
     * english units */
    m_IUsPerDecimil = aIusPerDecimil;
    m_iuPerDeviceUnit = 1.0 / aIusPerDecimil; // Gives a DXF in decimils
    m_iuPerDeviceUnit *= GetUnitScaling();    // Get the scaling factor for the current units

    m_plotMirror = false;                     // No mirroring on DXF
    m_currentColor = COLOR4D::BLACK;
}


bool DXF_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    wxASSERT( m_outputFile );

    // DXF HEADER - Boilerplate
    // Defines the minimum for drawing i.e. the angle system and the
    // 4 linetypes (CONTINUOUS, DOTDASH, DASHED and DOTTED)
    fmt::print( m_outputFile,
            "  0\n"
            "SECTION\n"
            "  2\n"
            "HEADER\n"
            "  9\n"
            "$ANGBASE\n"
            "  50\n"
            "0.0\n"
            "  9\n"
            "$ANGDIR\n"
            "  70\n"
            "1\n"
            "  9\n"
            "$MEASUREMENT\n"
            "  70\n"
            "{}\n"
            "  0\n"
            "ENDSEC\n"
            "  0\n"
            "SECTION\n"
            "  2\n"
            "TABLES\n"
            "  0\n"
            "TABLE\n"
            "  2\n"
            "LTYPE\n"
            "  70\n"
            "4\n"
            "  0\n"
            "LTYPE\n"
            "  5\n"
            "40F\n"
            "  2\n"
            "CONTINUOUS\n"
            "  70\n"
            "0\n"
            "  3\n"
            "Solid line\n"
            "  72\n"
            "65\n"
            "  73\n"
            "0\n"
            "  40\n"
            "0.0\n"
            "  0\n"
            "LTYPE\n"
            "  5\n"
            "410\n"
            "  2\n"
            "DASHDOT\n"
            " 70\n"
            "0\n"
            "  3\n"
            "Dash Dot ____ _ ____ _\n"
            " 72\n"
            "65\n"
            " 73\n"
            "4\n"
            " 40\n"
            "2.0\n"
            " 49\n"
            "1.25\n"
            " 49\n"
            "-0.25\n"
            " 49\n"
            "0.25\n"
            " 49\n"
            "-0.25\n"
            "  0\n"
            "LTYPE\n"
            "  5\n"
            "411\n"
            "  2\n"
            "DASHED\n"
            " 70\n"
            "0\n"
            "  3\n"
            "Dashed __ __ __ __ __\n"
            " 72\n"
            "65\n"
            " 73\n"
            "2\n"
            " 40\n"
            "0.75\n"
            " 49\n"
            "0.5\n"
            " 49\n"
            "-0.25\n"
            "  0\n"
            "LTYPE\n"
            "  5\n"
            "43B\n"
            "  2\n"
            "DOTTED\n"
            " 70\n"
            "0\n"
            "  3\n"
            "Dotted .  .  .  .\n"
            " 72\n"
            "65\n"
            " 73\n"
            "2\n"
            " 40\n"
            "0.2\n"
            " 49\n"
            "0.0\n"
            " 49\n"
            "-0.2\n"
            "  0\n"
            "ENDTAB\n",
             GetMeasurementDirective() );

    // Text styles table
    // Defines 4 text styles, one for each bold/italic combination
    fmt::print( m_outputFile,
	            "  0\n"
	            "TABLE\n"
	            "  2\n"
	            "STYLE\n"
	            "  70\n"
	            "4\n" );

    static const char *style_name[4] = {"KICAD", "KICADB", "KICADI", "KICADBI"};

    for(int i = 0; i < 4; i++ )
    {
        fmt::print( m_outputFile,
                 "  0\n"
                 "STYLE\n"
                 "  2\n"
                 "{}\n"         // Style name
                 "  70\n"
                 "0\n"          // Standard flags
                 "  40\n"
                 "0\n"          // Non-fixed height text
                 "  41\n"
                 "1\n"          // Width factor (base)
                 "  42\n"
                 "1\n"          // Last height (mandatory)
                 "  50\n"
                 "{:g}\n"         // Oblique angle
                 "  71\n"
                 "0\n"          // Generation flags (default)
                 "  3\n"
                 // The standard ISO font (when kicad is build with it
                 // the dxf text in acad matches *perfectly*)
                 "isocp.shx\n", // Font name (when not bigfont)
                 // Apply a 15 degree angle to italic text
                 style_name[i], i < 2 ? 0 : DXF_OBLIQUE_ANGLE );
    }

    EDA_COLOR_T numLayers = NBCOLORS;

    // If printing in monochrome, only output the black layer
    if( !GetColorMode() )
        numLayers = static_cast<EDA_COLOR_T>( 1 );

    // Layer table - one layer per color
    fmt::print( m_outputFile,
             "  0\n"
             "ENDTAB\n"
             "  0\n"
             "TABLE\n"
             "  2\n"
             "LAYER\n"
             "  70\n"
             "{}\n", (int)numLayers );

    /* The layer/colors palette. The acad/DXF palette is divided in 3 zones:

       - The primary colors (1 - 9)
       - An HSV zone (10-250, 5 values x 2 saturations x 10 hues
       - Greys (251 - 255)
     */

    wxASSERT( numLayers <= NBCOLORS );

    for( EDA_COLOR_T i = BLACK; i < numLayers; i = static_cast<EDA_COLOR_T>( int( i ) + 1 )  )
    {
        fmt::print( m_outputFile,
                 "  0\n"
                 "LAYER\n"
                 "  2\n"
                 "{}\n"         // Layer name
                 "  70\n"
                 "0\n"          // Standard flags
                 "  62\n"
                 "{}\n"         // Color number
                 "  6\n"
                 "CONTINUOUS\n",// Linetype name
                 dxf_layer[i].name, dxf_layer[i].color );
    }

    // End of layer table, begin entities
    fmt::print( m_outputFile,
           "  0\n"
           "ENDTAB\n"
           "  0\n"
           "ENDSEC\n"
           "  0\n"
           "SECTION\n"
           "  2\n"
           "ENTITIES\n" );

    return true;
}


bool DXF_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );

    // DXF FOOTER
    fmt::print( m_outputFile,
            "  0\n"
           "ENDSEC\n"
           "  0\n"
           "EOF\n" );
    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void DXF_PLOTTER::SetColor( const COLOR4D& color )
{
    if( ( m_colorMode )
       || ( color == COLOR4D::BLACK )
       || ( color == COLOR4D::WHITE ) )
    {
        m_currentColor = color;
    }
    else
    {
        m_currentColor = COLOR4D::BLACK;
    }
}


void DXF_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                        int aCornerRadius )
{
    wxASSERT( m_outputFile );

    if( aCornerRadius > 0 )
    {
        BOX2I box( p1, VECTOR2I( p2.x - p1.x, p2.y - p1.y ) );
        box.Normalize();
        SHAPE_RECT rect( box );
        rect.SetRadius( aCornerRadius );
        PlotPoly( rect.Outline(), fill, width, nullptr );
        return;
    }

    if( p1 != p2 )
    {
        MoveTo( p1 );
        LineTo( VECTOR2I( p1.x, p2.y ) );
        LineTo( VECTOR2I( p2.x, p2.y ) );
        LineTo( VECTOR2I( p2.x, p1.y ) );
        FinishTo( VECTOR2I( p1.x, p1.y ) );
    }
    else
    {
        // Draw as a point
        wxString cname = getDXFColorName( m_currentColor );
        VECTOR2D point_dev = userToDeviceCoordinates( p1 );

        fmt::print( m_outputFile, "0\nPOINT\n8\n{}\n10\n{}\n20\n{}\n",
                 TO_UTF8( cname ),
                 formatCoord( point_dev.x ),
                 formatCoord( point_dev.y ) );
    }
}


void DXF_PLOTTER::Circle( const VECTOR2I& centre, int diameter, FILL_T fill, int width )
{
    wxASSERT( m_outputFile );
    double   radius = userToDeviceSize( diameter / 2 );
    VECTOR2D centre_dev = userToDeviceCoordinates( centre );

    wxString cname = getDXFColorName( m_currentColor );

    if( radius > 0 )
    {
        if( fill == FILL_T::NO_FILL )
        {
            fmt::print( m_outputFile, "0\nCIRCLE\n8\n{}\n10\n{}\n20\n{}\n40\n{}\n",
                        TO_UTF8( cname ),
                        formatCoord( centre_dev.x ),
                        formatCoord( centre_dev.y ),
                        formatCoord( radius ) );
        }
        else if( fill == FILL_T::FILLED_SHAPE )
        {
            double r = radius * 0.5;
            fmt::print( m_outputFile, "0\nPOLYLINE\n" );
            fmt::print( m_outputFile, "8\n{}\n66\n1\n70\n1\n", TO_UTF8( cname ) );
            fmt::print( m_outputFile, "40\n{}\n41\n{}\n",
                                    formatCoord( radius ),
                                    formatCoord( radius ) );
            fmt::print( m_outputFile, "0\nVERTEX\n8\n{}\n", TO_UTF8( cname ) );
            fmt::print( m_outputFile, "10\n{}\n 20\n{}\n42\n1.0\n",
                                    formatCoord( centre_dev.x-r ),
                                    formatCoord( centre_dev.y ) );
            fmt::print( m_outputFile, "0\nVERTEX\n8\n{}\n", TO_UTF8( cname ) );
            fmt::print( m_outputFile, "10\n{}\n 20\n{}\n42\n1.0\n",
                                    formatCoord( centre_dev.x+r ),
                                    formatCoord( centre_dev.y ) );
            fmt::print( m_outputFile, "0\nSEQEND\n" );
        }
    }
    else
    {
        // Draw as a point
        fmt::print( m_outputFile, "0\nPOINT\n8\n{}\n10\n{}\n20\n{}\n",
                    TO_UTF8( cname ),
                    formatCoord( centre_dev.x ),
                    formatCoord( centre_dev.y ) );
    }
}


void DXF_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                            void* aData )
{
    if( aCornerList.size() <= 1 )
        return;

    unsigned last = aCornerList.size() - 1;

    // Plot outlines with lines (thickness = 0) to define the polygon
    if( aWidth <= 0 || aFill == FILL_T::NO_FILL  )
    {
        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            LineTo( aCornerList[ii] );

        // Close polygon if 'fill' requested
        if( aFill != FILL_T::NO_FILL )
        {
            if( aCornerList[last] != aCornerList[0] )
                LineTo( aCornerList[0] );
        }

        PenFinish();
        return;
    }

    // The polygon outline has thickness, and is filled
    // Build and plot the polygon which contains the initial
    // polygon and its thick outline
    SHAPE_POLY_SET  bufferOutline;
    SHAPE_POLY_SET  bufferPolybase;

    bufferPolybase.NewOutline();

    // enter outline as polygon:
    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        TransformOvalToPolygon( bufferOutline, aCornerList[ ii - 1 ], aCornerList[ ii ],
                                aWidth, GetPlotterArcHighDef(), ERROR_INSIDE );
    }

    // enter the initial polygon:
    for( const VECTOR2I& corner : aCornerList )
        bufferPolybase.Append( corner );

    // Merge polygons to build the polygon which contains the initial
    // polygon and its thick outline

    // create the outline which contains thick outline:
    bufferPolybase.BooleanAdd( bufferOutline );
    bufferPolybase.Fracture();

    if( bufferPolybase.OutlineCount() < 1 )      // should not happen
        return;

    const SHAPE_LINE_CHAIN& path = bufferPolybase.COutline( 0 );

    if( path.PointCount() < 2 )           // should not happen
        return;

    // Now, output the final polygon to DXF file:
    last = path.PointCount() - 1;
    VECTOR2I point = path.CPoint( 0 );

    VECTOR2I startPoint( point.x, point.y );
    MoveTo( startPoint );

    for( int ii = 1; ii < path.PointCount(); ii++ )
    {
        point = path.CPoint( ii );
        LineTo( VECTOR2I( point.x, point.y ) );
    }

    // Close polygon, if needed
    point = path.CPoint( last );
    VECTOR2I endPoint( point.x, point.y );

    if( endPoint != startPoint )
        LineTo( startPoint );

    PenFinish();
}


std::vector<VECTOR2I> arcPts( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                              const EDA_ANGLE& aAngle, double aRadius )
{
    std::vector<VECTOR2I> pts;

    /*
     * Arcs are not so easily approximated by beziers (in the general case), so we approximate
     * them in the old way
     */
    EDA_ANGLE       startAngle = -aStartAngle;
    EDA_ANGLE       endAngle = startAngle - aAngle;
    VECTOR2I        start;
    VECTOR2I        end;
    const EDA_ANGLE delta( 5, DEGREES_T );   // increment to draw circles

    if( startAngle > endAngle )
        std::swap( startAngle, endAngle );

    // Usual trig arc plotting routine...
    start.x = KiROUND( aCenter.x + aRadius * ( -startAngle ).Cos() );
    start.y = KiROUND( aCenter.y + aRadius * ( -startAngle ).Sin() );
    pts.emplace_back( start );

    for( EDA_ANGLE ii = startAngle + delta; ii < endAngle; ii += delta )
    {
        end.x = KiROUND( aCenter.x + aRadius * ( -ii ).Cos() );
        end.y = KiROUND( aCenter.y + aRadius * ( -ii ).Sin() );
        pts.emplace_back( end );
    }

    end.x = KiROUND( aCenter.x + aRadius * ( -endAngle ).Cos() );
    end.y = KiROUND( aCenter.y + aRadius * ( -endAngle ).Sin() );
    pts.emplace_back( end );

    return pts;
}


void DXF_PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill, int aWidth, void* aData )
{
    std::set<size_t>      handledArcs;
    std::vector<VECTOR2I> cornerList;

    for( int ii = 0; ii < aLineChain.SegmentCount(); ++ii )
    {
        if( aLineChain.IsArcSegment( ii ) )
        {
            size_t arcIndex = aLineChain.ArcIndex( ii );

            if( !handledArcs.contains( arcIndex ) )
            {
                handledArcs.insert( arcIndex );
                const SHAPE_ARC& arc( aLineChain.Arc( arcIndex ) );
                std::vector<VECTOR2I> pts = arcPts( arc.GetCenter(), arc.GetStartAngle(),
                                                    arc.GetCentralAngle(), arc.GetRadius() );

                for( const VECTOR2I& pt : std::ranges::reverse_view( pts ) )
                    cornerList.emplace_back( pt );
            }
        }
        else
        {
            const SEG& seg( aLineChain.Segment( ii ) );
            cornerList.emplace_back( seg.A );
            cornerList.emplace_back( seg.B );
        }
    }

    if( aLineChain.IsClosed() && cornerList.front() != cornerList.back() )
        cornerList.emplace_back( aLineChain.CPoint( 0 ) );

    PlotPoly( cornerList, aFill, aWidth, aData );
}


void DXF_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    wxASSERT( m_outputFile );

    if( plume == 'Z' )
    {
        return;
    }

    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    VECTOR2D pen_lastpos_dev = userToDeviceCoordinates( m_penLastpos );

    if( m_penLastpos != pos && plume == 'D' )
    {
        wxASSERT( m_currentLineType >= LINE_STYLE::FIRST_TYPE
                  && m_currentLineType <= LINE_STYLE::LAST_TYPE );

        // DXF LINE
        wxString    cname = getDXFColorName( m_currentColor );
        const char* lname = getDXFLineType( static_cast<LINE_STYLE>( m_currentLineType ) );
        fmt::print( m_outputFile, "0\nLINE\n8\n{}\n6\n{}\n10\n{}\n20\n{}\n11\n{}\n21\n{}\n",
                    TO_UTF8( cname ),
                    lname,
                    formatCoord( pen_lastpos_dev.x ),
                    formatCoord( pen_lastpos_dev.y ),
                    formatCoord( pos_dev.x ),
                    formatCoord( pos_dev.y ) );
    }

    m_penLastpos = pos;
}


void DXF_PLOTTER::SetDash( int aLineWidth, LINE_STYLE aLineStyle )
{
    wxASSERT( aLineStyle >= LINE_STYLE::FIRST_TYPE
                && aLineStyle <= LINE_STYLE::LAST_TYPE );

    m_currentLineType = aLineStyle;
}


void DXF_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( m_outputFile );

    if( aRadius <= 0 )
        return;

    EDA_ANGLE startAngle = -aStartAngle;
    EDA_ANGLE endAngle = startAngle - aAngle;

    // In DXF, arcs are drawn CCW.
    // If startAngle > endAngle, it is CW. So transform it to CCW
    if( endAngle < startAngle )
        std::swap( startAngle, endAngle );

    VECTOR2D centre_device = userToDeviceCoordinates( aCenter );
    double   radius_device = userToDeviceSize( aRadius );

    // Emit a DXF ARC entity
    wxString cname = getDXFColorName( m_currentColor );
    fmt::print( m_outputFile,
                "0\nARC\n8\n{}\n10\n{}\n20\n{}\n40\n{}\n50\n{:.8f}\n51\n{:.8f}\n",
                TO_UTF8( cname ),
                formatCoord( centre_device.x ),
                formatCoord( centre_device.y ),
                formatCoord( radius_device ),
                startAngle.AsDegrees(),
                endAngle.AsDegrees() );
}


void DXF_PLOTTER::ThickSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        std::vector<VECTOR2I> cornerList;
        SHAPE_POLY_SET outlineBuffer;
        TransformOvalToPolygon( outlineBuffer, aStart, aEnd, aWidth, GetPlotterArcHighDef(),
                                ERROR_INSIDE );
        const SHAPE_LINE_CHAIN& path = outlineBuffer.COutline( 0 );

        cornerList.reserve( path.PointCount() );

        for( int jj = 0; jj < path.PointCount(); jj++ )
            cornerList.emplace_back( path.CPoint( jj ).x, path.CPoint( jj ).y );

        // Ensure the polygon is closed
        if( cornerList[0] != cornerList[cornerList.size() - 1] )
            cornerList.push_back( cornerList[0] );

        PlotPoly( cornerList, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
    else
    {
        MoveTo( aStart );
        FinishTo( aEnd );
    }
}


void DXF_PLOTTER::ThickArc( const VECTOR2D& centre, const EDA_ANGLE& aStartAngle,
                            const EDA_ANGLE& aAngle, double aRadius, int aWidth, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        Arc( centre, aStartAngle, aAngle, aRadius - aWidth/2, FILL_T::NO_FILL, DXF_LINE_WIDTH );
        Arc( centre, aStartAngle, aAngle, aRadius + aWidth/2, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
    else
    {
        Arc( centre, aStartAngle, aAngle, aRadius, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
}


void DXF_PLOTTER::ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        VECTOR2I offsetp1( p1.x - width/2, p1.y - width/2 );
        VECTOR2I offsetp2( p2.x + width/2, p2.y + width/2 );
        Rect( offsetp1, offsetp2, FILL_T::NO_FILL, DXF_LINE_WIDTH, 0 );

        offsetp1.x += width;
        offsetp1.y += width;
        offsetp2.x -= width;
        offsetp2.y -= width;
        Rect( offsetp1, offsetp2, FILL_T::NO_FILL, DXF_LINE_WIDTH, 0 );
    }
    else
    {
        Rect( p1, p2, FILL_T::NO_FILL, DXF_LINE_WIDTH, 0 );
    }
}


void DXF_PLOTTER::ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        Circle( pos, diametre - width, FILL_T::NO_FILL, DXF_LINE_WIDTH );
        Circle( pos, diametre + width, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
    else
    {
        Circle( pos, diametre, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    }
}


void DXF_PLOTTER::FilledCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
        Circle( pos, diametre, FILL_T::NO_FILL, DXF_LINE_WIDTH );
    else
        Circle( pos, diametre, FILL_T::FILLED_SHAPE, 0 );
}


void DXF_PLOTTER::ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData )
{
    const PLOT_PARAMS* cfg = static_cast<const PLOT_PARAMS*>( aData );

    if( cfg && cfg->GetDXFPlotMode() == SKETCH )
    {
        SHAPE_POLY_SET outline = aPoly.CloneDropTriangulation();
        outline.Inflate( aWidth / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, GetPlotterArcHighDef() );
        PLOTTER::PlotPoly( outline.COutline( 0 ), FILL_T::NO_FILL, DXF_LINE_WIDTH, aData );

        outline = aPoly.CloneDropTriangulation();
        outline.Deflate( aWidth / 2, CORNER_STRATEGY::ROUND_ALL_CORNERS, GetPlotterArcHighDef() );
        PLOTTER::PlotPoly( outline.COutline( 0 ), FILL_T::NO_FILL, DXF_LINE_WIDTH, aData );
    }
    else
    {
        PLOTTER::PlotPoly( aPoly.COutline( 0 ), FILL_T::NO_FILL, aWidth, aData );
    }
}


void DXF_PLOTTER::FlashPadOval( const VECTOR2I& aPos, const VECTOR2I& aSize,
                                const EDA_ANGLE& aOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I  size( aSize );
    EDA_ANGLE orient( aOrient );

    /* The chip is reduced to an oval tablet with size.y > size.x
     * (Oval vertical orientation 0) */
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient += ANGLE_90;
    }

    ThickOval( aPos, size, orient, DXF_LINE_WIDTH, aData );
}


void DXF_PLOTTER::FlashPadCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    wxASSERT( m_outputFile );
    Circle( pos, diametre, FILL_T::NO_FILL, DXF_LINE_WIDTH );
}


void DXF_PLOTTER::FlashPadRect( const VECTOR2I& aPos, const VECTOR2I& aPadSize,
                                const EDA_ANGLE& aOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I size, start, end;

    size.x = aPadSize.x / 2;
    size.y = aPadSize.y / 2;

    if( size.x < 0 )
        size.x = 0;

    if( size.y < 0 )
        size.y = 0;

    // If a dimension is zero, the trace is reduced to 1 line
    if( size.x == 0 )
    {
        start = VECTOR2I( aPos.x, aPos.y - size.y );
        end = VECTOR2I( aPos.x, aPos.y + size.y );
        RotatePoint( start, aPos, aOrient );
        RotatePoint( end, aPos, aOrient );
        MoveTo( start );
        FinishTo( end );
        return;
    }

    if( size.y == 0 )
    {
        start = VECTOR2I( aPos.x - size.x, aPos.y );
        end = VECTOR2I( aPos.x + size.x, aPos.y );
        RotatePoint( start, aPos, aOrient );
        RotatePoint( end, aPos, aOrient );
        MoveTo( start );
        FinishTo( end );
        return;
    }

    start = VECTOR2I( aPos.x - size.x, aPos.y - size.y );
    RotatePoint( start, aPos, aOrient );
    MoveTo( start );

    end = VECTOR2I( aPos.x - size.x, aPos.y + size.y );
    RotatePoint( end, aPos, aOrient );
    LineTo( end );

    end = VECTOR2I( aPos.x + size.x, aPos.y + size.y );
    RotatePoint( end, aPos, aOrient );
    LineTo( end );

    end = VECTOR2I( aPos.x + size.x, aPos.y - size.y );
    RotatePoint( end, aPos, aOrient );
    LineTo( end );

    FinishTo( start );
}


void DXF_PLOTTER::FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                     int aCornerRadius, const EDA_ANGLE& aOrient, void* aData )
{
    SHAPE_POLY_SET outline;
    TransformRoundChamferedRectToPolygon( outline, aPadPos, aSize, aOrient, aCornerRadius, 0.0, 0,
                                          0, GetPlotterArcHighDef(), ERROR_INSIDE );

    // TransformRoundRectToPolygon creates only one convex polygon
    SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );

    MoveTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );

    for( int ii = 1; ii < poly.PointCount(); ++ii )
        LineTo( VECTOR2I( poly.CPoint( ii ).x, poly.CPoint( ii ).y ) );

    FinishTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );
}


void DXF_PLOTTER::FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                  const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                  void* aData )
{
    for( int cnt = 0; cnt < aPolygons->OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = aPolygons->Outline( cnt );

        MoveTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );

        for( int ii = 1; ii < poly.PointCount(); ++ii )
            LineTo( VECTOR2I( poly.CPoint( ii ).x, poly.CPoint( ii ).y ) );

        FinishTo( VECTOR2I( poly.CPoint( 0 ).x, poly.CPoint( 0 ).y ) );
    }
}


void DXF_PLOTTER::FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                  const EDA_ANGLE& aPadOrient, void* aData )
{
    wxASSERT( m_outputFile );
    VECTOR2I coord[4]; /* coord actual corners of a trapezoidal trace */

    for( int ii = 0; ii < 4; ii++ )
    {
        coord[ii] = aCorners[ii];
        RotatePoint( coord[ii], aPadOrient );
        coord[ii] += aPadPos;
    }

    // Plot edge:
    MoveTo( coord[0] );
    LineTo( coord[1] );
    LineTo( coord[2] );
    LineTo( coord[3] );
    FinishTo( coord[0] );
}


void DXF_PLOTTER::FlashRegularPolygon( const VECTOR2I& aShapePos, int aRadius, int aCornerCount,
                                       const EDA_ANGLE& aOrient, void* aData )
{
    // Do nothing
    wxASSERT( 0 );
}


/**
 * Check if a given string contains non-ASCII characters.
 *
 * @param string String to check.
 * @return true if it contains some non-ASCII character, false if all characters are
 *         inside ASCII range (<=255).
 */
bool containsNonAsciiChars( const wxString& string )
{
    for( unsigned i = 0; i < string.length(); i++ )
    {
        wchar_t ch = string[i];

        if( ch > 255 )
            return true;
    }
    return false;
}


void DXF_PLOTTER::Text( const VECTOR2I&        aPos,
                        const COLOR4D&         aColor,
                        const wxString&        aText,
                        const EDA_ANGLE&       aOrient,
                        const VECTOR2I&        aSize,
                        enum GR_TEXT_H_ALIGN_T aH_justify,
                        enum GR_TEXT_V_ALIGN_T aV_justify,
                        int                    aWidth,
                        bool                   aItalic,
                        bool                   aBold,
                        bool                   aMultilineAllowed,
                        KIFONT::FONT*          aFont,
                        const KIFONT::METRICS& aFontMetrics,
                        void*                  aData )
{
    // Fix me: see how to use DXF text mode for multiline texts
    if( aMultilineAllowed && !aText.Contains( wxT( "\n" ) ) )
        aMultilineAllowed = false;  // the text has only one line.

    bool processSuperSub = aText.Contains( wxT( "^{" ) ) || aText.Contains( wxT( "_{" ) );

    if( m_textAsLines || containsNonAsciiChars( aText ) || aMultilineAllowed || processSuperSub )
    {
        // output text as graphics.
        // Perhaps multiline texts could be handled as DXF text entity
        // but I do not want spend time about this (JPC)
        PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
                       aBold, aMultilineAllowed, aFont, aFontMetrics, aData );
    }
    else
    {
        TEXT_ATTRIBUTES attrs;
        attrs.m_Halign = aH_justify;
        attrs.m_Valign =aV_justify;
        attrs.m_StrokeWidth = aWidth;
        attrs.m_Angle = aOrient;
        attrs.m_Italic = aItalic;
        attrs.m_Bold = aBold;
        attrs.m_Mirrored = aSize.x < 0;
        attrs.m_Multiline = false;
        plotOneLineOfText( aPos, aColor, aText, attrs );
    }
}


void DXF_PLOTTER::PlotText( const VECTOR2I&        aPos,
                            const COLOR4D&         aColor,
                            const wxString&        aText,
                            const TEXT_ATTRIBUTES& aAttributes,
                            KIFONT::FONT*          aFont,
                            const KIFONT::METRICS& aFontMetrics,
                            void*                  aData )
{
    TEXT_ATTRIBUTES attrs = aAttributes;

    // Fix me: see how to use DXF text mode for multiline texts
    if( attrs.m_Multiline && !aText.Contains( wxT( "\n" ) ) )
        attrs.m_Multiline = false;  // the text has only one line.

    bool processSuperSub = aText.Contains( wxT( "^{" ) ) || aText.Contains( wxT( "_{" ) );

    if( m_textAsLines || containsNonAsciiChars( aText ) || attrs.m_Multiline || processSuperSub )
    {
        // output text as graphics.
        // Perhaps multiline texts could be handled as DXF text entity
        // but I do not want spend time about that (JPC)
        PLOTTER::PlotText( aPos, aColor, aText, aAttributes, aFont, aFontMetrics, aData );
    }
    else
    {
        plotOneLineOfText( aPos, aColor, aText, attrs );
    }
}


void DXF_PLOTTER::plotOneLineOfText( const VECTOR2I& aPos, const COLOR4D& aColor,
                                     const wxString& aText, const TEXT_ATTRIBUTES& aAttributes )
{
    /* Emit text as a text entity. This loses formatting and shape but it's
       more useful as a CAD object */
    VECTOR2D origin_dev = userToDeviceCoordinates( aPos );
    SetColor( aColor );
    wxString cname = getDXFColorName( m_currentColor );
    VECTOR2D size_dev = userToDeviceSize( aAttributes.m_Size );
    int h_code = 0, v_code = 0;

    switch( aAttributes.m_Halign )
    {
    case GR_TEXT_H_ALIGN_LEFT:   h_code = 0; break;
    case GR_TEXT_H_ALIGN_CENTER: h_code = 1; break;
    case GR_TEXT_H_ALIGN_RIGHT:  h_code = 2; break;
    case GR_TEXT_H_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
    }

    switch( aAttributes.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:    v_code = 3; break;
    case GR_TEXT_V_ALIGN_CENTER: v_code = 2; break;
    case GR_TEXT_V_ALIGN_BOTTOM: v_code = 1; break;
    case GR_TEXT_V_ALIGN_INDETERMINATE: wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) ); break;
    }

    std::string textStyle = "KICAD";
    if( aAttributes.m_Bold )
    {
        if( aAttributes.m_Italic )
            textStyle = "KICADBI";
        else
            textStyle = "KICADB";
    }
    else if( aAttributes.m_Italic )
        textStyle = "KICADI";

    // Position, size, rotation and alignment
    // The two alignment point usages is somewhat idiot (see the DXF ref)
    // Anyway since we don't use the fit/aligned options, they're the same
    fmt::print( m_outputFile,
             "  0\n"
             "TEXT\n"
             "  7\n"
             "{}\n"          // Text style
             "  8\n"
             "{}\n"          // Layer name
             "  10\n"
             "{}\n"          // First point X
             "  11\n"
             "{}\n"          // Second point X
             "  20\n"
             "{}\n"          // First point Y
             "  21\n"
             "{}\n"          // Second point Y
             "  40\n"
             "{}\n"          // Text height
             "  41\n"
             "{}\n"          // Width factor
             "  50\n"
             "{:.8f}\n"        // Rotation
             "  51\n"
             "{:.8f}\n"        // Oblique angle
             "  71\n"
             "{}\n"          // Mirror flags
             "  72\n"
             "{}\n"          // H alignment
             "  73\n"
             "{}\n",         // V alignment
             textStyle,
             TO_UTF8( cname ),
             formatCoord( origin_dev.x ),
             formatCoord( origin_dev.x ),
             formatCoord( origin_dev.y ),
             formatCoord( origin_dev.y ),
             formatCoord( size_dev.y ),
             formatCoord( fabs( size_dev.x / size_dev.y ) ),
             aAttributes.m_Angle.AsDegrees(),
             aAttributes.m_Italic ? DXF_OBLIQUE_ANGLE : 0,
             aAttributes.m_Mirrored ? 2 : 0, // X mirror flag
             h_code, v_code );

    /* There are two issue in emitting the text:
       - Our overline character (~) must be converted to the appropriate
       control sequence %%O or %%o
       - Text encoding in DXF is more or less unspecified since depends on
       the DXF declared version, the acad version reading it *and* some
       system variables to be put in the header handled only by newer acads
       Also before R15 unicode simply is not supported (you need to use
       bigfonts which are a massive PITA). Common denominator solution:
       use Latin1 (and however someone could choke on it, anyway). Sorry
       for the extended latin people. If somewant want to try fixing this
       recent version seems to use UTF-8 (and not UCS2 like the rest of
       Windows)

       XXX Actually there is a *third* issue: older DXF formats are limited
       to 255 bytes records (it was later raised to 2048); since I'm lazy
       and text so long is not probable I just don't implement this rule.
       If someone is interested in fixing this, you have to emit the first
       partial lines with group code 3 (max 250 bytes each) and then finish
       with a group code 1 (less than 250 bytes). The DXF refs explains it
       in no more details...
     */

    int braceNesting = 0;
    int overbarDepth = -1;

    fmt::print( m_outputFile, "  1\n" );

    for( unsigned int i = 0; i < aText.length(); i++ )
    {
        /* Here I do a bad thing: writing the output one byte at a time!
           but today I'm lazy and I have no idea on how to coerce a Unicode
           wxString to spit out latin1 encoded text ...

           At least stdio is *supposed* to do output buffering, so there is
           hope is not too slow */
        wchar_t ch = aText[i];

        if( ch > 255 )
        {
            // I can't encode this...
            putc( '?', m_outputFile );
        }
        else
        {
            if( aText[i] == '~' && i+1 < aText.length() && aText[i+1] == '{' )
            {
                fmt::print( m_outputFile, "%%o" );
                overbarDepth = braceNesting;

                // Skip the '{'
                i++;
                continue;
            }
            else if( aText[i] == '{' )
            {
                braceNesting++;
            }
            else if( aText[i] == '}' )
            {
                if( braceNesting > 0 )
                    braceNesting--;

                if( braceNesting == overbarDepth )
                {
                    fmt::print( m_outputFile, "%%O" );
                    overbarDepth = -1;
                    continue;
                }
            }

            putc( ch, m_outputFile );
        }
    }

    fmt::print( m_outputFile, "\n" );
}
