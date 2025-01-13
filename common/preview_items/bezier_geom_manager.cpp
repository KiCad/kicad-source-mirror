/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "preview_items/bezier_geom_manager.h"

using namespace KIGFX::PREVIEW;


bool BEZIER_GEOM_MANAGER::acceptPoint( const VECTOR2I& aPt )
{
    switch( getStep() )
    {
    case SET_START: return setStart( aPt );
    case SET_CONTROL1: return setControlC1( aPt );
    case SET_END: return setEnd( aPt );
    case SET_CONTROL2: return setControlC2( aPt );
    case COMPLETE: return false;
    }

    return false;
}


VECTOR2I BEZIER_GEOM_MANAGER::GetStart() const
{
    return m_start;
}


VECTOR2I BEZIER_GEOM_MANAGER::GetControlC1() const
{
    return m_controlC1;
}


VECTOR2I BEZIER_GEOM_MANAGER::GetEnd() const
{
    return m_end;
}


VECTOR2I BEZIER_GEOM_MANAGER::GetControlC2() const
{
    // The actual bezier C2 point is the reflection over the end point
    // so that the cursor will be on the C1 point of the next bezier.
    return m_end - ( m_controlC2 - m_end );
}


bool BEZIER_GEOM_MANAGER::setStart( const VECTOR2I& aStart )
{
    m_start = aStart;

    // Prevents weird-looking loops if the control points aren't initialized
    m_end = aStart;
    m_controlC1 = aStart;
    m_controlC2 = aStart;
    return true;
}


bool BEZIER_GEOM_MANAGER::setControlC1( const VECTOR2I& aControlC1 )
{
    m_controlC1 = aControlC1;
    m_end = m_controlC1;
    m_controlC2 = m_controlC1;
    // It's possible to set the control 1 point to the same as the start point
    return true;
}


bool BEZIER_GEOM_MANAGER::setEnd( const VECTOR2I& aEnd )
{
    m_end = aEnd;
    m_controlC2 = m_end;
    return m_end != m_start;
}


bool BEZIER_GEOM_MANAGER::setControlC2( const VECTOR2I& aControlC2 )
{
    m_controlC2 = aControlC2;

    // It's possible to set the control 2 point to the same as the end point
    return true;
}
