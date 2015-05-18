/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013-2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>

using namespace KIGFX;

void VIEW_CONTROLS::ShowCursor( bool aEnabled )
{
    m_view->GetGAL()->SetCursorEnabled( aEnabled );
}


void VIEW_CONTROLS::setCenter( const VECTOR2D& aCenter )
{
    if( !m_panBoundary.Contains( aCenter ) )
    {
        VECTOR2D newCenter( aCenter );

        if( aCenter.x < m_panBoundary.GetLeft() )
            newCenter.x = m_panBoundary.GetLeft();
        else if( aCenter.x > m_panBoundary.GetRight() )
            newCenter.x = m_panBoundary.GetRight();

        if( aCenter.y < m_panBoundary.GetTop() )
            newCenter.y = m_panBoundary.GetTop();
        else if( aCenter.y > m_panBoundary.GetBottom() )
            newCenter.y = m_panBoundary.GetBottom();

        m_view->SetCenter( newCenter );
    }
    else
    {
        m_view->SetCenter( aCenter );
    }
}


void VIEW_CONTROLS::setScale( double aScale, const VECTOR2D& aAnchor )
{
    if( aScale < m_minScale )
        aScale = m_minScale;
    else if( aScale > m_maxScale )
        aScale = m_maxScale;

    m_view->SetScale( aScale, aAnchor );
}
