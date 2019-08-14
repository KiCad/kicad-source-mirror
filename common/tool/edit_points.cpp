/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
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
    EDA_ITEM( NOT_USED ), m_parent( aParent )
{
}


EDIT_POINT* EDIT_POINTS::FindPoint( const VECTOR2I& aLocation, KIGFX::VIEW *aView ) // fixme: ugly
{
    float size = aView->ToWorld( EDIT_POINT::POINT_SIZE );

    std::deque<EDIT_POINT>::iterator pit, pitEnd;
    for( pit = m_points.begin(), pitEnd = m_points.end(); pit != pitEnd; ++pit )
    {
        EDIT_POINT& point = *pit;

        if( point.WithinPoint( aLocation, size ) )
            return &point;
    }

    std::deque<EDIT_LINE>::iterator lit, litEnd;
    for( lit = m_lines.begin(), litEnd = m_lines.end(); lit != litEnd; ++lit )
    {
        EDIT_LINE& line = *lit;

        if( line.WithinPoint( aLocation, size ) )
            return &line;
    }

    return NULL;
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

    return NULL;
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

    return NULL;
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

    return NULL;
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

    return NULL;
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
    auto gal = aView->GetGAL();

    if( aView->GetGAL()->GetClearColor().GetBrightness() > 0.5 )
        gal->SetFillColor( KIGFX::COLOR4D( 0, 0, 0, 1.0 ) );
    else
        gal->SetFillColor( KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    gal->SetIsFill( true );
    gal->SetIsStroke( false );
    gal->PushDepth();
    gal->SetLayerDepth( gal->GetMinDepth() );

    float size = aView->ToWorld( EDIT_POINT::POINT_SIZE );

    for( const EDIT_POINT& point : m_points )
        gal->DrawRectangle( point.GetPosition() - size / 2, point.GetPosition() + size / 2 );

    for( const EDIT_LINE& line : m_lines )
    {
        gal->DrawCircle( line.GetPosition(), size / 2 );
    }

    gal->PopDepth();
}
