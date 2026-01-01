/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * CairoGal - Graphics Abstraction Layer for Cairo
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

#include <wx/image.h>
#include <wx/log.h>

#include <gal/cairo/cairo_gal.h>
#include <gal/cairo/cairo_compositor.h>
#include <gal/definitions.h>
#include <geometry/shape_poly_set.h>
#include <math/vector2wx.h>
#include <math/util.h> // for KiROUND
#include <trigo.h>
#include <bitmap_base.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <pixman.h>

using namespace KIGFX;


CAIRO_GAL_BASE::CAIRO_GAL_BASE( GAL_DISPLAY_OPTIONS& aDisplayOptions ) : GAL( aDisplayOptions )
{
    // Initialise grouping
    m_isGrouping = false;
    m_isElementAdded = false;
    m_groupCounter = 0;
    m_currentGroup = nullptr;

    m_lineWidth = 1.0;
    m_lineWidthInPixels = 1.0;
    m_lineWidthIsOdd = true;

    // Initialise Cairo state
    cairo_matrix_init_identity( &m_cairoWorldScreenMatrix );
    m_currentContext = nullptr;
    m_context = nullptr;
    m_surface = nullptr;

    // Grid color settings are different in Cairo and OpenGL
    SetGridColor( COLOR4D( 0.1, 0.1, 0.1, 0.8 ) );
    SetAxesColor( COLOR4D( BLUE ) );

    // Avoid uninitialized variables:
    cairo_matrix_init_identity( &m_currentXform );
    cairo_matrix_init_identity( &m_currentWorld2Screen );
}


CAIRO_GAL_BASE::~CAIRO_GAL_BASE()
{
    ClearCache();

    if( m_surface )
        cairo_surface_destroy( m_surface );

    if( m_context )
        cairo_destroy( m_context );

    for( _cairo_surface* imageSurface : m_imageSurfaces )
        cairo_surface_destroy( imageSurface );
}


void CAIRO_GAL_BASE::BeginDrawing()
{
    resetContext();
}


void CAIRO_GAL_BASE::EndDrawing()
{
    // Force remaining objects to be drawn
    Flush();
}


void CAIRO_GAL_BASE::updateWorldScreenMatrix()
{
    cairo_matrix_multiply( &m_currentWorld2Screen, &m_currentXform, &m_cairoWorldScreenMatrix );
}


const VECTOR2D CAIRO_GAL_BASE::xform( double x, double y )
{
    VECTOR2D rv;

    rv.x = m_currentWorld2Screen.xx * x + m_currentWorld2Screen.xy * y + m_currentWorld2Screen.x0;
    rv.y = m_currentWorld2Screen.yx * x + m_currentWorld2Screen.yy * y + m_currentWorld2Screen.y0;
    return rv;
}


const VECTOR2D CAIRO_GAL_BASE::xform( const VECTOR2D& aP )
{
    return xform( aP.x, aP.y );
}


double CAIRO_GAL_BASE::angle_xform( double aAngle )
{
    // calculate rotation angle due to the rotation transform
    // and if flipped on X axis.
    double world_rotation = -std::atan2( m_currentWorld2Screen.xy, m_currentWorld2Screen.xx );

    // When flipped on X axis, the rotation angle is M_PI - initial angle:
    if( IsFlippedX() )
        world_rotation = M_PI - world_rotation;

    return std::fmod( aAngle + world_rotation, 2.0 * M_PI );
}


void CAIRO_GAL_BASE::arc_angles_xform_and_normalize( double& aStartAngle, double& aEndAngle )
{
    // 360 deg arcs have a specific calculation.
    bool is_360deg_arc = std::abs( aEndAngle - aStartAngle ) >= 2 * M_PI;
    double startAngle = aStartAngle;
    double endAngle = aEndAngle;

    // When the view is flipped, the coordinates are flipped by the matrix transform
    // However, arc angles need to be "flipped": the flipped angle is M_PI - initial angle.
    if( IsFlippedX() )
    {
        startAngle = M_PI - startAngle;
        endAngle = M_PI - endAngle;
    }

    // Normalize arc angles
    normalize( startAngle, endAngle );

    // now rotate arc according to the rotation transform matrix
    // Remark:
    // We call angle_xform() to calculate angles according to the flip/rotation
    // transform and normalize between -2M_PI and +2M_PI.
    // Therefore, if aStartAngle = aEndAngle + 2*n*M_PI, the transform gives
    // aEndAngle = aStartAngle
    // So, if this is the case, force the aEndAngle value to draw a circle.
    aStartAngle = angle_xform( startAngle );

    if( is_360deg_arc ) // arc is a full circle
        aEndAngle = aStartAngle + 2 * M_PI;
    else
        aEndAngle = angle_xform( endAngle );
}


double CAIRO_GAL_BASE::xform( double x )
{
    double dx = m_currentWorld2Screen.xx * x;
    double dy = m_currentWorld2Screen.yx * x;
    return sqrt( dx * dx + dy * dy );
}


static double roundp( double x )
{
    return floor( x + 0.5 ) + 0.5;
}


const VECTOR2D CAIRO_GAL_BASE::roundp( const VECTOR2D& v )
{
    if( m_lineWidthIsOdd )
        return VECTOR2D( ::roundp( v.x ), ::roundp( v.y ) );
    else
        return VECTOR2D( floor( v.x + 0.5 ), floor( v.y + 0.5 ) );
}


void CAIRO_GAL_BASE::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    syncLineWidth();

    VECTOR2D p0 = roundp( xform( aStartPoint ) );
    VECTOR2D p1 = roundp( xform( aEndPoint ) );

    cairo_move_to( m_currentContext, p0.x, p0.y );
    cairo_line_to( m_currentContext, p1.x, p1.y );
    flushPath();
    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::syncLineWidth( bool aForceWidth, double aWidth )
{
    double w = floor( xform( aForceWidth ? aWidth : m_lineWidth ) + 0.5 );

    if( w <= 1.0 )
    {
        w = 1.0;
        cairo_set_line_join( m_currentContext, CAIRO_LINE_JOIN_MITER );
        cairo_set_line_cap( m_currentContext, CAIRO_LINE_CAP_BUTT );
        cairo_set_line_width( m_currentContext, 1.0 );
        m_lineWidthIsOdd = true;
    }
    else
    {
        cairo_set_line_join( m_currentContext, CAIRO_LINE_JOIN_ROUND );
        cairo_set_line_cap( m_currentContext, CAIRO_LINE_CAP_ROUND );
        cairo_set_line_width( m_currentContext, w );
        m_lineWidthIsOdd = ( (int) w % 2 ) == 1;
    }

    m_lineWidthInPixels = w;
}


void CAIRO_GAL_BASE::DrawSegmentChain( const std::vector<VECTOR2D>& aPointList, double aWidth )
{
    for( size_t i = 0; i + 1 < aPointList.size(); ++i )
        DrawSegment( aPointList[i], aPointList[i + 1], aWidth );
}


void CAIRO_GAL_BASE::DrawSegmentChain( const SHAPE_LINE_CHAIN& aLineChain, double aWidth )
{
    int numPoints = aLineChain.PointCount();

    if( aLineChain.IsClosed() )
        numPoints += 1;

    for( int i = 0; i + 1 < numPoints; ++i )
        DrawSegment( aLineChain.CPoint( i ), aLineChain.CPoint( i + 1 ), aWidth );
}


