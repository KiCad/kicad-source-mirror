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

#include <math/util.h>      // for KiROUND
#include <math/vector2d.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <tool/grid_helper.h>


GRID_HELPER::GRID_HELPER( TOOL_MANAGER* aToolMgr ) :
    m_toolMgr( aToolMgr )
{
    m_maskTypes = ALL;
    m_enableSnap = true;
    m_enableSnapLine = true;
    m_enableGrid = true;
    m_snapItem = nullptr;
}


GRID_HELPER::~GRID_HELPER()
{
}


VECTOR2I GRID_HELPER::GetGrid() const
{
    VECTOR2D size = m_toolMgr->GetView()->GetGAL()->GetGridSize();

    return VECTOR2I( KiROUND( size.x ), KiROUND( size.y ) );
}


VECTOR2I GRID_HELPER::GetOrigin() const
{
    VECTOR2D origin = m_toolMgr->GetView()->GetGAL()->GetGridOrigin();

    return VECTOR2I( origin );
}


void GRID_HELPER::SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin )
{
    if( aEnable )
    {
        m_auxAxis = aOrigin;
        m_viewAxis.SetPosition( wxPoint( aOrigin ) );
        m_toolMgr->GetView()->SetVisible( &m_viewAxis, true );
    }
    else
    {
        m_auxAxis = OPT<VECTOR2I>();
        m_toolMgr->GetView()->SetVisible( &m_viewAxis, false );
    }
}


VECTOR2I GRID_HELPER::AlignGrid( const VECTOR2I& aPoint ) const
{
    const VECTOR2D gridOffset( GetOrigin() );
    const VECTOR2D grid( GetGrid() );

    VECTOR2I nearest( KiROUND( ( aPoint.x - gridOffset.x ) / grid.x ) * grid.x + gridOffset.x,
                      KiROUND( ( aPoint.y - gridOffset.y ) / grid.y ) * grid.y + gridOffset.y );

    return nearest;
}


VECTOR2I GRID_HELPER::Align( const VECTOR2I& aPoint ) const
{
    if( !canUseGrid() )
        return aPoint;

    VECTOR2I nearest = AlignGrid( aPoint );

    if( !m_auxAxis )
        return nearest;

    if( std::abs( m_auxAxis->x - aPoint.x ) < std::abs( nearest.x - aPoint.x ) )
        nearest.x = m_auxAxis->x;

    if( std::abs( m_auxAxis->y - aPoint.y ) < std::abs( nearest.y - aPoint.y ) )
        nearest.y = m_auxAxis->y;

    return nearest;
}


