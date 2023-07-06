/**
 * @file DXF_plotter.cpp
 * @brief Kicad: specialized plotter for DXF files format
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <plotters/plotter_dxf.h>
#include <macros.h>
#include <string_utils.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <fmt/core.h>

/**
 * Oblique angle for DXF native text
 * (I don't remember if 15 degrees is the ISO value... it looks nice anyway)
 */
static const double DXF_OBLIQUE_ANGLE = 15;

/* The layer/colors palette. The acad/DXF palette is divided in 3 zones:

   - The primary colors (1 - 9)
   - An HSV zone (10-250, 5 values x 2 saturations x 10 hues
   - Greys (251 - 255)

   There is *no* black... the white does it on paper, usually, and
   anyway it depends on the plotter configuration, since DXF colors
   are meant to be logical only (they represent *both* line color and
   width); later version with plot styles only complicate the matter!

   As usual, brown and magenta/purple are difficult to place since
   they are actually variations of other colors.
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


static const char* getDXFLineType( PLOT_DASH_TYPE aType )
{
    switch( aType )
    {
    case PLOT_DASH_TYPE::DEFAULT:
    case PLOT_DASH_TYPE::SOLID:
        return "CONTINUOUS";
    case PLOT_DASH_TYPE::DASH:
        return "DASHED";
    case PLOT_DASH_TYPE::DOT:
        return "DOTTED";
    case PLOT_DASH_TYPE::DASHDOT:
        return "DASHDOT";
    default:
        wxFAIL_MSG( "Unhandled PLOT_DASH_TYPE" );
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
    case DXF_UNITS::MILLIMETERS:
        m_unitScalingFactor = 0.00254;
        m_measurementDirective = 1;
        break;

    case DXF_UNITS::INCHES:
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
    fprintf( m_outputFile,
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
            "%u\n"
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
    fputs( "  0\n"
           "TABLE\n"
           "  2\n"
           "STYLE\n"
           "  70\n"
           "4\n", m_outputFile );

    static const char *style_name[4] = {"KICAD", "KICADB", "KICADI", "KICADBI"};
    for(int i = 0; i < 4; i++ )
    {
        fprintf( m_outputFile,
                 "  0\n"
                 "STYLE\n"
                 "  2\n"
                 "%s\n"         // Style name
                 "  70\n"
                 "0\n"          // Standard flags
                 "  40\n"
                 "0\n"          // Non-fixed height text
                 "  41\n"
                 "1\n"          // Width factor (base)
                 "  42\n"
                 "1\n"          // Last height (mandatory)
                 "  50\n"
                 "%g\n"         // Oblique angle
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
    fprintf( m_outputFile,
             "  0\n"
             "ENDTAB\n"
             "  0\n"
             "TABLE\n"
             "  2\n"
             "LAYER\n"
             "  70\n"
             "%d\n", numLayers );

    /* The layer/colors palette. The acad/DXF palette is divided in 3 zones:

       - The primary colors (1 - 9)
       - An HSV zone (10-250, 5 values x 2 saturations x 10 hues
       - Greys (251 - 255)
     */

    wxASSERT( numLayers <= NBCOLORS );

    for( EDA_COLOR_T i = BLACK; i < numLayers; i = static_cast<EDA_COLOR_T>( int( i ) + 1 )  )
    {
        fprintf( m_outputFile,
                 "  0\n"
                 "LAYER\n"
                 "  2\n"
                 "%s\n"         // Layer name
                 "  70\n"
                 "0\n"          // Standard flags
                 "  62\n"
                 "%d\n"         // Color number
                 "  6\n"
                 "CONTINUOUS\n",// Linetype name
                 dxf_layer[i].name, dxf_layer[i].color );
    }

    // End of layer table, begin entities
    fputs( "  0\n"
           "ENDTAB\n"
           "  0\n"
           "ENDSEC\n"
           "  0\n"
           "SECTION\n"
           "  2\n"
           "ENTITIES\n", m_outputFile );

    return true;
}


bool DXF_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );

    // DXF FOOTER
    fputs( "  0\n"
           "ENDSEC\n"
           "  0\n"
           "EOF\n", m_outputFile );
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


void DXF_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width )
{
    wxASSERT( m_outputFile );
    MoveTo( p1 );
    LineTo( VECTOR2I( p1.x, p2.y ) );
    LineTo( VECTOR2I( p2.x, p2.y ) );
    LineTo( VECTOR2I( p2.x, p1.y ) );
    FinishTo( VECTOR2I( p1.x, p1.y ) );
}


