/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/* Some info on basic items SVG format, used here:
 *  The root element of all SVG files is the <svg> element.
 *
 *  The <g> element is used to group SVG shapes together.
 *  Once grouped you can transform the whole group of shapes as if it was a single shape.
 *  This is an advantage compared to a nested <svg> element
 *  which cannot be the target of transformation by itself.
 *
 *  The <rect> element represents a rectangle.
 *  Using this element you can draw rectangles of various width, height,
 *  with different stroke (outline) and fill colors, with sharp or rounded corners etc.
 *
 *  <svg xmlns="http://www.w3.org/2000/svg"
 *    xmlns:xlink="http://www.w3.org/1999/xlink">
 *
 *   <rect x="10" y="10" height="100" width="100"
 *         style="stroke:#006600; fill: #00cc00"/>
 *
 *  </svg>
 *
 *  The <circle> element is used to draw circles.
 *   <circle cx="40" cy="40" r="24" style="stroke:#006600; fill:#00cc00"/>
 *
 *  The <ellipse> element is used to draw ellipses.
 *  An ellipse is a circle that does not have equal height and width.
 *  Its radius in the x and y directions are different, in other words.
 *  <ellipse cx="40" cy="40" rx="30" ry="15"
 *          style="stroke:#006600; fill:#00cc00"/>
 *
 *  The <line> element is used to draw lines.
 *
 *   <line x1="0"  y1="10" x2="0"   y2="100" style="stroke:#006600;"/>
 *   <line x1="10" y1="10" x2="100" y2="100" style="stroke:#006600;"/>
 *
 *  The <polyline> element is used to draw multiple connected lines
 *  Here is a simple example:
 *
 *   <polyline points="0,0  30,0  15,30" style="stroke:#006600;"/>
 *
 *  The <polygon> element is used to draw with multiple (3 or more) sides / edges.
 *  Here is a simple example:
 *
 *   <polygon points="0,0  50,0  25,50" style="stroke:#660000; fill:#cc3333;"/>
 *
 *  The <path> element is used to draw advanced shapes combined from lines and arcs,
 *  with or without fill.
 *  It is probably the most advanced and versatile SVG shape of them all.
 *  It is probably also the hardest element to master.
 *   <path d="M50,50
 *            A30,30 0 0,1 35,20
 *            L100,100
 *            M110,110
 *            L100,0"
 *         style="stroke:#660000; fill:none;"/>
 *
 *  Draw an elliptic arc: it is one of basic path command:
 * <path d="M(startx,starty) A(radiusx,radiusy)
 *          rotation-axe-x
 *          flag_arc_large,flag_sweep endx,endy">
 * flag_arc_large: 0 = small arc > 180 deg, 1 = large arc > 180 deg
 * flag_sweep : 0 = CCW, 1 = CW
 * The center of ellipse is automatically calculated.
 */

#include <core/base64.h>
#include <eda_shape.h>
#include <string_utils.h>
#include <font/font.h>
#include <macros.h>
#include <trigo.h>
#include <fmt/format.h>

#include <cstdint>
#include <wx/mstream.h>

#include <plotters/plotters_pslike.h>

// Note:
// During tests, we (JPC) found issues when the coordinates used 6 digits in mantissa
// especially for stroke-width using very small (but not null) values < 0.00001 mm
// So to avoid this king of issue, we are using 4 digits in mantissa
// The resolution (m_precision ) is 0.1 micron, that looks enough for a SVG file

/**
 * Translates '<' to "&lt;", '>' to "&gt;" and so on, according to the spec:
 * http://www.w3.org/TR/2000/WD-xml-c14n-20000119.html#charescaping
 * May be moved to a library if needed generally, but not expecting that.
 */
