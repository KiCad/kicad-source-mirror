/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <plotters/plotter_png.h>
#include <geometry/shape_poly_set.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <wx/image.h>

#include <cmath>


PNG_PLOTTER::PNG_PLOTTER() :
        m_surface( nullptr ),
        m_context( nullptr ),
        m_dpi( DEFAULT_PNG_DPI ),
        m_width( 0 ),
        m_height( 0 ),
        m_antialias( false ),
        m_backgroundColor( COLOR4D( 0, 0, 0, 0 ) ),
        m_currentColor( COLOR4D::BLACK )
{
    // The base class destructor calls fclose() on m_outputFile; ours is unused.
    m_outputFile = nullptr;
}


PNG_PLOTTER::~PNG_PLOTTER()
{
    if( m_context )
    {
        cairo_destroy( m_context );
        m_context = nullptr;
    }

    if( m_surface )
    {
        cairo_surface_destroy( m_surface );
        m_surface = nullptr;
    }
}


bool PNG_PLOTTER::OpenFile( const wxString& aFullFilename )
{
    // The cairo surface accumulates draws in memory; the file is written in EndPlot.
    m_filename = aFullFilename;
    m_outputFile = nullptr;
    return true;
}


bool PNG_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    // Cairo image surfaces are limited to INT16_MAX in either dimension. Beyond that, surface
    // creation returns CAIRO_STATUS_INVALID_SIZE. Reject up front rather than risking a silent
    // multi-gigabyte allocation that fails late.
    constexpr int MAX_PNG_DIMENSION = 32767;

    if( m_width <= 0 || m_height <= 0 || m_width > MAX_PNG_DIMENSION || m_height > MAX_PNG_DIMENSION )
        return false;

    if( m_context )
    {
        cairo_destroy( m_context );
        m_context = nullptr;
    }

    if( m_surface )
    {
        cairo_surface_destroy( m_surface );
        m_surface = nullptr;
    }

    m_surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, m_width, m_height );

    if( cairo_surface_status( m_surface ) != CAIRO_STATUS_SUCCESS )
    {
        cairo_surface_destroy( m_surface );
        m_surface = nullptr;
        return false;
    }

    m_context = cairo_create( m_surface );

    if( cairo_status( m_context ) != CAIRO_STATUS_SUCCESS )
    {
        cairo_destroy( m_context );
        m_context = nullptr;
        cairo_surface_destroy( m_surface );
        m_surface = nullptr;
        return false;
    }

    cairo_set_antialias( m_context, m_antialias ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE );

    if( m_backgroundColor.a > 0 )
    {
        cairo_set_source_rgba( m_context, m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b,
                               m_backgroundColor.a );
        cairo_paint( m_context );
    }

    cairo_set_line_cap( m_context, CAIRO_LINE_CAP_ROUND );
    cairo_set_line_join( m_context, CAIRO_LINE_JOIN_ROUND );

    // Force the SetColor / SetCurrentLineWidth caches to miss on the first call so the new
    // Cairo context picks up the requested state (the background paint above set the source
    // colour to m_backgroundColor, so caching "BLACK" from a previous plot would skip the
    // necessary cairo_set_source_rgba).
    m_currentColor = COLOR4D::UNSPECIFIED;
    m_currentPenWidth = -1;

    return true;
}


bool PNG_PLOTTER::EndPlot()
{
    if( !m_context )
        return false;

    cairo_surface_flush( m_surface );

    // Two valid usage patterns: OpenFile()+StartPlot()+EndPlot() writes to m_filename here,
    // while StartPlot()+EndPlot()+SaveFile() leaves rendering in the surface for an explicit
    // save. Tearing down the surface in EndPlot would break the second pattern. Cleanup
    // happens at the next StartPlot or in the destructor.
    if( !m_filename.IsEmpty() )
        return SaveFile( m_filename );

    return true;
}


bool PNG_PLOTTER::SaveFile( const wxString& aPath )
{
    if( !m_surface )
        return false;

    cairo_status_t status = cairo_surface_write_to_png( m_surface, aPath.ToUTF8().data() );

    return status == CAIRO_STATUS_SUCCESS;
}


