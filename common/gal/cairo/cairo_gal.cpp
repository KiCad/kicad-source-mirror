/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012-2020 Kicad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
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
#include <math/util.h>      // for KiROUND
#include <bitmap_base.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <pixman.h>

using namespace KIGFX;



CAIRO_GAL_BASE::CAIRO_GAL_BASE( GAL_DISPLAY_OPTIONS& aDisplayOptions ) :
    GAL( aDisplayOptions )
{
    // Initialise grouping
    isGrouping          = false;
    isElementAdded      = false;
    groupCounter        = 0;
    currentGroup        = nullptr;

    lineWidth = 1.0;
    linePixelWidth = 1.0;
    lineWidthInPixels = 1.0;
    lineWidthIsOdd = true;

    // Initialise Cairo state
    cairo_matrix_init_identity( &cairoWorldScreenMatrix );
    currentContext      = nullptr;
    context             = nullptr;
    surface             = nullptr;

    // Grid color settings are different in Cairo and OpenGL
    SetGridColor( COLOR4D( 0.1, 0.1, 0.1, 0.8 ) );
    SetAxesColor( COLOR4D( BLUE ) );

    // Avoid unitialized variables:
    cairo_matrix_init_identity( &currentXform );
    cairo_matrix_init_identity( &currentWorld2Screen );
}


CAIRO_GAL_BASE::~CAIRO_GAL_BASE()
{
    ClearCache();

    if( surface )
        cairo_surface_destroy( surface );

    if( context )
        cairo_destroy( context );

    for( auto imageSurface : imageSurfaces )
        cairo_surface_destroy( imageSurface );
}


void CAIRO_GAL_BASE::beginDrawing()
{
    resetContext();
}


void CAIRO_GAL_BASE::endDrawing()
{
    // Force remaining objects to be drawn
    Flush();
}

void CAIRO_GAL_BASE::updateWorldScreenMatrix()
{
    cairo_matrix_multiply( &currentWorld2Screen, &currentXform, &cairoWorldScreenMatrix );
}


const VECTOR2D CAIRO_GAL_BASE::xform( double x, double y )
{
    VECTOR2D rv;

    rv.x = currentWorld2Screen.xx * x + currentWorld2Screen.xy * y + currentWorld2Screen.x0;
    rv.y = currentWorld2Screen.yx * x + currentWorld2Screen.yy * y + currentWorld2Screen.y0;
    return rv;
}


const VECTOR2D CAIRO_GAL_BASE::xform( const VECTOR2D& aP )
{
    return xform( aP.x, aP.y );
}


const double CAIRO_GAL_BASE::angle_xform( const double aAngle )
{
    // calculate rotation angle due to the rotation transform
    // and if flipped on X axis.
    double world_rotation = -std::atan2( currentWorld2Screen.xy, currentWorld2Screen.xx );

    // When flipped on X axis, the rotation angle is M_PI - initial angle:
    if( IsFlippedX() )
        world_rotation = M_PI - world_rotation;

    return std::fmod( aAngle + world_rotation, 2.0 * M_PI );
}


void CAIRO_GAL_BASE::arc_angles_xform_and_normalize( double& aStartAngle, double& aEndAngle )
{
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
    SWAP( startAngle, >, endAngle );

    // now rotate arc according to the rotation transform matrix
    // Remark:
    // We call angle_xform() to calculate angles according to the flip/rotation
    // transform and normatize between -2M_PI and +2M_PI.
    // Therefore, if aStartAngle = aEndAngle + 2*n*M_PI, the transform gives
    // aEndAngle = aStartAngle
    // So, if this is the case, force the aEndAngle value to draw a circle.
    aStartAngle = angle_xform( startAngle );

    if( std::abs( aEndAngle - aStartAngle ) >= 2*M_PI )     // arc is a full circle
        aEndAngle = aStartAngle + 2*M_PI;
    else
        aEndAngle = angle_xform( endAngle );
}


const double CAIRO_GAL_BASE::xform( double x )
{
    double dx = currentWorld2Screen.xx * x;
    double dy = currentWorld2Screen.yx * x;
    return sqrt( dx * dx + dy * dy );
}


static double roundp( double x )
{
    return floor( x + 0.5 ) + 0.5;
}


const VECTOR2D CAIRO_GAL_BASE::roundp( const VECTOR2D& v )
{
    if( lineWidthIsOdd && isStrokeEnabled )
        return VECTOR2D( ::roundp( v.x ), ::roundp( v.y ) );
    else
        return VECTOR2D( floor( v.x + 0.5 ), floor( v.y + 0.5 ) );
}


void CAIRO_GAL_BASE::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    syncLineWidth();

    auto p0 = roundp( xform( aStartPoint ) );
    auto p1 = roundp( xform( aEndPoint ) );

    cairo_move_to( currentContext, p0.x, p0.y );
    cairo_line_to( currentContext, p1.x, p1.y );
    flushPath();
    isElementAdded = true;
}


void CAIRO_GAL_BASE::syncLineWidth( bool aForceWidth, double aWidth )
{
    auto w =  floor( xform( aForceWidth ? aWidth : lineWidth ) + 0.5 );

    if (w <= 1.0)
    {
        w = 1.0;
        cairo_set_line_join( currentContext, CAIRO_LINE_JOIN_MITER );
        cairo_set_line_cap( currentContext, CAIRO_LINE_CAP_BUTT );
        cairo_set_line_width( currentContext, 1.0 );
        lineWidthIsOdd = true;
    }
    else
    {
        cairo_set_line_join( currentContext, CAIRO_LINE_JOIN_ROUND );
        cairo_set_line_cap( currentContext, CAIRO_LINE_CAP_ROUND );
        cairo_set_line_width( currentContext, w );
        lineWidthIsOdd = ((int)w % 2) == 1;
    }

    lineWidthInPixels = w;
}