static wxString XmlEsc( const wxString& aStr, bool isAttribute = false )
{
    wxString    escaped;

    escaped.reserve( aStr.length() );

    for( wxString::const_iterator it = aStr.begin();  it != aStr.end();  ++it )
    {
        const wxChar c = *it;

        switch( c )
        {
        case wxS( '<' ):
            escaped.append( wxS( "&lt;" ) );
            break;
        case wxS( '>' ):
            escaped.append( wxS( "&gt;" ) );
            break;
        case wxS( '&' ):
            escaped.append( wxS( "&amp;" ) );
            break;
        case wxS( '\r' ):
            escaped.append( wxS( "&#xD;" ) );
            break;
        default:
            if( isAttribute )
            {
                switch( c )
                {
                case wxS( '"' ):
                    escaped.append( wxS( "&quot;" ) );
                    break;
                case wxS( '\t' ):
                    escaped.append( wxS( "&#x9;" ) );
                    break;
                case wxS( '\n' ):
                    escaped.append( wxS( "&#xA;" ));
                    break;
                default:
                    escaped.append(c);
                }
            }
            else
            {
                escaped.append(c);
            }
        }
    }

    return escaped;
}


SVG_PLOTTER::SVG_PLOTTER( const PROJECT* aProject ) :
    PSLIKE_PLOTTER( aProject )
{
    m_graphics_changed = true;
    SetTextMode( PLOT_TEXT_MODE::STROKE );
    m_fillMode        = FILL_T::NO_FILL; // or FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR
    m_pen_rgb_color   = 0;               // current color value (black)
    m_brush_rgb_color = 0;               // current color value with alpha(black)
    m_brush_alpha     = 1.0;
    m_dashed          = LINE_STYLE::SOLID;
    m_precision       = 4;               // default: 4 digits in mantissa.
}


void SVG_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                               double aScale, bool aMirror )
{
    m_plotMirror    = aMirror;
    m_yaxisReversed = true;     // unlike other plotters, SVG has Y axis reversed
    m_plotOffset    = aOffset;
    m_plotScale     = aScale;
    m_IUsPerDecimil = aIusPerDecimil;

    // Compute the paper size in IUs. for historical reasons the page size is in mils
    m_paperSize   = m_pageInfo.GetSizeMils();
    m_paperSize.x *= 10.0 * aIusPerDecimil;
    m_paperSize.y *= 10.0 * aIusPerDecimil;

    // gives now a default value to iuPerDeviceUnit (because the units of the caller is now known)
    double iusPerMM = m_IUsPerDecimil / 2.54 * 1000;
    m_iuPerDeviceUnit = 1 / iusPerMM;

    SetSvgCoordinatesFormat( 4 );
}


void SVG_PLOTTER::SetSvgCoordinatesFormat( unsigned aPrecision )
{
    // Only number of digits in mantissa are adjustable.
    // SVG units are always mm
    m_precision = aPrecision;
}


void SVG_PLOTTER::setFillMode( FILL_T fill )
{
    if( m_fillMode != fill )
    {
        m_graphics_changed = true;
        m_fillMode = fill;
    }
}


