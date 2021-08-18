/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file PS_plotter.cpp
 * @brief KiCad: specialized plotter for PS files format
 */

#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <trigo.h>

#include <plotters/plotters_pslike.h>


/* Forward declaration of the font width metrics
   (yes extern! this is the way to forward declare variables */
extern const double hv_widths[256];
extern const double hvb_widths[256];
extern const double hvo_widths[256];
extern const double hvbo_widths[256];

const double PSLIKE_PLOTTER::postscriptTextAscent = 0.718;


// return a id used to select a ps macro (see StartPlot() ) from a FILL_TYPE
// fill mode, for arc, rect, circle and poly draw primitives
static int getFillId( FILL_TYPE aFill )
{
    if( aFill == FILL_TYPE::NO_FILL )
        return 0;

    if( aFill == FILL_TYPE::FILLED_SHAPE )
        return 1;

    return 2;
}


void PSLIKE_PLOTTER::SetColor( const COLOR4D& color )
{
    if( m_colorMode )
    {
        if( m_negativeMode )
            emitSetRGBColor( 1 - color.r, 1 - color.g, 1 - color.b );
        else
            emitSetRGBColor( color.r, color.g, color.b );
    }
    else
    {
        /* B/W Mode - Use BLACK or WHITE for all items
         * note the 2 colors are used in B&W mode, mainly by Pcbnew to draw
         * holes in white on pads in black
         */
        double k = 1; // White

        if( color != COLOR4D::WHITE )
            k = 0;

        if( m_negativeMode )
            emitSetRGBColor( 1 - k, 1 - k, 1 - k );
        else
            emitSetRGBColor( k, k, k );
    }
}


void PSLIKE_PLOTTER::FlashPadOval( const wxPoint& aPadPos, const wxSize& aSize,
                                   double aPadOrient, OUTLINE_MODE aTraceMode, void* aData )
{
    wxASSERT( m_outputFile );
    int x0, y0, x1, y1, delta;
    wxSize size( aSize );

    // The pad is reduced to an oval by dy > dx
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        aPadOrient = AddAngles( aPadOrient, 900 );
    }

    delta = size.y - size.x;
    x0    = 0;
    y0    = -delta / 2;
    x1    = 0;
    y1    = delta / 2;
    RotatePoint( &x0, &y0, aPadOrient );
    RotatePoint( &x1, &y1, aPadOrient );

    if( aTraceMode == FILLED )
        ThickSegment( wxPoint( aPadPos.x + x0, aPadPos.y + y0 ),
                      wxPoint( aPadPos.x + x1, aPadPos.y + y1 ), size.x, aTraceMode, nullptr );
    else
        sketchOval( aPadPos, size, aPadOrient, -1 );
}


void PSLIKE_PLOTTER::FlashPadCircle( const wxPoint& aPadPos, int aDiameter,
                                     OUTLINE_MODE aTraceMode, void* aData )
{
    if( aTraceMode == FILLED )
    {
        Circle( aPadPos, aDiameter, FILL_TYPE::FILLED_SHAPE, 0 );
    }
    else    // Plot a ring:
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        int linewidth = GetCurrentLineWidth();

        // avoid aDiameter <= 1 )
        if( linewidth > aDiameter-2 )
            linewidth = aDiameter-2;

        Circle( aPadPos, aDiameter - linewidth, FILL_TYPE::NO_FILL, linewidth );
    }

    SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
}


void PSLIKE_PLOTTER::FlashPadRect( const wxPoint& aPadPos, const wxSize& aSize,
                                   double aPadOrient, OUTLINE_MODE aTraceMode, void* aData )
{
    static std::vector< wxPoint > cornerList;
    wxSize size( aSize );
    cornerList.clear();

    if( aTraceMode == FILLED )
        SetCurrentLineWidth( 0 );
    else
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );

    size.x -= GetCurrentLineWidth();
    size.y -= GetCurrentLineWidth();

    if( size.x < 1 )
        size.x = 1;

    if( size.y < 1 )
        size.y = 1;

    int dx = size.x / 2;
    int dy = size.y / 2;

    wxPoint corner;
    corner.x = aPadPos.x - dx;
    corner.y = aPadPos.y + dy;
    cornerList.push_back( corner );
    corner.x = aPadPos.x - dx;
    corner.y = aPadPos.y - dy;
    cornerList.push_back( corner );
    corner.x = aPadPos.x + dx;
    corner.y = aPadPos.y - dy;
    cornerList.push_back( corner );
    corner.x = aPadPos.x + dx;
    corner.y = aPadPos.y + dy,
    cornerList.push_back( corner );

    for( unsigned ii = 0; ii < cornerList.size(); ii++ )
    {
        RotatePoint( &cornerList[ii], aPadPos, aPadOrient );
    }

    cornerList.push_back( cornerList[0] );

    PlotPoly( cornerList, ( aTraceMode == FILLED ) ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL,
              GetCurrentLineWidth() );
}


