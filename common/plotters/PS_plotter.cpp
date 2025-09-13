/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file PS_plotter.cpp
 * @brief KiCad: specialized plotter for PS files format
 */

#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <geometry/shape_rect.h>
#include <string_utils.h>
#include <trigo.h>
#include <fmt/format.h>

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
static int getFillId( FILL_T aFill )
{
    if( aFill == FILL_T::NO_FILL )
        return 0;

    if( aFill == FILL_T::FILLED_SHAPE )
        return 1;

    return 2;
}


void PSLIKE_PLOTTER::SetColor( const COLOR4D& color )
{
    if( m_colorMode )
    {
        if( m_negativeMode )
            emitSetRGBColor( 1 - color.r, 1 - color.g, 1 - color.b, color.a );
        else
            emitSetRGBColor( color.r, color.g, color.b, color.a );
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
            emitSetRGBColor( 1 - k, 1 - k, 1 - k, 1.0 );
        else
            emitSetRGBColor( k, k, k, 1.0 );
    }
}


void PSLIKE_PLOTTER::FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                   const EDA_ANGLE& aPadOrient, void* aData )
{
    wxASSERT( m_outputFile );

    VECTOR2I  size( aSize );
    EDA_ANGLE orient( aPadOrient );

    // The pad is reduced to an oval by dy > dx
    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient += ANGLE_90;
    }

    int      delta = size.y - size.x;
    VECTOR2I a( 0, -delta / 2 );
    VECTOR2I b( 0, delta / 2 );

    RotatePoint( a, orient );
    RotatePoint( b, orient );

    ThickSegment( a + aPadPos, b + aPadPos, size.x, aData );
}


void PSLIKE_PLOTTER::FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter, void* aData )
{
    Circle( aPadPos, aDiameter, FILL_T::FILLED_SHAPE, 0 );
}


void PSLIKE_PLOTTER::FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                   const EDA_ANGLE& aPadOrient, void* aData )
{
    std::vector<VECTOR2I> cornerList;
    VECTOR2I size( aSize );
    cornerList.reserve( 4 );

    int dx = size.x / 2;
    int dy = size.y / 2;

    VECTOR2I corner;
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
        RotatePoint( cornerList[ii], aPadPos, aPadOrient );

    cornerList.push_back( cornerList[0] );

    PlotPoly( cornerList, FILL_T::FILLED_SHAPE, 0, aData );
}


void PSLIKE_PLOTTER::FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                        int aCornerRadius, const EDA_ANGLE& aOrient, void* aData )
{
    SHAPE_POLY_SET outline;
    TransformRoundChamferedRectToPolygon( outline, aPadPos, aSize, aOrient, aCornerRadius, 0.0, 0,
                                          0, GetPlotterArcHighDef(), ERROR_INSIDE );

    std::vector<VECTOR2I> cornerList;

    // TransformRoundRectToPolygon creates only one convex polygon
    SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );
    cornerList.reserve( poly.PointCount() );

    for( int ii = 0; ii < poly.PointCount(); ++ii )
        cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

    // Close polygon
    cornerList.push_back( cornerList[0] );

    PlotPoly( cornerList, FILL_T::FILLED_SHAPE, 0, aData );
}


void PSLIKE_PLOTTER::FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize,
                                     const EDA_ANGLE& aOrient, SHAPE_POLY_SET* aPolygons,
                                     void* aData )
{
    std::vector<VECTOR2I> cornerList;

    for( int cnt = 0; cnt < aPolygons->OutlineCount(); ++cnt )
    {
        SHAPE_LINE_CHAIN& poly = aPolygons->Outline( cnt );
        cornerList.clear();

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            cornerList.emplace_back( poly.CPoint( ii ).x, poly.CPoint( ii ).y );

        // Close polygon
        cornerList.push_back( cornerList[0] );

        PlotPoly( cornerList, FILL_T::FILLED_SHAPE, 0, aData );
    }
}


