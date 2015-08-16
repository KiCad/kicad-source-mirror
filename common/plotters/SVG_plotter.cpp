/**
 * @file SVG_plotter.cpp
 * @brief Kicad: specialized plotter for SVG files format
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fctsys.h>
#include <trigo.h>
#include <eda_base_frame.h>
#include <eda_rect.h>
#include <base_struct.h>
#include <common.h>
#include <plotter.h>
#include <macros.h>
#include <kicad_string.h>



/**
 * Function XmlEsc
 * translates '<' to "&lt;", '>' to "&gt;" and so on, according to the spec:
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
    SetTextMode( PLOTTEXTMODE_STROKE );
    m_fillMode = NO_FILL;               // or FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR
    m_pen_rgb_color = 0;                // current color value (black)
    m_brush_rgb_color = 0;              // current color value (black)
    m_dashed = false;
}


void SVG_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                               double aScale, bool aMirror )
{
    m_plotMirror = aMirror;
    m_yaxisReversed = true;     // unlike other plotters, SVG has Y axis reversed
    plotOffset  = aOffset;
    plotScale   = aScale;
    m_IUsPerDecimil = aIusPerDecimil;
    iuPerDeviceUnit = 1.0 / aIusPerDecimil;
    /* Compute the paper size in IUs */
    paperSize   = pageInfo.GetSizeMils();
    paperSize.x *= 10.0 * aIusPerDecimil;
    paperSize.y *= 10.0 * aIusPerDecimil;
    SetDefaultLineWidth( 100 * aIusPerDecimil );    // arbitrary default
}


void SVG_PLOTTER::SetColor( COLOR4D color )
{
    PSLIKE_PLOTTER::SetColor( color );

    if( m_graphics_changed )
        setSVGPlotStyle();
}


void SVG_PLOTTER::setFillMode( FILL_T fill )
{
    if( m_fillMode != fill )
    {
        m_graphics_changed = true;
        m_fillMode = fill;
    }
}


void SVG_PLOTTER::setSVGPlotStyle( bool aIsGroup, const std::string& aExtraStyle )
{
    if( aIsGroup )
        fputs( "</g>\n<g ", outputFile );

    // output the background fill color
    fprintf( outputFile, "style=\"fill:#%6.6lX; ", m_brush_rgb_color );

    switch( m_fillMode )
    {
    case NO_FILL:
        fputs( "fill-opacity:0.0; ", outputFile );
        break;

    case FILLED_SHAPE:
        fputs( "fill-opacity:1.0; ", outputFile );
        break;

    case FILLED_WITH_BG_BODYCOLOR:
        fputs( "fill-opacity:0.6; ", outputFile );
        break;
    }

    double pen_w = userToDeviceSize( GetCurrentLineWidth() );
    fprintf( outputFile, "\nstroke:#%6.6lX; stroke-width:%g; stroke-opacity:1; \n",
             m_pen_rgb_color, pen_w  );
    fputs( "stroke-linecap:round; stroke-linejoin:round;", outputFile );

    switch( m_dashed )
    {
    case PLOTDASHTYPE_DASH:
        fprintf( outputFile, "stroke-dasharray:%g,%g;",
                 GetDashMarkLenIU(), GetDashGapLenIU() );
        break;
    case PLOTDASHTYPE_DOT:
        fprintf( outputFile, "stroke-dasharray:%g,%g;",
                 GetDotMarkLenIU(), GetDashGapLenIU() );
        break;
    case PLOTDASHTYPE_DASHDOT:
        fprintf( outputFile, "stroke-dasharray:%g,%g,%g,%g;",
                GetDashMarkLenIU(), GetDashGapLenIU(), GetDotMarkLenIU(), GetDashGapLenIU() );
        break;
    }

    if( aExtraStyle.length() )
    {
        fputs( aExtraStyle.c_str(), outputFile );
    }

    fputs( "\"", outputFile );

    if( aIsGroup )
    {
        fputs( ">", outputFile );
        m_graphics_changed = false;
    }

    fputs( "\n", outputFile );
}

/* Set the current line width (in IUs) for the next plot
 */
void SVG_PLOTTER::SetCurrentLineWidth( int width, void* aData )
{
    int pen_width;

    if( width >= 0 )
        pen_width = width;
    else
        pen_width = defaultPenWidth;

    if( pen_width != currentPenWidth )
    {
        m_graphics_changed  = true;
        currentPenWidth     = pen_width;
    }

    if( m_graphics_changed )
        setSVGPlotStyle();
}


