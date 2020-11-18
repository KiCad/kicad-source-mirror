/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __GRID_HELPER_H
#define __GRID_HELPER_H

#include <vector>
#include <math/vector2d.h>
#include <origin_viewitem.h>

class LSET;
class SCH_ITEM;
class SEG;

class EE_GRID_HELPER {
public:

    EE_GRID_HELPER( TOOL_MANAGER* aToolMgr );
    ~EE_GRID_HELPER();

    VECTOR2I GetGrid() const;
    VECTOR2I GetOrigin() const;

    /**
     * Function GetSnapped
     * If the EE_GRID_HELPER has highlighted a snap point (target shown), this function
     * will return a pointer to the item to which it snapped.
     *
     * @return NULL if not snapped.  Pointer to snapped item otherwise
     */
    SCH_ITEM* GetSnapped() const;

    void SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin = VECTOR2I( 0, 0 ) );

    VECTOR2I Align( const VECTOR2I& aPoint ) const;

    VECTOR2I AlignGrid( const VECTOR2I& aPoint ) const;

    VECTOR2I AlignToWire( const VECTOR2I& aPoint, const SEG& aSeg );

    VECTOR2I BestDragOrigin( const VECTOR2I& aMousePos, const std::vector<SCH_ITEM*>& aItem );

    VECTOR2I BestSnapAnchor( const VECTOR2I& aOrigin, SCH_ITEM* aDraggedItem );
    VECTOR2I BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
                             const std::vector<SCH_ITEM*>& aSkip = {} );

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

    void SetSnap( bool aSnap )
    {
        m_enableSnap = aSnap;
    }

    void SetSnapLine( bool aSnap )
    {
        m_enableSnapLine = aSnap;
    }

private:
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
        ANCHOR( VECTOR2I aPos, int aFlags = CORNER | SNAPPABLE, SCH_ITEM* aItem = NULL ) :
            pos( aPos ),
            flags( aFlags ),
            item( aItem )
        { };

        VECTOR2I pos;
        int flags;
        SCH_ITEM* item;

        double Distance( const VECTOR2I& aP ) const
        {
            return ( aP - pos ).EuclideanNorm();
        }
    };

    std::vector<ANCHOR> m_anchors;

    std::set<SCH_ITEM*> queryVisible( const BOX2I& aArea,
                                        const std::vector<SCH_ITEM*>& aSkip ) const;

    void addAnchor( const VECTOR2I& aPos, int aFlags, SCH_ITEM* aItem )
    {
        m_anchors.emplace_back( ANCHOR( aPos, aFlags, aItem ) );
    }

    ANCHOR* nearestAnchor( const VECTOR2I& aPos, int aFlags, LSET aMatchLayers );

    /**
     * computeAnchors inserts the local anchor points in to the grid helper for the specified
     * schematic item, given the reference point and the direction of use for the point.
     *
     * @param aItem The schematic item for which to compute the anchors
     * @param aRefPos The point for which to compute the anchors (if used by the component)
     * @param aFrom Is this for an anchor that is designating a source point (aFrom=true) or not
     */
    void computeAnchors( SCH_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom = false );

    void clearAnchors()
    {
        m_anchors.clear();
    }

    TOOL_MANAGER* m_toolMgr;
    OPT<VECTOR2I> m_auxAxis;

    bool          m_enableSnap;      // If true, allow snapping to other items on the layers
    bool          m_enableSnapLine;  // If true, allow drawing lines from snap points
    ANCHOR*       m_snapItem;        // Pointer to the currently snapped item in m_anchors
                                     //   (NULL if not snapped)
    VECTOR2I      m_skipPoint;       // When drawing a line, we avoid snapping to the source point

    KIGFX::ORIGIN_VIEWITEM m_viewSnapPoint;
    KIGFX::ORIGIN_VIEWITEM m_viewSnapLine;
    KIGFX::ORIGIN_VIEWITEM m_viewAxis;
};

#endif