void DXF_PLOTTER::Circle( const VECTOR2I& centre, int diameter, FILL_T fill, int width )
{
    wxASSERT( m_outputFile );
    double   radius = userToDeviceSize( diameter / 2 );
    VECTOR2D centre_dev = userToDeviceCoordinates( centre );

    if( radius > 0 )
    {
        wxString cname = getDXFColorName( m_currentColor );

        if( fill == FILL_T::NO_FILL )
        {
            fprintf( m_outputFile, "0\nCIRCLE\n8\n%s\n10\n%s\n20\n%s\n40\n%s\n",
                     TO_UTF8( cname ),
                     formatCoord( centre_dev.x ).c_str(),
                     formatCoord( centre_dev.y ).c_str(),
                     formatCoord( radius ).c_str() );
        }
        else if( fill == FILL_T::FILLED_SHAPE )
        {
            double r = radius*0.5;
            fprintf( m_outputFile, "0\nPOLYLINE\n" );
            fprintf( m_outputFile, "8\n%s\n66\n1\n70\n1\n", TO_UTF8( cname ) );
            fprintf( m_outputFile, "40\n%s\n41\n%s\n",
                                    formatCoord( radius ).c_str(),
                                    formatCoord( radius ).c_str() );
            fprintf( m_outputFile, "0\nVERTEX\n8\n%s\n", TO_UTF8( cname ) );
            fprintf( m_outputFile, "10\n%s\n 20\n%s\n42\n1.0\n",
                                    formatCoord( centre_dev.x-r ).c_str(),
                                    formatCoord( centre_dev.y ).c_str() );
            fprintf( m_outputFile, "0\nVERTEX\n8\n%s\n", TO_UTF8( cname ) );
            fprintf( m_outputFile, "10\n%s\n 20\n%s\n42\n1.0\n",
                                    formatCoord( centre_dev.x+r ).c_str(),
                                    formatCoord( centre_dev.y ).c_str() );
            fprintf( m_outputFile, "0\nSEQEND\n");
        }
    }
}


void DXF_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                            void* aData )
{
    if( aCornerList.size() <= 1 )
        return;

    unsigned last = aCornerList.size() - 1;

    // Plot outlines with lines (thickness = 0) to define the polygon
    if( aWidth <= 0  )
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

    // if the polygon outline has thickness, and is not filled
    // (i.e. is a polyline) plot outlines with thick segments
    if( aWidth > 0 && aFill == FILL_T::NO_FILL )
    {
        MoveTo( aCornerList[0] );

        for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
            ThickSegment( aCornerList[ii-1], aCornerList[ii], aWidth, FILLED, nullptr );

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
    for( unsigned ii = 0; ii < aCornerList.size(); ii++ )
    {
        bufferPolybase.Append( aCornerList[ii] );
    }

    // Merge polygons to build the polygon which contains the initial
    // polygon and its thick outline

    // create the outline which contains thick outline:
    bufferPolybase.BooleanAdd( bufferOutline, SHAPE_POLY_SET::PM_FAST );
    bufferPolybase.Fracture( SHAPE_POLY_SET::PM_FAST );

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
        wxASSERT( m_currentLineType >= PLOT_DASH_TYPE::FIRST_TYPE
                  && m_currentLineType <= PLOT_DASH_TYPE::LAST_TYPE );
        // DXF LINE
        wxString    cname = getDXFColorName( m_currentColor );
        const char* lname = getDXFLineType( static_cast<PLOT_DASH_TYPE>( m_currentLineType ) );
        fprintf( m_outputFile, "0\nLINE\n8\n%s\n6\n%s\n10\n%s\n20\n%s\n11\n%s\n21\n%s\n",
                 TO_UTF8( cname ), lname,
                 formatCoord( pen_lastpos_dev.x ).c_str(),
                 formatCoord( pen_lastpos_dev.y ).c_str(),
                 formatCoord( pos_dev.x ).c_str(),
                 formatCoord( pos_dev.y ).c_str() );
    }

    m_penLastpos = pos;
}


void DXF_PLOTTER::SetDash( int aLineWidth, PLOT_DASH_TYPE aLineStyle )
{
    wxASSERT( aLineStyle >= PLOT_DASH_TYPE::FIRST_TYPE
                && aLineStyle <= PLOT_DASH_TYPE::LAST_TYPE );

    m_currentLineType = aLineStyle;
}


void DXF_PLOTTER::ThickSegment( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
                                OUTLINE_MODE aPlotMode, void* aData )
{
    if( aPlotMode == SKETCH )
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

        PlotPoly( cornerList, FILL_T::NO_FILL );
    }
    else
    {
        MoveTo( aStart );
        FinishTo( aEnd );
    }
}


void DXF_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aEndAngle, double aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( m_outputFile );

    if( aRadius <= 0 )
        return;

    EDA_ANGLE startAngle( aStartAngle );
    EDA_ANGLE endAngle( aEndAngle );

    // In DXF, arcs are drawn CCW.
    // If startAngle > endAngle, it is CW. So transform it to CCW
    if( startAngle > endAngle )
        std::swap( startAngle, endAngle );

    VECTOR2D centre_device = userToDeviceCoordinates( aCenter );
    double   radius_device = userToDeviceSize( aRadius );

    // Emit a DXF ARC entity
    wxString cname = getDXFColorName( m_currentColor );
    fprintf( m_outputFile,
             "0\nARC\n8\n%s\n10\n%s\n20\n%s\n40\n%s\n50\n%.8f\n51\n%.8f\n",
             TO_UTF8( cname ),
             formatCoord( centre_device.x ).c_str(),
             formatCoord( centre_device.y ).c_str(),
             formatCoord( radius_device ).c_str(),
             startAngle.AsDegrees(), endAngle.AsDegrees() );
}