void PSLIKE_PLOTTER::FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners,
                                     const EDA_ANGLE& aPadOrient, void* aData )
{
    static std::vector<VECTOR2I> cornerList;
    cornerList.clear();

    for( int ii = 0; ii < 4; ii++ )
        cornerList.push_back( aCorners[ii] );

    for( int ii = 0; ii < 4; ii++ )
    {
        RotatePoint( cornerList[ii], aPadOrient );
        cornerList[ii] += aPadPos;
    }

    cornerList.push_back( cornerList[0] );
    PlotPoly( cornerList, FILL_T::FILLED_SHAPE, 0, aData );
}


void PSLIKE_PLOTTER::FlashRegularPolygon( const VECTOR2I& aShapePos, int aRadius, int aCornerCount,
                                          const EDA_ANGLE& aOrient, void* aData )
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

    for( wchar_t asciiCode : aText)
    {
        // Skip the negation marks and untabled points.
        if( asciiCode < 256 )
            tally += width_table[asciiCode];
    }

    // Widths are proportional to height, but height is enlarged by a scaling factor.
    return KiROUND( aXSize * tally / postscriptTextAscent );
}


void PS_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
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


void PSLIKE_PLOTTER::computeTextParameters( const VECTOR2I&          aPos,
                                            const wxString&          aText,
                                            const EDA_ANGLE&         aOrient,
                                            const VECTOR2I&          aSize,
                                            bool                     aMirror,
                                            enum GR_TEXT_H_ALIGN_T   aH_justify,
                                            enum GR_TEXT_V_ALIGN_T   aV_justify,
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
    VECTOR2I start_pos = aPos;

    // This is an approximation of the text bounds (in IUs)
    int tw = returnPostscriptTextWidth( aText, aSize.x, aItalic, aWidth );
    int th = aSize.y;
    int dx = 0, dy = 0;

    switch( aH_justify )
    {
    case GR_TEXT_H_ALIGN_CENTER: dx = -tw / 2; break;
    case GR_TEXT_H_ALIGN_RIGHT:  dx = -tw;     break;
    case GR_TEXT_H_ALIGN_LEFT:   dx = 0;       break;
    case GR_TEXT_H_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_V_ALIGN_CENTER: dy = th / 2; break;
    case GR_TEXT_V_ALIGN_TOP:    dy = th;     break;
    case GR_TEXT_V_ALIGN_BOTTOM: dy = 0;      break;
    case GR_TEXT_V_ALIGN_INDETERMINATE:
        wxFAIL_MSG( wxT( "Indeterminate state legal only in dialogs." ) );
        break;
    }

    RotatePoint( &dx, &dy, aOrient );
    RotatePoint( &tw, &th, aOrient );
    start_pos.x += dx;
    start_pos.y += dy;
    VECTOR2D pos_dev = userToDeviceCoordinates( start_pos );
    VECTOR2D sz_dev = userToDeviceSize( aSize );

    // Now returns the final values... the widening factor
    *wideningFactor = sz_dev.x / sz_dev.y;

    // Mirrored texts must be plotted as mirrored!
    if( m_plotMirror ^ aMirror )
        *wideningFactor = -*wideningFactor;

    // The CTM transformation matrix
    double alpha = m_plotMirror ? aOrient.Invert().AsRadians() : aOrient.AsRadians();
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
        fmt::print( m_outputFile, "{:g} setlinewidth\n", userToDeviceSize( aWidth ) );

    m_currentPenWidth = aWidth;
}


void PS_PLOTTER::emitSetRGBColor( double r, double g, double b, double a )
{
    wxASSERT( m_outputFile );

    // Postscript treats all colors as opaque, so the best we can do with alpha is generate
    // an appropriate blended color assuming white paper.  (It's possible that a halftone would
    // work better on *some* drivers, but most drivers are known to still treat halftones as
    // opaque and remove any colors underneath them.)
    if( a < 1.0 )
    {
        r = ( r * a ) + ( 1 - a );
        g = ( g * a ) + ( 1 - a );
        b = ( b * a ) + ( 1 - a );
    }

    // XXX why %.3g ? shouldn't %g suffice? who cares...
    fmt::print( m_outputFile, "{:.3g} {:.3g} {:.3g} setrgbcolor\n", r, g, b );
}


