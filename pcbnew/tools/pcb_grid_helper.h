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

#ifndef PCB_GRID_HELPER_H
#define PCB_GRID_HELPER_H

#include <initializer_list>
#include <vector>

#include <settings/snap_settings.h>
#include <tool/grid_helper.h>
#include <snap/snap_resolver.h>
#include <geometry/intersection.h>
#include <geometry/nearest.h>

#include <board.h>


class LSET;
class PCB_ARC;
class SHAPE_ARC;
class TOOL_MANAGER;
struct MAGNETIC_SETTINGS;
struct PCB_SELECTION_FILTER_OPTIONS;

class PCB_GRID_HELPER : public GRID_HELPER, public BOARD_LISTENER
{
    friend class PCBGridHelperTestFixture;
public:

    PCB_GRID_HELPER();
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

    VECTOR2I Align( const VECTOR2I& aPoint, GRID_HELPER_GRIDS aGrid ) const override;

    VECTOR2I AlignToSegment ( const VECTOR2I& aPoint, const SEG& aSeg );

    VECTOR2I BestDragOrigin( const VECTOR2I& aMousePos, std::vector<BOARD_ITEM*>& aItem,
                             GRID_HELPER_GRIDS aGrid = GRID_HELPER_GRIDS::GRID_CURRENT,
                             const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter = nullptr );

    VECTOR2I AlignToArc ( const VECTOR2I& aPoint, const SHAPE_ARC& aSeg );

    VECTOR2I SnapToPad( const VECTOR2I& aMousePos, std::deque<PAD*>& aPads );

    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aRemovedItem ) override;

    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;

    /**
     * Chooses the "best" snap anchor around the given point, optionally taking layers from
     * the reference item.  The reference item will not be snapped to (it is being dragged or
     * created) and we choose the layers that can be snapped based on the reference item layer
     * @param aOrigin Point we want to snap from
     * @param aReferenceItem Reference item for layer/type special casing
     * @return snapped screen point
     */
    SNAP_RESULT ResolveSnap( const VECTOR2I& aOrigin, BOARD_ITEM* aReferenceItem,
                             GRID_HELPER_GRIDS aGrid = GRID_HELPER_GRIDS::GRID_CURRENT );
    SNAP_RESULT ResolveSnap( const VECTOR2I& aOrigin, const LSET& aLayers,
                             GRID_HELPER_GRIDS aGrid = GRID_HELPER_GRIDS::GRID_CURRENT,
                             const std::vector<BOARD_ITEM*>& aSkip = {},
                             std::optional<VECTOR2I>         aMovingReferencePoint = std::nullopt );
    void ClearSnapFeedback();

    /**
     * Turn off the geometry the snap system draws across the canvas to explain itself.
     *
     * A tool that only wants a point on an item never follows an extension anywhere, so the
     * lines are noise over the board it is asking the user to read.
     */
    void SetConstructionGeometryEnabled( bool aEnable ) { m_constructionGeometryEnabled = aEnable; }

    GRID_HELPER_GRIDS GetItemGrid( const EDA_ITEM* aItem ) const override;

    VECTOR2D GetGridSize( GRID_HELPER_GRIDS aGrid ) const override;

    /**
     * Add construction geometry for a set of board items.
     *
     * @param aItems The items for which to add construction geometry
     * @param aExtensionOnly If true, the construction geometry only includes extensions of the
     *                       items, if false it also overlays the items themselves.
     * @param aIsPersistent If true, the construction geometry is considered "persistent" and will
     *                      always be shown and won't be replaced by later temporary geometry.
     */
    void AddConstructionItems( std::vector<BOARD_ITEM*> aItems, bool aExtensionOnly,
                               bool aIsPersistent );

    /// A single anchor point contributed by an item, before it is registered with the helper.
    struct ANCHOR_SPEC
    {
        VECTOR2I pos;
        int      flags;
        int      pointType;
    };

    /**
     * Return the snap/drag anchor points that a track arc contributes.
     *
     * The arc's derived geometric center is rarely grid-aligned, so it is never offered as the
     * arc's own drag origin. The stored start/mid/end points (grid-aligned when the arc is)
     * carry the drag-origin role instead, keeping pasted arcs on grid.
     *
     * @param aArc The arc for which to compute anchors.
     * @param aFrom True when the arc is the drag source (its own points may become the origin).
     */
    static std::vector<ANCHOR_SPEC> GetArcAnchors( const PCB_ARC& aArc, bool aFrom );

private:
    static BOX2I layoutBounds( const BOARD_ITEM& aItem );

    /**
     * True when the footprint's own contents are the layout objects.
     *
     * The board arranges footprints, so a footprint's children collapse into it.  The footprint
     * editor arranges those children, where the same collapse would leave a single object and
     * make alignment and equal spacing impossible.
     */
    bool editingInsideFootprint() const;

    /// Snap inference settings for whichever editor owns this helper.
    SNAP_INFERENCE_SETTINGS snapInferenceSettings() const;

    std::vector<BOARD_ITEM*> queryVisible( std::initializer_list<BOX2I>    aAreas,
                                           const std::vector<BOARD_ITEM*>& aSkip ) const;

    /**
     * Find the nearest anchor point to the given position with matching flags.
     *
     * @param return The nearest anchor point, or nullptr if none found
     */
    ANCHOR* nearestAnchor( const VECTOR2I& aPos, int aFlags );

    /**
     * computeAnchors inserts the local anchor points in to the grid helper for the specified
     * container of board items, including points implied by intersections or other relationships
     * between the items.
     */
    void computeAnchors( const std::vector<BOARD_ITEM*>& aItems, const VECTOR2I& aRefPos,
                         bool aFrom, const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter,
                         const LSET* aLayers, bool aForDrag );

    /**
     * computeAnchors inserts the local anchor points in to the grid helper for the specified
     * board item, given the reference point and the direction of use for the point.
     *
     * @param aItem The board item for which to compute the anchors
     * @param aRefPos The point for which to compute the anchors (if used by the component)
     * @param aFrom Is this for an anchor that is designating a source point (aFrom=true) or not
     */
    void computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos, bool aFrom,
                         const PCB_SELECTION_FILTER_OPTIONS* aSelectionFilter );

private:
    MAGNETIC_SETTINGS*         m_magneticSettings;

    std::vector<NEARABLE_GEOM>      m_pointOnLineCandidates;

    bool                             m_constructionGeometryEnabled = true;
};

#endif