void PNG_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    if( aWidth == m_currentPenWidth )
        return;

    m_currentPenWidth = aWidth;

    if( m_context )
    {
        double deviceWidth = userToDeviceSize( static_cast<double>( aWidth ) );
        cairo_set_line_width( m_context, deviceWidth > 0 ? deviceWidth : 1.0 );
    }
}


void PNG_PLOTTER::SetColor( const COLOR4D& aColor )
{
    COLOR4D effective;

    if( m_colorMode )
    {
        effective = aColor;

        if( m_negativeMode )
        {
            effective.r = 1.0 - effective.r;
            effective.g = 1.0 - effective.g;
            effective.b = 1.0 - effective.b;
        }
    }
    else
    {
        double k = ( aColor == COLOR4D::WHITE ) ? 1.0 : 0.0;

        if( m_negativeMode )
            k = 1.0 - k;

        effective = COLOR4D( k, k, k, 1.0 );
    }

    if( effective == m_currentColor )
        return;

    m_currentColor = effective;

    if( m_context )
        cairo_set_source_rgba( m_context, effective.r, effective.g, effective.b, effective.a );
}


void PNG_PLOTTER::SetClearCompositing( bool aClear )
{
    if( m_context )
        cairo_set_operator( m_context, aClear ? CAIRO_OPERATOR_CLEAR : CAIRO_OPERATOR_OVER );
}


void PNG_PLOTTER::SetDash( int aLineWidth, LINE_STYLE aLineStyle )
{
    if( !m_context )
        return;

    if( aLineStyle == LINE_STYLE::SOLID || aLineStyle == LINE_STYLE::DEFAULT )
    {
        cairo_set_dash( m_context, nullptr, 0, 0 );
        return;
    }

    // Dash patterns are in device units (pixels), scaled by line width
    double base = std::max( 1.0, userToDeviceSize( static_cast<double>( aLineWidth ) ) );
    double dash[6];
    int    num_dashes = 0;

    switch( aLineStyle )
    {
    case LINE_STYLE::DASH:
        dash[0] = 4.0 * base;
        dash[1] = 2.0 * base;
        num_dashes = 2;
        break;

    case LINE_STYLE::DOT:
        dash[0] = 1.0 * base;
        dash[1] = 2.0 * base;
        num_dashes = 2;
        break;

    case LINE_STYLE::DASHDOT:
        dash[0] = 4.0 * base;
        dash[1] = 2.0 * base;
        dash[2] = 1.0 * base;
        dash[3] = 2.0 * base;
        num_dashes = 4;
        break;

    case LINE_STYLE::DASHDOTDOT:
        dash[0] = 4.0 * base;
        dash[1] = 2.0 * base;
        dash[2] = 1.0 * base;
        dash[3] = 2.0 * base;
        dash[4] = 1.0 * base;
        dash[5] = 2.0 * base;
        num_dashes = 6;
        break;

    default:
        cairo_set_dash( m_context, nullptr, 0, 0 );
        return;
    }

    cairo_set_dash( m_context, dash, num_dashes, 0 );
}


void PNG_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil, double aScale, bool aMirror )
{
    m_plotOffset = aOffset;
    m_IUsPerDecimil = aIusPerDecimil;
    m_plotScale = aScale;
    m_plotMirror = aMirror;

    // 10000 decimils per inch; m_dpi pixels per inch.
    m_iuPerDeviceUnit = m_IUsPerDecimil * 10000.0 / m_dpi;
}


void PNG_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T aFill, int aWidth, int aCornerRadius )
{
    if( !m_context )
        return;

    VECTOR2D start = userToDeviceCoordinates( p1 );
    VECTOR2D end = userToDeviceCoordinates( p2 );

    double x = std::min( start.x, end.x );
    double y = std::min( start.y, end.y );
    double width = std::abs( end.x - start.x );
    double height = std::abs( end.y - start.y );

    if( aFill == FILL_T::NO_FILL )
    {
        SetCurrentLineWidth( aWidth );
        strokeRect( x, y, width, height );
    }
    else
    {
        fillRect( x, y, width, height );
    }
}


