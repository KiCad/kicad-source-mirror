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

#include <cmath>


PNG_PLOTTER::PNG_PLOTTER() :
        m_surface( nullptr ),
        m_context( nullptr ),
        m_dpi( 300 ),
        m_width( 0 ),
        m_height( 0 ),
        m_antialias( false ),
        m_backgroundColor( COLOR4D( 0, 0, 0, 0 ) ),
        m_currentColor( COLOR4D::BLACK )
{
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


bool PNG_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    if( m_width <= 0 || m_height <= 0 )
        return false;

    // Clean up any existing surface
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

    // Create Cairo image surface with ARGB32 format for transparency support
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

    // Configure anti-aliasing
    if( m_antialias )
        cairo_set_antialias( m_context, CAIRO_ANTIALIAS_DEFAULT );
    else
        cairo_set_antialias( m_context, CAIRO_ANTIALIAS_NONE );

    // Fill with background color if specified (non-transparent)
    if( m_backgroundColor.a > 0 )
    {
        cairo_set_source_rgba( m_context, m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b,
                               m_backgroundColor.a );
        cairo_paint( m_context );
    }

    // Set default line properties
    cairo_set_line_cap( m_context, CAIRO_LINE_CAP_ROUND );
    cairo_set_line_join( m_context, CAIRO_LINE_JOIN_ROUND );

    return true;
}


bool PNG_PLOTTER::EndPlot()
{
    if( !m_context )
        return false;

    // Flush any pending drawing operations
    cairo_surface_flush( m_surface );

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
    m_currentPenWidth = aWidth;

    if( m_context )
    {
        double deviceWidth = userToDeviceSize( static_cast<double>( aWidth ) );
        cairo_set_line_width( m_context, deviceWidth > 0 ? deviceWidth : 1.0 );
    }
}


void PNG_PLOTTER::SetColor( const COLOR4D& aColor )
{
    m_currentColor = aColor;

    if( m_context )
    {
        cairo_set_source_rgba( m_context, aColor.r, aColor.g, aColor.b, aColor.a );
    }
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

    // For now, only solid lines are supported in the basic implementation
    cairo_set_dash( m_context, nullptr, 0, 0 );
}


void PNG_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil, double aScale, bool aMirror )
{
    m_plotOffset = aOffset;
    m_IUsPerDecimil = aIusPerDecimil;
    m_plotScale = aScale;
    m_plotMirror = aMirror;

    // Calculate device scale factor
    // DPI defines pixels per inch, and there are 10000 decimils per inch
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

    // Cairo uses radians, angles measured from positive X axis
    // KiCad angles are in degrees/decidegrees, positive is counter-clockwise
    double startRad = aStartAngle.AsRadians();
    double endRad = ( aStartAngle + aAngle ).AsRadians();

    // Cairo draws arcs counter-clockwise from start to end angle
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
        if( m_penState == 'U' )
            cairo_move_to( m_context, m_penLastpos.x, m_penLastpos.y );

        cairo_line_to( m_context, pos.x, pos.y );
        m_penState = 'D';
        break;

    case 'Z':
        cairo_stroke( m_context );
        m_penState = 'Z';
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

    // Close the path for filled polygons
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


void PNG_PLOTTER::FlashPadCircle( const VECTOR2I& aPadPos, int aDiameter, void* aData )
{
    Circle( aPadPos, aDiameter, FILL_T::FILLED_SHAPE, 0 );
}


void PNG_PLOTTER::FlashPadOval( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                                void* aData )
{
    // For simplicity, render oval as a thick line between two semicircle centers
    // with round end caps (which is what an oval pad is)
    int width = std::min( aSize.x, aSize.y );
    int len = std::max( aSize.x, aSize.y ) - width;

    if( len == 0 )
    {
        // It's actually a circle
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

    // Rotate delta by pad orientation
    RotatePoint( delta, aPadOrient );

    VECTOR2I start = aPadPos - delta;
    VECTOR2I end = aPadPos + delta;

    ThickSegment( start, end, width, aData );
}


void PNG_PLOTTER::FlashPadRect( const VECTOR2I& aPadPos, const VECTOR2I& aSize, const EDA_ANGLE& aPadOrient,
                                void* aData )
{
    // For rotated rectangles, compute the 4 corners and draw as polygon
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
    // Generate rounded rectangle polygon and draw it
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

    // Apply offset
    pos.x -= m_plotOffset.x;
    pos.y -= m_plotOffset.y;

    // Apply scale
    pos.x = pos.x * m_plotScale / m_iuPerDeviceUnit;
    pos.y = pos.y * m_plotScale / m_iuPerDeviceUnit;

    // Handle mirroring
    if( m_plotMirror )
        pos.x = m_width - pos.x;

    // Input is already in Y-down screen convention (matching Cairo): gerbview
    // passes GERBER_DRAW_ITEM::GetABPosition() output which, for non-mirrored
    // B-axis files, already negates gerber-native Y so Y grows downward to the
    // image. No additional flip is needed here, and adding one would
    // double-invert and produce vertically flipped PNGs (issue 24048).
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