void CAIRO_GAL_BASE::DrawSegment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint,
                             double aWidth )
{
    if( isFillEnabled )
    {
        syncLineWidth( true, aWidth );

        auto p0 = roundp( xform( aStartPoint ) );
        auto p1 = roundp( xform( aEndPoint ) );

        cairo_move_to( currentContext, p0.x, p0.y );
        cairo_line_to( currentContext, p1.x, p1.y );
        cairo_set_source_rgba( currentContext, fillColor.r, fillColor.g, fillColor.b, fillColor.a );
        cairo_stroke( currentContext );
    }
    else
    {
        aWidth /= 2.0;
        SetLineWidth( 1.0 );
        syncLineWidth();

        // Outline mode for tracks
        VECTOR2D startEndVector = aEndPoint - aStartPoint;
        double   lineAngle      = atan2( startEndVector.y, startEndVector.x );

        double sa = sin( lineAngle + M_PI / 2.0 );
        double ca = cos( lineAngle + M_PI / 2.0 );

        auto pa0 = xform ( aStartPoint + VECTOR2D(aWidth * ca, aWidth * sa ) );
        auto pa1 = xform ( aStartPoint - VECTOR2D(aWidth * ca, aWidth * sa ) );
        auto pb0 = xform ( aEndPoint + VECTOR2D(aWidth * ca, aWidth * sa ) );
        auto pb1 = xform ( aEndPoint - VECTOR2D(aWidth * ca, aWidth * sa ) );
        auto pa = xform( aStartPoint );
        auto pb = xform( aEndPoint );
        auto rb = (pa0 - pa).EuclideanNorm();

        cairo_set_source_rgba( currentContext, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

        cairo_move_to( currentContext, pa0.x, pa0.y );
        cairo_line_to( currentContext, pb0.x, pb0.y );

        cairo_move_to( currentContext, pa1.x, pa1.y );
        cairo_line_to( currentContext, pb1.x, pb1.y );

        cairo_arc( currentContext, pb.x, pb.y, rb, lineAngle - M_PI / 2.0, lineAngle + M_PI / 2.0 );
        cairo_arc( currentContext, pa.x, pa.y, rb, lineAngle + M_PI / 2.0, lineAngle + 3.0 * M_PI / 2.0 );

        flushPath();
    }

    isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawCircle( const VECTOR2D& aCenterPoint, double aRadius )
{
    syncLineWidth();

    auto c = roundp( xform( aCenterPoint ) );
    auto r = ::roundp( xform( aRadius ) );

    cairo_set_line_width( currentContext, std::min( 2.0 * r, lineWidthInPixels ) );
    cairo_new_sub_path( currentContext );
    cairo_arc( currentContext, c.x, c.y, r, 0.0, 2 * M_PI );
    cairo_close_path( currentContext );
    flushPath();
    isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawArc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                         double aEndAngle )
{
    syncLineWidth();

    // calculate start and end arc angles according to the rotation transform matrix
    // and normalize:
    arc_angles_xform_and_normalize( aStartAngle, aEndAngle );

    double r = xform( aRadius );

    // N.B. This is backwards. We set this because we want to adjust the center
    // point that changes both endpoints.  In the worst case, this is twice as far.
    // We cannot adjust radius or center based on the other because this causes the
    // whole arc to change position/size
    lineWidthIsOdd = !( static_cast<int>( aRadius ) % 2 );

    auto mid = roundp( xform( aCenterPoint ) );

    cairo_set_line_width( currentContext, lineWidthInPixels );
    cairo_new_sub_path( currentContext );

    if( isFillEnabled )
        cairo_move_to( currentContext, mid.x, mid.y );

    cairo_arc( currentContext, mid.x, mid.y, r, aStartAngle, aEndAngle );

    if( isFillEnabled )
        cairo_close_path( currentContext );

    flushPath();

    isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawArcSegment( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle,
                                double aEndAngle, double aWidth )
{
    if( isFillEnabled )
    {
        lineWidth = aWidth;
        isStrokeEnabled = true;
        isFillEnabled = false;
        DrawArc( aCenterPoint, aRadius, aStartAngle, aEndAngle );
        isFillEnabled = true;
        isStrokeEnabled = false;
        return;
    }

    syncLineWidth();

    // calculate start and end arc angles according to the rotation transform matrix
    // and normalize:
    double startAngleS = aStartAngle;
    double endAngleS = aEndAngle;
    arc_angles_xform_and_normalize( startAngleS, endAngleS );

    double r = xform( aRadius );

    // N.B. This is backwards. We set this because we want to adjust the center
    // point that changes both endpoints.  In the worst case, this is twice as far.
    // We cannot adjust radius or center based on the other because this causes the
    // whole arc to change position/size
    lineWidthIsOdd = !( static_cast<int>( aRadius ) % 2 );

    auto mid = roundp( xform( aCenterPoint ) );
    double width = xform( aWidth / 2.0 );
    auto startPointS = VECTOR2D( r, 0.0 ).Rotate( startAngleS );
    auto endPointS = VECTOR2D( r, 0.0 ).Rotate( endAngleS );

    cairo_save( currentContext );

    cairo_set_source_rgba( currentContext, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

    cairo_translate( currentContext, mid.x, mid.y );

    cairo_new_sub_path( currentContext );
    cairo_arc( currentContext, 0, 0, r - width, startAngleS, endAngleS );

    cairo_new_sub_path( currentContext );
    cairo_arc( currentContext, 0, 0, r + width, startAngleS, endAngleS );

    cairo_new_sub_path( currentContext );
    cairo_arc_negative( currentContext, startPointS.x, startPointS.y, width, startAngleS, startAngleS + M_PI );

    cairo_new_sub_path( currentContext );
    cairo_arc( currentContext, endPointS.x, endPointS.y, width, endAngleS, endAngleS + M_PI );

    cairo_restore( currentContext );
    flushPath();

    isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawRectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    // Calculate the diagonal points
    syncLineWidth();

    const auto p0 = roundp( xform( aStartPoint ) );
    const auto p1 = roundp( xform( VECTOR2D( aEndPoint.x, aStartPoint.y ) ) );
    const auto p2 = roundp( xform( aEndPoint ) );
    const auto p3 = roundp( xform( VECTOR2D( aStartPoint.x, aEndPoint.y ) ) );

    // The path is composed from 4 segments
    cairo_move_to( currentContext, p0.x, p0.y );
    cairo_line_to( currentContext, p1.x, p1.y );
    cairo_line_to( currentContext, p2.x, p2.y );
    cairo_line_to( currentContext, p3.x, p3.y );
    cairo_close_path( currentContext );
    flushPath();

    isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawPolygon( const SHAPE_POLY_SET& aPolySet )
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

    const auto sp = roundp( xform( aStartPoint ) );
    const auto cpa = roundp( xform( aControlPointA ) );
    const auto cpb = roundp( xform( aControlPointB ) );
    const auto ep = roundp( xform( aEndPoint ) );

    cairo_move_to( currentContext, sp.x, sp.y );
    cairo_curve_to( currentContext, cpa.x, cpa.y, cpb.x, cpb.y, ep.x, ep.y );
    cairo_line_to( currentContext, ep.x, ep.y );

    flushPath();
    isElementAdded = true;
}


void CAIRO_GAL_BASE::DrawBitmap( const BITMAP_BASE& aBitmap )
{
    cairo_save( currentContext );

    // We have to calculate the pixel size in users units to draw the image.
    // worldUnitLength is a factor used for converting IU to inches
    double scale = 1.0 / ( aBitmap.GetPPI() * worldUnitLength );

    // The position of the bitmap is the bitmap center.
    // move the draw origin to the top left bitmap corner:
    int w = aBitmap.GetSizePixels().x;
    int h = aBitmap.GetSizePixels().y;

    cairo_set_matrix( currentContext, &currentWorld2Screen );
    cairo_scale( currentContext, scale, scale );
    cairo_translate( currentContext, -w / 2.0, -h / 2.0 );

    cairo_new_path( currentContext );
    cairo_surface_t* image = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, w, h );
    cairo_surface_flush( image );

    unsigned char* pix_buffer = cairo_image_surface_get_data( image );
    // The pixel buffer of the initial bitmap:
    const wxImage& bm_pix_buffer = *aBitmap.GetImageData();

    uint32_t mask_color = ( bm_pix_buffer.GetMaskRed() << 16 ) +
            ( bm_pix_buffer.GetMaskGreen() << 8 ) +
            ( bm_pix_buffer.GetMaskBlue() );

    // Copy the source bitmap to the cairo bitmap buffer.
    // In cairo bitmap buffer, a ARGB32 bitmap is an ARGB pixel packed into a uint_32
    // 24 low bits only are used for color, top 8 are transparency.
    for( int row = 0; row < h; row++ )
    {
        for( int col = 0; col < w; col++ )
        {
            // Build the RGB24 pixel:
            uint32_t pixel = bm_pix_buffer.GetRed( col, row ) << 16;
            pixel += bm_pix_buffer.GetGreen( col, row ) << 8;
            pixel += bm_pix_buffer.GetBlue( col, row );

            if( bm_pix_buffer.HasAlpha() )
                pixel += bm_pix_buffer.GetAlpha( col, row ) << 24;
            else if( bm_pix_buffer.HasMask() && pixel == mask_color )
                pixel += ( wxALPHA_TRANSPARENT << 24 );
            else
                pixel += ( wxALPHA_OPAQUE << 24 );

            // Write the pixel to the cairo image buffer:
            uint32_t* pix_ptr = (uint32_t*) pix_buffer;
            *pix_ptr = pixel;
            pix_buffer += 4;
        }
    }

    cairo_surface_mark_dirty( image );
    cairo_set_source_surface( currentContext, image, 0, 0 );
    cairo_paint( currentContext );

    // store the image handle so it can be destroyed later
    imageSurfaces.push_back( image );

    isElementAdded = true;

    cairo_restore( currentContext );
}


void CAIRO_GAL_BASE::ResizeScreen( int aWidth, int aHeight )
{
    screenSize = VECTOR2I( aWidth, aHeight );
}


void CAIRO_GAL_BASE::Flush()
{
    storePath();
}


void CAIRO_GAL_BASE::ClearScreen()
{
    cairo_set_source_rgb( currentContext, m_clearColor.r, m_clearColor.g, m_clearColor.b );
    cairo_rectangle( currentContext, 0.0, 0.0, screenSize.x, screenSize.y );
    cairo_fill( currentContext );
}


void CAIRO_GAL_BASE::SetIsFill( bool aIsFillEnabled )
{
    storePath();
    isFillEnabled = aIsFillEnabled;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_FILL;
        groupElement.argument.boolArg = aIsFillEnabled;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetIsStroke( bool aIsStrokeEnabled )
{
    storePath();
    isStrokeEnabled = aIsStrokeEnabled;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_STROKE;
        groupElement.argument.boolArg = aIsStrokeEnabled;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetStrokeColor( const COLOR4D& aColor )
{
    storePath();
    strokeColor = aColor;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_STROKECOLOR;
        groupElement.argument.dblArg[0] = strokeColor.r;
        groupElement.argument.dblArg[1] = strokeColor.g;
        groupElement.argument.dblArg[2] = strokeColor.b;
        groupElement.argument.dblArg[3] = strokeColor.a;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetFillColor( const COLOR4D& aColor )
{
    storePath();
    fillColor = aColor;

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_FILLCOLOR;
        groupElement.argument.dblArg[0] = fillColor.r;
        groupElement.argument.dblArg[1] = fillColor.g;
        groupElement.argument.dblArg[2] = fillColor.b;
        groupElement.argument.dblArg[3] = fillColor.a;
        currentGroup->push_back( groupElement );
    }
}


void CAIRO_GAL_BASE::SetLineWidth( float aLineWidth )
{
    storePath();
    GAL::SetLineWidth( aLineWidth );

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SET_LINE_WIDTH;
        groupElement.argument.dblArg[0] = aLineWidth;
        currentGroup->push_back( groupElement );
    }
    else
    {
        lineWidth = aLineWidth;
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

    cairo_matrix_init( &cairoTransformation,
                       aTransformation.m_data[0][0],
                       aTransformation.m_data[1][0],
                       aTransformation.m_data[0][1],
                       aTransformation.m_data[1][1],
                       aTransformation.m_data[0][2],
                       aTransformation.m_data[1][2] );

    cairo_matrix_multiply( &newXform, &currentXform, &cairoTransformation );
    currentXform = newXform;
    updateWorldScreenMatrix();
}


void CAIRO_GAL_BASE::Rotate( double aAngle )
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_ROTATE;
        groupElement.argument.dblArg[0] = aAngle;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_matrix_rotate( &currentXform, aAngle );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Translate( const VECTOR2D& aTranslation )
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_TRANSLATE;
        groupElement.argument.dblArg[0] = aTranslation.x;
        groupElement.argument.dblArg[1] = aTranslation.y;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_matrix_translate ( &currentXform, aTranslation.x, aTranslation.y );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Scale( const VECTOR2D& aScale )
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SCALE;
        groupElement.argument.dblArg[0] = aScale.x;
        groupElement.argument.dblArg[1] = aScale.y;
        currentGroup->push_back( groupElement );
    }
    else
    {
        cairo_matrix_scale( &currentXform, aScale.x, aScale.y );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Save()
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_SAVE;
        currentGroup->push_back( groupElement );
    }
    else
    {
        xformStack.push_back( currentXform );
        updateWorldScreenMatrix();
    }
}


void CAIRO_GAL_BASE::Restore()
{
    storePath();

    if( isGrouping )
    {
        GROUP_ELEMENT groupElement;
        groupElement.command = CMD_RESTORE;
        currentGroup->push_back( groupElement );
    }
    else
    {
        if( !xformStack.empty() )
        {
            currentXform = xformStack.back();
            xformStack.pop_back();
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
    int groupNumber = getNewGroupNumber();
    groups.insert( std::make_pair( groupNumber, group ) );
    currentGroup = &groups[groupNumber];
    isGrouping   = true;

    return groupNumber;
}


void CAIRO_GAL_BASE::EndGroup()
{
    storePath();
    isGrouping = false;
}


void CAIRO_GAL_BASE::DrawGroup( int aGroupNumber )
{
    // This method implements a small Virtual Machine - all stored commands
    // are executed; nested calling is also possible

    storePath();

    for( GROUP::iterator it = groups[aGroupNumber].begin();
         it != groups[aGroupNumber].end(); ++it )
    {
        switch( it->command )
        {
        case CMD_SET_FILL:
            isFillEnabled = it->argument.boolArg;
            break;

        case CMD_SET_STROKE:
            isStrokeEnabled = it->argument.boolArg;
            break;

        case CMD_SET_FILLCOLOR:
            fillColor = COLOR4D( it->argument.dblArg[0], it->argument.dblArg[1], it->argument.dblArg[2],
                                 it->argument.dblArg[3] );
            break;

        case CMD_SET_STROKECOLOR:
            strokeColor = COLOR4D( it->argument.dblArg[0], it->argument.dblArg[1], it->argument.dblArg[2],
                                   it->argument.dblArg[3] );
            break;

        case CMD_SET_LINE_WIDTH:
            {
                // Make lines appear at least 1 pixel wide, no matter of zoom
                double x = 1.0, y = 1.0;
                cairo_device_to_user_distance( currentContext, &x, &y );
                double minWidth = std::min( fabs( x ), fabs( y ) );
                cairo_set_line_width( currentContext, std::max( it->argument.dblArg[0], minWidth ) );
            }
            break;


        case CMD_STROKE_PATH:
            cairo_set_source_rgba( currentContext, strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
            cairo_append_path( currentContext, it->cairoPath );
            cairo_stroke( currentContext );
            break;

        case CMD_FILL_PATH:
            cairo_set_source_rgba( currentContext, fillColor.r, fillColor.g, fillColor.b, strokeColor.a );
            cairo_append_path( currentContext, it->cairoPath );
            cairo_fill( currentContext );
            break;

            /*
        case CMD_TRANSFORM:
            cairo_matrix_t matrix;
            cairo_matrix_init( &matrix, it->argument.dblArg[0], it->argument.dblArg[1], it->argument.dblArg[2],
                               it->argument.dblArg[3], it->argument.dblArg[4], it->argument.dblArg[5] );
            cairo_transform( currentContext, &matrix );
            break;
            */

        case CMD_ROTATE:
            cairo_rotate( currentContext, it->argument.dblArg[0] );
            break;

        case CMD_TRANSLATE:
            cairo_translate( currentContext, it->argument.dblArg[0], it->argument.dblArg[1] );
            break;

        case CMD_SCALE:
            cairo_scale( currentContext, it->argument.dblArg[0], it->argument.dblArg[1] );
            break;

        case CMD_SAVE:
            cairo_save( currentContext );
            break;

        case CMD_RESTORE:
            cairo_restore( currentContext );
            break;

        case CMD_CALL_GROUP:
            DrawGroup( it->argument.intArg );
            break;
        }
    }
}


void CAIRO_GAL_BASE::ChangeGroupColor( int aGroupNumber, const COLOR4D& aNewColor )
{
    storePath();

    for( GROUP::iterator it = groups[aGroupNumber].begin();
         it != groups[aGroupNumber].end(); ++it )
    {
        if( it->command == CMD_SET_FILLCOLOR || it->command == CMD_SET_STROKECOLOR )
        {
            it->argument.dblArg[0] = aNewColor.r;
            it->argument.dblArg[1] = aNewColor.g;
            it->argument.dblArg[2] = aNewColor.b;
            it->argument.dblArg[3] = aNewColor.a;
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

    for( it = groups[aGroupNumber].begin(), end = groups[aGroupNumber].end(); it != end; ++it )
    {
        if( it->command == CMD_FILL_PATH || it->command == CMD_STROKE_PATH )
        {
            cairo_path_destroy( it->cairoPath );
        }
    }

    // Delete the group
    groups.erase( aGroupNumber );
}


void CAIRO_GAL_BASE::ClearCache()
{
    for( auto it = groups.begin(); it != groups.end(); )
        DeleteGroup( ( it++ )->first );
}


void CAIRO_GAL_BASE::SetNegativeDrawMode( bool aSetting )
{
    cairo_set_operator( currentContext, aSetting ? CAIRO_OPERATOR_CLEAR : CAIRO_OPERATOR_OVER );
}


void CAIRO_GAL_BASE::DrawCursor( const VECTOR2D& aCursorPosition )
{
    cursorPosition = aCursorPosition;
}


void CAIRO_GAL_BASE::EnableDepthTest( bool aEnabled )
{
}


void CAIRO_GAL_BASE::resetContext()
{
    for( auto imageSurface : imageSurfaces )
        cairo_surface_destroy( imageSurface );

    imageSurfaces.clear();

    ClearScreen();

    // Compute the world <-> screen transformations
    ComputeWorldScreenMatrix();

    cairo_matrix_init( &cairoWorldScreenMatrix, worldScreenMatrix.m_data[0][0],
                       worldScreenMatrix.m_data[1][0], worldScreenMatrix.m_data[0][1],
                       worldScreenMatrix.m_data[1][1], worldScreenMatrix.m_data[0][2],
                       worldScreenMatrix.m_data[1][2] );

    // we work in screen-space coordinates and do the transforms outside.
    cairo_identity_matrix( context );

    cairo_matrix_init_identity( &currentXform );

    // Start drawing with a new path
    cairo_new_path( context );
    isElementAdded = true;

    updateWorldScreenMatrix();

    lineWidth = 0;
}


void CAIRO_GAL_BASE::drawAxes( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    syncLineWidth();

    auto p0 = roundp( xform( aStartPoint ) );
    auto p1 = roundp( xform( aEndPoint ) );
    auto org = roundp( xform( VECTOR2D( 0.0, 0.0 ) ) );     // Axis origin = 0,0 coord

    cairo_set_source_rgba( currentContext, axesColor.r, axesColor.g, axesColor.b, axesColor.a );
    cairo_move_to( currentContext, p0.x, org.y);
    cairo_line_to( currentContext, p1.x, org.y );
    cairo_move_to( currentContext, org.x, p0.y );
    cairo_line_to( currentContext, org.x, p1.y );
    cairo_stroke( currentContext );
}


void CAIRO_GAL_BASE::drawGridLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    syncLineWidth();
    auto p0 = roundp( xform( aStartPoint ) );
    auto p1 = roundp( xform( aEndPoint ) );

    cairo_set_source_rgba( currentContext, gridColor.r, gridColor.g, gridColor.b, gridColor.a );
    cairo_move_to( currentContext, p0.x, p0.y );
    cairo_line_to( currentContext, p1.x, p1.y );
    cairo_stroke( currentContext );
}


void CAIRO_GAL_BASE::drawGridCross( const VECTOR2D& aPoint )
{
    syncLineWidth();
    VECTOR2D offset( 0, 0 );
    double size = 2.0 * lineWidthInPixels + 0.5;

    VECTOR2D p0 = roundp( xform( aPoint ) ) - VECTOR2D( size, 0 ) + offset;
    VECTOR2D p1 = roundp( xform( aPoint ) ) + VECTOR2D( size, 0 ) + offset;
    VECTOR2D p2 = roundp( xform( aPoint ) ) - VECTOR2D( 0, size ) + offset;
    VECTOR2D p3 = roundp( xform( aPoint ) ) + VECTOR2D( 0, size ) + offset;

    cairo_set_source_rgba( currentContext, gridColor.r, gridColor.g, gridColor.b, gridColor.a );
    cairo_move_to( currentContext, p0.x, p0.y );
    cairo_line_to( currentContext, p1.x, p1.y );
    cairo_move_to( currentContext, p2.x, p2.y );
    cairo_line_to( currentContext, p3.x, p3.y );
    cairo_stroke( currentContext );
}


void CAIRO_GAL_BASE::drawGridPoint( const VECTOR2D& aPoint, double aWidth, double aHeight )
{
    VECTOR2D p = roundp( xform( aPoint ) );

    double sw = std::max( 1.0, aWidth );
    double sh = std::max( 1.0, aHeight );

    cairo_set_source_rgba( currentContext, gridColor.r, gridColor.g, gridColor.b, gridColor.a );
    cairo_rectangle( currentContext, p.x - std::floor( sw / 2 ) - 0.5,
            p.y - std::floor( sh / 2 ) - 0.5, sw, sh );

    cairo_fill( currentContext );
}

void CAIRO_GAL_BASE::flushPath()
{
   if( isFillEnabled )
   {
       cairo_set_source_rgba( currentContext,
               fillColor.r, fillColor.g, fillColor.b, fillColor.a );

       if( isStrokeEnabled )
           cairo_fill_preserve( currentContext );
       else
           cairo_fill( currentContext );
   }

   if( isStrokeEnabled )
   {
       cairo_set_source_rgba( currentContext,
               strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );
       cairo_stroke( currentContext );
   }
}


void CAIRO_GAL_BASE::storePath()
{
    if( isElementAdded )
    {
        isElementAdded = false;

        if( !isGrouping )
        {
            if( isFillEnabled )
            {
                cairo_set_source_rgba( currentContext, fillColor.r, fillColor.g, fillColor.b, fillColor.a );
                cairo_fill_preserve( currentContext );
            }

            if( isStrokeEnabled )
            {
                cairo_set_source_rgba( currentContext, strokeColor.r, strokeColor.g,
                                      strokeColor.b, strokeColor.a );
                cairo_stroke_preserve( currentContext );
            }
        }
        else
        {
            // Copy the actual path, append it to the global path list
            // then check, if the path needs to be stroked/filled and
            // add this command to the group list;
            if( isStrokeEnabled )
            {
                GROUP_ELEMENT groupElement;
                groupElement.cairoPath = cairo_copy_path( currentContext );
                groupElement.command   = CMD_STROKE_PATH;
                currentGroup->push_back( groupElement );
            }

            if( isFillEnabled )
            {
                GROUP_ELEMENT groupElement;
                groupElement.cairoPath = cairo_copy_path( currentContext );
                groupElement.command   = CMD_FILL_PATH;
                currentGroup->push_back( groupElement );
            }
        }

        cairo_new_path( currentContext );
    }
}


void CAIRO_GAL_BASE::blitCursor( wxMemoryDC& clientDC )
{
    if( !IsCursorEnabled() )
        return;

    auto p = ToScreen( cursorPosition );

    const auto cColor = getCursorColor();
    const int cursorSize = fullscreenCursor ? 8000 : 80;

    wxColour color( cColor.r * cColor.a * 255, cColor.g * cColor.a * 255,
                    cColor.b * cColor.a * 255, 255 );
    clientDC.SetPen( wxPen( color ) );
    clientDC.DrawLine( p.x - cursorSize / 2, p.y, p.x + cursorSize / 2, p.y );
    clientDC.DrawLine( p.x, p.y - cursorSize / 2, p.x, p.y + cursorSize / 2 );
}


void CAIRO_GAL_BASE::drawPoly( const std::deque<VECTOR2D>& aPointList )
{
    wxCHECK( aPointList.size() > 1, /* void */ );

    // Iterate over the point list and draw the segments
    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();

    syncLineWidth();

    const auto p = roundp( xform( it->x, it->y ) );

    cairo_move_to( currentContext, p.x, p.y );

    for( ++it; it != aPointList.end(); ++it )
    {
        const auto p2 = roundp( xform( it->x, it->y ) );

        cairo_line_to( currentContext, p2.x, p2.y );
    }

    flushPath();
    isElementAdded = true;
}


void CAIRO_GAL_BASE::drawPoly( const VECTOR2D aPointList[], int aListSize )
{
    wxCHECK( aListSize > 1, /* void */ );

    // Iterate over the point list and draw the segments
    const VECTOR2D* ptr = aPointList;

    syncLineWidth();

    const auto p = roundp( xform( ptr->x, ptr->y ) );
    cairo_move_to( currentContext, p.x, p.y );

    for( int i = 1; i < aListSize; ++i )
    {
        ++ptr;
        const auto p2 = roundp( xform( ptr->x, ptr->y ) );
        cairo_line_to( currentContext, p2.x, p2.y );
    }

    flushPath();
    isElementAdded = true;
}


void CAIRO_GAL_BASE::drawPoly( const SHAPE_LINE_CHAIN& aLineChain )
{
    wxCHECK( aLineChain.PointCount() > 1, /* void */ );

    syncLineWidth();

    auto numPoints = aLineChain.PointCount();

    if( aLineChain.IsClosed() )
        numPoints += 1;

    const VECTOR2I start = aLineChain.CPoint( 0 );
    const auto p = roundp( xform( start.x, start.y ) );
    cairo_move_to( currentContext, p.x, p.y );

    for( int i = 1; i < numPoints; ++i )
    {
        const VECTOR2I& pw = aLineChain.CPoint( i );
        const auto ps = roundp( xform( pw.x, pw.y ) );
        cairo_line_to( currentContext, ps.x, ps.y );
    }

    flushPath();
    isElementAdded = true;
}


unsigned int CAIRO_GAL_BASE::getNewGroupNumber()
{
    wxASSERT_MSG( groups.size() < std::numeric_limits<unsigned int>::max(),
                  wxT( "There are no free slots to store a group" ) );

    while( groups.find( groupCounter ) != groups.end() )
        groupCounter++;

    return groupCounter++;
}


CAIRO_GAL::CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions,
        wxWindow* aParent, wxEvtHandler* aMouseListener,
        wxEvtHandler* aPaintListener, const wxString& aName ) :
    CAIRO_GAL_BASE( aDisplayOptions ),
    wxWindow( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxEXPAND, aName )
{
    // Initialise compositing state
    mainBuffer          = 0;
    overlayBuffer       = 0;
    validCompositor     = false;
    SetTarget( TARGET_NONCACHED );

    bitmapBuffer  = nullptr;
    wxOutput      = nullptr;

    parentWindow  = aParent;
    mouseListener = aMouseListener;
    paintListener = aPaintListener;

    // Connecting the event handlers
    Connect( wxEVT_PAINT,           wxPaintEventHandler( CAIRO_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION,          wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN,       wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP,         wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK,     wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN,     wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP,       wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK,   wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN,      wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP,        wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK,    wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL,      wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW,    wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
#endif

    SetSize( aParent->GetClientSize() );
    screenSize = VECTOR2I( aParent->GetClientSize() );

    // Allocate memory for pixel storage
    allocateBitmaps();

    isInitialized = false;
}


CAIRO_GAL::~CAIRO_GAL()
{
    deleteBitmaps();
}


void CAIRO_GAL::beginDrawing()
{
    initSurface();

    CAIRO_GAL_BASE::beginDrawing();

    if( !validCompositor )
        setCompositor();

    compositor->SetMainContext( context );
    compositor->SetBuffer( mainBuffer );
}


void CAIRO_GAL::endDrawing()
{
    CAIRO_GAL_BASE::endDrawing();

    // Merge buffers on the screen
    compositor->DrawBuffer( mainBuffer );
    compositor->DrawBuffer( overlayBuffer );

    // Now translate the raw context data from the format stored
    // by cairo into a format understood by wxImage.

    pixman_image_t* dstImg = pixman_image_create_bits(
            wxPlatformInfo::Get().GetEndianness() == wxENDIAN_LITTLE ? PIXMAN_b8g8r8 :
                                                                       PIXMAN_r8g8b8,
            screenSize.x, screenSize.y, (uint32_t*) wxOutput, wxBufferWidth * 3 );
    pixman_image_t* srcImg = pixman_image_create_bits( PIXMAN_a8r8g8b8, screenSize.x, screenSize.y,
            (uint32_t*) bitmapBuffer, wxBufferWidth * 4 );

    pixman_image_composite( PIXMAN_OP_SRC, srcImg, NULL, dstImg,
            0, 0, 0, 0, 0, 0, screenSize.x, screenSize.y );

    // Free allocated memory
    pixman_image_unref( srcImg );
    pixman_image_unref( dstImg );

    wxImage img( wxBufferWidth, screenSize.y, wxOutput, true );
    wxBitmap bmp( img );
    wxMemoryDC mdc( bmp );
    wxClientDC clientDC( this );

    // Now it is the time to blit the mouse cursor
    blitCursor( mdc );
    clientDC.Blit( 0, 0, screenSize.x, screenSize.y, &mdc, 0, 0, wxCOPY );

    deinitSurface();
}


void CAIRO_GAL::PostPaint( wxPaintEvent& aEvent )
{
    // posts an event to m_paint_listener to ask for redraw the canvas.
    if( paintListener )
    {
        wxPostEvent( paintListener, aEvent );
    }
}


void CAIRO_GAL::ResizeScreen( int aWidth, int aHeight )
{
    CAIRO_GAL_BASE::ResizeScreen( aWidth, aHeight );

    // Recreate the bitmaps
    deleteBitmaps();
    allocateBitmaps();

    if( validCompositor )
        compositor->Resize( aWidth, aHeight );

    validCompositor = false;

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
    if( !validCompositor )
        return;

    // Cairo grouping prevents display of overlapping items on the same layer in the lighter color
    if( isInitialized )
        storePath();

    switch( aTarget )
    {
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        compositor->SetBuffer( mainBuffer );
        break;

    case TARGET_OVERLAY:
        compositor->SetBuffer( overlayBuffer );
        break;
    }

    currentTarget = aTarget;
}


RENDER_TARGET CAIRO_GAL::GetTarget() const
{
    return currentTarget;
}


void CAIRO_GAL::ClearTarget( RENDER_TARGET aTarget )
{
    // Save the current state
    unsigned int currentBuffer = compositor->GetBuffer();

    switch( aTarget )
    {
    // Cached and noncached items are rendered to the same buffer
    default:
    case TARGET_CACHED:
    case TARGET_NONCACHED:
        compositor->SetBuffer( mainBuffer );
        break;

    case TARGET_OVERLAY:
        compositor->SetBuffer( overlayBuffer );
        break;
    }

    compositor->ClearBuffer( COLOR4D::BLACK );

    // Restore the previous state
    compositor->SetBuffer( currentBuffer );
}


void CAIRO_GAL::initSurface()
{
    if( isInitialized )
        return;

    surface = cairo_image_surface_create_for_data( bitmapBuffer, GAL_FORMAT,
                                                wxBufferWidth, screenSize.y, stride );

    context = cairo_create( surface );

#ifdef DEBUG
    cairo_status_t status = cairo_status( context );
    wxASSERT_MSG( status == CAIRO_STATUS_SUCCESS, wxT( "Cairo context creation error" ) );
#endif /* DEBUG */
    currentContext = context;

    isInitialized = true;
}


void CAIRO_GAL::deinitSurface()
{
    if( !isInitialized )
        return;

    cairo_destroy( context );
    context = nullptr;
    cairo_surface_destroy( surface );
    surface = nullptr;

    isInitialized = false;
}


void CAIRO_GAL::allocateBitmaps()
{
    wxBufferWidth = screenSize.x;
    while( ( ( wxBufferWidth * 3 ) % 4 ) != 0 ) wxBufferWidth++;

    // Create buffer, use the system independent Cairo context backend
    stride     = cairo_format_stride_for_width( GAL_FORMAT, wxBufferWidth );
    bufferSize = stride * screenSize.y;

    wxASSERT( bitmapBuffer == nullptr );
    bitmapBuffer = new unsigned char[bufferSize * 4];

    wxASSERT( wxOutput == nullptr );
    wxOutput = new unsigned char[wxBufferWidth * 3 * screenSize.y];
}


void CAIRO_GAL::deleteBitmaps()
{
    delete[] bitmapBuffer;
    bitmapBuffer = nullptr;

    delete[] wxOutput;
    wxOutput = nullptr;
}


void CAIRO_GAL::setCompositor()
{
    // Recreate the compositor with the new Cairo context
    compositor.reset( new CAIRO_COMPOSITOR( &currentContext ) );
    compositor->Resize( screenSize.x, screenSize.y );
    compositor->SetAntialiasingMode( options.cairo_antialiasing_mode );

    // Prepare buffers
    mainBuffer = compositor->CreateBuffer();
    overlayBuffer = compositor->CreateBuffer();

    validCompositor = true;
}


void CAIRO_GAL::onPaint( wxPaintEvent& aEvent )
{
    PostPaint( aEvent );
}


void CAIRO_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( mouseListener )
        wxPostEvent( mouseListener, aEvent );
}


bool CAIRO_GAL::updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions )
{
    bool refresh = false;

    if( validCompositor && aOptions.cairo_antialiasing_mode != compositor->GetAntialiasingMode() )
    {

        compositor->SetAntialiasingMode( options.cairo_antialiasing_mode );
        validCompositor = false;
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


void CAIRO_GAL_BASE::DrawGrid()
{
    SetTarget( TARGET_NONCACHED );

    // Draw the grid
    // For the drawing the start points, end points and increments have
    // to be calculated in world coordinates
    VECTOR2D worldStartPoint = screenWorldMatrix * VECTOR2D( 0.0, 0.0 );
    VECTOR2D worldEndPoint   = screenWorldMatrix * VECTOR2D( screenSize );

    // Compute the line marker or point radius of the grid
    // Note: generic grids can't handle sub-pixel lines without
    // either losing fine/course distinction or having some dots
    // fail to render
    float marker = std::fmax( 1.0f, gridLineWidth ) / worldScale;
    float doubleMarker = 2.0f * marker;

    // Draw axes if desired
    if( axesEnabled )
    {
        SetLineWidth( marker );
        drawAxes( worldStartPoint, worldEndPoint );
    }

    if( !gridVisibility || gridSize.x == 0 || gridSize.y == 0 )
        return;

    VECTOR2D gridScreenSize( gridSize );

    double gridThreshold = KiROUND( computeMinGridSpacing() / worldScale );

    if( gridStyle == GRID_STYLE::SMALL_CROSS )
        gridThreshold *= 2.0;

    // If we cannot display the grid density, scale down by a tick size and
    // try again.  Eventually, we get some representation of the grid
    while( std::min( gridScreenSize.x,  gridScreenSize.y ) <= gridThreshold )
    {
        gridScreenSize = gridScreenSize * static_cast<double>( gridTick );
    }

    // Compute grid starting and ending indexes to draw grid points on the
    // visible screen area
    // Note: later any point coordinate will be offsetted by gridOrigin
    int gridStartX = KiROUND( ( worldStartPoint.x - gridOrigin.x ) / gridScreenSize.x );
    int gridEndX   = KiROUND( ( worldEndPoint.x - gridOrigin.x ) / gridScreenSize.x );
    int gridStartY = KiROUND( ( worldStartPoint.y - gridOrigin.y ) / gridScreenSize.y );
    int gridEndY   = KiROUND( ( worldEndPoint.y - gridOrigin.y ) / gridScreenSize.y );

    // Ensure start coordinate > end coordinate

    SWAP( gridStartX, >, gridEndX );
    SWAP( gridStartY, >, gridEndY );

    // Ensure the grid fills the screen
    --gridStartX; ++gridEndX;
    --gridStartY; ++gridEndY;

    // Draw the grid behind all other layers
    SetLayerDepth( depthRange.y * 0.75 );

    if( gridStyle == GRID_STYLE::LINES )
    {
        // Now draw the grid, every coarse grid line gets the double width

        // Vertical lines
        for( int j = gridStartY; j <= gridEndY; j++ )
        {
            const double y = j * gridScreenSize.y + gridOrigin.y;

            if( axesEnabled && y == 0.0 )
                continue;

            SetLineWidth( ( j % gridTick ) ? marker : doubleMarker );
            drawGridLine( VECTOR2D( gridStartX * gridScreenSize.x + gridOrigin.x, y ),
                          VECTOR2D( gridEndX * gridScreenSize.x + gridOrigin.x, y ) );
        }

        // Horizontal lines
        for( int i = gridStartX; i <= gridEndX; i++ )
        {
            const double x = i * gridScreenSize.x + gridOrigin.x;

            if( axesEnabled && x == 0.0 )
                continue;

            SetLineWidth( ( i % gridTick ) ? marker : doubleMarker );
            drawGridLine( VECTOR2D( x, gridStartY * gridScreenSize.y + gridOrigin.y ),
                          VECTOR2D( x, gridEndY * gridScreenSize.y + gridOrigin.y ) );

        }
    }
    else    // Dots or Crosses grid
    {
        lineWidthIsOdd = true;
        isStrokeEnabled = true;
        for( int j = gridStartY; j <= gridEndY; j++ )
        {
            bool tickY = ( j % gridTick == 0 );

            for( int i = gridStartX; i <= gridEndX; i++ )
            {
                bool tickX = ( i % gridTick == 0 );
                VECTOR2D pos{ i * gridScreenSize.x + gridOrigin.x,
                              j * gridScreenSize.y + gridOrigin.y };

                if( gridStyle == GRID_STYLE::SMALL_CROSS )
                {
                    SetLineWidth( ( tickX && tickY ) ? doubleMarker : marker );
                    drawGridCross( pos );
                }
                else if( gridStyle == GRID_STYLE::DOTS )
                {
                    double doubleGridLineWidth = gridLineWidth * 2.0f;
                    drawGridPoint( pos, ( tickX ) ? doubleGridLineWidth : gridLineWidth,
                                        ( tickY ) ? doubleGridLineWidth : gridLineWidth );
                }
            }
        }
    }
}

