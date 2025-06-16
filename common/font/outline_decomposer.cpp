/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#include <advanced_config.h>
#include <font/outline_decomposer.h>
#include <bezier_curves.h>

using namespace KIFONT;

OUTLINE_DECOMPOSER::OUTLINE_DECOMPOSER( FT_Outline& aOutline ) :
        m_outline( aOutline ),
        m_contours( nullptr )
{
}


static VECTOR2D toVector2D( const FT_Vector* aFreeTypeVector )
{
    return VECTOR2D( (double) aFreeTypeVector->x * GLYPH_SIZE_SCALER,
                     (double) aFreeTypeVector->y * GLYPH_SIZE_SCALER );
}


void OUTLINE_DECOMPOSER::newContour()
{
    CONTOUR contour;
    contour.m_Orientation = FT_Outline_Get_Orientation( &m_outline );
    m_contours->push_back( std::move( contour ) );
}


void OUTLINE_DECOMPOSER::addContourPoint( const VECTOR2D& p )
{
    // don't add repeated points
    if( m_contours->back().m_Points.empty() || m_contours->back().m_Points.back() != p )
        m_contours->back().m_Points.push_back( p );
}


int OUTLINE_DECOMPOSER::moveTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    decomposer->m_lastEndPoint = toVector2D( aEndPoint );

    decomposer->newContour();
    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::lineTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    decomposer->m_lastEndPoint = toVector2D( aEndPoint );

    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::quadraticTo( const FT_Vector* aControlPoint, const FT_Vector* aEndPoint,
                                     void* aCallbackData )
{
    return cubicTo( aControlPoint, nullptr, aEndPoint, aCallbackData );
}


int OUTLINE_DECOMPOSER::cubicTo( const FT_Vector* aFirstControlPoint,
                                 const FT_Vector* aSecondControlPoint, const FT_Vector* aEndPoint,
                                 void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    std::vector<VECTOR2D> bezier;
    bezier.push_back( decomposer->m_lastEndPoint );
    bezier.push_back( toVector2D( aFirstControlPoint ) );

    if( aSecondControlPoint )
    {
        // aSecondControlPoint == nullptr for quadratic Beziers
        bezier.push_back( toVector2D( aSecondControlPoint ) );
    }

    bezier.push_back( toVector2D( aEndPoint ) );

    std::vector<VECTOR2D> result;
    BEZIER_POLY           converter( bezier );
    converter.GetPoly( result, ADVANCED_CFG::GetCfg().m_FontErrorSize );

    for( const VECTOR2D& p : result )
        decomposer->addContourPoint( p );

    decomposer->m_lastEndPoint = toVector2D( aEndPoint );

    return 0;
}


bool OUTLINE_DECOMPOSER::OutlineToSegments( std::vector<CONTOUR>* aContours )
{
    m_contours = aContours;

    FT_Outline_Funcs callbacks;

    callbacks.move_to = moveTo;
    callbacks.line_to = lineTo;
    callbacks.conic_to = quadraticTo;
    callbacks.cubic_to = cubicTo;
    callbacks.shift = 0;
    callbacks.delta = 0;

    FT_Error e = FT_Outline_Decompose( &m_outline, &callbacks, this );

    if( e )
        return false;

    for( CONTOUR& c : *m_contours )
        c.m_Winding = winding( c.m_Points );

    return true;
}


int OUTLINE_DECOMPOSER::winding( const std::vector<VECTOR2D>& aContour ) const
{
    // -1 == counterclockwise, 1 == clockwise

    const int cw = 1;
    const int ccw = -1;

    if( aContour.size() < 2 )
    {
        // zero or one points, so not a clockwise contour - in fact not a contour at all
        //
        // It could also be argued that a contour needs 3 extremum points at a minimum to be
        // considered a proper contour (ie. a glyph (subpart) outline, or a hole)
        return 0;
    }

    double sum = 0.0;
    size_t len = aContour.size();

    for( size_t i = 0; i < len - 1; i++ )
    {
        VECTOR2D p1 = aContour[ i ];
        VECTOR2D p2 = aContour[ i + 1 ];

        sum += ( ( p2.x - p1.x ) * ( p2.y + p1.y ) );
    }

    sum += ( ( aContour[0].x - aContour[len - 1].x ) * ( aContour[0].y + aContour[len - 1].y ) );

    if( sum > 0.0 )
        return cw;
    if( sum < 0.0 )
        return ccw;

    return 0;
}