void PNG_PLOTTER::Circle( const VECTOR2I& aCenter, int aDiameter, FILL_T aFill, int aWidth )
{
    if( !m_context )
        return;

    VECTOR2D center = userToDeviceCoordinates( aCenter );
    double   radius = userToDeviceSize( static_cast<double>( aDiameter ) / 2.0 );

    if( aFill == FILL_T::NO_FILL )
    {
        SetCurrentLineWidth( aWidth );
        strokeCircle( center.x, center.y, radius );
    }
    else
    {
        fillCircle( center.x, center.y, radius );
    }
}


void PNG_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle, double aRadius,
                       FILL_T aFill, int aWidth )
{
    if( !m_context )
        return;

    VECTOR2D center = userToDeviceCoordinates( VECTOR2I( aCenter.x, aCenter.y ) );
    double   deviceRadius = userToDeviceSize( aRadius );

    double startRad = aStartAngle.AsRadians();
    double endRad = ( aStartAngle + aAngle ).AsRadians();

    if( aAngle.AsDegrees() < 0 )
        cairo_arc_negative( m_context, center.x, center.y, deviceRadius, startRad, endRad );
    else
        cairo_arc( m_context, center.x, center.y, deviceRadius, startRad, endRad );

    if( aFill == FILL_T::NO_FILL )
    {
        SetCurrentLineWidth( aWidth );
        cairo_stroke( m_context );
    }
    else
    {
        cairo_fill( m_context );
    }
}


void PNG_PLOTTER::PenTo( const VECTOR2I& aPos, char aPlume )
{
    if( !m_context )
        return;

    VECTOR2D pos = userToDeviceCoordinates( aPos );

    switch( aPlume )
    {
    case 'U':
        cairo_move_to( m_context, pos.x, pos.y );
        m_penState = 'U';
        break;

    case 'D':
        if( m_penState == 'Z' )
            cairo_move_to( m_context, pos.x, pos.y );
        else
            cairo_line_to( m_context, pos.x, pos.y );

        m_penState = 'D';
        break;

    case 'Z':
        if( m_penState != 'Z' )
        {
            cairo_stroke( m_context );
            m_penState = 'Z';
            m_penLastpos.x = -1;
            m_penLastpos.y = -1;
        }
        break;
    }

    m_penLastpos = VECTOR2I( pos.x, pos.y );
}


void PNG_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth, void* aData )
{
    if( !m_context || aCornerList.size() < 2 )
        return;

    VECTOR2D start = userToDeviceCoordinates( aCornerList[0] );
    cairo_move_to( m_context, start.x, start.y );

    for( size_t i = 1; i < aCornerList.size(); i++ )
    {
        VECTOR2D pt = userToDeviceCoordinates( aCornerList[i] );
        cairo_line_to( m_context, pt.x, pt.y );
    }

    if( aFill != FILL_T::NO_FILL )
    {
        cairo_close_path( m_context );
        cairo_fill( m_context );
    }
    else
    {
        SetCurrentLineWidth( aWidth );
        cairo_stroke( m_context );
    }
}