void SVG_PLOTTER::setSVGPlotStyle( int aLineWidth, bool aIsGroup, const std::string& aExtraStyle )
{
    if( aIsGroup )
        fmt::print( m_outputFile, "</g>\n<g " );

    fmt::print( m_outputFile, "style=\"" );

    if( m_fillMode == FILL_T::NO_FILL )
    {
        fmt::print( m_outputFile, "fill:none; " );
    }
    else
    {
        // output the background fill color
        fmt::print( m_outputFile, "fill:#{:06X}; ", m_brush_rgb_color );

        switch( m_fillMode )
        {
        case FILL_T::FILLED_SHAPE:
        case FILL_T::FILLED_WITH_BG_BODYCOLOR:
        case FILL_T::FILLED_WITH_COLOR:
            fmt::print( m_outputFile, "fill-opacity:{:.{}f}; ", m_brush_alpha, m_precision );
            break;
        default: break;
        }
    }

    double pen_w = userToDeviceSize( aLineWidth );

    if( pen_w <= 0 )
    {
        fmt::print( m_outputFile, "stroke:none;" );
    }
    else
    {
        // Fix a strange issue found in Inkscape: aWidth < 100 nm create issues on degrouping
        // objects.
        // So we use only 4 digits in mantissa for stroke-width.
        // TODO: perhaps used only 3 or 4 digits in mantissa for all values in mm, because some
        // issues were previously reported reported when using nm as integer units
        fmt::print( m_outputFile, "\nstroke:#{:06X}; stroke-width:{:.{}f}; stroke-opacity:1; \n",
                    m_pen_rgb_color, pen_w, m_precision );
        fmt::print( m_outputFile, "stroke-linecap:round; stroke-linejoin:round;" );

        //set any extra attributes for non-solid lines
        switch( m_dashed )
        {
        case LINE_STYLE::DASH:
            fmt::print( m_outputFile, "stroke-dasharray:{:.{}f},{:.{}f};",
                        GetDashMarkLenIU( aLineWidth ), m_precision,
                        GetDashGapLenIU( aLineWidth ), m_precision );
            break;

        case LINE_STYLE::DOT:
            fmt::print( m_outputFile, "stroke-dasharray:{:f},{:f};", GetDotMarkLenIU( aLineWidth ),
                        GetDashGapLenIU( aLineWidth ) );
            break;

        case LINE_STYLE::DASHDOT:
            fmt::print( m_outputFile, "stroke-dasharray:{:f},{:f},{:f},{:f};",
                        GetDashMarkLenIU( aLineWidth ),
                        GetDashGapLenIU( aLineWidth ), GetDotMarkLenIU( aLineWidth ),
                        GetDashGapLenIU( aLineWidth ) );
            break;

        case LINE_STYLE::DASHDOTDOT:
            fmt::print( m_outputFile, "stroke-dasharray:{:f},{:f},{:f},{:f},{:f},{:f};",
                        GetDashMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ),
                        GetDotMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ),
                        GetDotMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ) );
            break;

        case LINE_STYLE::DEFAULT:
        case LINE_STYLE::SOLID:
        default:
            //do nothing
            break;
        }
    }

    if( aExtraStyle.length() )
        fmt::print( m_outputFile, "{}", aExtraStyle );

    fmt::print( m_outputFile, "\"" );

    if( aIsGroup )
    {
        fmt::print( m_outputFile, ">" );
        m_graphics_changed = false;
    }

    fmt::print( m_outputFile, "\n" );
}


void SVG_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    if( aWidth == DO_NOT_SET_LINE_WIDTH )
        return;
    else if( aWidth == USE_DEFAULT_LINE_WIDTH )
        aWidth = m_renderSettings->GetDefaultPenWidth();

    // Note: aWidth == 0 is fine: used for filled shapes with no outline thickness
    wxASSERT_MSG( aWidth >= 0, "Plotter called to set negative pen width" );

    if( aWidth != m_currentPenWidth )
    {
        m_graphics_changed  = true;
        m_currentPenWidth   = aWidth;
    }
}


void SVG_PLOTTER::StartBlock( void* aData )
{
    // We can't use <g></g> for blocks because we're already using it for graphics context, and
    // our graphics context handling is lazy (ie: it leaves the last group open until the context
    // changes).
}


void SVG_PLOTTER::EndBlock( void* aData )
{
}


void SVG_PLOTTER::StartLayer( const wxString& aLayerName )
{
    // Close any pending graphics context group
    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );

    // Start a new named layer group with inkscape-compatible layer attributes
    fmt::print( m_outputFile, "<g id=\"{}\" inkscape:label=\"{}\" inkscape:groupmode=\"layer\">\n",
                TO_UTF8( aLayerName ), TO_UTF8( aLayerName ) );
}


void SVG_PLOTTER::EndLayer()
{
    // Close any pending graphics context group first
    fmt::print( m_outputFile, "</g>\n" );
    // Then close the layer group
    fmt::print( m_outputFile, "</g>\n" );
    m_graphics_changed = true; // Force new graphics context on next draw
}


void SVG_PLOTTER::emitSetRGBColor( double r, double g, double b, double a )
{
    uint32_t red = (uint32_t) ( 255.0 * r );
    uint32_t green = (uint32_t) ( 255.0 * g );
    uint32_t blue = (uint32_t) ( 255.0 * b );
    uint32_t rgb_color = ( red << 16 ) | ( green << 8 ) | blue;

    if( m_pen_rgb_color != rgb_color || m_brush_alpha != a )
    {
        m_graphics_changed = true;
        m_pen_rgb_color = rgb_color;

        // Currently, use the same color for brush and pen (i.e. to draw and fill a contour).
        m_brush_rgb_color = rgb_color;
        m_brush_alpha = a;
    }
}


