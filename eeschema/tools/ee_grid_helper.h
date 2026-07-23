/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EE_GRID_HELPER_H
#define EE_GRID_HELPER_H

#include <math/vector2d.h>
#include <origin_viewitem.h>
#include <settings/snap_settings.h>
#include <tool/grid_helper.h>
#include <snap/snap_resolver.h>
#include "sch_selection.h"

class SCH_ITEM;
class SCH_PIN;


class EE_GRID_HELPER : public GRID_HELPER
{
    friend class EEGridHelperTestFixture;

public:

    EE_GRID_HELPER();
    EE_GRID_HELPER( TOOL_MANAGER* aToolMgr );
    ~EE_GRID_HELPER() override;

    /**
     * Function GetSnapped
     * If the EE_GRID_HELPER has highlighted a snap point (target shown), this function
     * will return a pointer to the item to which it snapped.
     *
     * @return NULL if not snapped.  Pointer to snapped item otherwise
     */
    SCH_ITEM* GetSnapped() const;

    VECTOR2D GetGridSize( GRID_HELPER_GRIDS aGrid ) const override;
    using GRID_HELPER::GetGrid;

    GRID_HELPER_GRIDS GetItemGrid( const EDA_ITEM* aItem ) const override;

    VECTOR2I BestDragOrigin( const VECTOR2I& aMousePos, GRID_HELPER_GRIDS aGrid,
                             const SCH_SELECTION& aItems );

    SNAP_RESULT ResolveSnap( const VECTOR2I& aOrigin, GRID_HELPER_GRIDS aGrid, SCH_ITEM* aSkip );
    SNAP_RESULT ResolveSnap( const VECTOR2I& aOrigin, GRID_HELPER_GRIDS aGrid, const SCH_SELECTION& aSkip = {},
                             std::optional<VECTOR2I> aMovingReferencePoint = std::nullopt );

    void AddConstructionItems( std::vector<SCH_ITEM*> aItems, bool aExtensionOnly,
                               bool aIsPersistent );

private:
    /**
     * Bounds a layout relation should align to.
     *
     * Fields travel with their symbol and are moved independently, so including them would make
     * a symbol's alignment edges shift for reasons unrelated to the symbol's placement.
     */
    static BOX2I layoutBounds( const SCH_ITEM& aItem );

    /// Pin connection points the item offers as alignment anchors.
    static std::vector<std::pair<const SCH_PIN*, VECTOR2I>> layoutPins( SCH_ITEM& aItem );

    /// Snap inference settings for whichever editor owns this helper.
    SNAP_INFERENCE_SETTINGS snapInferenceSettings() const;

    std::set<SCH_ITEM*> queryVisible( const BOX2I& aArea, const SCH_SELECTION& aSkipList ) const;

    ANCHOR* nearestAnchor( const VECTOR2I& aPos, int aFlags, GRID_HELPER_GRIDS aGrid );

    /**
     * Insert the local anchor points in to the grid helper for the specified
     * schematic item, given the reference point and the direction of use for the point.
     *
     * @param aItem The schematic item for which to compute the anchors
     * @param aRefPos The point for which to compute the anchors (if used by the symbol)
     * @param aFrom Is this for an anchor that is designating a source point (aFrom=true) or not
     * @param aIncludeText if true will compute anchors for text items
     */
    void computeAnchors( SCH_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom = false,
                         bool aIncludeText = false );
};

#endif
