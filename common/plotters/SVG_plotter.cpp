/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base64.h>
#include <eda_shape.h>
#include <string_utils.h>
#include <font/font.h>
#include <macros.h>
#include <trigo.h>

#include <cstdint>
#include <wx/mstream.h>

#include <plotters/plotters_pslike.h>

// Note:
// During tests, we (JPC) found issues when the coordinates used 6 digits in mantissa
// especially for stroke-width using very small (but not null) values < 0.00001 mm
// So to avoid this king of issue, we are using 4 digits in mantissa
// The resolution (m_precision ) is 0.1 micron, that looks enougt for a SVG file

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
                escaped.append(c);
        }
    }

    return escaped;
}


SVG_PLOTTER::SVG_PLOTTER()
{
    m_graphics_changed = true;
    SetTextMode( PLOT_TEXT_MODE::STROKE );
    m_fillMode        = FILL_T::NO_FILL; // or FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR
    m_pen_rgb_color   = 0;          // current color value (black)
    m_brush_rgb_color = 0;          // current color value (black)
    m_brush_alpha     = 1.0;
    m_dashed          = PLOT_DASH_TYPE::SOLID;
    m_useInch         = false;      // millimeters are always the svg unit
    m_precision       = 4;          // default: 4 digits in mantissa.
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


void SVG_PLOTTER::SetColor( const COLOR4D& color )
{
    PSLIKE_PLOTTER::SetColor( color );

    if( m_graphics_changed )
        setSVGPlotStyle( GetCurrentLineWidth() );
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
        fputs( "</g>\n<g ", m_outputFile );

    // output the background fill color
    fprintf( m_outputFile, "style=\"fill:#%6.6lX; ", m_brush_rgb_color );

    switch( m_fillMode )
    {
    case FILL_T::NO_FILL:
        fputs( "fill-opacity:0.0; ", m_outputFile );
        break;

    case FILL_T::FILLED_SHAPE:
    case FILL_T::FILLED_WITH_BG_BODYCOLOR:
    case FILL_T::FILLED_WITH_COLOR:
        fprintf( m_outputFile, "fill-opacity:%.*f; ", m_precision, m_brush_alpha );
        break;
    }

    double pen_w = userToDeviceSize( aLineWidth );

    if( pen_w < 0.0 )   // Ensure pen width validity
        pen_w = 0.0;

    // Fix a strange issue found in Inkscape: aWidth < 100 nm create issues on degrouping objects
    // So we use only 4 digits in mantissa for stroke-width.
    // TODO: perhaps used only 3 or 4 digits in mantissa for all values in mm, because some
    // issues were previously reported reported when using nm as integer units

    fprintf( m_outputFile, "\nstroke:#%6.6lX; stroke-width:%.*f; stroke-opacity:1; \n",
             m_pen_rgb_color, m_precision, pen_w  );
    fputs( "stroke-linecap:round; stroke-linejoin:round;", m_outputFile );

    //set any extra attributes for non-solid lines
    switch( m_dashed )
    {
    case PLOT_DASH_TYPE::DASH:
        fprintf( m_outputFile, "stroke-dasharray:%.*f,%.*f;",
                 m_precision, GetDashMarkLenIU( aLineWidth ),
                 m_precision, GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DOT:
        fprintf( m_outputFile, "stroke-dasharray:%f,%f;",
                 GetDotMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DASHDOT:
        fprintf( m_outputFile, "stroke-dasharray:%f,%f,%f,%f;",
                 GetDashMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ),
                 GetDotMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DASHDOTDOT:
        fprintf( m_outputFile, "stroke-dasharray:%f,%f,%f,%f,%f,%f;",
                 GetDashMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ),
                 GetDotMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ),
                 GetDotMarkLenIU( aLineWidth ), GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DEFAULT:
    case PLOT_DASH_TYPE::SOLID:
    default:
        //do nothing
        break;
    }

    if( aExtraStyle.length() )
        fputs( aExtraStyle.c_str(), m_outputFile );

    fputs( "\"", m_outputFile );

    if( aIsGroup )
    {
        fputs( ">", m_outputFile );
        m_graphics_changed = false;
    }

    fputs( "\n", m_outputFile );
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

    if( m_graphics_changed )
        setSVGPlotStyle( aWidth );
}


void SVG_PLOTTER::StartBlock( void* aData )
{
    std::string* idstr = reinterpret_cast<std::string*>( aData );

    fputs( "<g ", m_outputFile );

    if( idstr )
        fprintf( m_outputFile, "id=\"%s\"", idstr->c_str() );

    fprintf( m_outputFile, ">\n" );
}


void SVG_PLOTTER::EndBlock( void* aData )
{
    fprintf( m_outputFile, "</g>\n" );

    m_graphics_changed = true;
}


void SVG_PLOTTER::emitSetRGBColor( double r, double g, double b, double a )
{
    int red     = (int) ( 255.0 * r );
    int green   = (int) ( 255.0 * g );
    int blue    = (int) ( 255.0 * b );
    long rgb_color = (red << 16) | (green << 8) | blue;

    if( m_pen_rgb_color != rgb_color )
    {
        m_graphics_changed = true;
        m_pen_rgb_color = rgb_color;

        // Currently, use the same color for brush and pen (i.e. to draw and fill a contour).
        m_brush_rgb_color = rgb_color;
        m_brush_alpha = a;
    }
}


void SVG_PLOTTER::SetDash( int aLineWidth, PLOT_DASH_TYPE aLineStyle )
{
    if( m_dashed != aLineStyle )
    {
        m_graphics_changed = true;
        m_dashed = aLineStyle;
    }

    if( m_graphics_changed )
        setSVGPlotStyle( aLineWidth );
}


void SVG_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width )
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

    // Rectangles having a 0 size value for height or width are just not drawn on Inkscape,
    // so use a line when happens.
    if( rect_dev.GetSize().x == 0.0 || rect_dev.GetSize().y == 0.0 )    // Draw a line
    {
        fprintf( m_outputFile,
                 "<line x1=\"%.*f\" y1=\"%.*f\" x2=\"%.*f\" y2=\"%.*f\" />\n",
                 m_precision, rect_dev.GetPosition().x, m_precision, rect_dev.GetPosition().y,
                 m_precision, rect_dev.GetEnd().x, m_precision, rect_dev.GetEnd().y );
    }
    else
    {
        fprintf( m_outputFile,
                 "<rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" rx=\"%f\" />\n",
                 rect_dev.GetPosition().x, rect_dev.GetPosition().y,
                 rect_dev.GetSize().x, rect_dev.GetSize().y,
                 0.0 /* radius of rounded corners */ );
    }
}