void SVG_PLOTTER::StartBlock( void* aData )
{
    std::string* idstr = reinterpret_cast<std::string*>( aData );

    fputs( "<g ", outputFile );
    if( idstr )
        fprintf( outputFile, "id=\"%s\"", idstr->c_str() );

    fprintf( outputFile, ">\n" );
}


void SVG_PLOTTER::EndBlock( void* aData )
{
    fprintf( outputFile, "</g>\n" );

    m_graphics_changed = true;
}


/* initialize m_red, m_green, m_blue ( 0 ... 255)
 * from reduced values r, g ,b ( 0.0 to 1.0 )
 */
void SVG_PLOTTER::emitSetRGBColor( double r, double g, double b )
{
    int red     = (int) ( 255.0 * r );
    int green   = (int) ( 255.0 * g );
    int blue    = (int) ( 255.0 * b );
    long rgb_color = (red << 16) | (green << 8) | blue;

    if( m_pen_rgb_color != rgb_color )
    {
        m_graphics_changed = true;
        m_pen_rgb_color = rgb_color;

        // Currently, use the same color for brush and pen
        // (i.e. to draw and fill a contour)
        m_brush_rgb_color = rgb_color;
    }
}


/**
 * SVG supports dashed lines
 */
void SVG_PLOTTER::SetDash( int dashed )
{
    if( m_dashed != dashed )
    {
        m_graphics_changed = true;
        m_dashed = dashed;
    }

    if( m_graphics_changed )
        setSVGPlotStyle();
}


void SVG_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill, int width )
{
    EDA_RECT rect( p1, wxSize( p2.x -p1.x,  p2.y -p1.y ) );
    rect.Normalize();
    DPOINT  org_dev  = userToDeviceCoordinates( rect.GetOrigin() );
    DPOINT  end_dev = userToDeviceCoordinates( rect.GetEnd() );
    DSIZE  size_dev = end_dev - org_dev;
    // Ensure size of rect in device coordinates is > 0
    // I don't know if this is a SVG issue or a Inkscape issue, but
    // Inkscape has problems with negative or null values for width and/or height, so avoid them
    DBOX rect_dev( org_dev, size_dev);
    rect_dev.Normalize();

    setFillMode( fill );
    SetCurrentLineWidth( width );

    // Rectangles having a 0 size value for height or width are just not drawn on Inscape,
    // so use a line when happens.
    if( rect_dev.GetSize().x == 0.0 || rect_dev.GetSize().y == 0.0 )    // Draw a line
        fprintf( outputFile,
                 "<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" />\n",
                 rect_dev.GetPosition().x, rect_dev.GetPosition().y,
                 rect_dev.GetEnd().x, rect_dev.GetEnd().y
                 );

    else
        fprintf( outputFile,
                 "<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" rx=\"%g\" />\n",
                 rect_dev.GetPosition().x, rect_dev.GetPosition().y,
                 rect_dev.GetSize().x, rect_dev.GetSize().y,
                 0.0   // radius of rounded corners
                 );
}


void SVG_PLOTTER::Circle( const wxPoint& pos, int diametre, FILL_T fill, int width )
{
    DPOINT  pos_dev = userToDeviceCoordinates( pos );
    double  radius  = userToDeviceSize( diametre / 2.0 );

    setFillMode( fill );
    SetCurrentLineWidth( width );

    // If diameter is less than width, switch to filled mode
    if( fill == NO_FILL && diametre < width )
    {
        setFillMode( FILLED_SHAPE );
        SetCurrentLineWidth( 0 );

        radius = userToDeviceSize( ( diametre / 2.0 ) + ( width / 2.0 ) );
    }

    fprintf( outputFile,
             "<circle cx=\"%g\" cy=\"%g\" r=\"%g\" /> \n",
             pos_dev.x, pos_dev.y, radius );
}