void SVG_PLOTTER::SetDash( int aLineWidth, LINE_STYLE aLineStyle )
{
    if( m_dashed != aLineStyle )
    {
        m_graphics_changed = true;
        m_dashed = aLineStyle;
    }
}


void SVG_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                        int aCornerRadius )
{
    BOX2I rect( p1, VECTOR2I( p2.x - p1.x, p2.y - p1.y ) );
    rect.Normalize();

    VECTOR2D  org_dev  = userToDeviceCoordinates( rect.GetOrigin() );
    VECTOR2D  end_dev = userToDeviceCoordinates( rect.GetEnd() );
    VECTOR2D  size_dev = end_dev - org_dev;

    // Ensure size of rect in device coordinates is > 0
    // I don't know if this is a SVG issue or a Inkscape issue, but
    // Inkscape has problems with negative or null values for width and/or height, so avoid them
    BOX2D rect_dev( org_dev, size_dev );
    rect_dev.Normalize();

    setFillMode( fill );
    SetCurrentLineWidth( width );

    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );

    // Rectangles having a 0 size value for height or width are just not drawn on Inkscape,
    // so use a line when happens.
    if( rect_dev.GetSize().x == 0.0 || rect_dev.GetSize().y == 0.0 )    // Draw a line
    {
        fmt::print( m_outputFile,
                    "<line x1=\"{:.{}f}\" y1=\"{:.{}f}\" x2=\"{:.{}f}\" y2=\"{:.{}f}\" />\n",
                    rect_dev.GetPosition().x, m_precision,
                    rect_dev.GetPosition().y, m_precision,
                    rect_dev.GetEnd().x, m_precision,
                    rect_dev.GetEnd().y, m_precision );
    }
    else
    {
        fmt::print( m_outputFile,
                      "<rect x=\"{:f}\" y=\"{:f}\" width=\"{:f}\" height=\"{:f}\" rx=\"{:f}\" />\n",
                      rect_dev.GetPosition().x,
                      rect_dev.GetPosition().y,
                      rect_dev.GetSize().x,
                      rect_dev.GetSize().y,
                      userToDeviceSize( aCornerRadius ) );
    }
}


void SVG_PLOTTER::Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width )
{
    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    double   radius  = userToDeviceSize( diametre / 2.0 );

    setFillMode( fill );
    SetCurrentLineWidth( width );

    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );

    // If diameter is less than width, switch to filled mode
    if( fill == FILL_T::NO_FILL && diametre < GetCurrentLineWidth() )
    {
        setFillMode( FILL_T::FILLED_SHAPE );
        width = GetCurrentLineWidth();
        SetCurrentLineWidth( 0 );

        radius = userToDeviceSize( ( diametre / 2.0 ) + ( width / 2.0 ) );
    }

    fmt::print( m_outputFile,
                  "<circle cx=\"{:.{}f}\" cy=\"{:.{}f}\" r=\"{:.{}f}\" /> \n",
                  pos_dev.x, m_precision,
                  pos_dev.y, m_precision,
                  radius, m_precision );
}