void PS_PLOTTER::SetDash( int aLineWidth, LINE_STYLE aLineStyle )
{
    switch( aLineStyle )
    {
    case LINE_STYLE::DASH:
        fmt::print( m_outputFile, "[{} {}] 0 setdash\n",
                    (int) GetDashMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case LINE_STYLE::DOT:
        fmt::print( m_outputFile, "[{} {}] 0 setdash\n",
                    (int) GetDotMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case LINE_STYLE::DASHDOT:
        fmt::print( m_outputFile, "[{} {} {} {}] 0 setdash\n",
                    (int) GetDashMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ),
                    (int) GetDotMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case LINE_STYLE::DASHDOTDOT:
        fmt::print( m_outputFile, "[{} {} {} {} {} {}] 0 setdash\n",
                    (int) GetDashMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ),
                    (int) GetDotMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ),
                    (int) GetDotMarkLenIU( aLineWidth ),
                    (int) GetDashGapLenIU( aLineWidth ) );
        break;

    default:
        fmt::print( m_outputFile, "solidline\n" );
    }
}


void PS_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                       int aCornerRadius )
{
    SetCurrentLineWidth( width );

    if( fill == FILL_T::NO_FILL && GetCurrentLineWidth() <= 0 )
        return;

    if( aCornerRadius > 0 )
    {
        BOX2I box( p1, VECTOR2I( p2.x - p1.x, p2.y - p1.y ) );
        box.Normalize();
        SHAPE_RECT rect( box );
        rect.SetRadius( aCornerRadius );
        PlotPoly( rect.Outline(), fill, width, nullptr );
        return;
    }

    VECTOR2D p1_dev = userToDeviceCoordinates( p1 );
    VECTOR2D p2_dev = userToDeviceCoordinates( p2 );

    fmt::print( m_outputFile, "{:g} {:g} {:g} {:g} rect{}\n",
                p1_dev.x,
                p1_dev.y,
                p2_dev.x - p1_dev.x,
                p2_dev.y - p1_dev.y,
                getFillId( fill ) );
}


void PS_PLOTTER::Circle( const VECTOR2I& pos, int diametre, FILL_T fill, int width )
{
    SetCurrentLineWidth( width );

    if( fill == FILL_T::NO_FILL && GetCurrentLineWidth() <= 0 )
        return;

    wxASSERT( m_outputFile );
    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    double   radius = userToDeviceSize( diametre / 2.0 );

    fmt::print( m_outputFile, "{:g} {:g} {:g} cir{}\n",
                pos_dev.x,
                pos_dev.y,
                radius,
                getFillId( fill ) );
}


void PS_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                      const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( m_outputFile );

    VECTOR2D center_device = userToDeviceCoordinates( aCenter );
    double   radius_device = userToDeviceSize( aRadius );

    EDA_ANGLE endA = aStartAngle + aAngle;
    VECTOR2D  start( aRadius * aStartAngle.Cos(), aRadius * aStartAngle.Sin() );
    VECTOR2D  end( aRadius * endA.Cos(), aRadius * endA.Sin() );

    start += aCenter;
    end += aCenter;

    VECTOR2D  start_device = userToDeviceCoordinates( start );
    VECTOR2D  end_device = userToDeviceCoordinates( end );
    EDA_ANGLE startAngle( start_device - center_device );
    EDA_ANGLE endAngle( end_device - center_device );

    // userToDeviceCoordinates gets our start/ends out of order
    if( !m_plotMirror ^ ( aAngle < ANGLE_0 ) )
        std::swap( startAngle, endAngle );

    SetCurrentLineWidth( aWidth );

    fmt::print( m_outputFile, "{:g} {:g} {:g} {:g} {:g} arc{}\n",
                center_device.x,
                center_device.y,
                radius_device,
                startAngle.AsDegrees(),
                endAngle.AsDegrees(),
                getFillId( aFill ) );
}


