/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <gal/graphics_abstraction_layer.h>
#include <gal/color4d.h>
#include <gal/painter.h>
#include <math/util.h>      // for KiROUND
#include <geometry/seg.h>
#include <wx/string.h>
#include <vector>
#include <algorithm>
#include "tool/edit_points.h"


bool EDIT_POINT::WithinPoint( const VECTOR2I& aPoint, unsigned int aSize ) const
{
    // Corners of the EDIT_POINT square
    VECTOR2I topLeft = GetPosition() - aSize;
    VECTOR2I bottomRight = GetPosition() + aSize;

    return ( aPoint.x > topLeft.x && aPoint.y > topLeft.y &&
             aPoint.x < bottomRight.x && aPoint.y < bottomRight.y );
}


EDIT_POINTS::EDIT_POINTS( EDA_ITEM* aParent ) :
        EDA_ITEM( NOT_USED ),
        m_parent( aParent ),
        m_swapX( false ),
        m_swapY( false ),
        m_allowPoints( true )
{
}


EDIT_POINT* EDIT_POINTS::FindPoint( const VECTOR2I& aLocation, KIGFX::VIEW *aView ) // fixme: ugly
{
    unsigned size = std::abs( KiROUND( aView->ToWorld( EDIT_POINT::POINT_SIZE ) ) );

    if( m_allowPoints )
    {
        for( EDIT_POINT& point : m_points )
        {
            if( point.WithinPoint( aLocation, size ) )
                return &point;
        }
    }

    for( EDIT_LINE& line : m_lines )
    {
        if( line.WithinPoint( aLocation, size ) )
            return &line;
    }

    return nullptr;
}


int EDIT_POINTS::GetContourStartIdx( int aPointIdx ) const
{
    int lastIdx = 0;

    for( int idx : m_contours )
    {
        if( idx >= aPointIdx )
            return lastIdx;

        lastIdx = idx + 1;
    }

    return lastIdx;
}


int EDIT_POINTS::GetContourEndIdx( int aPointIdx ) const
{
    for( int idx : m_contours )
    {
        if( idx >= aPointIdx )
            return idx;
    }

    return m_points.size() - 1;
}


bool EDIT_POINTS::IsContourStart( int aPointIdx ) const
{
    for( int idx : m_contours )
    {
        if( idx + 1 == aPointIdx )
            return true;

        // the list is sorted, so we cannot expect it any further
        if( idx > aPointIdx )
            break;
    }

    return ( aPointIdx == 0 );
}


bool EDIT_POINTS::IsContourEnd( int aPointIdx ) const
{
    for( int idx : m_contours )
    {
        if( idx == aPointIdx )
            return true;

        // the list is sorted, so we cannot expect it any further
        if( idx > aPointIdx )
            break;
    }

    // the end of the list surely is the end of a contour
    return ( aPointIdx == (int) m_points.size() - 1 );
}


EDIT_POINT* EDIT_POINTS::Previous( const EDIT_POINT& aPoint, bool aTraverseContours )
{
    for( unsigned int i = 0; i < m_points.size(); ++i )
    {
        if( m_points[i] == aPoint )
        {
            if( !aTraverseContours && IsContourStart( i ) )
                return &m_points[GetContourEndIdx( i )];

            if( i == 0 )
                return &m_points[m_points.size() - 1];
            else
                return &m_points[i - 1];
        }
    }

    return nullptr;
}


EDIT_LINE* EDIT_POINTS::Previous( const EDIT_LINE& aLine )
{
    for( unsigned int i = 0; i < m_lines.size(); ++i )
    {
        if( m_lines[i] == aLine )
        {
            if( i == 0 )
                return &m_lines[m_lines.size() - 1];
            else
                return &m_lines[i - 1];
        }
    }

    return nullptr;
}


EDIT_POINT* EDIT_POINTS::Next( const EDIT_POINT& aPoint, bool aTraverseContours )
{
    for( unsigned int i = 0; i < m_points.size(); ++i )
    {
        if( m_points[i] == aPoint )
        {
            if( !aTraverseContours && IsContourEnd( i ) )
                return &m_points[GetContourStartIdx( i )];

            if( i == m_points.size() - 1 )
                return &m_points[0];
            else
                return &m_points[i + 1];
        }
    }

    return nullptr;
}