void SVG_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    /* Draws an arc of a circle, centered on (xc,yc), with starting point (x1, y1) and ending
     * at (x2, y2). The current pen is used for the outline and the current brush for filling
     * the shape.
     *
     *  The arc is drawn in an anticlockwise direction from the start point to the end point.
     */
    if( aRadius <= 0 )
    {
        Circle( aCenter, aWidth, FILL_T::FILLED_SHAPE, 0 );
        return;
    }

    EDA_ANGLE startAngle = -aStartAngle;
    EDA_ANGLE endAngle = startAngle - aAngle;

    if( endAngle < startAngle )
        std::swap( startAngle, endAngle );

    // Calculate start point.
    VECTOR2D  centre_device  = userToDeviceCoordinates( aCenter );
    double  radius_device  = userToDeviceSize( aRadius );

    if( m_plotMirror )
    {
        if( m_mirrorIsHorizontal )
        {
            std::swap( startAngle, endAngle );
            startAngle = ANGLE_180 - startAngle;
            endAngle = ANGLE_180 - endAngle;
        }
        else
        {
            startAngle = -startAngle;
            endAngle = -endAngle;
        }
    }

    VECTOR2D  start;
    start.x = radius_device;
    RotatePoint( start, startAngle );
    VECTOR2D  end;
    end.x = radius_device;
    RotatePoint( end, endAngle );
    start += centre_device;
    end += centre_device;

    double theta1 = startAngle.AsRadians();

    if( theta1 < 0 )
        theta1 = theta1 + M_PI * 2;

    double theta2 = endAngle.AsRadians();

    if( theta2 < 0 )
        theta2 = theta2 + M_PI * 2;

    if( theta2 < theta1 )
        theta2 = theta2 + M_PI * 2;

    int flg_arc = 0;    // flag for large or small arc. 0 means less than 180 degrees

    if( fabs( theta2 - theta1 ) > M_PI )
        flg_arc = 1;

    int flg_sweep = 0;             // flag for sweep always 0

    // Draw a single arc: an arc is one of 3 curve commands (2 other are 2 bezier curves)
    // params are start point, radius1, radius2, X axe rotation,
    // flag arc size (0 = small arc > 180 deg, 1 = large arc > 180 deg),
    // sweep arc ( 0 = CCW, 1 = CW),
    // end point
    if( aFill != FILL_T::NO_FILL )
    {
        // Filled arcs (in Eeschema) consist of the pie wedge and a stroke only on the arc
        // This needs to be drawn in two steps.
        setFillMode( aFill );
        SetCurrentLineWidth( 0 );

        if( m_graphics_changed )
            setSVGPlotStyle( GetCurrentLineWidth() );

        fmt::print( m_outputFile,
                   "<path d=\"M{:.{}f} {:.{}f} A{:.{}f} {:.{}f} 0.0 {:d} {:d} {:.{}f} {:.{}f} L {:.{}f} {:.{}f} Z\" />\n",
                    start.x, m_precision,
                    start.y, m_precision,
                    radius_device, m_precision,
                    radius_device, m_precision,
                    flg_arc,
                    flg_sweep,
                    end.x, m_precision,
                    end.y, m_precision,
                    centre_device.x, m_precision,
                    centre_device.y, m_precision );
    }

    setFillMode( FILL_T::NO_FILL );
    SetCurrentLineWidth( aWidth );

    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );

    fmt::print( m_outputFile,
                "<path d=\"M{:.{}f} {:.{}f} A{:.{}f} {:.{}f} 0.0 {:d} {:d} {:.{}f} {:.{}f}\" />\n",
                start.x, m_precision,
                start.y, m_precision,
                radius_device, m_precision,
                radius_device, m_precision,
                flg_arc,
                flg_sweep,
                end.x, m_precision,
                end.y, m_precision );
}


void SVG_PLOTTER::BezierCurve( const VECTOR2I& aStart, const VECTOR2I& aControl1,
                               const VECTOR2I& aControl2, const VECTOR2I& aEnd,
                               int aTolerance, int aLineThickness )
{
#if 1
    setFillMode( FILL_T::NO_FILL );
    SetCurrentLineWidth( aLineThickness );

    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );

    VECTOR2D start  = userToDeviceCoordinates( aStart );
    VECTOR2D ctrl1  = userToDeviceCoordinates( aControl1 );
    VECTOR2D ctrl2  = userToDeviceCoordinates( aControl2 );
    VECTOR2D end  = userToDeviceCoordinates( aEnd );

    // Generate a cubic curve: start point and 3 other control points.
    fmt::print( m_outputFile,
                "<path d=\"M{:.{}f},{:.{}f} C{:.{}f},{:.{}f} {:.{}f},{:.{}f} {:.{}f},{:.{}f}\" />\n",
                start.x, m_precision,
                start.y, m_precision,
                ctrl1.x,m_precision,
                ctrl1.y, m_precision,
                ctrl2.x, m_precision,
                ctrl2.y, m_precision,
                end.x, m_precision,
                end.y, m_precision  );
#else
    PLOTTER::BezierCurve( aStart, aControl1, aControl2, aEnd, aTolerance, aLineThickness );