void PSLIKE_PLOTTER::FlashPadRoundRect( const wxPoint& aPadPos, const wxSize& aSize,
                                        int aCornerRadius, double aOrient,
                                        OUTLINE_MODE aTraceMode, void* aData )
{
    wxSize size( aSize );

    if( aTraceMode == FILLED )
    {
        SetCurrentLineWidth( 0 );
    }
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        size.x -= GetCurrentLineWidth();
        size.y -= GetCurrentLineWidth();
        aCornerRadius -= GetCurrentLineWidth() / 2;
    }


    SHAPE_POLY_SET outline;
    TransformRoundChamferedRectToPolygon( outline, aPadPos, size, aOrient, aCornerRadius,
                                          0.0, 0, 0, GetPlotterArcHighDef(), ERROR_INSIDE );

    std::vector< wxPoint > cornerList;

    // TransformRoundRectToPolygon creates only one convex polygon
    SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );
    cornerList.reserve( poly.PointCount() );

    for( int ii = 0; ii < poly.PointCount(); ++ii )
        cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

    // Close polygon
    cornerList.push_back( cornerList[0] );

    PlotPoly( cornerList, ( aTraceMode == FILLED ) ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL,
              GetCurrentLineWidth() );
}


void PSLIKE_PLOTTER::FlashPadCustom( const wxPoint& aPadPos, const wxSize& aSize,
                                     double aOrient, SHAPE_POLY_SET* aPolygons,
                                     OUTLINE_MODE aTraceMode, void* aData )
{
    wxSize size( aSize );

    if( aTraceMode == FILLED )
    {
        SetCurrentLineWidth( 0 );
    }
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        size.x -= GetCurrentLineWidth();
        size.y -= GetCurrentLineWidth();
    }


    std::vector< wxPoint > cornerList;

    for( int cnt = 0; cnt < aPolygons->OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = aPolygons->Outline( cnt );
        cornerList.clear();

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

        // Close polygon
        cornerList.push_back( cornerList[0] );

        PlotPoly( cornerList, ( aTraceMode == FILLED ) ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL,
                  GetCurrentLineWidth() );
    }
}


void PSLIKE_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                     double aPadOrient, OUTLINE_MODE aTraceMode, void* aData )
{
    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    for( int ii = 0; ii < 4; ii++ )
        cornerList.push_back( aCorners[ii] );

    if( aTraceMode == FILLED )
    {
        SetCurrentLineWidth( 0 );
    }
    else
    {
        SetCurrentLineWidth( USE_DEFAULT_LINE_WIDTH );
        int w = GetCurrentLineWidth();

        // offset polygon by w
        // coord[0] is assumed the lower left
        // coord[1] is assumed the upper left
        // coord[2] is assumed the upper right
        // coord[3] is assumed the lower right

        /* Trace the outline. */
        cornerList[0].x += w;
        cornerList[0].y -= w;
        cornerList[1].x += w;
        cornerList[1].y += w;
        cornerList[2].x -= w;
        cornerList[2].y += w;
        cornerList[3].x -= w;
        cornerList[3].y -= w;
    }

    for( int ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &cornerList[ii], aPadOrient );
        cornerList[ii] += aPadPos;
    }

    cornerList.push_back( cornerList[0] );
    PlotPoly( cornerList, ( aTraceMode == FILLED ) ? FILL_TYPE::FILLED_SHAPE : FILL_TYPE::NO_FILL,
              GetCurrentLineWidth() );
}


void PSLIKE_PLOTTER::FlashRegularPolygon( const wxPoint& aShapePos, int aRadius, int aCornerCount,
                                          double aOrient, OUTLINE_MODE aTraceMode, void* aData )
{
    // Do nothing
    wxASSERT( 0 );
}