void SVG_PLOTTER::Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width )
{
    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    double   radius  = userToDeviceSize( diametre / 2.0 );

    setFillMode( fill );
    SetCurrentLineWidth( width );

    // If diameter is less than width, switch to filled mode
    if( fill == FILL_T::NO_FILL && diametre < width )
    {
        setFillMode( FILL_T::FILLED_SHAPE );
        SetCurrentLineWidth( 0 );

        radius = userToDeviceSize( ( diametre / 2.0 ) + ( width / 2.0 ) );
    }

    fprintf( m_outputFile,
             "<circle cx=\"%.*f\" cy=\"%.*f\" r=\"%.*f\" /> \n",
             m_precision, pos_dev.x, m_precision, pos_dev.y, m_precision, radius );
}


void SVG_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aEndAngle, double aRadius, FILL_T aFill, int aWidth )
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

    EDA_ANGLE startAngle( aStartAngle );
    EDA_ANGLE endAngle( aEndAngle );

    if( startAngle > endAngle )
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

        fprintf( m_outputFile, "<path d=\"M%.*f %.*f A%.*f %.*f 0.0 %d %d %.*f %.*f L %.*f %.*f Z\" />\n",
                 m_precision, start.x, m_precision, start.y,
                 m_precision, radius_device, m_precision, radius_device,
                 flg_arc, flg_sweep,
                 m_precision, end.x, m_precision, end.y,
                 m_precision, centre_device.x, m_precision, centre_device.y  );
    }

    setFillMode( FILL_T::NO_FILL );
    SetCurrentLineWidth( aWidth );
    fprintf( m_outputFile, "<path d=\"M%.*f %.*f A%.*f %.*f 0.0 %d %d %.*f %.*f\" />\n",
             m_precision, start.x, m_precision, start.y,
             m_precision, radius_device, m_precision, radius_device,
             flg_arc, flg_sweep,
             m_precision, end.x, m_precision, end.y  );
}


