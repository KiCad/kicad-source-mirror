/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <functional>
using namespace std::placeholders;

#include <gal/graphics_abstraction_layer.h>
#include <math/util.h>      // for KiROUND
#include <math/vector2d.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <tool/grid_helper.h>
#include <settings/app_settings.h>


GRID_HELPER::GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
    m_toolMgr( aToolMgr )
{
    m_maskTypes = ALL;
    m_enableSnap = true;
    m_enableSnapLine = true;
    m_enableGrid = true;
    m_snapItem = std::nullopt;
}


GRID_HELPER::~GRID_HELPER()
{
}


VECTOR2I GRID_HELPER::GetGrid() const
{
    VECTOR2D size = m_toolMgr->GetView()->GetGAL()->GetGridSize();

    return VECTOR2I( KiROUND( size.x ), KiROUND( size.y ) );
}


VECTOR2D GRID_HELPER::GetVisibleGrid() const
{
    return m_toolMgr->GetView()->GetGAL()->GetVisibleGridSize();
}


VECTOR2I GRID_HELPER::GetOrigin() const
{
    VECTOR2D origin = m_toolMgr->GetView()->GetGAL()->GetGridOrigin();

    return VECTOR2I( origin );
}


GRID_HELPER_GRIDS GRID_HELPER::GetSelectionGrid( const SELECTION& aSelection ) const
{
    GRID_HELPER_GRIDS grid = GetItemGrid( aSelection.Front() );

    // Find the largest grid of all the items and use that
    for( EDA_ITEM* item : aSelection )
    {
        GRID_HELPER_GRIDS itemGrid = GetItemGrid( item );

        if( GetGridSize( itemGrid ) > GetGridSize( grid ) )
            grid = itemGrid;
    }

    return grid;
}


VECTOR2D GRID_HELPER::GetGridSize( GRID_HELPER_GRIDS aGrid ) const
{
    return m_toolMgr->GetView()->GetGAL()->GetGridSize();
}


void GRID_HELPER::SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin )
{
    if( aEnable )
    {
        m_auxAxis = aOrigin;
        m_viewAxis.SetPosition( aOrigin );
        m_toolMgr->GetView()->SetVisible( &m_viewAxis, true );
    }
    else
    {
        m_auxAxis = std::optional<VECTOR2I>();
        m_toolMgr->GetView()->SetVisible( &m_viewAxis, false );
    }
}


VECTOR2I GRID_HELPER::AlignGrid( const VECTOR2I& aPoint ) const
{
    return computeNearest( aPoint, GetGrid(), GetOrigin() );
}


VECTOR2I GRID_HELPER::AlignGrid( const VECTOR2I& aPoint, const VECTOR2D& aGrid,
                                 const VECTOR2D& aOffset ) const
{
    return computeNearest( aPoint, aGrid, aOffset );
}


VECTOR2I GRID_HELPER::computeNearest( const VECTOR2I& aPoint, const VECTOR2I& aGrid,
                                      const VECTOR2I& aOffset ) const
{
    return VECTOR2I( KiROUND( (double) ( aPoint.x - aOffset.x ) / aGrid.x ) * aGrid.x + aOffset.x,
                     KiROUND( (double) ( aPoint.y - aOffset.y ) / aGrid.y ) * aGrid.y + aOffset.y );
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint ) const
{
    return Align( aPoint, GetGrid(), GetOrigin() );
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint, const VECTOR2D& aGrid,
                             const VECTOR2D& aOffset ) const
{
    if( !canUseGrid() )
        return aPoint;

    VECTOR2I nearest = AlignGrid( aPoint, aGrid, aOffset );

    if( !m_auxAxis )
        return nearest;

    if( std::abs( m_auxAxis->x - aPoint.x ) < std::abs( nearest.x - aPoint.x ) )
        nearest.x = m_auxAxis->x;

    if( std::abs( m_auxAxis->y - aPoint.y ) < std::abs( nearest.y - aPoint.y ) )
        nearest.y = m_auxAxis->y;

    return nearest;
}


bool GRID_HELPER::canUseGrid() const
{
    return m_enableGrid && m_toolMgr->GetView()->GetGAL()->GetGridSnapping();
}


std::optional<VECTOR2I> GRID_HELPER::GetSnappedPoint() const
{
    if( m_snapItem )
        return m_snapItem->pos;

    return std::nullopt;
}