std::string PSLIKE_PLOTTER::encodeStringForPlotter( const wxString& aUnicode )
{
    // Write on a std::string a string escaped for postscript/PDF
    std::string converted;

    converted += '(';

    for( unsigned i = 0; i < aUnicode.Len(); i++ )
    {
        // Laziness made me use stdio buffering yet another time...
        wchar_t ch = aUnicode[i];

        if( ch < 256 )
        {
            switch (ch)
            {
            // The ~ shouldn't reach the outside
            case '~':
                break;

            // These characters must be escaped
            case '(':
            case ')':
            case '\\':
                converted += '\\';
                KI_FALLTHROUGH;

            default:
                converted += ch;
                break;
            }
        }
    }

    converted += ')';

    return converted;
}


int PSLIKE_PLOTTER::returnPostscriptTextWidth( const wxString& aText, int aXSize,
                                               bool aItalic, bool aBold )
{
    const double *width_table = aBold ? ( aItalic ? hvbo_widths : hvb_widths )
                                      : ( aItalic ? hvo_widths : hv_widths );
    double tally = 0;

    for( unsigned i = 0; i < aText.length(); i++ )
    {
        wchar_t AsciiCode = aText[i];

        // Skip the negation marks and untabled points.
        if( AsciiCode != '~' && AsciiCode < 256 )
        {
            tally += width_table[AsciiCode];
        }
    }

    // Widths are proportional to height, but height is enlarged by a scaling factor.
    return KiROUND( aXSize * tally / postscriptTextAscent );
}


void PSLIKE_PLOTTER::postscriptOverlinePositions( const wxString& aText, int aXSize,
                                                  bool aItalic, bool aBold,
                                                  std::vector<int> *pos_pairs )
{
    /* XXX This function is *too* similar to returnPostscriptTextWidth.
       Consider merging them... */
    const double *width_table = aBold ? ( aItalic ? hvbo_widths : hvb_widths )
                                      : ( aItalic ? hvo_widths : hv_widths );
    double tally = 0;

    for( unsigned i = 0; i < aText.length(); i++ )
    {
        wchar_t AsciiCode = aText[i];

        // Skip the negation marks and untabled points
        if( AsciiCode != '~' && AsciiCode < 256 )
        {
            tally += width_table[AsciiCode];
        }
        else
        {
            if( AsciiCode == '~' )
                pos_pairs->push_back( KiROUND( aXSize * tally / postscriptTextAscent ) );
        }
    }

    // Special rule: we have to complete the last bar if the ~ aren't matched
    if( pos_pairs->size() % 2 == 1 )
        pos_pairs->push_back( KiROUND( aXSize * tally / postscriptTextAscent ) );
}


void PS_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror )
{
    wxASSERT( !m_outputFile );
    m_plotMirror = aMirror;
    m_plotOffset = aOffset;
    m_plotScale = aScale;
    m_IUsPerDecimil = aIusPerDecimil;
    m_iuPerDeviceUnit = 1.0 / aIusPerDecimil;

    /* Compute the paper size in IUs */
    m_paperSize = m_pageInfo.GetSizeMils();
    m_paperSize.x *= 10.0 * aIusPerDecimil;
    m_paperSize.y *= 10.0 * aIusPerDecimil;
}


void PSLIKE_PLOTTER::computeTextParameters( const wxPoint&           aPos,
                                            const wxString&          aText,
                                            int                      aOrient,
                                            const wxSize&            aSize,
                                            bool                     aMirror,
                                            enum EDA_TEXT_HJUSTIFY_T aH_justify,
                                            enum EDA_TEXT_VJUSTIFY_T aV_justify,
                                            int                      aWidth,
                                            bool                     aItalic,
                                            bool                     aBold,
                                            double                   *wideningFactor,
                                            double                   *ctm_a,
                                            double                   *ctm_b,
                                            double                   *ctm_c,
                                            double                   *ctm_d,
                                            double                   *ctm_e,
                                            double                   *ctm_f,
                                            double                   *heightFactor )
{
    // Compute the starting position (compensated for alignment)
    wxPoint start_pos = aPos;

    // This is an approximation of the text bounds (in IUs)
    int tw = returnPostscriptTextWidth( aText, aSize.x, aItalic, aWidth );
    int th = aSize.y;
    int dx, dy;

    switch( aH_justify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        dx = -tw / 2;
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        dx = -tw;
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        dx = 0;
        break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        dy = th / 2;
        break;

    case GR_TEXT_VJUSTIFY_TOP:
        dy = th;
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        dy = 0;
        break;
    }

    RotatePoint( &dx, &dy, aOrient );
    RotatePoint( &tw, &th, aOrient );
    start_pos.x += dx;
    start_pos.y += dy;
    DPOINT pos_dev = userToDeviceCoordinates( start_pos );
    DPOINT sz_dev = userToDeviceSize( aSize );

    // Now returns the final values... the widening factor
    *wideningFactor = sz_dev.x / sz_dev.y;

    // Mirrored texts must be plotted as mirrored!
    if( m_plotMirror )
    {
        *wideningFactor = -*wideningFactor;
        aOrient = -aOrient;
    }

    // The CTM transformation matrix
    double alpha = DECIDEG2RAD( aOrient );
    double sinalpha = sin( alpha );
    double cosalpha = cos( alpha );

    *ctm_a = cosalpha;
    *ctm_b = sinalpha;
    *ctm_c = -sinalpha;
    *ctm_d = cosalpha;
    *ctm_e = pos_dev.x;
    *ctm_f = pos_dev.y;

    // This is because the letters are less than 1 unit high
    *heightFactor = sz_dev.y / postscriptTextAscent;
}