void PS_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                           void* aData )
{
    SetCurrentLineWidth( aWidth );

    if( aFill == FILL_T::NO_FILL && GetCurrentLineWidth() <= 0 )
        return;

    if( aCornerList.size() <= 1 )
        return;

    VECTOR2D pos = userToDeviceCoordinates( aCornerList[0] );
    fmt::print( m_outputFile, "newpath\n{:g} {:g} moveto\n", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fmt::print( m_outputFile, "{:g} {:g} lineto\n", pos.x, pos.y );
    }

    // Close/(fill) the path
    fmt::print( m_outputFile, "poly{}\n", getFillId( aFill ) );
}


void PS_PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aCornerList, FILL_T aFill, int aWidth,
                           void* aData )
{
    std::vector<VECTOR2I> cornerList;
    cornerList.reserve( aCornerList.PointCount() );

    for( int ii = 0; ii < aCornerList.PointCount(); ii++ )
        cornerList.emplace_back( aCornerList.CPoint( ii ) );

    if( aCornerList.IsClosed() && cornerList.front() != cornerList.back() )
        cornerList.emplace_back( aCornerList.CPoint( 0 ) );

    PlotPoly( cornerList, aFill, aWidth, aData );
}


void PS_PLOTTER::PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor )
{
    VECTOR2I pix_size; // size of the bitmap in pixels
    pix_size.x = aImage.GetWidth();
    pix_size.y = aImage.GetHeight();
    VECTOR2D drawsize( aScaleFactor * pix_size.x,
                       aScaleFactor * pix_size.y ); // requested size of image

    // calculate the bottom left corner position of bitmap
    VECTOR2I start = aPos;
    start.x -= drawsize.x / 2;    // left
    start.y += drawsize.y / 2;    // bottom (Y axis reversed)

    // calculate the top right corner position of bitmap
    VECTOR2I end;
    end.x = start.x + drawsize.x;
    end.y = start.y - drawsize.y;

    fmt::print( m_outputFile, "/origstate save def\n" );
    fmt::print( m_outputFile, "/pix {} string def\n", pix_size.x );

    // Locate lower-left corner of image
    VECTOR2D start_dev = userToDeviceCoordinates( start );
    fmt::print( m_outputFile, "{:g} {:g} translate\n", start_dev.x, start_dev.y );

    // Map image size to device
    VECTOR2D end_dev = userToDeviceCoordinates( end );
    fmt::print( m_outputFile, "{:g} {:g} scale\n",
                std::abs( end_dev.x - start_dev.x ),
                std::abs( end_dev.y - start_dev.y ) );

    // Dimensions of source image (in pixels
    fmt::print( m_outputFile, "{} {} 8", pix_size.x, pix_size.y );

    //  Map unit square to source
    fmt::print( m_outputFile, " [{} 0 0 {} 0 {}]\n", pix_size.x, -pix_size.y, pix_size.y );

    // include image data in ps file
    fmt::print( m_outputFile, "{{currentfile pix readhexstring pop}}\n" );

    if( m_colorMode )
        fmt::print( m_outputFile, "false 3 colorimage\n" );
    else
        fmt::print( m_outputFile, "image\n" );

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
                fmt::print( m_outputFile, "\n" );
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
                fmt::print( m_outputFile, "{:02X}{:02X}{:02X}", red, green, blue );
            }
            else
            {
                // Greyscale conversion (CIE 1931)
                unsigned char grey = KiROUND( red * 0.2126 + green * 0.7152 + blue * 0.0722 );

                fmt::print( m_outputFile, "{:02X}", grey );
            }
        }
    }

    fmt::print( m_outputFile, "\n" );
    fmt::print( m_outputFile, "origstate restore\n" );
}


void PS_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    wxASSERT( m_outputFile );

    if( plume == 'Z' )
    {
        if( m_penState != 'Z' )
        {
            fmt::print( m_outputFile, "stroke\n" );
            m_penState     = 'Z';
            m_penLastpos.x = -1;
            m_penLastpos.y = -1;
        }

        return;
    }

    if( m_penState == 'Z' )
    {
        fmt::print( m_outputFile, "newpath\n" );
    }

    if( m_penState != plume || pos != m_penLastpos )
    {
        VECTOR2D pos_dev = userToDeviceCoordinates( pos );
        fmt::print( m_outputFile, "{:g} {:g} {}to\n",
                    pos_dev.x,
                    pos_dev.y,
                    ( plume=='D' ) ? "line" : "move" );
    }

    m_penState   = plume;
    m_penLastpos = pos;
}