void SVG_PLOTTER::BezierCurve( const VECTOR2I& aStart, const VECTOR2I& aControl1,
                               const VECTOR2I& aControl2, const VECTOR2I& aEnd,
                               int aTolerance, int aLineThickness )
{
#if 1
    setFillMode( FILL_T::NO_FILL );
    SetCurrentLineWidth( aLineThickness );

    VECTOR2D start  = userToDeviceCoordinates( aStart );
    VECTOR2D ctrl1  = userToDeviceCoordinates( aControl1 );
    VECTOR2D ctrl2  = userToDeviceCoordinates( aControl2 );
    VECTOR2D end  = userToDeviceCoordinates( aEnd );

    // Generate a cubic curve: start point and 3 other control points.
    fprintf( m_outputFile, "<path d=\"M%.*f,%.*f C%.*f,%.*f %.*f,%.*f %.*f,%.*f\" />\n",
             m_precision, start.x, m_precision, start.y,
             m_precision, ctrl1.x, m_precision, ctrl1.y,
             m_precision, ctrl2.x, m_precision, ctrl2.y,
             m_precision, end.x, m_precision, end.y  );
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
    fprintf( m_outputFile, "<path ");

    switch( aFill )
    {
    case FILL_T::NO_FILL:
        setSVGPlotStyle( aWidth, false, "fill:none" );
        break;

    case FILL_T::FILLED_WITH_BG_BODYCOLOR:
    case FILL_T::FILLED_SHAPE:
    case FILL_T::FILLED_WITH_COLOR:
        setSVGPlotStyle( aWidth, false, "fill-rule:evenodd;" );
        break;
    }

    VECTOR2D pos = userToDeviceCoordinates( aCornerList[0] );
    fprintf( m_outputFile, "d=\"M %.*f,%.*f\n", m_precision, pos.x, m_precision, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size() - 1; ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fprintf( m_outputFile, "%.*f,%.*f\n", m_precision, pos.x, m_precision, pos.y );
    }

    // If the corner list ends where it begins, then close the poly
    if( aCornerList.front() == aCornerList.back() )
    {
        fprintf( m_outputFile, "Z\" /> \n" );
    }
    else
    {
        pos = userToDeviceCoordinates( aCornerList.back() );
        fprintf( m_outputFile, "%.*f,%.*f\n\" /> \n", m_precision, pos.x, m_precision, pos.y );
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
            aImage.SaveFile( img_stream, wxBITMAP_TYPE_PNG );
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

        fprintf( m_outputFile,
                 "<image x=\"%f\" y=\"%f\" xlink:href=\"data:image/png;base64,",
                 userToDeviceSize( start.x ), userToDeviceSize( start.y ) );

        for( size_t i = 0; i < encoded.size(); i++ )
        {
            fprintf( m_outputFile, "%c", static_cast<char>( encoded[i] ) );

            if( ( i % 64 )  == 63 )
                fprintf( m_outputFile, "\n" );
        }

        fprintf( m_outputFile, "\"\npreserveAspectRatio=\"none\" width=\"%.*f\" height=\"%.*f\" />",
                 m_precision, userToDeviceSize( drawsize.x ), m_precision, userToDeviceSize( drawsize.y ) );
    }
}


void SVG_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    if( plume == 'Z' )
    {
        if( m_penState != 'Z' )
        {
            fputs( "\" />\n", m_outputFile );
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
        {
            setFillMode( FILL_T::NO_FILL );
            setSVGPlotStyle( GetCurrentLineWidth() );
        }

        fprintf( m_outputFile, "<path d=\"M%.*f %.*f\n",
                 m_precision, pos_dev.x,
                 m_precision, pos_dev.y );
    }
    else if( m_penState != plume || pos != m_penLastpos )
    {
        VECTOR2D pos_dev = userToDeviceCoordinates( pos );

        fprintf( m_outputFile, "L%.*f %.*f\n",
                 m_precision, pos_dev.x,
                 m_precision, pos_dev.y );
    }

    m_penState    = plume;
    m_penLastpos  = pos;
}


bool SVG_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    wxASSERT( m_outputFile );

    static const char*  header[] =
    {
        "<?xml version=\"1.0\" standalone=\"no\"?>\n",
        " <!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \n",
        " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"> \n",
        "<svg\n"
        "  xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
        "  xmlns=\"http://www.w3.org/2000/svg\"\n",
        "  xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n",
        "  version=\"1.1\"\n",
        nullptr
    };

    // Write header.
    for( int ii = 0; header[ii] != nullptr; ii++ )
    {
        fputs( header[ii], m_outputFile );
    }

    // Write viewport pos and size
    VECTOR2D origin;    // TODO set to actual value
    fprintf( m_outputFile, "  width=\"%.*fmm\" height=\"%.*fmm\" viewBox=\"%.*f %.*f %.*f %.*f\">\n",
             m_precision, (double) m_paperSize.x / m_IUsPerDecimil * 2.54 / 1000,
             m_precision, (double) m_paperSize.y / m_IUsPerDecimil * 2.54 / 1000,
             m_precision, origin.x, m_precision, origin.y,
             m_precision, m_paperSize.x * m_iuPerDeviceUnit,
             m_precision, m_paperSize.y * m_iuPerDeviceUnit);

    // Write title
    char    date_buf[250];
    time_t  ltime = time( nullptr );
    strftime( date_buf, 250, "%Y/%m/%d %H:%M:%S", localtime( &ltime ) );

    fprintf( m_outputFile,
             "<title>SVG Image created as %s date %s </title>\n",
             TO_UTF8( XmlEsc( wxFileName( m_filename ).GetFullName() ) ), date_buf );

    // End of header
    fprintf( m_outputFile, "  <desc>Image generated by %s </desc>\n",
             TO_UTF8( XmlEsc( m_creator ) ) );

    // output the pen and brush color (RVB values in hex) and opacity
    double opacity = 1.0;      // 0.0 (transparent to 1.0 (solid)
    fprintf( m_outputFile,
             "<g style=\"fill:#%6.6lX; fill-opacity:%.*f;stroke:#%6.6lX; stroke-opacity:%.*f;\n",
             m_brush_rgb_color, m_precision, m_brush_alpha, m_pen_rgb_color, m_precision, opacity );

    // output the pen cap and line joint
    fputs( "stroke-linecap:round; stroke-linejoin:round;\"\n", m_outputFile );
    fputs( " transform=\"translate(0 0) scale(1 1)\">\n", m_outputFile );
    return true;
}


