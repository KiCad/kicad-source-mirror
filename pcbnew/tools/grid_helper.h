/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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
#include <core/optional.h>
#include <origin_viewitem.h>
#include <layers_id_colors_and_visibility.h>
#include <geometry/seg.h>

class PCB_BASE_FRAME;

class GRID_HELPER {
public:

    GRID_HELPER( PCB_BASE_FRAME* aFrame );
    ~GRID_HELPER();

    VECTOR2I GetGrid() const;
    VECTOR2I GetOrigin() const;

    /**
     * Function GetSnapped
     * If the GRID_HELPER has highlighted a snap point (target shown), this function
     * will return a pointer to the item to which it snapped.
     *
     * @return NULL if not snapped.  Pointer to snapped item otherwise
     */
    BOARD_ITEM* GetSnapped() const;

    void SetAuxAxes( bool aEnable, const VECTOR2I& aOrigin = VECTOR2I( 0, 0 ) );

    VECTOR2I Align( const VECTOR2I& aPoint ) const;

    VECTOR2I AlignToSegment ( const VECTOR2I& aPoint, const SEG& aSeg );

    VECTOR2I BestDragOrigin( const VECTOR2I& aMousePos, BOARD_ITEM* aItem );
    VECTOR2I BestSnapAnchor( const VECTOR2I& aOrigin, BOARD_ITEM* aDraggedItem );
    VECTOR2I BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
                             const std::vector<BOARD_ITEM*> aSkip = {} );

    void SetSnap( bool aSnap )
    {
        m_enableSnap = aSnap;
    }

    void SetUseGrid( bool aGrid = true )
    {
        m_enableGrid = aGrid;
    }

private:
    enum ANCHOR_FLAGS {
        CORNER = 0x1,
        OUTLINE = 0x2,
        SNAPPABLE = 0x4,
        ORIGIN = 0x8
    };

    struct ANCHOR
    {
        ANCHOR( VECTOR2I aPos, int aFlags = CORNER | SNAPPABLE, BOARD_ITEM* aItem = NULL ) :
            pos( aPos ),
            flags( aFlags ),
            item( aItem )
        { };

        VECTOR2I pos;
        int flags;
        BOARD_ITEM* item;

        double Distance( const VECTOR2I& aP ) const
        {
            return ( aP - pos ).EuclideanNorm();
        }
    };

    std::vector<ANCHOR> m_anchors;

    std::set<BOARD_ITEM*> queryVisible( const BOX2I& aArea,
                                        const std::vector<BOARD_ITEM*> aSkip ) const;

    void addAnchor( const VECTOR2I& aPos, int aFlags, BOARD_ITEM* aItem )
    {
        m_anchors.emplace_back( ANCHOR( aPos, aFlags, aItem ) );
    }

    ANCHOR* nearestAnchor( const VECTOR2I& aPos, int aFlags, LSET aMatchLayers );

    /**
     * computeAnchors inserts the local anchor points in to the grid helper for the specified
     * board item, given the reference point and the direction of use for the point.
     *
     * @param aItem The board item for which to compute the anchors
     * @param aRefPos The point for which to compute the anchors (if used by the component)
     * @param aFrom Is this for an anchor that is designating a source point (aFrom=true) or not
     */
    void computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom = false );

    void clearAnchors()
    {
        m_anchors.clear();
    }

    PCB_BASE_FRAME* m_frame;
    OPT<VECTOR2I> m_auxAxis;

    bool m_enableSnap;              ///< If true, allow snapping to other items on the layers
    bool m_enableGrid;              ///< If true, allow snapping to grid
    int m_snapSize;                 ///< Sets the radius in screen units for snapping to items
    ANCHOR* m_snapItem;             ///< Pointer to the currently snapped item in m_anchors (NULL if not snapped)

    KIGFX::ORIGIN_VIEWITEM m_viewSnapPoint;
    KIGFX::ORIGIN_VIEWITEM m_viewAxis;
};

#endif