#endif
}


void SVG_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill,
                            int aWidth, void* aData )
{
    if( aCornerList.size() <= 1 )
        return;

    setFillMode( aFill );
    SetCurrentLineWidth( aWidth );
    fmt::print( m_outputFile, "<path " );

    switch( aFill )
    {
    case FILL_T::NO_FILL:
    case FILL_T::HATCH:
    case FILL_T::REVERSE_HATCH:
    case FILL_T::CROSS_HATCH:
        setSVGPlotStyle( aWidth, false, "fill:none" );
        break;

    case FILL_T::FILLED_WITH_BG_BODYCOLOR:
    case FILL_T::FILLED_SHAPE:
    case FILL_T::FILLED_WITH_COLOR:
        setSVGPlotStyle( aWidth, false, "fill-rule:evenodd;" );
        break;
    }

    VECTOR2D pos = userToDeviceCoordinates( aCornerList[0] );
    fmt::print( m_outputFile, "d=\"M {:.{}f},{:.{}f}\n", pos.x, m_precision, pos.y, m_precision );

    for( unsigned ii = 1; ii < aCornerList.size() - 1; ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fmt::print( m_outputFile, "{:.{}f},{:.{}f}\n", pos.x, m_precision, pos.y, m_precision );
    }

    // If the corner list ends where it begins, then close the poly
    if( aCornerList.front() == aCornerList.back() )
    {
        fmt::print( m_outputFile, "Z\" /> \n" );
    }
    else
    {
        pos = userToDeviceCoordinates( aCornerList.back() );
        fmt::print( m_outputFile,
                    "{:.{}f},{:.{}f}\n\" /> \n",
                    pos.x, m_precision,
                    pos.y, m_precision );
    }
}


void SVG_PLOTTER::PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor )
{
    VECTOR2I pix_size( aImage.GetWidth(), aImage.GetHeight() );

    // Requested size (in IUs)
    VECTOR2D drawsize( aScaleFactor * pix_size.x, aScaleFactor * pix_size.y );

    // calculate the bitmap start position
    VECTOR2I start( aPos.x - drawsize.x / 2, aPos.y - drawsize.y / 2 );

    // Rectangles having a 0 size value for height or width are just not drawn on Inkscape,
    // so use a line when happens.
    if( drawsize.x == 0.0 || drawsize.y == 0.0 )    // Draw a line
    {
        PLOTTER::PlotImage( aImage, aPos, aScaleFactor );
    }
    else
    {
        wxMemoryOutputStream img_stream;

        if( m_colorMode )
        {
            aImage.SaveFile( img_stream, wxBITMAP_TYPE_PNG );
        }
        else    // Plot in B&W
        {
            wxImage image = aImage.ConvertToGreyscale();
            image.SaveFile( img_stream, wxBITMAP_TYPE_PNG );
        }

        size_t input_len = img_stream.GetOutputStreamBuffer()->GetBufferSize();
        std::vector<uint8_t> buffer( input_len );
        std::vector<uint8_t> encoded;

        img_stream.CopyTo( buffer.data(), buffer.size() );
        base64::encode( buffer, encoded );

        VECTOR2D pos = userToDeviceCoordinates( start );
        fmt::print( m_outputFile,
                    "<image x=\"{:f}\" y=\"{:f}\" xlink:href=\"data:image/png;base64,", pos.x, pos.y );

        for( size_t i = 0; i < encoded.size(); i++ )
        {
            fmt::print( m_outputFile, "{}", static_cast<char>( encoded[i] ) );

            if( ( i % 64 )  == 63 )
                fmt::print( m_outputFile, "\n" );
        }

        fmt::print( m_outputFile,
                    "\"\npreserveAspectRatio=\"none\" width=\"{:.{}f}\" height=\"{:.{}f}\" />",
                    userToDeviceSize( drawsize.x ), m_precision,
                    userToDeviceSize( drawsize.y ), m_precision );
    }
}