void CAIRO_GAL_BASE::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                                  double aWidth )
{
    if( m_isFillEnabled )
    {
        syncLineWidth( true, aWidth );

        VECTOR2D p0 = roundp( xform( aStartPoint ) );
        VECTOR2D p1 = roundp( xform( aEndPoint ) );

        cairo_move_to( m_currentContext, p0.x, p0.y );
        cairo_line_to( m_currentContext, p1.x, p1.y );
        cairo_set_source_rgba( m_currentContext, m_fillColor.r, m_fillColor.g, m_fillColor.b,
                               m_fillColor.a );
        cairo_stroke( m_currentContext );
    }
    else
    {
        aWidth /= 2.0;
        SetLineWidth( 1.0 );
        syncLineWidth();

        // Outline mode for tracks
        VECTOR2D startEndVector = aEndPoint - aStartPoint;
        double   lineAngle = atan2( startEndVector.y, startEndVector.x );

        double sa = sin( lineAngle + M_PI / 2.0 );
        double ca = cos( lineAngle + M_PI / 2.0 );

        VECTOR2D pa0 = xform( aStartPoint + VECTOR2D( aWidth * ca, aWidth * sa ) );
        VECTOR2D pa1 = xform( aStartPoint - VECTOR2D( aWidth * ca, aWidth * sa ) );
        VECTOR2D pb0 = xform( aEndPoint + VECTOR2D( aWidth * ca, aWidth * sa ) );
        VECTOR2D pb1 = xform( aEndPoint - VECTOR2D( aWidth * ca, aWidth * sa ) );

        cairo_set_source_rgba( m_currentContext, m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                               m_strokeColor.a );

        cairo_move_to( m_currentContext, pa0.x, pa0.y );
        cairo_line_to( m_currentContext, pb0.x, pb0.y );

        cairo_move_to( m_currentContext, pa1.x, pa1.y );
        cairo_line_to( m_currentContext, pb1.x, pb1.y );
        flushPath();

        // Calculate the segment angle and arc center in normal/mirrored transform for rounded ends.
        VECTOR2D center_a = xform( aStartPoint );
        VECTOR2D center_b = xform( aEndPoint );
        startEndVector = center_b - center_a;
        lineAngle = atan2( startEndVector.y, startEndVector.x );
        double radius = ( pa0 - center_a ).EuclideanNorm();

        // Draw the rounded end point of the segment
        double arcStartAngle = lineAngle - M_PI / 2.0;
        cairo_arc( m_currentContext, center_b.x, center_b.y, radius, arcStartAngle,
                   arcStartAngle + M_PI );

        // Draw the rounded start point of the segment
        arcStartAngle = lineAngle + M_PI / 2.0;
        cairo_arc( m_currentContext, center_a.x, center_a.y, radius, arcStartAngle,
                   arcStartAngle + M_PI );

        flushPath();
    }

    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawHoleWall( const VECTOR2D& aCenterPoint, double aRadius, double aWallWidth )
{
    DrawCircle( aCenterPoint, aRadius + aWallWidth );
}


void CAIRO_GAL_BASE::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    syncLineWidth();

    VECTOR2D c = roundp( xform( aCenterPoint ) );
    double   r = ::roundp( xform( aRadius ) );

    cairo_set_line_width( m_currentContext, std::min( 2.0 * r, m_lineWidthInPixels ) );
    cairo_new_sub_path( m_currentContext );
    cairo_arc( m_currentContext, c.x, c.y, r, 0.0, 2 * M_PI );
    cairo_close_path( m_currentContext );
    flushPath();
    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawArc( const VECTOR2D& aCenterPoint, double aRadius,
                              const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle )
{
    syncLineWidth();

    double startAngle = aStartAngle.AsRadians();
    double endAngle = startAngle + aAngle.AsRadians();

    // calculate start and end arc angles according to the rotation transform matrix
    // and normalize:
    arc_angles_xform_and_normalize( startAngle, endAngle );

    double r = xform( aRadius );

    // Adjust center and radius slightly to better match the rounding of endpoints.
    VECTOR2D mid = roundp( xform( aCenterPoint ) );

    VECTOR2D startPointS = VECTOR2D( r, 0.0 );
    VECTOR2D endPointS = VECTOR2D( r, 0.0 );
    RotatePoint( startPointS, -EDA_ANGLE( startAngle, RADIANS_T ) );
    RotatePoint( endPointS, -EDA_ANGLE( endAngle, RADIANS_T ) );

    VECTOR2D refStart = roundp( xform( aCenterPoint ) + startPointS );
    VECTOR2D refEnd = roundp( xform( aCenterPoint ) + endPointS );

    r = ( ( refStart - mid ).EuclideanNorm() + ( refEnd - mid ).EuclideanNorm() ) / 2.0;

    cairo_set_line_width( m_currentContext, m_lineWidthInPixels );
    cairo_new_sub_path( m_currentContext );

    if( m_isFillEnabled )
        cairo_move_to( m_currentContext, mid.x, mid.y );

    cairo_arc( m_currentContext, mid.x, mid.y, r, startAngle, endAngle );

    if( m_isFillEnabled )
        cairo_close_path( m_currentContext );

    flushPath();

    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius,
                                     const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                                     double aWidth, double aMaxError )
{
    // Note: aMaxError is not used because Cairo can draw true arcs
    if( m_isFillEnabled )
    {
        m_lineWidth = aWidth;
        m_isStrokeEnabled = true;
        m_isFillEnabled = false;
        DrawArc( aCenterPoint, aRadius, aStartAngle, aAngle );
        m_isFillEnabled = true;
        m_isStrokeEnabled = false;
        return;
    }

    syncLineWidth();

    // calculate start and end arc angles according to the rotation transform matrix
    // and normalize:
    double startAngleS = aStartAngle.AsRadians();
    double endAngleS = startAngleS + aAngle.AsRadians();
    arc_angles_xform_and_normalize( startAngleS, endAngleS );

    double r = xform( aRadius );

    VECTOR2D mid = xform( aCenterPoint );
    double   width = xform( aWidth / 2.0 );
    VECTOR2D startPointS = VECTOR2D( r, 0.0 );
    VECTOR2D endPointS = VECTOR2D( r, 0.0 );
    RotatePoint( startPointS, -EDA_ANGLE( startAngleS, RADIANS_T ) );
    RotatePoint( endPointS, -EDA_ANGLE( endAngleS, RADIANS_T ) );

    cairo_save( m_currentContext );

    cairo_set_source_rgba( m_currentContext, m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                           m_strokeColor.a );

    cairo_translate( m_currentContext, mid.x, mid.y );

    cairo_new_sub_path( m_currentContext );
    cairo_arc( m_currentContext, 0, 0, r - width, startAngleS, endAngleS );

    cairo_new_sub_path( m_currentContext );
    cairo_arc( m_currentContext, 0, 0, r + width, startAngleS, endAngleS );

    cairo_new_sub_path( m_currentContext );
    cairo_arc_negative( m_currentContext, startPointS.x, startPointS.y, width, startAngleS,
                        startAngleS + M_PI );

    cairo_new_sub_path( m_currentContext );
    cairo_arc( m_currentContext, endPointS.x, endPointS.y, width, endAngleS, endAngleS + M_PI );

    cairo_restore( m_currentContext );
    flushPath();

    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    // Calculate the diagonal points
    syncLineWidth();

    const VECTOR2D p0 = roundp( xform( aStartPoint ) );
    const VECTOR2D p1 = roundp( xform( VECTOR2D( aEndPoint.x, aStartPoint.y ) ) );
    const VECTOR2D p2 = roundp( xform( aEndPoint ) );
    const VECTOR2D p3 = roundp( xform( VECTOR2D( aStartPoint.x, aEndPoint.y ) ) );

    // The path is composed from 4 segments
    cairo_move_to( m_currentContext, p0.x, p0.y );
    cairo_line_to( m_currentContext, p1.x, p1.y );
    cairo_line_to( m_currentContext, p2.x, p2.y );
    cairo_line_to( m_currentContext, p3.x, p3.y );
    cairo_close_path( m_currentContext );
    flushPath();

    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawPolygon( const SHAPE_POLY_SET& aPolySet, bool aStrokeTriangulation )
{
    for( int i = 0; i < aPolySet.OutlineCount(); ++i )
        drawPoly( aPolySet.COutline( i ) );
}


void CAIRO_GAL_BASE::DrawPolygon( const SHAPE_LINE_CHAIN& aPolygon )
{
    drawPoly( aPolygon );
}


void CAIRO_GAL_BASE::DrawCurve( const VECTOR2D& aStartPoint, const VECTOR2D& aControlPointA,
                                const VECTOR2D& aControlPointB, const VECTOR2D& aEndPoint,
                                double aFilterValue )
{
    // Note: aFilterValue is not used because the cubic Bezier curve is
    // supported by Cairo.
    syncLineWidth();

    const VECTOR2D sp = roundp( xform( aStartPoint ) );
    const VECTOR2D cpa = roundp( xform( aControlPointA ) );
    const VECTOR2D cpb = roundp( xform( aControlPointB ) );
    const VECTOR2D ep = roundp( xform( aEndPoint ) );

    cairo_move_to( m_currentContext, sp.x, sp.y );
    cairo_curve_to( m_currentContext, cpa.x, cpa.y, cpb.x, cpb.y, ep.x, ep.y );
    cairo_line_to( m_currentContext, ep.x, ep.y );

    flushPath();
    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawBitmap( const BITMAP_BASE& aBitmap, double alphaBlend )
{
    cairo_save( m_currentContext );

    alphaBlend = std::clamp( alphaBlend, 0.0, 1.0 );

    // We have to calculate the pixel size in users units to draw the image.
    // m_worldUnitLength is a factor used for converting IU to inches
    double scale = 1.0 / ( aBitmap.GetPPI() * m_worldUnitLength );

    // The position of the bitmap is the bitmap center.
    // move the draw origin to the top left bitmap corner:
    int w = aBitmap.GetSizePixels().x;
    int h = aBitmap.GetSizePixels().y;

    cairo_set_matrix( m_currentContext, &m_currentWorld2Screen );
    cairo_scale( m_currentContext, scale, scale );
    cairo_translate( m_currentContext, -w / 2.0, -h / 2.0 );

    cairo_new_path( m_currentContext );
    cairo_surface_t* image = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, w, h );
    cairo_surface_flush( image );

    unsigned char* pix_buffer = cairo_image_surface_get_data( image );

    // The pixel buffer of the initial bitmap:
    const wxImage& bm_pix_buffer = *aBitmap.GetImageData();

    uint32_t mask_color = ( bm_pix_buffer.GetMaskRed() << 16 )
                          + ( bm_pix_buffer.GetMaskGreen() << 8 ) + ( bm_pix_buffer.GetMaskBlue() );

    // Copy the source bitmap to the cairo bitmap buffer.
    // In cairo bitmap buffer, a ARGB32 bitmap is an ARGB pixel packed into a uint_32
    // 24 low bits only are used for color, top 8 are transparency.
    for( int row = 0; row < h; row++ )
    {
        for( int col = 0; col < w; col++ )
        {
            unsigned char r = bm_pix_buffer.GetRed( col, row );
            unsigned char g = bm_pix_buffer.GetGreen( col, row );
            unsigned char b = bm_pix_buffer.GetBlue( col, row );
            unsigned char a = wxALPHA_OPAQUE;

            if( bm_pix_buffer.HasAlpha() )
            {
                a = bm_pix_buffer.GetAlpha( col, row );

                // ARGB32 format needs pre-multiplied alpha
                r = uint32_t( r ) * a / 0xFF;
                g = uint32_t( g ) * a / 0xFF;
                b = uint32_t( b ) * a / 0xFF;
            }
            else if( bm_pix_buffer.HasMask() && (uint32_t)( r << 16 | g << 8 | b ) == mask_color )
            {
                a = wxALPHA_TRANSPARENT;
            }

            // Build the ARGB24 pixel:
            uint32_t pixel = a << 24 | r << 16 | g << 8 | b;

            // Write the pixel to the cairo image buffer:
            uint32_t* pix_ptr = (uint32_t*) pix_buffer;
            *pix_ptr = pixel;
            pix_buffer += 4;
        }
    }

    cairo_surface_mark_dirty( image );
    cairo_set_source_surface( m_currentContext, image, 0, 0 );
    cairo_paint_with_alpha( m_currentContext, alphaBlend );

    // store the image handle so it can be destroyed later
    m_imageSurfaces.push_back( image );

    m_isElementAdded = true;

    cairo_restore( m_currentContext );
}


void CAIRO_GAL_BASE::ResizeScreen( int aWidth, int aHeight )
{
    m_screenSize = VECTOR2I( aWidth, aHeight );
}


void CAIRO_GAL_BASE::Flush()
{
    storePath();
}


void CAIRO_GAL_BASE::ClearScreen()
{
    cairo_set_source_rgb( m_currentContext, m_clearColor.r, m_clearColor.g, m_clearColor.b );
    cairo_rectangle( m_currentContext, 0.0, 0.0, m_screenSize.x, m_screenSize.y );
    cairo_fill( m_currentContext );
}


void CAIRO_GAL_BASE::SetIsFill( bool aIsFillEnabled )
{
    storePath();
    m_isFillEnabled = aIsFillEnabled;

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SET_FILL;
        groupElement.m_Argument.BoolArg = aIsFillEnabled;
        m_currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetIsStroke( bool aIsStrokeEnabled )
{
    storePath();
    m_isStrokeEnabled = aIsStrokeEnabled;

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SET_STROKE;
        groupElement.m_Argument.BoolArg = aIsStrokeEnabled;
        m_currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetStrokeColor( const COLOR4D& aColor )
{
    storePath();
    m_strokeColor = aColor;

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SET_STROKECOLOR;
        groupElement.m_Argument.DblArg[0] = m_strokeColor.r;
        groupElement.m_Argument.DblArg[1] = m_strokeColor.g;
        groupElement.m_Argument.DblArg[2] = m_strokeColor.b;
        groupElement.m_Argument.DblArg[3] = m_strokeColor.a;
        m_currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetFillColor( const COLOR4D& aColor )
{
    storePath();
    m_fillColor = aColor;

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SET_FILLCOLOR;
        groupElement.m_Argument.DblArg[0] = m_fillColor.r;
        groupElement.m_Argument.DblArg[1] = m_fillColor.g;
        groupElement.m_Argument.DblArg[2] = m_fillColor.b;
        groupElement.m_Argument.DblArg[3] = m_fillColor.a;
        m_currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetLineWidth( float aLineWidth )
{
    storePath();
    GAL::SetLineWidth( aLineWidth );

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SET_LINE_WIDTH;
        groupElement.m_Argument.DblArg[0] = aLineWidth;
        m_currentGroup->push_back( groupElement );
    }
    else
    {
        m_lineWidth = aLineWidth;
    }
}


void CAIRO_GAL_BASE::SetLayerDepth( double aLayerDepth )
{
    super::SetLayerDepth( aLayerDepth );
    storePath();
}


void CAIRO_GAL_BASE::Transform( const MATRIX3x3D& aTransformation )
{
    cairo_matrix_t cairoTransformation, newXform;

    cairo_matrix_init( &cairoTransformation, aTransformation.m_data[0][0],
                       aTransformation.m_data[1][0], aTransformation.m_data[0][1],
                       aTransformation.m_data[1][1], aTransformation.m_data[0][2],
                       aTransformation.m_data[1][2] );

    cairo_matrix_multiply( &newXform, &m_currentXform, &cairoTransformation );
    m_currentXform = newXform;
    updateWorldScreenMatrix();
}


void CAIRO_GAL_BASE::Rotate( double aAngle )
{
    storePath();

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_ROTATE;
        groupElement.m_Argument.DblArg[0] = aAngle;
        m_currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_matrix_rotate( &m_currentXform, aAngle );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Translate( const VECTOR2D& aTranslation )
{
    storePath();

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_TRANSLATE;
        groupElement.m_Argument.DblArg[0] = aTranslation.x;
        groupElement.m_Argument.DblArg[1] = aTranslation.y;
        m_currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_matrix_translate( &m_currentXform, aTranslation.x, aTranslation.y );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Scale( const VECTOR2D& aScale )
{
    storePath();

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SCALE;
        groupElement.m_Argument.DblArg[0] = aScale.x;
        groupElement.m_Argument.DblArg[1] = aScale.y;
        m_currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_matrix_scale( &m_currentXform, aScale.x, aScale.y );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Save()
{
    storePath();

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_SAVE;
        m_currentGroup->push_back( groupElement );
    }
    else
    {
        m_xformStack.push_back( m_currentXform );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Restore()
{
    storePath();

    if( m_isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.m_Command = CMD_RESTORE;
        m_currentGroup->push_back( groupElement );
    }
    else
    {
        if( !m_xformStack.empty() )
        {
            m_currentXform = m_xformStack.back();
            m_xformStack.pop_back();
            updateWorldScreenMatrix();
        }
    }
}


int CAIRO_GAL_BASE::BeginGroup()
{
    // If the grouping is started: the actual path is stored in the group, when
    // a attribute was changed or when grouping stops with the end group method.
    storePath();

    GROUP group;
    int   groupNumber = getNewGroupNumber();
    m_groups.insert( std::make_pair( groupNumber, group ) );
    m_currentGroup = &m_groups[groupNumber];
    m_isGrouping = true;

    return groupNumber;
}


void CAIRO_GAL_BASE::EndGroup()
{
    storePath();
    m_isGrouping = false;
}


void CAIRO_GAL_BASE::DrawGroup( int aGroupNumber )
{
    // This method implements a small Virtual Machine - all stored commands
    // are executed; nested calling is also possible

    storePath();

    for( auto it = m_groups[aGroupNumber].begin(); it != m_groups[aGroupNumber].end(); ++it )
    {
        switch( it->m_Command )
        {
        case CMD_SET_FILL:
            m_isFillEnabled = it->m_Argument.BoolArg;
            break;

        case CMD_SET_STROKE:
            m_isStrokeEnabled = it->m_Argument.BoolArg;
            break;

        case CMD_SET_FILLCOLOR:
            m_fillColor = COLOR4D( it->m_Argument.DblArg[0], it->m_Argument.DblArg[1],
                                   it->m_Argument.DblArg[2], it->m_Argument.DblArg[3] );
            break;

        case CMD_SET_STROKECOLOR:
            m_strokeColor = COLOR4D( it->m_Argument.DblArg[0], it->m_Argument.DblArg[1],
                                     it->m_Argument.DblArg[2], it->m_Argument.DblArg[3] );
            break;

        case CMD_SET_LINE_WIDTH:
        {
            // Make lines appear at least 1 pixel wide, no matter of zoom
            double x = 1.0, y = 1.0;
            cairo_device_to_user_distance( m_currentContext, &x, &y );
            double minWidth = std::min( fabs( x ), fabs( y ) );
            cairo_set_line_width( m_currentContext,
                                  std::max( it->m_Argument.DblArg[0], minWidth ) );
            break;
        }


        case CMD_STROKE_PATH:
            cairo_set_source_rgba( m_currentContext, m_strokeColor.r, m_strokeColor.g,
                                   m_strokeColor.b, m_strokeColor.a );
            cairo_append_path( m_currentContext, it->m_CairoPath );
            cairo_stroke( m_currentContext );
            break;

        case CMD_FILL_PATH:
            cairo_set_source_rgba( m_currentContext, m_fillColor.r, m_fillColor.g, m_fillColor.b,
                                   m_strokeColor.a );
            cairo_append_path( m_currentContext, it->m_CairoPath );
            cairo_fill( m_currentContext );
            break;

            /*
        case CMD_TRANSFORM:
            cairo_matrix_t matrix;
            cairo_matrix_init( &matrix, it->argument.DblArg[0], it->argument.DblArg[1],
                               it->argument.DblArg[2], it->argument.DblArg[3],
                               it->argument.DblArg[4], it->argument.DblArg[5] );
            cairo_transform( m_currentContext, &matrix );
            break;
            */

        case CMD_ROTATE:
            cairo_rotate( m_currentContext, it->m_Argument.DblArg[0] );
            break;

        case CMD_TRANSLATE:
            cairo_translate( m_currentContext, it->m_Argument.DblArg[0], it->m_Argument.DblArg[1] );
            break;

        case CMD_SCALE:
            cairo_scale( m_currentContext, it->m_Argument.DblArg[0], it->m_Argument.DblArg[1] );
            break;

        case CMD_SAVE:
            cairo_save( m_currentContext );
            break;

        case CMD_RESTORE:
            cairo_restore( m_currentContext );
            break;

        case CMD_CALL_GROUP:
            DrawGroup( it->m_Argument.IntArg );
            break;
        }
    }
}


void CAIRO_GAL_BASE::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    storePath();

    for( auto it = m_groups[aGroupNumber].begin(); it != m_groups[aGroupNumber].end(); ++it )
    {
        if( it->m_Command == CMD_SET_FILLCOLOR || it->m_Command == CMD_SET_STROKECOLOR )
        {
            it->m_Argument.DblArg[0] = aNewColor.r;
            it->m_Argument.DblArg[1] = aNewColor.g;
            it->m_Argument.DblArg[2] = aNewColor.b;
            it->m_Argument.DblArg[3] = aNewColor.a;
        }
    }
}


void CAIRO_GAL_BASE::ChangeGroupDepth( int aGroupNumber, int aDepth )
{
    // Cairo does not have any possibilities to change the depth coordinate of stored items,
    // it depends only on the order of drawing
}


void CAIRO_GAL_BASE::DeleteGroup( int aGroupNumber )
{
    storePath();

    // Delete the Cairo paths
    std::deque<GROUP_ELEMENT>::iterator it, end;

    for( it = m_groups[aGroupNumber].begin(), end = m_groups[aGroupNumber].end(); it != end; ++it )
    {
        if( it->m_Command == CMD_FILL_PATH || it->m_Command == CMD_STROKE_PATH )
            cairo_path_destroy( it->m_CairoPath );
    }

    // Delete the group
    m_groups.erase( aGroupNumber );
}


void CAIRO_GAL_BASE::ClearCache()
{
    for( auto it = m_groups.begin(); it != m_groups.end(); )
        DeleteGroup( ( it++ )->first );
}


void CAIRO_GAL_BASE::SetNegativeDrawMode( bool aSetting )
{
    cairo_set_operator( m_currentContext, aSetting ? CAIRO_OPERATOR_CLEAR : CAIRO_OPERATOR_OVER );
}


void CAIRO_GAL::StartDiffLayer()
{
    SetTarget( TARGET_TEMP );
    ClearTarget( TARGET_TEMP );
}


void CAIRO_GAL::EndDiffLayer()
{
    m_compositor->DrawBuffer( m_tempBuffer, m_mainBuffer, CAIRO_OPERATOR_DIFFERENCE );
}


void CAIRO_GAL::StartNegativesLayer()
{
    SetTarget( TARGET_TEMP );
    ClearTarget( TARGET_TEMP );
}


void CAIRO_GAL::EndNegativesLayer()
{
    m_compositor->DrawBuffer( m_tempBuffer, m_mainBuffer, CAIRO_OPERATOR_OVER );
}


void CAIRO_GAL_BASE::DrawCursor( const VECTOR2D& aCursorPosition )
{
    m_cursorPosition = aCursorPosition;
}


void CAIRO_GAL_BASE::EnableDepthTest( bool aEnabled )
{
}


void CAIRO_GAL_BASE::resetContext()
{
    for( _cairo_surface* imageSurface : m_imageSurfaces )
        cairo_surface_destroy( imageSurface );

    m_imageSurfaces.clear();

    ClearScreen();

    // Compute the world <-> screen transformations
    ComputeWorldScreenMatrix();

    cairo_matrix_init( &m_cairoWorldScreenMatrix, m_worldScreenMatrix.m_data[0][0],
                       m_worldScreenMatrix.m_data[1][0], m_worldScreenMatrix.m_data[0][1],
                       m_worldScreenMatrix.m_data[1][1], m_worldScreenMatrix.m_data[0][2],
                       m_worldScreenMatrix.m_data[1][2] );

    // we work in screen-space coordinates and do the transforms outside.
    cairo_identity_matrix( m_context );

    cairo_matrix_init_identity( &m_currentXform );

    // Start drawing with a new path
    cairo_new_path( m_context );
    m_isElementAdded = true;

    updateWorldScreenMatrix();

    m_lineWidth = 0;
}


void CAIRO_GAL_BASE::drawAxes( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    syncLineWidth();

    VECTOR2D p0 = roundp( xform( aStartPoint ) );
    VECTOR2D p1 = roundp( xform( aEndPoint ) );
    VECTOR2D org = roundp( xform( VECTOR2D( 0.0, 0.0 ) ) ); // Axis origin = 0,0 coord

    cairo_set_source_rgba( m_currentContext, m_axesColor.r, m_axesColor.g, m_axesColor.b,
                           m_axesColor.a );
    cairo_move_to( m_currentContext, p0.x, org.y );
    cairo_line_to( m_currentContext, p1.x, org.y );
    cairo_move_to( m_currentContext, org.x, p0.y );
    cairo_line_to( m_currentContext, org.x, p1.y );
    cairo_stroke( m_currentContext );
}


void CAIRO_GAL_BASE::drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    syncLineWidth();
    VECTOR2D p0 = roundp( xform( aStartPoint ) );
    VECTOR2D p1 = roundp( xform( aEndPoint ) );

    cairo_set_source_rgba( m_currentContext, m_gridColor.r, m_gridColor.g, m_gridColor.b,
                           m_gridColor.a );
    cairo_move_to( m_currentContext, p0.x, p0.y );
    cairo_line_to( m_currentContext, p1.x, p1.y );
    cairo_stroke( m_currentContext );
}


void CAIRO_GAL_BASE::drawGridCross( const VECTOR2D& aPoint )
{
    syncLineWidth();
    VECTOR2D offset( 0, 0 );
    double   size = 2.0 * m_lineWidthInPixels + 0.5;

    VECTOR2D p0 = roundp( xform( aPoint ) ) - VECTOR2D( size, 0 ) + offset;
    VECTOR2D p1 = roundp( xform( aPoint ) ) + VECTOR2D( size, 0 ) + offset;
    VECTOR2D p2 = roundp( xform( aPoint ) ) - VECTOR2D( 0, size ) + offset;
    VECTOR2D p3 = roundp( xform( aPoint ) ) + VECTOR2D( 0, size ) + offset;

    cairo_set_source_rgba( m_currentContext, m_gridColor.r, m_gridColor.g, m_gridColor.b,
                           m_gridColor.a );
    cairo_move_to( m_currentContext, p0.x, p0.y );
    cairo_line_to( m_currentContext, p1.x, p1.y );
    cairo_move_to( m_currentContext, p2.x, p2.y );
    cairo_line_to( m_currentContext, p3.x, p3.y );
    cairo_stroke( m_currentContext );
}


void CAIRO_GAL_BASE::drawGridPoint( const VECTOR2D& aPoint, double aWidth, double aHeight )
{
    VECTOR2D p = roundp( xform( aPoint ) );

    double sw = std::max( 1.0, aWidth );
    double sh = std::max( 1.0, aHeight );

    cairo_set_source_rgba( m_currentContext, m_gridColor.r, m_gridColor.g, m_gridColor.b,
                           m_gridColor.a );
    cairo_rectangle( m_currentContext, p.x - std::floor( sw / 2 ) - 0.5,
                     p.y - std::floor( sh / 2 ) - 0.5, sw, sh );

    cairo_fill( m_currentContext );
}


void CAIRO_GAL_BASE::flushPath()
{
    if( m_isFillEnabled )
    {
        cairo_set_source_rgba( m_currentContext, m_fillColor.r, m_fillColor.g, m_fillColor.b,
                               m_fillColor.a );

        if( m_isStrokeEnabled )
        {
            cairo_set_line_width( m_currentContext, m_lineWidthInPixels );
            cairo_fill_preserve( m_currentContext );
        }
        else
        {
            cairo_fill( m_currentContext );
        }
    }

    if( m_isStrokeEnabled )
    {
        cairo_set_line_width( m_currentContext, m_lineWidthInPixels );
        cairo_set_source_rgba( m_currentContext, m_strokeColor.r, m_strokeColor.g, m_strokeColor.b,
                               m_strokeColor.a );
        cairo_stroke( m_currentContext );
    }
}


void CAIRO_GAL_BASE::storePath()
{
    if( m_isElementAdded )
    {
        m_isElementAdded = false;

        if( !m_isGrouping )
        {
            if( m_isFillEnabled )
            {
                cairo_set_source_rgba( m_currentContext, m_fillColor.r, m_fillColor.g,
                                       m_fillColor.b, m_fillColor.a );
                cairo_fill_preserve( m_currentContext );
            }

            if( m_isStrokeEnabled )
            {
                cairo_set_source_rgba( m_currentContext, m_strokeColor.r, m_strokeColor.g,
                                       m_strokeColor.b, m_strokeColor.a );
                cairo_stroke_preserve( m_currentContext );
            }
        }
        else
        {
            // Copy the actual path, append it to the global path list
            // then check, if the path needs to be stroked/filled and
            // add this command to the group list;
            if( m_isStrokeEnabled )
            {
                GROUP_ELEMENT groupElement;
                groupElement.m_CairoPath = cairo_copy_path( m_currentContext );
                groupElement.m_Command = CMD_STROKE_PATH;
                m_currentGroup->push_back( groupElement );
            }

            if( m_isFillEnabled )
            {
                GROUP_ELEMENT groupElement;
                groupElement.m_CairoPath = cairo_copy_path( m_currentContext );
                groupElement.m_Command = CMD_FILL_PATH;
                m_currentGroup->push_back( groupElement );
            }
        }

        cairo_new_path( m_currentContext );
    }
}


void CAIRO_GAL_BASE::blitCursor( wxMemoryDC& clientDC )
{
    if( !IsCursorEnabled() )
        return;

    VECTOR2D      p = ToScreen( m_cursorPosition );
    const COLOR4D cColor = getCursorColor();

    wxColour color( cColor.r * cColor.a * 255, cColor.g * cColor.a * 255, cColor.b * cColor.a * 255,
                    255 );
    clientDC.SetPen( wxPen( color ) );

    if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_CROSS )
    {
        clientDC.DrawLine( 0, p.y, m_screenSize.x, p.y );
        clientDC.DrawLine( p.x, 0, p.x, m_screenSize.y );
    }
    else if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL )
    {
        // Oversized but that's ok
        int diagonalSize = m_screenSize.x + m_screenSize.y;
        clientDC.DrawLine( p.x - diagonalSize, p.y - diagonalSize,
                           p.x + diagonalSize, p.y + diagonalSize );
        clientDC.DrawLine( p.x - diagonalSize, p.y + diagonalSize,
                           p.x + diagonalSize, p.y - diagonalSize );
    }
    else
    {
        const int cursorSize = 80;
        clientDC.DrawLine( p.x - cursorSize / 2, p.y, p.x + cursorSize / 2, p.y );
        clientDC.DrawLine( p.x, p.y - cursorSize / 2, p.x, p.y + cursorSize / 2 );
    }
}


void CAIRO_GAL_BASE::drawPoly( const std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.size() <= 1 )
        return;

    // Iterate over the point list and draw the segments
    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();

    syncLineWidth();

    const VECTOR2D p = roundp( xform( it->x, it->y ) );

    cairo_move_to( m_currentContext, p.x, p.y );

    for( ++it; it != aPointList.end(); ++it )
    {
        const VECTOR2D p2 = roundp( xform( it->x, it->y ) );

        cairo_line_to( m_currentContext, p2.x, p2.y );
    }

    flushPath();
    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::drawPoly( const std::vector<VECTOR2D>& aPointList )
{
    if( aPointList.size() <= 1 )
        return;

    // Iterate over the point list and draw the segments
    std::vector<VECTOR2D>::const_iterator it = aPointList.begin();

    syncLineWidth();

    const VECTOR2D p = roundp( xform( it->x, it->y ) );

    cairo_move_to( m_currentContext, p.x, p.y );

    for( ++it; it != aPointList.end(); ++it )
    {
        const VECTOR2D p2 = roundp( xform( it->x, it->y ) );

        cairo_line_to( m_currentContext, p2.x, p2.y );
    }

    flushPath();
    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::drawPoly( const VECTOR2D aPointList[], int aListSize )
{
    if( aListSize <= 1 )
        return;

    // Iterate over the point list and draw the segments
    const VECTOR2D* ptr = aPointList;

    syncLineWidth();

    const VECTOR2D p = roundp( xform( ptr->x, ptr->y ) );
    cairo_move_to( m_currentContext, p.x, p.y );

    for( int i = 1; i < aListSize; ++i )
    {
        ++ptr;
        const VECTOR2D p2 = roundp( xform( ptr->x, ptr->y ) );
        cairo_line_to( m_currentContext, p2.x, p2.y );
    }

    flushPath();
    m_isElementAdded = true;
}


void CAIRO_GAL_BASE::drawPoly( const SHAPE_LINE_CHAIN& aLineChain )
{
    if( aLineChain.PointCount() <= 1 )
        return;

    syncLineWidth();

    auto numPoints = aLineChain.PointCount();

    if( aLineChain.IsClosed() )
        numPoints += 1;

    const VECTOR2I start = aLineChain.CPoint( 0 );
    const VECTOR2D p = roundp( xform( start.x, start.y ) );
    cairo_move_to( m_currentContext, p.x, p.y );

    for( int i = 1; i < numPoints; ++i )
    {
        const VECTOR2I& pw = aLineChain.CPoint( i );
        const VECTOR2D  ps = roundp( xform( pw.x, pw.y ) );
        cairo_line_to( m_currentContext, ps.x, ps.y );
    }

    flushPath();
    m_isElementAdded = true;
}


unsigned int CAIRO_GAL_BASE::getNewGroupNumber()
{
    wxASSERT_MSG( m_groups.size() < std::numeric_limits<unsigned int>::max(),
                  wxT( "There are no free slots to store a group" ) );

    while( m_groups.find( m_groupCounter ) != m_groups.end() )
        m_groupCounter++;

    return m_groupCounter++;
}


CAIRO_GAL::CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, wxWindow* aParent,
                      wxEvtHandler* aMouseListener, wxEvtHandler* aPaintListener,
                      const wxString& aName ) :
        CAIRO_GAL_BASE( aDisplayOptions ),
        wxWindow( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxEXPAND, aName )
{
    // Initialise compositing state
    m_mainBuffer = 0;
    m_overlayBuffer = 0;
    m_tempBuffer = 0;
    m_savedBuffer = 0;
    m_validCompositor = false;
    m_currentTarget = TARGET_NONCACHED;
    SetTarget( TARGET_NONCACHED );

#ifdef _WIN32
    // need to fix broken cairo rendering on Windows with wx 3.3
    SetDoubleBuffered( false );
#endif

    m_bitmapBuffer = nullptr;
    m_wxOutput = nullptr;

    m_parentWindow = aParent;
    m_mouseListener = aMouseListener;
    m_paintListener = aPaintListener;

    // Connect the native cursor handler
    Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( CAIRO_GAL::onSetNativeCursor ), nullptr,
             this );

    // Connecting the event handlers
    Connect( wxEVT_PAINT, wxPaintEventHandler( CAIRO_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );

#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
#endif

    Bind( wxEVT_GESTURE_ZOOM, &CAIRO_GAL::skipGestureEvent, this );
    Bind( wxEVT_GESTURE_PAN, &CAIRO_GAL::skipGestureEvent, this );

    SetSize( aParent->GetClientSize() );
    m_screenSize = ToVECTOR2I( aParent->GetClientSize() );

    // Allocate memory for pixel storage
    allocateBitmaps();

    m_isInitialized = false;
}


CAIRO_GAL::~CAIRO_GAL()
{
    deleteBitmaps();
}


void CAIRO_GAL::BeginDrawing()
{
    initSurface();

    CAIRO_GAL_BASE::BeginDrawing();

    if( !m_validCompositor )
        setCompositor();

    m_compositor->SetMainContext( m_context );
    m_compositor->SetBuffer( m_mainBuffer );
}


void CAIRO_GAL::EndDrawing()
{
    CAIRO_GAL_BASE::EndDrawing();

    // Merge buffers on the screen
    m_compositor->DrawBuffer( m_mainBuffer );
    m_compositor->DrawBuffer( m_overlayBuffer );

    // Now translate the raw context data from the format stored
    // by cairo into a format understood by wxImage.
    int height = m_screenSize.y;
    int stride = m_stride;

    unsigned char* srcRow = m_bitmapBuffer;
    unsigned char* dst = m_wxOutput;

    for( int y = 0; y < height; y++ )
    {
        for( int x = 0; x < stride; x += 4 )
        {
            const unsigned char* src = srcRow + x;

#if defined( __BYTE_ORDER__ ) && ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
            // XRGB
            dst[0] = src[1];
            dst[1] = src[2];
            dst[2] = src[3];
#else
            // BGRX
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
#endif

            dst += 3;
        }

        srcRow += stride;
    }

    wxImage    img( m_wxBufferWidth, m_screenSize.y, m_wxOutput, true );
    wxBitmap   bmp( img );
    wxMemoryDC mdc( bmp );
    wxClientDC clientDC( this );

    // Now it is the time to blit the mouse cursor
    blitCursor( mdc );
    clientDC.Blit( 0, 0, m_screenSize.x, m_screenSize.y, &mdc, 0, 0, wxCOPY );

    deinitSurface();
}


void CAIRO_GAL::PostPaint( wxPaintEvent& aEvent )
{
    // posts an event to m_paint_listener to ask for redraw the canvas.
    if( m_paintListener )
        wxPostEvent( m_paintListener, aEvent );
}


void CAIRO_GAL::ResizeScreen( int aWidth, int aHeight )
{
    CAIRO_GAL_BASE::ResizeScreen( aWidth, aHeight );

    // Recreate the bitmaps
    deleteBitmaps();
    allocateBitmaps();

    if( m_validCompositor )
        m_compositor->Resize( aWidth, aHeight );

    m_validCompositor = false;

    SetSize( wxSize( aWidth, aHeight ) );
}


bool CAIRO_GAL::Show( bool aShow )
{
    bool s = wxWindow::Show( aShow );

    if( aShow )
        wxWindow::Raise();

    return s;
}


int CAIRO_GAL::BeginGroup()
{
    initSurface();
    return CAIRO_GAL_BASE::BeginGroup();
}


void CAIRO_GAL::EndGroup()
{
    CAIRO_GAL_BASE::EndGroup();
    deinitSurface();
}


void CAIRO_GAL::SetTarget( RENDER_TARGET aTarget )
{
    // If the compositor is not set, that means that there is a recaching process going on
    // and we do not need the compositor now
    if( !m_validCompositor )
        return;

    // Cairo grouping prevents display of overlapping items on the same layer in the lighter color
    if( m_isInitialized )
        storePath();

    switch( aTarget )
    {
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED: m_compositor->SetBuffer( m_mainBuffer );    break;
    case TARGET_OVERLAY:   m_compositor->SetBuffer( m_overlayBuffer ); break;
    case TARGET_TEMP:      m_compositor->SetBuffer( m_tempBuffer );    break;
    }

    m_currentTarget = aTarget;
}


RENDER_TARGET CAIRO_GAL::GetTarget() const
{
    return m_currentTarget;
}


void CAIRO_GAL::ClearTarget( RENDER_TARGET aTarget )
{
    // Save the current state
    unsigned int currentBuffer = m_compositor->GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED: m_compositor->SetBuffer( m_mainBuffer );    break;
    case TARGET_OVERLAY:   m_compositor->SetBuffer( m_overlayBuffer ); break;
    case TARGET_TEMP:      m_compositor->SetBuffer( m_tempBuffer );    break;
    }

    m_compositor->ClearBuffer( COLOR4D::BLACK );

    // Restore the previous state
    m_compositor->SetBuffer( currentBuffer );
}


void CAIRO_GAL::initSurface()
{
    if( m_isInitialized )
        return;

    m_surface = cairo_image_surface_create_for_data( m_bitmapBuffer, GAL_FORMAT, m_wxBufferWidth,
                                                     m_screenSize.y, m_stride );

    m_context = cairo_create( m_surface );

#ifdef DEBUG
    cairo_status_t status = cairo_status( m_context );
    wxASSERT_MSG( status == CAIRO_STATUS_SUCCESS, wxT( "Cairo context creation error" ) );
#endif /* DEBUG */

    m_currentContext = m_context;

    m_isInitialized = true;
}


void CAIRO_GAL::deinitSurface()
{
    if( !m_isInitialized )
        return;

    cairo_destroy( m_context );
    m_context = nullptr;
    cairo_surface_destroy( m_surface );
    m_surface = nullptr;

    m_isInitialized = false;
}


void CAIRO_GAL::allocateBitmaps()
{
    m_wxBufferWidth = m_screenSize.x;

    // Create buffer, use the system independent Cairo context backend
    m_stride = cairo_format_stride_for_width( GAL_FORMAT, m_wxBufferWidth );
    m_bufferSize = m_stride * m_screenSize.y;

    wxASSERT( m_bitmapBuffer == nullptr );
    m_bitmapBuffer = new unsigned char[m_bufferSize];

    wxASSERT( m_wxOutput == nullptr );
    m_wxOutput = new unsigned char[m_wxBufferWidth * 3 * m_screenSize.y];
}


void CAIRO_GAL::deleteBitmaps()
{
    delete[] m_bitmapBuffer;
    m_bitmapBuffer = nullptr;

    delete[] m_wxOutput;
    m_wxOutput = nullptr;
}


void CAIRO_GAL::setCompositor()
{
    // Recreate the compositor with the new Cairo context
    m_compositor.reset( new CAIRO_COMPOSITOR( &m_currentContext ) );
    m_compositor->Resize( m_screenSize.x, m_screenSize.y );
    m_compositor->SetAntialiasingMode( m_options.antialiasing_mode );

    // Prepare buffers
    m_mainBuffer = m_compositor->CreateBuffer();
    m_overlayBuffer = m_compositor->CreateBuffer();
    m_tempBuffer = m_compositor->CreateBuffer();

    m_validCompositor = true;
}


void CAIRO_GAL::onPaint( wxPaintEvent& aEvent )
{
    PostPaint( aEvent );
}


void CAIRO_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


void CAIRO_GAL::skipGestureEvent( wxGestureEvent& aEvent )
{
    // Post the gesture event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


bool CAIRO_GAL::updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions )
{
    bool refresh = false;

    if( m_validCompositor &&
        aOptions.antialiasing_mode != m_compositor->GetAntialiasingMode() )
    {
        m_compositor->SetAntialiasingMode( m_options.antialiasing_mode );
        m_validCompositor = false;
        deinitSurface();

        refresh = true;
    }

    if( super::updatedGalDisplayOptions( aOptions ) )
    {
        Refresh();
        refresh = true;
    }

    return refresh;
}


bool CAIRO_GAL::SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI )
{
    // Store the current cursor type and get the wx cursor for it
    if( !GAL::SetNativeCursorStyle( aCursor, aHiDPI ) )
        return false;

    m_currentwxCursor = CURSOR_STORE::GetCursor( m_currentNativeCursor, aHiDPI );

#if wxCHECK_VERSION( 3, 3, 0 )
    wxWindow::SetCursorBundle( m_currentwxCursor );
#else
    wxWindow::SetCursor( m_currentwxCursor );
#endif

    return true;
}


void CAIRO_GAL::onSetNativeCursor( wxSetCursorEvent& aEvent )
{
#if wxCHECK_VERSION( 3, 3, 0 )
    aEvent.SetCursor( m_currentwxCursor.GetCursorFor( this ) );
#else
    aEvent.SetCursor( m_currentwxCursor );
#endif
}


void CAIRO_GAL_BASE::DrawGrid()
{
    SetTarget( TARGET_NONCACHED );

    // Draw the grid
    // For the drawing the start points, end points and increments have
    // to be calculated in world coordinates
    VECTOR2D worldStartPoint = m_screenWorldMatrix * VECTOR2D( 0.0, 0.0 );
    VECTOR2D worldEndPoint = m_screenWorldMatrix * VECTOR2D( m_screenSize );

    // Compute the line marker or point radius of the grid
    // Note: generic grids can't handle sub-pixel lines without
    // either losing fine/course distinction or having some dots
    // fail to render
    float marker = std::fmax( 1.0f, m_gridLineWidth ) / m_worldScale;
    float doubleMarker = 2.0f * marker;

    // Draw axes if desired
    if( m_axesEnabled )
    {
        SetLineWidth( marker );
        drawAxes( worldStartPoint, worldEndPoint );
    }

    if( !m_gridVisibility || m_gridSize.x == 0 || m_gridSize.y == 0 )
        return;

    VECTOR2D gridScreenSize( m_gridSize );

    double gridThreshold = KiROUND( computeMinGridSpacing() / m_worldScale );

    if( m_gridStyle == GRID_STYLE::SMALL_CROSS )
        gridThreshold *= 2.0;

    // If we cannot display the grid density, scale down by a tick size and
    // try again.  Eventually, we get some representation of the grid
    while( std::min( gridScreenSize.x, gridScreenSize.y ) <= gridThreshold )
    {
        gridScreenSize = gridScreenSize * static_cast<double>( m_gridTick );
    }

    // Compute grid starting and ending indexes to draw grid points on the
    // visible screen area
    // Note: later any point coordinate will be offsetted by m_gridOrigin
    int gridStartX = KiROUND( ( worldStartPoint.x - m_gridOrigin.x ) / gridScreenSize.x );
    int gridEndX = KiROUND( ( worldEndPoint.x - m_gridOrigin.x ) / gridScreenSize.x );
    int gridStartY = KiROUND( ( worldStartPoint.y - m_gridOrigin.y ) / gridScreenSize.y );
    int gridEndY = KiROUND( ( worldEndPoint.y - m_gridOrigin.y ) / gridScreenSize.y );

    // Ensure start coordinate < end coordinate
    normalize( gridStartX, gridEndX );
    normalize( gridStartY, gridEndY );

    // Ensure the grid fills the screen
    --gridStartX;
    ++gridEndX;
    --gridStartY;
    ++gridEndY;

    // Draw the grid behind all other layers
    SetLayerDepth( m_depthRange.y * 0.75 );

    if( m_gridStyle == GRID_STYLE::LINES )
    {
        // Now draw the grid, every coarse grid line gets the double width

        // Vertical lines
        for( int j = gridStartY; j <= gridEndY; j++ )
        {
            const double y = j * gridScreenSize.y + m_gridOrigin.y;

            if( m_axesEnabled && y == 0.0 )
                continue;

            SetLineWidth( ( j % m_gridTick ) ? marker : doubleMarker );
            drawGridLine( VECTOR2D( gridStartX * gridScreenSize.x + m_gridOrigin.x, y ),
                          VECTOR2D( gridEndX * gridScreenSize.x + m_gridOrigin.x, y ) );
        }

        // Horizontal lines
        for( int i = gridStartX; i <= gridEndX; i++ )
        {
            const double x = i * gridScreenSize.x + m_gridOrigin.x;

            if( m_axesEnabled && x == 0.0 )
                continue;

            SetLineWidth( ( i % m_gridTick ) ? marker : doubleMarker );
            drawGridLine( VECTOR2D( x, gridStartY * gridScreenSize.y + m_gridOrigin.y ),
                          VECTOR2D( x, gridEndY * gridScreenSize.y + m_gridOrigin.y ) );
        }
    }
    else // Dots or Crosses grid
    {
        m_lineWidthIsOdd = true;
        m_isStrokeEnabled = true;

        for( int j = gridStartY; j <= gridEndY; j++ )
        {
            bool tickY = ( j % m_gridTick == 0 );

            for( int i = gridStartX; i <= gridEndX; i++ )
            {
                bool     tickX = ( i % m_gridTick == 0 );
                VECTOR2D pos{ i * gridScreenSize.x + m_gridOrigin.x,
                              j * gridScreenSize.y + m_gridOrigin.y };

                if( m_gridStyle == GRID_STYLE::SMALL_CROSS )
                {
                    SetLineWidth( ( tickX && tickY ) ? doubleMarker : marker );
                    drawGridCross( pos );
                }
                else if( m_gridStyle == GRID_STYLE::DOTS )
                {
                    double doubleGridLineWidth = m_gridLineWidth * 2.0f;
                    drawGridPoint( pos, ( tickX ) ? doubleGridLineWidth : m_gridLineWidth,
                                   ( tickY ) ? doubleGridLineWidth : m_gridLineWidth );
                }
            }
        }
    }
}


void CAIRO_GAL_BASE::DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth, int aTotal )
{
    if( aGlyph.IsStroke() )
    {
        const KIFONT::STROKE_GLYPH& glyph = static_cast<const KIFONT::STROKE_GLYPH&>( aGlyph );

        for( const std::vector<VECTOR2D>& pointList : glyph )
            drawPoly( pointList );
    }
    else if( aGlyph.IsOutline() )
    {
        const KIFONT::OUTLINE_GLYPH& glyph = static_cast<const KIFONT::OUTLINE_GLYPH&>( aGlyph );

        if( aNth == 0 )
        {
            cairo_close_path( m_currentContext );
            flushPath();

            cairo_new_path( m_currentContext );
            SetIsFill( true );
            SetIsStroke( false );
        }

        // eventually glyphs should not be drawn as polygons at all,
        // but as bitmaps with antialiasing, this is just a stopgap measure
        // of getting some form of outline font display

        glyph.Triangulate(
                [&]( const VECTOR2D& aVertex1, const VECTOR2D& aVertex2, const VECTOR2D& aVertex3 )
                {
                    syncLineWidth();

                    const VECTOR2D p0 = roundp( xform( aVertex1 ) );
                    const VECTOR2D p1 = roundp( xform( aVertex2 ) );
                    const VECTOR2D p2 = roundp( xform( aVertex3 ) );

                    cairo_move_to( m_currentContext, p0.x, p0.y );
                    cairo_line_to( m_currentContext, p1.x, p1.y );
                    cairo_line_to( m_currentContext, p2.x, p2.y );
                    cairo_close_path( m_currentContext );
                    cairo_set_fill_rule( m_currentContext, CAIRO_FILL_RULE_EVEN_ODD );
                    flushPath();
                    cairo_fill( m_currentContext );
                } );

        if( aNth == aTotal - 1 )
        {
            flushPath();
            SetIsFill( false );
            SetIsStroke( true );
            m_isElementAdded = true;
        }
    }
}