void PS_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    wxASSERT( m_outputFile );

    if( aWidth == DO_NOT_SET_LINE_WIDTH )
        return;
    else if( aWidth == USE_DEFAULT_LINE_WIDTH )
        aWidth = m_renderSettings->GetDefaultPenWidth();
    else if( aWidth == 0 )
        aWidth = 1;

    wxASSERT_MSG( aWidth > 0, "Plotter called to set negative pen width" );

    if( aWidth != GetCurrentLineWidth() )
        fprintf( m_outputFile, "%g setlinewidth\n", userToDeviceSize( aWidth ) );

    m_currentPenWidth = aWidth;
}


void PS_PLOTTER::emitSetRGBColor( double r, double g, double b )
{
    wxASSERT( m_outputFile );

    // XXX why %.3g ? shouldn't %g suffice? who cares...
    fprintf( m_outputFile, "%.3g %.3g %.3g setrgbcolor\n", r, g, b );
}


void PS_PLOTTER::SetDash( PLOT_DASH_TYPE dashed )
{
    switch( dashed )
    {
    case PLOT_DASH_TYPE::DASH:
        fprintf( m_outputFile, "[%d %d] 0 setdash\n",
                 (int) GetDashMarkLenIU(), (int) GetDashGapLenIU() );
        break;
    case PLOT_DASH_TYPE::DOT:
        fprintf( m_outputFile, "[%d %d] 0 setdash\n",
                 (int) GetDotMarkLenIU(), (int) GetDashGapLenIU() );
        break;
    case PLOT_DASH_TYPE::DASHDOT:
        fprintf( m_outputFile, "[%d %d %d %d] 0 setdash\n",
                 (int) GetDashMarkLenIU(), (int) GetDashGapLenIU(),
                 (int) GetDotMarkLenIU(), (int) GetDashGapLenIU() );
        break;
    default:
        fputs( "solidline\n", m_outputFile );
    }
}


void PS_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_TYPE fill, int width )
{
    DPOINT p1_dev = userToDeviceCoordinates( p1 );
    DPOINT p2_dev = userToDeviceCoordinates( p2 );

    SetCurrentLineWidth( width );
    fprintf( m_outputFile, "%g %g %g %g rect%d\n", p1_dev.x, p1_dev.y,
             p2_dev.x - p1_dev.x, p2_dev.y - p1_dev.y, getFillId( fill ) );
}


void PS_PLOTTER::Circle( const wxPoint& pos, int diametre, FILL_TYPE fill, int width )
{
    wxASSERT( m_outputFile );
    DPOINT pos_dev = userToDeviceCoordinates( pos );
    double radius = userToDeviceSize( diametre / 2.0 );

    SetCurrentLineWidth( width );
    fprintf( m_outputFile, "%g %g %g cir%d\n", pos_dev.x, pos_dev.y, radius, getFillId( fill ) );
}


void PS_PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int radius, FILL_TYPE fill, int width )
{
    wxASSERT( m_outputFile );

    if( radius <= 0 )
        return;

    if( StAngle > EndAngle )
        std::swap( StAngle, EndAngle );

    SetCurrentLineWidth( width );

    // Calculate start point.
    DPOINT centre_dev = userToDeviceCoordinates( centre );
    double radius_dev = userToDeviceSize( radius );

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

    fprintf( m_outputFile, "%g %g %g %g %g arc%d\n", centre_dev.x, centre_dev.y,
             radius_dev, StAngle / 10.0, EndAngle / 10.0, getFillId( fill ) );
}


