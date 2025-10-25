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

#ifndef PREVIEW_ITEMS_TWO_POINT_GEOMETRY_MANAGER_H
#define PREVIEW_ITEMS_TWO_POINT_GEOMETRY_MANAGER_H

#include <math/vector2d.h>
#include <geometry/geometry_utils.h>

namespace KIGFX
{
class GAL;

namespace PREVIEW
{

/**
 * Represent a very simple geometry manager for items that have a start and end point.
 */
class TWO_POINT_GEOMETRY_MANAGER
{
public:
    ///< Set the origin of the ruler (the fixed end)
    void SetOrigin( const VECTOR2I& aOrigin )
    {
        m_origin = aOrigin;
        m_originSet = true;
    }

    VECTOR2I GetOrigin() const
    {
        return m_origin;
    }

    /**
     * Set the current end of the rectangle (the end that moves with the cursor.
     */
    void SetEnd( const VECTOR2I& aEnd )
    {
        switch( m_angleSnap )
        {
        case LEADER_MODE::DEG45: m_end = GetVectorSnapped45( aEnd - m_origin ) + m_origin; break;
        case LEADER_MODE::DEG90: m_end = GetVectorSnapped90( aEnd - m_origin ) + m_origin; break;
        default: m_end = aEnd; break;
        }
    }

    VECTOR2I GetEnd() const
    {
        return m_end;
    }

    void SetAngleSnap( LEADER_MODE aSnap ) { m_angleSnap = aSnap; }

    LEADER_MODE GetAngleSnap() const { return m_angleSnap; }

    /**
     * @return true if the manager is in the initial state
     */
    bool IsReset() const
    {
        return !m_originSet;
    }

    /**
     * Reset the manager to the initial state
     */
    void Reset()
    {
        m_originSet = false;
    }

    bool IsEmpty() const
    {
        return !m_originSet || m_origin == m_end;
    }

private:
    VECTOR2I        m_origin;
    VECTOR2I        m_end;
    LEADER_MODE     m_angleSnap = LEADER_MODE::DIRECT;
    bool            m_originSet = false;
};

} // PREVIEW
} // KIGFX

#endif // PREVIEW_ITEMS_TWO_POINT_GEOMETRY_MANAGER_H