void PNG_PLOTTER::PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor )
{
    if( !m_context || !aImage.IsOk() )
        return;

    int imgW = aImage.GetWidth();
    int imgH = aImage.GetHeight();

    if( imgW == 0 || imgH == 0 )
        return;

    // wxImage stores RGB data; convert to Cairo's premultiplied native-endian ARGB32.
    cairo_surface_t* imgSurface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, imgW, imgH );

    if( cairo_surface_status( imgSurface ) != CAIRO_STATUS_SUCCESS )
    {
        cairo_surface_destroy( imgSurface );
        return;
    }

    const unsigned char* srcData = aImage.GetData();
    const unsigned char* alphaData = aImage.HasAlpha() ? aImage.GetAlpha() : nullptr;
    unsigned char*       dstBytes = cairo_image_surface_get_data( imgSurface );
    int                  dstStride = cairo_image_surface_get_stride( imgSurface );

    for( int y = 0; y < imgH; y++ )
    {
        const unsigned char* srcRow = srcData + y * imgW * 3;
        const unsigned char* alphaRow = alphaData ? alphaData + y * imgW : nullptr;
        uint32_t*            dstRow = reinterpret_cast<uint32_t*>( dstBytes + y * dstStride );

        if( !alphaRow )
        {
            for( int x = 0; x < imgW; x++ )
            {
                uint32_t r = srcRow[x * 3 + 0];
                uint32_t g = srcRow[x * 3 + 1];
                uint32_t b = srcRow[x * 3 + 2];
                dstRow[x] = ( 0xFFu << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
            }
        }
        else
        {
            for( int x = 0; x < imgW; x++ )
            {
                uint32_t r = srcRow[x * 3 + 0];
                uint32_t g = srcRow[x * 3 + 1];
                uint32_t b = srcRow[x * 3 + 2];
                uint32_t a = alphaRow[x];

                if( a < 255 )
                {
                    r = ( r * a + 127 ) / 255;
                    g = ( g * a + 127 ) / 255;
                    b = ( b * a + 127 ) / 255;
                }

                dstRow[x] = ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
            }
        }
    }

    cairo_surface_mark_dirty( imgSurface );

    VECTOR2D pos = userToDeviceCoordinates( aPos );
    double   drawW = userToDeviceSize( static_cast<double>( imgW ) * aScaleFactor );
    double   drawH = userToDeviceSize( static_cast<double>( imgH ) * aScaleFactor );

    // aPos is the image centre; adjust to top-left for Cairo
    pos.x -= drawW / 2.0;
    pos.y -= drawH / 2.0;

    cairo_save( m_context );
    cairo_translate( m_context, pos.x, pos.y );
    cairo_scale( m_context, drawW / imgW, drawH / imgH );
    cairo_set_source_surface( m_context, imgSurface, 0, 0 );
    cairo_paint( m_context );
    cairo_restore( m_context );

    cairo_surface_destroy( imgSurface );
}


void PNG_PLOTTER::FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter, void* aData )
{
    Circle( aPadPos, aDiameter, FILL_T::FILLED_SHAPE, 0 );
}


void PNG_PLOTTER::FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                                void* aData )
{
    // An oval is a thick segment between two semicircle centres with round end caps.
    int width = std::min( aSize.x, aSize.y );
    int len = std::max( aSize.x, aSize.y ) - width;

    if( len == 0 )
    {
        FlashPadCircle( aPadPos, width, aData );
        return;
    }

    VECTOR2I delta;

    if( aSize.x > aSize.y )
    {
        delta.x = len / 2;
        delta.y = 0;
    }
    else
    {
        delta.x = 0;
        delta.y = len / 2;
    }

    RotatePoint( delta, aPadOrient );

    VECTOR2I start = aPadPos - delta;
    VECTOR2I end = aPadPos + delta;

    ThickSegment( start, end, width, aData );
}


void PNG_PLOTTER::FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                                void* aData )
{
    std::vector<VECTOR2I> corners;

    int dx = aSize.x / 2;
    int dy = aSize.y / 2;

    corners.push_back( VECTOR2I( -dx, -dy ) );
    corners.push_back( VECTOR2I( -dx, dy ) );
    corners.push_back( VECTOR2I( dx, dy ) );
    corners.push_back( VECTOR2I( dx, -dy ) );

    for( VECTOR2I& corner : corners )
    {
        RotatePoint( corner, aPadOrient );
        corner += aPadPos;
    }

    PlotPoly( corners, FILL_T::FILLED_SHAPE, 0, aData );
}


void PNG_PLOTTER::FlashPadRoundRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize, int aCornerRadius,
                                     const EDA_ANGLE& aOrient, void* aData )
{
    SHAPE_POLY_SET outline;
    TransformRoundChamferedRectToPolygon( outline, aPadPos, aSize, aOrient, aCornerRadius, 0.0, 0, 0,
                                          GetPlotterArcHighDef(), ERROR_INSIDE );

    if( outline.OutlineCount() > 0 )
    {
        const SHAPE_LINE_CHAIN& poly = outline.COutline( 0 );
        std::vector<VECTOR2I>   corners;

        for( int i = 0; i < poly.PointCount(); i++ )
            corners.push_back( poly.CPoint( i ) );

        PlotPoly( corners, FILL_T::FILLED_SHAPE, 0, aData );
    }
}