void PS_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_TYPE aFill, int aWidth, void * aData )
{
    if( aCornerList.size() <= 1 )
        return;

    SetCurrentLineWidth( aWidth );

    DPOINT pos = userToDeviceCoordinates( aCornerList[0] );
    fprintf( m_outputFile, "newpath\n%g %g moveto\n", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fprintf( m_outputFile, "%g %g lineto\n", pos.x, pos.y );
    }

    // Close/(fill) the path
    fprintf( m_outputFile, "poly%d\n", getFillId( aFill ) );
}


void PS_PLOTTER::PlotImage( const wxImage& aImage, const wxPoint& aPos, double aScaleFactor )
{
    wxSize pix_size;                // size of the bitmap in pixels
    pix_size.x = aImage.GetWidth();
    pix_size.y = aImage.GetHeight();
    DPOINT drawsize( aScaleFactor * pix_size.x,
                     aScaleFactor * pix_size.y ); // requested size of image

    // calculate the bottom left corner position of bitmap
    wxPoint start = aPos;
    start.x -= drawsize.x / 2;    // left
    start.y += drawsize.y / 2;    // bottom (Y axis reversed)

    // calculate the top right corner position of bitmap
    wxPoint end;
    end.x = start.x + drawsize.x;
    end.y = start.y - drawsize.y;

    fprintf( m_outputFile, "/origstate save def\n" );
    fprintf( m_outputFile, "/pix %d string def\n", pix_size.x );

    // Locate lower-left corner of image
    DPOINT start_dev = userToDeviceCoordinates( start );
    fprintf( m_outputFile, "%g %g translate\n", start_dev.x, start_dev.y );

    // Map image size to device
    DPOINT end_dev = userToDeviceCoordinates( end );
    fprintf( m_outputFile, "%g %g scale\n",
             std::abs(end_dev.x - start_dev.x), std::abs(end_dev.y - start_dev.y));

    // Dimensions of source image (in pixels
    fprintf( m_outputFile, "%d %d 8", pix_size.x, pix_size.y );

    //  Map unit square to source
    fprintf( m_outputFile, " [%d 0 0 %d 0 %d]\n", pix_size.x, -pix_size.y , pix_size.y);

    // include image data in ps file
    fprintf( m_outputFile, "{currentfile pix readhexstring pop}\n" );

    if( m_colorMode )
        fputs( "false 3 colorimage\n", m_outputFile );
    else
        fputs( "image\n", m_outputFile );

    // Single data source, 3 colors, Output RGB data (hexadecimal)
    // (or the same downscaled to gray)
    int jj = 0;

    for( int yy = 0; yy < pix_size.y; yy ++ )
    {
        for( int xx = 0; xx < pix_size.x; xx++, jj++ )
        {
            if( jj >= 16 )
            {
                jj = 0;
                fprintf( m_outputFile, "\n");
            }

            int red, green, blue;
            red = aImage.GetRed( xx, yy) & 0xFF;
            green = aImage.GetGreen( xx, yy) & 0xFF;
            blue = aImage.GetBlue( xx, yy) & 0xFF;

            // PS doesn't support alpha, so premultiply against white background
            if( aImage.HasAlpha() )
            {
                unsigned char alpha = aImage.GetAlpha( xx, yy ) & 0xFF;

                if( alpha < 0xFF )
                {
                    float a = 1.0 - ( (float) alpha / 255.0 );
                    red =   ( int )( red   + ( a * 0xFF ) ) & 0xFF;
                    green = ( int )( green + ( a * 0xFF ) ) & 0xFF;
                    blue =  ( int )( blue  + ( a * 0xFF ) ) & 0xFF;
                }
            }

            if( aImage.HasMask() )
            {
                if( red == aImage.GetMaskRed() && green == aImage.GetMaskGreen()
                        && blue == aImage.GetMaskBlue() )
                {
                    red = 0xFF;
                    green = 0xFF;
                    blue = 0xFF;
                }
            }

            if( m_colorMode )
            {
                fprintf( m_outputFile, "%2.2X%2.2X%2.2X", red, green, blue );
            }
            else
            {
                // Greyscale conversion (CIE 1931)
                unsigned char grey = KiROUND( red * 0.2126 + green * 0.7152 + blue * 0.0722 );

                fprintf( m_outputFile, "%2.2X", grey );
            }
        }
    }

    fprintf( m_outputFile, "\n");
    fprintf( m_outputFile, "origstate restore\n" );
}