bool PS_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    wxASSERT( m_outputFile );

    std::string PSMacro =
    "%%BeginProlog\n"
    "/line { newpath moveto lineto stroke } bind def\n"
    "/cir0 { newpath 0 360 arc stroke } bind def\n"
    "/cir1 { newpath 0 360 arc gsave fill grestore stroke } bind def\n"
    "/cir2 { newpath 0 360 arc gsave fill grestore stroke } bind def\n"
    "/arc0 { newpath arc stroke } bind def\n"
    "/arc1 { newpath 4 index 4 index moveto arc closepath gsave fill\n"
    "    grestore stroke } bind def\n"
    "/arc2 { newpath 4 index 4 index moveto arc closepath gsave fill\n"
    "    grestore stroke } bind def\n"
    "/poly0 { stroke } bind def\n"
    "/poly1 { closepath gsave fill grestore stroke } bind def\n"
    "/poly2 { closepath gsave fill grestore stroke } bind def\n"
    "/rect0 { rectstroke } bind def\n"
    "/rect1 { rectfill } bind def\n"
    "/rect2 { rectfill } bind def\n"
    "/linemode0 { 0 setlinecap 0 setlinejoin 0 setlinewidth } bind def\n"
    "/linemode1 { 1 setlinecap 1 setlinejoin } bind def\n"
    "/dashedline { [200] 100 setdash } bind def\n"
    "/solidline { [] 0 setdash } bind def\n"

    // This is for 'hidden' text (search anchors for PDF)
    "/phantomshow { moveto\n"
    "    /KicadFont findfont 0.000001 scalefont setfont\n"
    "    show } bind def\n"

    // This is for regular postscript text
    "/textshow { gsave\n"
    "    findfont exch scalefont setfont concat 1 scale 0 0 moveto show\n"
    "    } bind def\n"

    // Utility for getting Latin1 encoded fonts
    "/reencodefont {\n"
    "  findfont dup length dict begin\n"
    "  { 1 index /FID ne\n"
    "    { def }\n"
    "    { pop pop } ifelse\n"
    "  } forall\n"
    "  /Encoding ISOLatin1Encoding def\n"
    "  currentdict\n"
    "  end } bind def\n"

    // Remap AdobeStandard fonts to Latin1
    "/KicadFont /Helvetica reencodefont definefont pop\n"
    "/KicadFont-Bold /Helvetica-Bold reencodefont definefont pop\n"
    "/KicadFont-Oblique /Helvetica-Oblique reencodefont definefont pop\n"
    "/KicadFont-BoldOblique /Helvetica-BoldOblique reencodefont definefont pop\n"
    "%%EndProlog\n";

    time_t time1970 = time( nullptr );

    fmt::print( m_outputFile, "%!PS-Adobe-3.0\n" ); // Print header

    fmt::print( m_outputFile, "%%Creator: {}\n", TO_UTF8( m_creator ) );

    /* A "newline" character ("\n") is not included in the following string,
       because it is provided by the ctime() function. */
    fmt::print( m_outputFile, "%%CreationDate: {}", ctime( &time1970 ) );
    fmt::print( m_outputFile, "%%Title: {}\n", encodeStringForPlotter( m_title ).c_str() );
    fmt::print( m_outputFile, "%%Pages: 1\n" );
    fmt::print( m_outputFile, "%%PageOrder: Ascend\n" );

    // Print boundary box in 1/72 pixels per inch, box is in mils
    const double BIGPTsPERMIL = 0.072;

    /* The coordinates of the lower left corner of the boundary
       box need to be "rounded down", but the coordinates of its
       upper right corner need to be "rounded up" instead. */
    VECTOR2I psPaperSize = m_pageInfo.GetSizeMils();

    if( !m_pageInfo.IsPortrait() )
    {
        psPaperSize.x = m_pageInfo.GetHeightMils();
        psPaperSize.y = m_pageInfo.GetWidthMils();
    }

    fmt::print( m_outputFile, "%%BoundingBox: 0 0 {} {}\n",
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

    wxString pageType = m_pageInfo.GetTypeAsString();
    if( m_pageInfo.IsCustom() )
        pageType = "Custom";

    fmt::print( m_outputFile, "%%DocumentMedia: {} {} {} 0 () ()\n",
                TO_UTF8( pageType ),
                KiROUND( psPaperSize.x * BIGPTsPERMIL ),
                KiROUND( psPaperSize.y * BIGPTsPERMIL ) );

    if( m_pageInfo.IsPortrait() )
        fmt::print( m_outputFile, "%%Orientation: Portrait\n" );
    else
        fmt::print( m_outputFile, "%%Orientation: Landscape\n" );

    fmt::print( m_outputFile, "%%EndComments\n" );

    // Now specify various other details.
    fmt::print( m_outputFile, "{}", PSMacro );

    // The following strings are output here (rather than within PSMacro[])
    // to highlight that it has been provided to ensure that the contents of
    // the postscript file comply with the Document Structuring Convention.
    std::string page_num = encodeStringForPlotter( aPageNumber );

    fmt::print( m_outputFile, "%%Page: {} 1\n", page_num );

    fmt::print( m_outputFile,
                "%%BeginPageSetup\n"
                "gsave\n"
                "0.0072 0.0072 scale\n"    // Configure postscript for decimils coordinates
                "linemode1\n" );


    // Rototranslate the coordinate to achieve the landscape layout
    if( !m_pageInfo.IsPortrait() )
        fmt::print( m_outputFile, "{} 0 translate 90 rotate\n", 10 * psPaperSize.x );

    // Apply the user fine scale adjustments
    if( plotScaleAdjX != 1.0 || plotScaleAdjY != 1.0 )
        fmt::print( m_outputFile, "{:g} {:g} scale\n", plotScaleAdjX, plotScaleAdjY );

    // Set default line width
    fmt::print( m_outputFile, "{:g} setlinewidth\n",
                userToDeviceSize( m_renderSettings->GetDefaultPenWidth() ) );
    fmt::print( m_outputFile, "%%EndPageSetup\n" );

    return true;
}