EDIT_LINE* EDIT_POINTS::Next( const EDIT_LINE& aLine )
{
    for( unsigned int i = 0; i < m_lines.size(); ++i )
    {
        if( m_lines[i] == aLine )
        {
            if( i == m_lines.size() - 1 )
                return &m_lines[0];
            else
                return &m_lines[i + 1];
        }
    }

    return nullptr;
}


const BOX2I EDIT_POINTS::ViewBBox() const
{
    BOX2I box;
    bool empty = true;

    for( const auto& point : m_points )
    {
        if( empty )
        {
            box.SetOrigin( point.GetPosition() );
            empty = false;
        }
        else
        {
            box.Merge( point.GetPosition() );
        }
    }

    for( const auto& line : m_lines )
    {
        if( empty )
        {
            box.SetOrigin( line.GetOrigin().GetPosition() );
            box.SetEnd( line.GetEnd().GetPosition() );
            empty = false;
        }
        else
        {
            box.Merge( line.GetOrigin().GetPosition() );
            box.Merge( line.GetEnd().GetPosition() );
        }
    }

    return box;
}


void EDIT_POINTS::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL*             gal = aView->GetGAL();
    KIGFX::RENDER_SETTINGS* settings = aView->GetPainter()->GetSettings();
    KIGFX::COLOR4D          drawColor = settings->GetLayerColor( LAYER_AUX_ITEMS );

    // Don't assume LAYER_AUX_ITEMS is always a good choice.  Compare with background.
    if( aView->GetGAL()->GetClearColor().Distance( drawColor ) < 0.5 )
        drawColor.Invert();

    // Linear darkening doesn't fit well with human color perception, and there's no guarantee
    // that there's enough room for contrast either.
    KIGFX::COLOR4D borderColor;
    KIGFX::COLOR4D highlightColor;
    double         brightness = drawColor.GetBrightness();

    if( brightness > 0.5 )
    {
        borderColor = drawColor.Darkened( 0.7 ).WithAlpha( 0.8 );
        highlightColor = drawColor.Darkened( 0.5 ).WithAlpha( 0.8 );
    }
    else if( brightness > 0.2 )
    {
        borderColor = drawColor.Brightened( 0.4 ).WithAlpha( 0.8 );
        highlightColor = drawColor.Brightened( 0.3 ).WithAlpha( 0.8 );
    }
    else
    {
        borderColor = drawColor.Brightened( 0.7 ).WithAlpha( 0.8 );
        highlightColor = drawColor.Brightened( 0.5 ).WithAlpha( 0.8 );
    }

    KIGFX::GAL_SCOPED_ATTRS scopedAttrs( *gal, KIGFX::GAL_SCOPED_ATTRS::ALL_ATTRS );
    gal->SetFillColor( drawColor );
    gal->SetStrokeColor( borderColor );
    gal->SetIsFill( true );
    gal->SetIsStroke( true );
    gal->SetLayerDepth( gal->GetMinDepth() );

    double size       = aView->ToWorld( EDIT_POINT::POINT_SIZE ) / 2.0;
    double borderSize = aView->ToWorld( EDIT_POINT::BORDER_SIZE );
    double hoverSize  = aView->ToWorld( EDIT_POINT::HOVER_SIZE );

    auto drawPoint =
            [&]( const EDIT_POINT& aPoint, bool aDrawCircle = false )
            {
                if( aPoint.IsHover() || aPoint.IsActive() )
                {
                    gal->SetStrokeColor( highlightColor );
                    gal->SetLineWidth( hoverSize );
                }
                else
                {
                    gal->SetStrokeColor( borderColor );
                    gal->SetLineWidth( borderSize );
                }

                gal->SetFillColor( drawColor );

                if( aDrawCircle )
                    gal->DrawCircle( aPoint.GetPosition(), size );
                else
                    gal->DrawRectangle( aPoint.GetPosition() - size, aPoint.GetPosition() + size );
            };

    for( const EDIT_POINT& point : m_points )
        drawPoint( point, point.DrawCircle() );

    for( const EDIT_LINE& line : m_lines )
    {
        if( line.HasCenterPoint() )
        {
            drawPoint( line.GetPosition(), true );
        }

        if( line.DrawLine() )
        {
            gal->SetLineWidth( borderSize / 4 );
            gal->SetStrokeColor( borderColor );
            gal->DrawLine( line.GetOrigin().GetPosition(), line.GetEnd().GetPosition() );
        }
    }

}