void PS_PLOTTER::PenTo( const wxPoint& pos, char plume )
{
    wxASSERT( m_outputFile );

    if( plume == 'Z' )
    {
        if( m_penState != 'Z' )
        {
            fputs( "stroke\n", m_outputFile );
            m_penState     = 'Z';
            m_penLastpos.x = -1;
            m_penLastpos.y = -1;
        }

        return;
    }

    if( m_penState == 'Z' )
    {
        fputs( "newpath\n", m_outputFile );
    }

    if( m_penState != plume || pos != m_penLastpos )
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );
        fprintf( m_outputFile, "%g %g %sto\n",
                 pos_dev.x, pos_dev.y,
                 ( plume=='D' ) ? "line" : "move" );
    }

    m_penState   = plume;
    m_penLastpos = pos;
}


bool PS_PLOTTER::StartPlot()
{
    wxASSERT( m_outputFile );
    wxString           msg;

    static const char* PSMacro[] =
    {
    "%%BeginProlog\n",
    "/line { newpath moveto lineto stroke } bind def\n",
    "/cir0 { newpath 0 360 arc stroke } bind def\n",
    "/cir1 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
    "/cir2 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
    "/arc0 { newpath arc stroke } bind def\n",
    "/arc1 { newpath 4 index 4 index moveto arc closepath gsave fill\n",
    "    grestore stroke } bind def\n",
    "/arc2 { newpath 4 index 4 index moveto arc closepath gsave fill\n",
    "    grestore stroke } bind def\n",
    "/poly0 { stroke } bind def\n",
    "/poly1 { closepath gsave fill grestore stroke } bind def\n",
    "/poly2 { closepath gsave fill grestore stroke } bind def\n",
    "/rect0 { rectstroke } bind def\n",
    "/rect1 { rectfill } bind def\n",
    "/rect2 { rectfill } bind def\n",
    "/linemode0 { 0 setlinecap 0 setlinejoin 0 setlinewidth } bind def\n",
    "/linemode1 { 1 setlinecap 1 setlinejoin } bind def\n",
    "/dashedline { [200] 100 setdash } bind def\n",
    "/solidline { [] 0 setdash } bind def\n",

    // This is for 'hidden' text (search anchors for PDF)
    "/phantomshow { moveto\n",
    "    /KicadFont findfont 0.000001 scalefont setfont\n",
    "    show } bind def\n",

    // This is for regular postscript text
    "/textshow { gsave\n",
    "    findfont exch scalefont setfont concat 1 scale 0 0 moveto show\n",
    "    } bind def\n",

    // Utility for getting Latin1 encoded fonts
    "/reencodefont {\n",
    "  findfont dup length dict begin\n",
    "  { 1 index /FID ne\n",
    "    { def }\n",
    "    { pop pop } ifelse\n",
    "  } forall\n",
    "  /Encoding ISOLatin1Encoding def\n",
    "  currentdict\n",
    "  end } bind def\n"

    // Remap AdobeStandard fonts to Latin1
    "/KicadFont /Helvetica reencodefont definefont pop\n",
    "/KicadFont-Bold /Helvetica-Bold reencodefont definefont pop\n",
    "/KicadFont-Oblique /Helvetica-Oblique reencodefont definefont pop\n",
    "/KicadFont-BoldOblique /Helvetica-BoldOblique reencodefont definefont pop\n",
    "%%EndProlog\n",
    nullptr
    };

    time_t time1970 = time( nullptr );

    fputs( "%!PS-Adobe-3.0\n", m_outputFile );    // Print header

    fprintf( m_outputFile, "%%%%Creator: %s\n", TO_UTF8( m_creator ) );

    /* A "newline" character ("\n") is not included in the following string,
       because it is provided by the ctime() function. */
    fprintf( m_outputFile, "%%%%CreationDate: %s", ctime( &time1970 ) );
    fprintf( m_outputFile, "%%%%Title: %s\n", encodeStringForPlotter( m_title ).c_str() );
    fprintf( m_outputFile, "%%%%Pages: 1\n" );
    fprintf( m_outputFile, "%%%%PageOrder: Ascend\n" );

    // Print boundary box in 1/72 pixels per inch, box is in mils
    const double BIGPTsPERMIL = 0.072;

    /* The coordinates of the lower left corner of the boundary
       box need to be "rounded down", but the coordinates of its
       upper right corner need to be "rounded up" instead. */
    wxSize psPaperSize = m_pageInfo.GetSizeMils();

    if( !m_pageInfo.IsPortrait() )
        psPaperSize.Set( m_pageInfo.GetHeightMils(), m_pageInfo.GetWidthMils() );

    fprintf( m_outputFile, "%%%%BoundingBox: 0 0 %d %d\n",
             (int) ceil( psPaperSize.x * BIGPTsPERMIL ),
             (int) ceil( psPaperSize.y * BIGPTsPERMIL ) );

    // Specify the size of the sheet and the name associated with that size.
    // (If the "User size" option has been selected for the sheet size,
    // identify the sheet size as "Custom" (rather than as "User"), but
    // otherwise use the name assigned by KiCad for each sheet size.)
    //
    // (The Document Structuring Convention also supports sheet weight,
    // sheet color, and sheet type properties being specified within a
    // %%DocumentMedia comment, but they are not being specified here;
    // a zero and two null strings are subsequently provided instead.)
    //
    // (NOTE: m_Size.y is *supposed* to be listed before m_Size.x;
    // the order in which they are specified is not wrong!)
    // Also note pageSize is given in mils, not in internal units and must be
    // converted to internal units.

    if( m_pageInfo.IsCustom() )
    {
        fprintf( m_outputFile, "%%%%DocumentMedia: Custom %d %d 0 () ()\n",
                 KiROUND( psPaperSize.x * BIGPTsPERMIL ),
                 KiROUND( psPaperSize.y * BIGPTsPERMIL ) );
    }
    else  // a standard paper size
    {
        fprintf( m_outputFile, "%%%%DocumentMedia: %s %d %d 0 () ()\n",
                 TO_UTF8( m_pageInfo.GetType() ),
                 KiROUND( psPaperSize.x * BIGPTsPERMIL ),
                 KiROUND( psPaperSize.y * BIGPTsPERMIL ) );
    }

    if( m_pageInfo.IsPortrait() )
        fprintf( m_outputFile, "%%%%Orientation: Portrait\n" );
    else
        fprintf( m_outputFile, "%%%%Orientation: Landscape\n" );

    fprintf( m_outputFile, "%%%%EndComments\n" );

    // Now specify various other details.

    for( int ii = 0; PSMacro[ii] != nullptr; ii++ )
    {
        fputs( PSMacro[ii], m_outputFile );
    }

    // The following string has been specified here (rather than within
    // PSMacro[]) to highlight that it has been provided to ensure that the
    // contents of the postscript file comply with the details specified
    // within the Document Structuring Convention.
    fputs( "%%Page: 1 1\n"
           "%%BeginPageSetup\n"
           "gsave\n"
           "0.0072 0.0072 scale\n"    // Configure postscript for decimils coordinates
           "linemode1\n", m_outputFile );


    // Rototranslate the coordinate to achieve the landscape layout
    if( !m_pageInfo.IsPortrait() )
        fprintf( m_outputFile, "%d 0 translate 90 rotate\n", 10 * psPaperSize.x );

    // Apply the user fine scale adjustments
    if( plotScaleAdjX != 1.0 || plotScaleAdjY != 1.0 )
        fprintf( m_outputFile, "%g %g scale\n", plotScaleAdjX, plotScaleAdjY );

    // Set default line width
    fprintf( m_outputFile, "%g setlinewidth\n",
             userToDeviceSize( m_renderSettings->GetDefaultPenWidth() ) );
    fputs( "%%EndPageSetup\n", m_outputFile );

    return true;
}