void SVG_PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle, int radius,
                       FILL_T fill, int width )
{
    /* Draws an arc of a circle, centred on (xc,yc), with starting point
     *  (x1, y1) and ending at (x2, y2). The current pen is used for the outline
     *  and the current brush for filling the shape.
     *
     *  The arc is drawn in an anticlockwise direction from the start point to
     *  the end point
     */

    if( radius <= 0 )
    {
        Circle( centre, width, FILLED_SHAPE, 0 );
        return;
    }

    if( StAngle > EndAngle )
        std::swap( StAngle, EndAngle );

    setFillMode( fill );
    SetCurrentLineWidth( width );

    // Calculate start point.
    DPOINT  centre_dev  = userToDeviceCoordinates( centre );
    double  radius_dev  = userToDeviceSize( radius );

    if( !m_yaxisReversed )   // Should be never the case
    {
        double tmp  = StAngle;
        StAngle     = -EndAngle;
        EndAngle    = -tmp;
    }

    if( m_plotMirror )
    {
        if( m_mirrorIsHorizontal )
        {
            StAngle = 1800.0 -StAngle;
            EndAngle = 1800.0 -EndAngle;
            std::swap( StAngle, EndAngle );
        }
        else
        {
            StAngle = -StAngle;
            EndAngle = -EndAngle;
        }
    }

    DPOINT  start;
    start.x = radius_dev;
    RotatePoint( &start.x, &start.y, StAngle );
    DPOINT  end;
    end.x = radius_dev;
    RotatePoint( &end.x, &end.y, EndAngle );
    start += centre_dev;
    end += centre_dev;

    double theta1 = DECIDEG2RAD( StAngle );

    if( theta1 < 0 )
        theta1 = theta1 + M_PI * 2;

    double theta2 = DECIDEG2RAD( EndAngle );

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
    fprintf( outputFile, "<path d=\"M%g %g A%g %g 0.0 %d %d %g %g \" />\n",
             start.x, start.y, radius_dev, radius_dev,
             flg_arc, flg_sweep,
             end.x, end.y  );
}


void SVG_PLOTTER::PlotPoly( const std::vector<wxPoint>& aCornerList,
                            FILL_T aFill, int aWidth, void * aData )
{
    if( aCornerList.size() <= 1 )
        return;

    setFillMode( aFill );
    SetCurrentLineWidth( aWidth );
    fprintf( outputFile, "<path ");

    switch( aFill )
    {
    case NO_FILL:
        setSVGPlotStyle( false, "fill:none" );
        break;

    case FILLED_WITH_BG_BODYCOLOR:
    case FILLED_SHAPE:
        setSVGPlotStyle( false, "fill-rule:evenodd;" );
        break;
    }

    DPOINT pos = userToDeviceCoordinates( aCornerList[0] );
    fprintf( outputFile, "d=\"M %d,%d\n", (int) pos.x, (int) pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fprintf( outputFile, "%d,%d\n", (int) pos.x, (int) pos.y );
    }

    fprintf( outputFile, "Z\" /> \n" );
}


/**
 * Postscript-likes at the moment are the only plot engines supporting bitmaps...
 */
void SVG_PLOTTER::PlotImage( const wxImage& aImage, const wxPoint& aPos,
                             double aScaleFactor )
{
    // in svg file we must insert a link to a png image file to plot an image
    // the image itself is not included in the svg file.
    // So we prefer skip the image, and just draw a rectangle,
    // like other plotters which do not support images

    PLOTTER::PlotImage( aImage, aPos, aScaleFactor );

}


void SVG_PLOTTER::PenTo( const wxPoint& pos, char plume )
{
    if( plume == 'Z' )
    {
        if( penState != 'Z' )
        {
            fputs( "\" />\n", outputFile );
            penState        = 'Z';
            penLastpos.x    = -1;
            penLastpos.y    = -1;
        }

        return;
    }

    if( penState == 'Z' )    // here plume = 'D' or 'U'
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );

        // Ensure we do not use a fill mode when moving tne pen,
        // in SVG mode (i;e. we are plotting only basic lines, not a filled area
        if( m_fillMode != NO_FILL )
        {
            setFillMode( NO_FILL );
            setSVGPlotStyle();
        }

        fprintf( outputFile, "<path d=\"M%d %d\n",
                 (int) pos_dev.x, (int) pos_dev.y );
    }
    else if( penState != plume || pos != penLastpos )
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );
        fprintf( outputFile, "L%d %d\n",
                 (int) pos_dev.x, (int) pos_dev.y );
    }

    penState    = plume;
    penLastpos  = pos;
}


/**
 * The code within this function
 * creates SVG files header
 */