void SVG_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    if( plume == 'Z' )
    {
        if( m_penState != 'Z' )
        {
            fmt::print( m_outputFile, "\" />\n" );
            m_penState        = 'Z';
            m_penLastpos.x    = -1;
            m_penLastpos.y    = -1;
        }

        return;
    }

    if( m_penState == 'Z' )    // here plume = 'D' or 'U'
    {
        VECTOR2D pos_dev = userToDeviceCoordinates( pos );

        // Ensure we do not use a fill mode when moving the pen,
        // in SVG mode (i;e. we are plotting only basic lines, not a filled area
        if( m_fillMode != FILL_T::NO_FILL )
            setFillMode( FILL_T::NO_FILL );

        if( m_graphics_changed )
            setSVGPlotStyle( GetCurrentLineWidth() );

        fmt::print( m_outputFile, "<path d=\"M{:.{}f} {:.{}f}\n",
                    pos_dev.x, m_precision,
                    pos_dev.y, m_precision );
    }
    else if( m_penState != plume || pos != m_penLastpos )
    {
        if( m_graphics_changed )
            setSVGPlotStyle( GetCurrentLineWidth() );

        VECTOR2D pos_dev = userToDeviceCoordinates( pos );

        fmt::print( m_outputFile, "L{:.{}f} {:.{}f}\n",
                    pos_dev.x, m_precision,
                    pos_dev.y, m_precision );
    }

    m_penState    = plume;
    m_penLastpos  = pos;
}


bool SVG_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    wxASSERT( m_outputFile );

    std::string header = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
                " <!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \n"
                " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"> \n"
                "<svg\n"
                "  xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
                "  xmlns=\"http://www.w3.org/2000/svg\"\n"
                "  xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
                "  xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
                "  version=\"1.1\"\n";

    // Write header.
    fmt::print( m_outputFile, "{}", header );

    // Write viewport pos and size
    VECTOR2D origin;    // TODO set to actual value
    fmt::print( m_outputFile,
                "  width=\"{:.{}f}mm\" height=\"{:.{}f}mm\" viewBox=\"{:.{}f} {:.{}f} {:.{}f} {:.{}f}\">\n",
                (double) m_paperSize.x / m_IUsPerDecimil * 2.54 / 1000, m_precision,
                (double) m_paperSize.y / m_IUsPerDecimil * 2.54 / 1000, m_precision,
                origin.x, m_precision, origin.y, m_precision,
                m_paperSize.x * m_iuPerDeviceUnit, m_precision,
                m_paperSize.y * m_iuPerDeviceUnit, m_precision );

    // Write title
    wxString date = GetISO8601CurrentDateTime();

    fmt::print( m_outputFile,
                "<title>SVG Image created as {} date {} </title>\n",
                TO_UTF8( XmlEsc( wxFileName( m_filename ).GetFullName() ) ),
                TO_UTF8( date ) );

    // End of header
    fmt::print( m_outputFile, "  <desc>Image generated by {} </desc>\n",
                TO_UTF8( XmlEsc( m_creator ) ) );

    // output the pen and brush color (RVB values in hex) and opacity
    double opacity = 1.0;      // 0.0 (transparent to 1.0 (solid)
    fmt::print( m_outputFile,
                  "<g style=\"fill:#{:06X}; fill-opacity:{:.{}f};stroke:#{:06X}; stroke-opacity:{:.{}f};\n",
                  m_brush_rgb_color,
                  m_brush_alpha,
                  m_precision,
                  m_pen_rgb_color,
                  opacity,
                  m_precision );

    // output the pen cap and line joint
    fmt::print( m_outputFile, "stroke-linecap:round; stroke-linejoin:round;\"\n" );
    fmt::print( m_outputFile, " transform=\"translate(0 0) scale(1 1)\">\n" );
    return true;
}