bool PS_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );
    fputs( "showpage\n"
           "grestore\n"
           "%%EOF\n", m_outputFile );
    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}



void PS_PLOTTER::Text( const wxPoint&       aPos,
                const COLOR4D&              aColor,
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
    SetCurrentLineWidth( aWidth );
    SetColor( aColor );

    // Draw the hidden postscript text (if requested)
    if( m_textMode == PLOT_TEXT_MODE::PHANTOM )
    {
        std::string ps_test = encodeStringForPlotter( aText );
        DPOINT pos_dev = userToDeviceCoordinates( aPos );
        fprintf( m_outputFile, "%s %g %g phantomshow\n", ps_test.c_str(), pos_dev.x, pos_dev.y );
    }

    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth,
                   aItalic, aBold, aMultilineAllowed );
}


/**
 * Character widths for Helvetica
 */
const double hv_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.355, 0.556, 0.556, 0.889, 0.667, 0.191,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.278, 0.278, 0.584, 0.584, 0.584, 0.556,
    1.015, 0.667, 0.667, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.500, 0.667, 0.556, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.278, 0.278, 0.278, 0.469, 0.556,
    0.333, 0.556, 0.556, 0.500, 0.556, 0.556, 0.278, 0.556,
    0.556, 0.222, 0.222, 0.500, 0.222, 0.833, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.500, 0.278, 0.556, 0.500, 0.722,
    0.500, 0.500, 0.500, 0.334, 0.260, 0.334, 0.584, 0.278,
    0.278, 0.278, 0.222, 0.556, 0.333, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.222, 0.222, 0.333, 0.333, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.500, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.260, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.556, 0.537, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.667, 0.667, 0.667, 0.667, 0.667, 0.667, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.500,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.584,
    0.611, 0.556, 0.556, 0.556, 0.556, 0.500, 0.556, 0.500
};