bool SVG_PLOTTER::EndPlot()
{
    fputs( "</g> \n</svg>\n", m_outputFile );
    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void SVG_PLOTTER::Text( const VECTOR2I&             aPos,
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
    setFillMode( FILL_T::NO_FILL );
    SetColor( aColor );
    SetCurrentLineWidth( aWidth );

    VECTOR2I    text_pos = aPos;
    const char* hjust = "start";

    switch( aH_justify )
    {
    case GR_TEXT_H_ALIGN_CENTER: hjust = "middle"; break;
    case GR_TEXT_H_ALIGN_RIGHT:  hjust = "end";    break;
    case GR_TEXT_H_ALIGN_LEFT:   hjust = "start";  break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_V_ALIGN_CENTER: text_pos.y += aSize.y / 2; break;
    case GR_TEXT_V_ALIGN_TOP:    text_pos.y += aSize.y;     break;
    case GR_TEXT_V_ALIGN_BOTTOM:                            break;
    }

    VECTOR2I text_size;

    // aSize.x or aSize.y is < 0 for mirrored texts.
    // The actual text size value is the absolute value
    text_size.x = std::abs( GraphicTextWidth( aText, aFont, aSize, aWidth, aBold, aItalic ) );
    text_size.y = std::abs( aSize.x * 4/3 ); // Hershey font height to em size conversion
    VECTOR2D anchor_pos_dev = userToDeviceCoordinates( aPos );
    VECTOR2D text_pos_dev = userToDeviceCoordinates( text_pos );
    VECTOR2D sz_dev = userToDeviceSize( text_size );

    if( !aOrient.IsZero() )
    {
        fprintf( m_outputFile,
                 "<g transform=\"rotate(%f %.*f %.*f)\">\n",
                 - aOrient.AsDegrees(), m_precision, anchor_pos_dev.x, m_precision, anchor_pos_dev.y );
    }

    fprintf( m_outputFile, "<text x=\"%.*f\" y=\"%.*f\"\n",
                            m_precision, text_pos_dev.x, m_precision, text_pos_dev.y );

    /// If the text is mirrored, we should also mirror the hidden text to match
    if( aSize.x < 0 )
        fprintf( m_outputFile, "transform=\"scale(-1 1) translate(%f 0)\"\n", -2 * text_pos_dev.x );

    fprintf( m_outputFile,
             "textLength=\"%.*f\" font-size=\"%.*f\" lengthAdjust=\"spacingAndGlyphs\"\n"
             "text-anchor=\"%s\" opacity=\"0\">%s</text>\n",
             m_precision, sz_dev.x, m_precision, sz_dev.y, hjust, TO_UTF8( XmlEsc( aText ) ) );

    if( !aOrient.IsZero() )
        fputs( "</g>\n", m_outputFile );

    fprintf( m_outputFile, "<g class=\"stroked-text\"><desc>%s</desc>\n",
             TO_UTF8( XmlEsc( aText ) ) );

    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
                   aBold, aMultilineAllowed, aFont );

    fputs( "</g>", m_outputFile );
}


void SVG_PLOTTER::PlotText( const VECTOR2I& aPos, const COLOR4D& aColor,
                    const wxString& aText,
                    const TEXT_ATTRIBUTES& aAttributes,
                    KIFONT::FONT* aFont,
                    void* aData )
{
    VECTOR2I size = aAttributes.m_Size;

    if( aAttributes.m_Mirrored )
        size.x = -size.x;

    SVG_PLOTTER::Text( aPos, aColor, aText, aAttributes.m_Angle, size,
                       aAttributes.m_Halign, aAttributes.m_Valign,
                       aAttributes.m_StrokeWidth,
                       aAttributes.m_Italic, aAttributes.m_Bold,
                       aAttributes.m_Multiline,
                       aFont, aData );
}