void DXF_PLOTTER::FlashPadOval( const VECTOR2I& aPos, const VECTOR2I& aSize,
                                const EDA_ANGLE& aOrient, OUTLINE_MODE aTraceMode, void* aData )
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

    sketchOval( aPos, size, orient, -1 );
}


void DXF_PLOTTER::FlashPadCircle( const VECTOR2I& pos, int diametre,
                                  OUTLINE_MODE trace_mode, void* aData )
{
    wxASSERT( m_outputFile );
    Circle( pos, diametre, FILL_T::NO_FILL );
}


void DXF_PLOTTER::FlashPadRect( const VECTOR2I& aPos, const VECTOR2I& aPadSize,
                                const EDA_ANGLE& aOrient, OUTLINE_MODE aTraceMode, void* aData )
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
                                     int aCornerRadius, const EDA_ANGLE& aOrient,
                                     OUTLINE_MODE aTraceMode, void* aData )
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
                                  OUTLINE_MODE aTraceMode, void* aData )
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
                                  const EDA_ANGLE& aPadOrient, OUTLINE_MODE aTraceMode,
                                  void* aData )
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
                                       const EDA_ANGLE& aOrient, OUTLINE_MODE aTraceMode,
                                       void* aData )
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


void DXF_PLOTTER::Text( const VECTOR2I&             aPos,
                        const COLOR4D&              aColor,
                        const wxString&             aText,
                        const EDA_ANGLE&            aOrient,
                        const VECTOR2I&             aSize,
                        enum GR_TEXT_H_ALIGN_T      aH_justify,
                        enum GR_TEXT_V_ALIGN_T      aV_justify,
                        int                         aWidth,
                        bool                        aItalic,
                        bool                        aBold,
                        bool                        aMultilineAllowed,
                        KIFONT::FONT*               aFont,
                        void*                       aData )
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
                       aBold, aMultilineAllowed, aFont, aData );
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


void DXF_PLOTTER::PlotText( const VECTOR2I& aPos, const COLOR4D& aColor,
                    const wxString& aText,
                    const TEXT_ATTRIBUTES& aAttributes,
                    KIFONT::FONT* aFont,
                    void* aData )
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
        PLOTTER::PlotText( aPos, aColor, aText, aAttributes, aFont, aData );
    }
    else
       plotOneLineOfText( aPos, aColor, aText, attrs );
}

void DXF_PLOTTER::plotOneLineOfText( const VECTOR2I& aPos, const COLOR4D& aColor,
                                    const wxString& aText,
                                    const TEXT_ATTRIBUTES& aAttributes )
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
    }

    switch( aAttributes.m_Valign )
    {
    case GR_TEXT_V_ALIGN_TOP:    v_code = 3; break;
    case GR_TEXT_V_ALIGN_CENTER: v_code = 2; break;
    case GR_TEXT_V_ALIGN_BOTTOM: v_code = 1; break;
    }

    // Position, size, rotation and alignment
    // The two alignment point usages is somewhat idiot (see the DXF ref)
    // Anyway since we don't use the fit/aligned options, they're the same
    fprintf( m_outputFile,
             "  0\n"
             "TEXT\n"
             "  7\n"
             "%s\n"          // Text style
             "  8\n"
             "%s\n"          // Layer name
             "  10\n"
             "%s\n"          // First point X
             "  11\n"
             "%s\n"          // Second point X
             "  20\n"
             "%s\n"          // First point Y
             "  21\n"
             "%s\n"          // Second point Y
             "  40\n"
             "%s\n"          // Text height
             "  41\n"
             "%s\n"          // Width factor
             "  50\n"
             "%.8f\n"        // Rotation
             "  51\n"
             "%.8f\n"        // Oblique angle
             "  71\n"
             "%d\n"          // Mirror flags
             "  72\n"
             "%d\n"          // H alignment
             "  73\n"
             "%d\n",         // V alignment
             aAttributes.m_Bold ?
                (aAttributes.m_Italic ? "KICADBI" : "KICADB")
                : (aAttributes.m_Italic ? "KICADI" : "KICAD"), TO_UTF8( cname ),
             formatCoord( origin_dev.x ).c_str(), formatCoord( origin_dev.x ).c_str(),
             formatCoord( origin_dev.y ).c_str(), formatCoord( origin_dev.y ).c_str(),
             formatCoord( size_dev.y ).c_str(), formatCoord( fabs( size_dev.x / size_dev.y ) ).c_str(),
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

    fputs( "  1\n", m_outputFile );

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
                fputs( "%%o", m_outputFile );
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
                    fputs( "%%O", m_outputFile );
                    overbarDepth = -1;
                    continue;
                }
            }

            putc( ch, m_outputFile );
        }
    }

    putc( '\n', m_outputFile );
}