/**
 * Character widths for Helvetica-Bold
 */
const double hvb_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.333, 0.474, 0.556, 0.556, 0.889, 0.722, 0.238,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.333, 0.584, 0.584, 0.584, 0.611,
    0.975, 0.722, 0.722, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.556, 0.722, 0.611, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.333, 0.278, 0.333, 0.584, 0.556,
    0.333, 0.556, 0.611, 0.556, 0.611, 0.556, 0.333, 0.611,
    0.611, 0.278, 0.278, 0.556, 0.278, 0.889, 0.611, 0.611,
    0.611, 0.611, 0.389, 0.556, 0.333, 0.611, 0.556, 0.778,
    0.556, 0.556, 0.500, 0.389, 0.280, 0.389, 0.584, 0.278,
    0.278, 0.278, 0.278, 0.556, 0.500, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.500, 0.500, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.556, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.280, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.611, 0.556, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.722, 0.722, 0.722, 0.722, 0.722, 0.722, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.556,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.584,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.556, 0.611, 0.556
};


/**
 * Character widths for Helvetica-Oblique
 */
const double hvo_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.355, 0.556, 0.556, 0.889, 0.667, 0.191,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.278, 0.278, 0.584, 0.584, 0.584, 0.556,
    1.015, 0.667, 0.667, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.500, 0.667, 0.556, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.278, 0.278, 0.278, 0.469, 0.556,
    0.333, 0.556, 0.556, 0.500, 0.556, 0.556, 0.278, 0.556,
    0.556, 0.222, 0.222, 0.500, 0.222, 0.833, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.500, 0.278, 0.556, 0.500, 0.722,
    0.500, 0.500, 0.500, 0.334, 0.260, 0.334, 0.584, 0.278,
    0.278, 0.278, 0.222, 0.556, 0.333, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.222, 0.222, 0.333, 0.333, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.500, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.260, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.556, 0.537, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.667, 0.667, 0.667, 0.667, 0.667, 0.667, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.500,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.584,
    0.611, 0.556, 0.556, 0.556, 0.556, 0.500, 0.556, 0.500
};


/**
 * Character widths for Helvetica-BoldOblique
 */
const double hvbo_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.333, 0.474, 0.556, 0.556, 0.889, 0.722, 0.238,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.333, 0.584, 0.584, 0.584, 0.611,
    0.975, 0.722, 0.722, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.556, 0.722, 0.611, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.333, 0.278, 0.333, 0.584, 0.556,
    0.333, 0.556, 0.611, 0.556, 0.611, 0.556, 0.333, 0.611,
    0.611, 0.278, 0.278, 0.556, 0.278, 0.889, 0.611, 0.611,
    0.611, 0.611, 0.389, 0.556, 0.333, 0.611, 0.556, 0.778,
    0.556, 0.556, 0.500, 0.389, 0.280, 0.389, 0.584, 0.278,
    0.278, 0.278, 0.278, 0.556, 0.500, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.500, 0.500, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.556, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.280, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.611, 0.556, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.722, 0.722, 0.722, 0.722, 0.722, 0.722, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.556,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.584,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.556, 0.611, 0.556
};