bool SVG_PLOTTER::EndPlot()
{
    fmt::print( m_outputFile, "</g> \n</svg>\n" );
    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void SVG_PLOTTER::Text( const VECTOR2I&        aPos,
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
    setFillMode( FILL_T::NO_FILL );
    SetColor( aColor );
    SetCurrentLineWidth( aWidth );

    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );

    VECTOR2I    text_pos = aPos;
    const char* hjust = "start";

    switch( aH_justify )
    {
    case GR_TEXT_H_ALIGN_CENTER: hjust = "middle"; break;
    case GR_TEXT_H_ALIGN_RIGHT:  hjust = "end";    break;
    case GR_TEXT_H_ALIGN_LEFT:   hjust = "start";  break;
    case GR_TEXT_H_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_V_ALIGN_CENTER: text_pos.y += aSize.y / 2; break;
    case GR_TEXT_V_ALIGN_TOP:    text_pos.y += aSize.y;     break;
    case GR_TEXT_V_ALIGN_BOTTOM:                            break;
    case GR_TEXT_V_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    VECTOR2I text_size;

    // aSize.x or aSize.y is < 0 for mirrored texts.
    // The actual text size value is the absolute value
    text_size.x = std::abs( GRTextWidth( aText, aFont, aSize, GetCurrentLineWidth(), aBold, aItalic,
                                         aFontMetrics ) );
    text_size.y = std::abs( aSize.x * 4/3 ); // Hershey font height to em size conversion
    VECTOR2D anchor_pos_dev = userToDeviceCoordinates( aPos );
    VECTOR2D text_pos_dev = userToDeviceCoordinates( text_pos );
    VECTOR2D sz_dev = userToDeviceSize( text_size );

    // Output the text as a hidden string (opacity = 0).  This allows WYSIWYG search to highlight
    // a selection in approximately the right area.  It also makes it easier for those that need
    // to edit the text (as text) in subsequent processes.
    {
        if( !aOrient.IsZero() )
        {
            fmt::print( m_outputFile,
                         "<g transform=\"rotate({:f} {:.{}f} {:.{}f})\">\n",
                         m_plotMirror ? aOrient.AsDegrees() : -aOrient.AsDegrees(),
                         anchor_pos_dev.x,
                         m_precision,
                         anchor_pos_dev.y,
                         m_precision );
        }

        fmt::print( m_outputFile,
                    "<text x=\"{:.{}f}\" y=\"{:.{}f}\"\n",
                    text_pos_dev.x, m_precision,
                    text_pos_dev.y, m_precision );

        /// If the text is mirrored, we should also mirror the hidden text to match
        if( m_plotMirror != ( aSize.x < 0 ) )
        {
            fmt::print( m_outputFile, "transform=\"scale(-1 1) translate({:f} 0)\"\n",
                        -2 * text_pos_dev.x );
        }

        fmt::print( m_outputFile,
                    "textLength=\"{:.{}f}\" font-size=\"{:.{}f}\" lengthAdjust=\"spacingAndGlyphs\"\n"
                    "text-anchor=\"{}\" opacity=\"0\" stroke-opacity=\"0\">{}</text>\n",
                    sz_dev.x,
                    m_precision,
                    sz_dev.y,
                    m_precision,
                    hjust,
                    TO_UTF8( XmlEsc( aText ) ) );

        if( !aOrient.IsZero() )
            fmt::print( m_outputFile, "</g>\n" );
    }

    // Output the text again as graphics with a <desc> tag (for non-WYSIWYG search and for
    // screen readers)
    {
        fmt::print( m_outputFile,
                    "<g class=\"stroked-text\"><desc>{}</desc>\n",
                    TO_UTF8( XmlEsc( aText ) ) );

        PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, GetCurrentLineWidth(),
                       aItalic, aBold, aMultilineAllowed, aFont, aFontMetrics );

        fmt::print( m_outputFile, "</g>" );
    }
}


void SVG_PLOTTER::PlotText( const VECTOR2I&        aPos,
                            const COLOR4D&         aColor,
                            const wxString&        aText,
                            const TEXT_ATTRIBUTES& aAttributes,
                            KIFONT::FONT*          aFont,
                            const KIFONT::METRICS& aFontMetrics,
                            void*                  aData )
{
    VECTOR2I size = aAttributes.m_Size;

    if( aAttributes.m_Mirrored )
        size.x = -size.x;

    SVG_PLOTTER::Text( aPos, aColor, aText, aAttributes.m_Angle, size, aAttributes.m_Halign,
                       aAttributes.m_Valign, aAttributes.m_StrokeWidth, aAttributes.m_Italic,
                       aAttributes.m_Bold, aAttributes.m_Multiline, aFont, aFontMetrics, aData );
}
