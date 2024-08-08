/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_GRID_HELPER_H
#define PCB_GRID_HELPER_H

#include <vector>

#include <pcb_item_containers.h>
#include <tool/grid_helper.h>
#include <board.h>

class LSET;
class SHAPE_ARC;
class TOOL_MANAGER;
struct MAGNETIC_SETTINGS;
struct PCB_SELECTION_FILTER_OPTIONS;

class PCB_GRID_HELPER : public GRID_HELPER, public BOARD_LISTENER
{
public:

    PCB_GRID_HELPER( TOOL_MANAGER* aToolMgr, MAGNETIC_SETTINGS* aMagneticSettings );
    ~PCB_GRID_HELPER() override;

    /**
     * Function GetSnapped
     * If the PCB_GRID_HELPER has highlighted a snap point (target shown), this function
     * will return a pointer to the item to which it snapped.
     *
     * @return NULL if not snapped.  Pointer to snapped item otherwise
     */
    BOARD_ITEM* GetSnapped() const;

    using GRID_HELPER::Align;
    using GRID_HELPER::AlignGrid;

    VECTOR2I AlignToSegment ( const VECTOR2I& aPoint, const SEG& aSeg );

    VECTOR2I BestDragOrigin( const VECTOR2I& aMousePos, std::vector<BOARD_ITEM*>& aItem,
                             GRID_HELPER_GRIDS aGrid = GRID_HELPER_GRIDS::GRID_CURRENT,
                             const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter = nullptr );

    VECTOR2I AlignToArc ( const VECTOR2I& aPoint, const SHAPE_ARC& aSeg );

    VECTOR2I AlignToNearestPad( const VECTOR2I& aMousePos, std::deque<PAD*>& aPads );

    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override
    {
        if( m_snapItem->item == aBoardItem )
        {
            m_snapItem = std::nullopt;
        }
    }

    /**
     * Chooses the "best" snap anchor around the given point, optionally taking layers from
     * the reference item.  The reference item will not be snapped to (it is being dragged or
     * created) and we choose the layers that can be snapped based on the reference item layer
     * @param aOrigin Point we want to snap from
     * @param aReferenceItem Reference item for layer/type special casing
     * @return snapped screen point
     */
    VECTOR2I BestSnapAnchor( const VECTOR2I& aOrigin, BOARD_ITEM* aReferenceItem,
                             GRID_HELPER_GRIDS aGrid = GRID_HELPER_GRIDS::GRID_CURRENT );
    VECTOR2I BestSnapAnchor( const VECTOR2I& aOrigin, const LSET& aLayers,
                             GRID_HELPER_GRIDS aGrid = GRID_HELPER_GRIDS::GRID_CURRENT,
                             const std::vector<BOARD_ITEM*>& aSkip = {} );

    GRID_HELPER_GRIDS GetItemGrid( const EDA_ITEM* aItem ) const override;

    VECTOR2D GetGridSize( GRID_HELPER_GRIDS aGrid ) const override;


private:
    std::set<BOARD_ITEM*> queryVisible( const BOX2I& aArea,
                                        const std::vector<BOARD_ITEM*>& aSkip ) const;

    ANCHOR* nearestAnchor( const VECTOR2I& aPos, int aFlags, LSET aMatchLayers );

    /**
     * computeAnchors inserts the local anchor points in to the grid helper for the specified
     * board item, given the reference point and the direction of use for the point.
     *
     * @param aItem The board item for which to compute the anchors
     * @param aRefPos The point for which to compute the anchors (if used by the component)
     * @param aFrom Is this for an anchor that is designating a source point (aFrom=true) or not
     */
    void computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom = false,
                         const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter = nullptr );

private:
    MAGNETIC_SETTINGS*     m_magneticSettings;
};

#endif