bool PS_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );
    fmt::print( m_outputFile,
                "showpage\n"
                "grestore\n"
                "%%EOF\n" );
    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void PS_PLOTTER::Text( const VECTOR2I&        aPos,
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
    SetCurrentLineWidth( aWidth );
    SetColor( aColor );

    // Draw the hidden postscript text (if requested)
    if( m_textMode == PLOT_TEXT_MODE::PHANTOM )
    {
        std::string ps_test = encodeStringForPlotter( aText );
        VECTOR2D pos_dev = userToDeviceCoordinates( aPos );
        fmt::print( m_outputFile, "{} {:g} {:g} phantomshow\n", ps_test.c_str(), pos_dev.x, pos_dev.y );
    }

    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, GetCurrentLineWidth(),
                   aItalic, aBold, aMultilineAllowed, aFont, aFontMetrics, aData );
}


void PS_PLOTTER::PlotText( const VECTOR2I&        aPos,
                           const COLOR4D&         aColor,
                           const wxString&        aText,
                           const TEXT_ATTRIBUTES& aAttributes,
                           KIFONT::FONT*          aFont,
                           const KIFONT::METRICS& aFontMetrics,
                           void*                  aData )
{
    SetCurrentLineWidth( aAttributes.m_StrokeWidth );
    SetColor( aColor );

    // Draw the hidden postscript text (if requested)
    if( m_textMode == PLOT_TEXT_MODE::PHANTOM )
    {
        std::string ps_test = encodeStringForPlotter( aText );
        VECTOR2D pos_dev = userToDeviceCoordinates( aPos );
        fmt::print( m_outputFile, "{} {:g} {:g} phantomshow\n",
                    ps_test,
                    pos_dev.x,
                    pos_dev.y );
    }

    PLOTTER::PlotText( aPos, aColor, aText, aAttributes, aFont, aFontMetrics, aData );
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