bool SVG_PLOTTER::StartPlot()
{
    wxASSERT( outputFile );
    wxString            msg;

    static const char*  header[] =
    {
        "<?xml version=\"1.0\" standalone=\"no\"?>\n",
        " <!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \n",
        " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"> \n",
        "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" \n",
        NULL
    };

    // Write header.
    for( int ii = 0; header[ii] != NULL; ii++ )
    {
        fputs( header[ii], outputFile );
    }

    // Write viewport pos and size
    wxPoint origin;    // TODO set to actual value
    fprintf( outputFile,
             "    width=\"%gcm\" height=\"%gcm\" viewBox=\"%d %d %d %d \">\n",
             (double) paperSize.x / m_IUsPerDecimil * 2.54 / 10000,
             (double) paperSize.y / m_IUsPerDecimil * 2.54 / 10000,
             origin.x, origin.y,
             (int) ( paperSize.x / m_IUsPerDecimil ),
             (int) ( paperSize.y / m_IUsPerDecimil) );

    // Write title
    char    date_buf[250];
    time_t  ltime = time( NULL );
    strftime( date_buf, 250, "%Y/%m/%d %H:%M:%S",
              localtime( &ltime ) );

    fprintf( outputFile,
             "<title>SVG Picture created as %s date %s </title>\n",
             TO_UTF8( XmlEsc( wxFileName( filename ).GetFullName() ) ), date_buf );
    // End of header
    fprintf( outputFile, "  <desc>Picture generated by %s </desc>\n",
             TO_UTF8( XmlEsc( creator ) ) );

    // output the pen and brush color (RVB values in hex) and opacity
    double opacity = 1.0;      // 0.0 (transparent to 1.0 (solid)
    fprintf( outputFile,
             "<g style=\"fill:#%6.6lX; fill-opacity:%g;stroke:#%6.6lX; stroke-opacity:%g;\n",
             m_brush_rgb_color, opacity, m_pen_rgb_color, opacity );

    // output the pen cap and line joint
    fputs( "stroke-linecap:round; stroke-linejoin:round; \"\n", outputFile );
    fputs( " transform=\"translate(0 0) scale(1 1)\">\n", outputFile );
    return true;
}


bool SVG_PLOTTER::EndPlot()
{
    fputs( "</g> \n</svg>\n", outputFile );
    fclose( outputFile );
    outputFile = NULL;

    return true;
}


void SVG_PLOTTER::Text( const wxPoint&              aPos,
                        const COLOR4D               aColor,
                        const wxString&             aText,
                        double                      aOrient,
                        const wxSize&               aSize,
                        enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                        enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                        int                         aWidth,
                        bool                        aItalic,
                        bool                        aBold,
                        bool                        aMultilineAllowed,
                        void*                       aData )
{
    setFillMode( NO_FILL );
    SetColor( aColor );
    SetCurrentLineWidth( aWidth );

    int width = currentPenWidth;

    if( aWidth <= 0 && aBold )
        width = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );

    if( aWidth <= 0 )
        width = currentPenWidth;

    wxPoint text_pos = aPos;
    const char *hjust = "start";

    switch( aH_justify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        hjust = "middle";
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        hjust = "end";
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        hjust = "start";
        break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        text_pos.y += aSize.y / 2;
        break;

    case GR_TEXT_VJUSTIFY_TOP:
        text_pos.y += aSize.y;
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        break;
    }

    wxSize text_size;
    text_size.x = GraphicTextWidth( aText, aSize, aItalic, width );
    text_size.y = aSize.x * 4/3; // Hershey font height to em size conversion
    DPOINT anchor_pos_dev = userToDeviceCoordinates( aPos );
    DPOINT text_pos_dev = userToDeviceCoordinates( text_pos );
    DPOINT sz_dev = userToDeviceSize( text_size );

    if( aOrient != 0 ) {
        fprintf( outputFile,
                 "<g transform=\"rotate(%g %g %g)\">\n",
                 - aOrient * 0.1, anchor_pos_dev.x, anchor_pos_dev.y );
    }

    fprintf( outputFile,
             "<text x=\"%g\" y=\"%g\"\n"
             "textLength=\"%g\" font-size=\"%g\" lengthAdjust=\"spacingAndGlyphs\"\n"
             "text-anchor=\"%s\" opacity=\"0\">%s</text>\n",
             text_pos_dev.x, text_pos_dev.y,
             sz_dev.x, sz_dev.y,
             hjust, TO_UTF8( XmlEsc( aText ) ) );

    if( aOrient != 0 )
        fputs( "</g>\n", outputFile );

    fprintf( outputFile,
             "<g class=\"stroked-text\"><desc>%s</desc>\n",
             TO_UTF8( XmlEsc( aText ) ) );
    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify,
                   aWidth, aItalic, aBold, aMultilineAllowed );
    fputs( "</g>", outputFile );
}
