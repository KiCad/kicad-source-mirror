/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GRID_HELPER_H
#define GRID_HELPER_H

#include <vector>
#include <math/vector2d.h>
#include <origin_viewitem.h>

class TOOL_MANAGER;
class EDA_ITEM;


class GRID_HELPER
{
public:
    GRID_HELPER( TOOL_MANAGER* aToolMgr );
    ~GRID_HELPER();

    VECTOR2I GetGrid() const;
    VECTOR2I GetOrigin() const;

    void SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin = VECTOR2I( 0, 0 ) );

    virtual VECTOR2I Align( const VECTOR2I& aPoint ) const;

    VECTOR2I AlignGrid( const VECTOR2I& aPoint ) const;

    void SetSkipPoint( const VECTOR2I& aPoint )
    {
        m_skipPoint = aPoint;
    }

    /**
     * We clear the skip point by setting it to an unreachable position, thereby preventing matching
     */
    void ClearSkipPoint()
    {
        m_skipPoint = VECTOR2I( std::numeric_limits<int>::min(), std::numeric_limits<int>::min() );
    }

    void SetSnap( bool aSnap ) { m_enableSnap = aSnap; }
    bool GetSnap() const { return m_enableSnap; }

    void SetUseGrid( bool aSnapToGrid ) { m_enableGrid = aSnapToGrid; }
    bool GetUseGrid() const { return m_enableGrid; }

    void SetSnapLine( bool aSnap ) { m_enableSnapLine = aSnap; }

protected:
    enum ANCHOR_FLAGS {
        CORNER = 1,
        OUTLINE = 2,
        SNAPPABLE = 4,
        ORIGIN = 8,
        VERTICAL = 16,
        HORIZONTAL = 32
    };

    struct ANCHOR
    {
        ANCHOR( VECTOR2I aPos, int aFlags = CORNER | SNAPPABLE, EDA_ITEM* aItem = NULL ) :
            pos( aPos ),
            flags( aFlags ),
            item( aItem )
        { };

        VECTOR2I  pos;
        int       flags;
        EDA_ITEM* item;

        double Distance( const VECTOR2I& aP ) const
        {
            return ( aP - pos ).EuclideanNorm();
        }
    };

    void addAnchor( const VECTOR2I& aPos, int aFlags, EDA_ITEM* aItem )
    {
        m_anchors.emplace_back( ANCHOR( aPos, aFlags, aItem ) );
    }

    void clearAnchors()
    {
        m_anchors.clear();
    }

protected:
    std::vector<ANCHOR>    m_anchors;

    TOOL_MANAGER*          m_toolMgr;
    OPT<VECTOR2I>          m_auxAxis;

    bool                   m_enableSnap;     // Allow snapping to other items on the layers
    bool                   m_enableGrid;     // If true, allow snapping to grid
    bool                   m_enableSnapLine; // Allow drawing lines from snap points
    ANCHOR*                m_snapItem;       // Pointer to the currently snapped item in m_anchors
                                             //   (NULL if not snapped)
    VECTOR2I               m_skipPoint;      // When drawing a line, we avoid snapping to the source
                                             //   point
    KIGFX::ORIGIN_VIEWITEM m_viewSnapPoint;
    KIGFX::ORIGIN_VIEWITEM m_viewSnapLine;
    KIGFX::ORIGIN_VIEWITEM m_viewAxis;
};

#endif
