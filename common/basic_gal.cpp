/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Implement a very basic GAL used to draw, plot and convert texts in segments
 * for DRC funstions, using the common GAL functions.
 * Draw functions use wxDC.
 * Plot functions use a PLOTTER
 * Convert texts in segments use a callback function created by the caller
 * @file basic_gal.cpp
 */

#include <gr_basic.h>
#include <plot_common.h>
#include <trigo.h>

#include <basic_gal.h>

using namespace KIGFX;

KIGFX::GAL_DISPLAY_OPTIONS basic_displayOptions;

// the basic GAL doesn't get an external display option object
BASIC_GAL basic_gal( basic_displayOptions );

const VECTOR2D BASIC_GAL::transform( const VECTOR2D& aPoint ) const
{
    VECTOR2D point = aPoint + m_transform.m_moveOffset - m_transform.m_rotCenter;
    point = point.Rotate( m_transform.m_rotAngle ) + m_transform.m_rotCenter;
    return point;
}

void BASIC_GAL::DrawPolyline( const std::deque<VECTOR2D>& aPointList )
{
    if( aPointList.empty() )
        return;

    std::deque<VECTOR2D>::const_iterator it = aPointList.begin();
    std::vector <wxPoint> polyline_corners;

    for( ; it != aPointList.end(); ++it )
    {
        VECTOR2D corner = transform(*it);
        polyline_corners.push_back( wxPoint( corner.x, corner.y ) );
    }

    if( m_DC )
    {
        if( isFillEnabled )
        {
            GRPoly( m_isClipped ? &m_clipBox : NULL, m_DC, polyline_corners.size(),
                    &polyline_corners[0], 0, GetLineWidth(), m_Color, m_Color );
        }
        else
        {
            for( unsigned ii = 1; ii < polyline_corners.size(); ++ii )
            {
                GRCSegm( m_isClipped ? &m_clipBox : NULL, m_DC, polyline_corners[ii-1],
                         polyline_corners[ii], GetLineWidth(), m_Color );
            }
        }
    }
    else if( m_plotter )
    {
        m_plotter->MoveTo( polyline_corners[0] );

        for( unsigned ii = 1; ii < polyline_corners.size(); ii++ )
        {
            m_plotter->LineTo( polyline_corners[ii] );
        }

        m_plotter->PenFinish();
    }
    else if( m_callback )
    {
        for( unsigned ii = 1; ii < polyline_corners.size(); ii++ )
        {
            m_callback( polyline_corners[ii-1].x, polyline_corners[ii-1].y,
                        polyline_corners[ii].x, polyline_corners[ii].y );
        }
    }
}

void BASIC_GAL::DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint )
{
    VECTOR2D startVector = transform( aStartPoint );
    VECTOR2D endVector = transform( aEndPoint );

    if( m_DC )
    {
        if( isFillEnabled )
        {
            GRLine( m_isClipped ? &m_clipBox : NULL, m_DC, startVector.x, startVector.y,
                    endVector.x, endVector.y, GetLineWidth(), m_Color );
        }
        else
        {
            GRCSegm( m_isClipped ? &m_clipBox : NULL, m_DC, startVector.x, startVector.y,
                    endVector.x, endVector.y, GetLineWidth(), 0, m_Color );
        }
    }
    else if( m_plotter )
    {
        m_plotter->MoveTo( wxPoint( startVector.x, startVector.y ) );
        m_plotter->LineTo( wxPoint( endVector.x, endVector.y ) );
        m_plotter->PenFinish();
    }
    else if( m_callback )
    {
            m_callback( startVector.x, startVector.y,
                        endVector.x, endVector.y );
    }
}