void PNG_PLOTTER::FlashPadCustom( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                                  SHAPE_POLY_SET* aPolygons, void* aData )
{
    if( !aPolygons || aPolygons->OutlineCount() == 0 )
        return;

    for( int i = 0; i < aPolygons->OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = aPolygons->COutline( i );
        std::vector<VECTOR2I>   corners;

        for( int j = 0; j < outline.PointCount(); j++ )
            corners.push_back( outline.CPoint( j ) );

        PlotPoly( corners, FILL_T::FILLED_SHAPE, 0, aData );
    }
}


void PNG_PLOTTER::FlashPadTrapez( const VECTOR2I& aPadPos, const VECTOR2I* aCorners, const EDA_ANGLE& aPadOrient,
                                  void* aData )
{
    std::vector<VECTOR2I> corners;

    for( int i = 0; i < 4; i++ )
    {
        VECTOR2I corner = aCorners[i];
        RotatePoint( corner, aPadOrient );
        corner += aPadPos;
        corners.push_back( corner );
    }

    PlotPoly( corners, FILL_T::FILLED_SHAPE, 0, aData );
}


void PNG_PLOTTER::FlashRegularPolygon( const VECTOR2I& aShapePos, int aDiameter, int aCornerCount,
                                       const EDA_ANGLE& aOrient, void* aData )
{
    std::vector<VECTOR2I> corners;
    double                radius = aDiameter / 2.0;
    EDA_ANGLE             delta = ANGLE_360 / aCornerCount;

    for( int i = 0; i < aCornerCount; i++ )
    {
        EDA_ANGLE angle = aOrient + delta * i;
        VECTOR2I  corner( radius * cos( angle.AsRadians() ), radius * sin( angle.AsRadians() ) );
        corner += aShapePos;
        corners.push_back( corner );
    }

    PlotPoly( corners, FILL_T::FILLED_SHAPE, 0, aData );
}


VECTOR2D PNG_PLOTTER::userToDeviceCoordinates( const VECTOR2I& aCoordinate )
{
    VECTOR2D pos( aCoordinate.x, aCoordinate.y );

    pos.x -= m_plotOffset.x;
    pos.y -= m_plotOffset.y;

    pos.x = pos.x * m_plotScale / m_iuPerDeviceUnit;
    pos.y = pos.y * m_plotScale / m_iuPerDeviceUnit;

    if( m_plotMirror )
        pos.x = m_width - pos.x;

    // pcbnew is Y-up, Cairo is Y-down; gerbview already emits Y-down so the caller decides.
    if( m_yaxisReversed )
        pos.y = m_height - pos.y;

    return pos;
}


VECTOR2D PNG_PLOTTER::userToDeviceSize( const VECTOR2I& aSize )
{
    return VECTOR2D( userToDeviceSize( static_cast<double>( aSize.x ) ),
                     userToDeviceSize( static_cast<double>( aSize.y ) ) );
}


double PNG_PLOTTER::userToDeviceSize( double aSize ) const
{
    return std::abs( aSize * m_plotScale / m_iuPerDeviceUnit );
}


void PNG_PLOTTER::fillRect( double aX, double aY, double aWidth, double aHeight )
{
    if( !m_context )
        return;

    cairo_rectangle( m_context, aX, aY, aWidth, aHeight );
    cairo_fill( m_context );
}


void PNG_PLOTTER::strokeRect( double aX, double aY, double aWidth, double aHeight )
{
    if( !m_context )
        return;

    cairo_rectangle( m_context, aX, aY, aWidth, aHeight );
    cairo_stroke( m_context );
}


void PNG_PLOTTER::fillCircle( double aCx, double aCy, double aRadius )
{
    if( !m_context )
        return;

    cairo_arc( m_context, aCx, aCy, aRadius, 0, 2 * M_PI );
    cairo_fill( m_context );
}


void PNG_PLOTTER::strokeCircle( double aCx, double aCy, double aRadius )
{
    if( !m_context )
        return;

    cairo_arc( m_context, aCx, aCy, aRadius, 0, 2 * M_PI );
    cairo_stroke( m_context );
}
