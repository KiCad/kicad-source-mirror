/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#pragma once

#include <vector>
#include <deque>

#include <preview_items/construction_geom.h>

class EDA_ITEM;

/**
 * A class that mananges "construction" objects and geometry.
 *
 * Probably only used by GRID_HELPERs, but it's neater to keep it separate,
 * as there's quite a bit of state to manage.
 */
class CONSTRUCTION_MANAGER
{
public:
    CONSTRUCTION_MANAGER( KIGFX::CONSTRUCTION_GEOM& aHelper );

    /**
     * Items to be used for the construction of "virtual" anchors, for example, when snapping to
     * a point involving an _extension_ of an existing line or arc.
     *
     * One item can have multiple construction items (e.g. an arc can have a circle and centre point).
     */

    enum class SOURCE
    {
        FROM_ITEMS,
        FROM_SNAP_LINE,
    };

    struct CONSTRUCTION_ITEM
    {
        SOURCE                                          Source;
        EDA_ITEM*                                       Item;
        std::vector<KIGFX::CONSTRUCTION_GEOM::DRAWABLE> Constructions;
    };

    // A single batch of construction items. Once batch contains all the items (and associated
    // construction geometry) that should be shown for one point of interest.
    // More than one batch may be shown on the screen at the same time.
    using CONSTRUCTION_ITEM_BATCH = std::vector<CONSTRUCTION_ITEM>;

    /**
     * Add a batch of construction items to the helper.
     *
     * @param aBatch The batch of construction items to add.
     * @param aIsPersistent If true, the batch is considered "persistent" and will always be shown
     *                      (and it will replace any previous persistent batch).
     *                      If false, the batch is temporary and may be pushed out by other batches.
     */
    void AddConstructionItems( CONSTRUCTION_ITEM_BATCH aBatch, bool aIsPersistent );

    /**
     * Check if all 'real' (non-null = constructed) the items in the batch are in the list of items
     * currently 'involved' in an active construction.
     */
    bool InvolvesAllGivenRealItems( const std::vector<EDA_ITEM*>& aItems ) const;

    /**
     * Set the reference-only points - these are points that are not snapped to, but can still
     * be used for connection to the snap line.
     */
    void SetReferenceOnlyPoints( std::vector<VECTOR2I> aPoints )
    {
        m_referenceOnlyPoints = std::move( aPoints );
    }

    const std::vector<VECTOR2I>& GetReferenceOnlyPoints() const { return m_referenceOnlyPoints; }

    /**
     * The snap point is a special point that is located at the last point the cursor
     * snapped to. If it is set, the construction manager may add extra construction
     * geometry to the helper extending from the snap point origin to the cursor,
     * which is the 'snap line'.
     */
    void SetSnapLineOrigin( const VECTOR2I& aOrigin );

    /**
     * Set the end point of the snap line.
     *
     * Passing std::nullopt will unset the end point, but keep the origin.
     */
    void SetSnapLineEnd( const OPT_VECTOR2I& aSnapPoint );

    /**
     * Clear the snap line origin and end points.
     */
    void ClearSnapLine();

    std::optional<VECTOR2I> GetSnapLineOrigin() const { return m_snapLineOrigin; }

    /**
     * Inform the construction manager that an anchor snap is wanted.
     *
     * This will also update the snap line if appropriate.
     */
    void SetSnappedAnchor( const VECTOR2I& aAnchorPos );

    // Get the list of additional geometry items that should be considered
    std::vector<CONSTRUCTION_ITEM_BATCH> GetConstructionItems() const;

    /**
     * If the snap line is active, return the best snap point that is closest to the cursor
     *
     * If there's no active snap line, return std::nullopt.
     *
     * If there's a snap very near, use that otherwise, use the grid point.
     * With this point, snap to it on an H/V axis.
     *
     * Then, if there's a grid point near, snap to it on an H/V axis
     *
     * @param aCursor The cursor position
     * @param aNearestGrid The nearest grid point to the cursor
     * @param aDistToNearest The distance to the nearest non-grid snap point, if any
     * @param snapRange The snap range
     */
    OPT_VECTOR2I GetNearestSnapLinePoint( const VECTOR2I& aCursor, const VECTOR2I& aNearestGrid,
                                          std::optional<int> aDistToNearest, int snapRange ) const;

    using GFX_UPDATE_CALLBACK = std::function<void( bool )>;
    /**
     * Set the callback to call when the construction geometry changes and a view may need updating.
     */
    void SetUpdateCallback( GFX_UPDATE_CALLBACK aCallback ) { m_updateCallback = aCallback; }

private:
    void updateView();

    // An (external) construction helper view item, that this manager adds/removes
    // construction objects to/from.
    KIGFX::CONSTRUCTION_GEOM& m_constructionGeomPreview;

    // Within one "operation", there is one set of construction items that are
    // "persistent", and are always shown. Usually the original item and any
    // extensions.
    std::optional<CONSTRUCTION_ITEM_BATCH> m_persistentConstructionBatch;

    // Temporary construction items are added and removed as needed
    std::deque<CONSTRUCTION_ITEM_BATCH> m_temporaryConstructionBatches;

    // Set of all items for which construction geometry has been added
    std::set<EDA_ITEM*> m_involvedItems;

    std::vector<VECTOR2I> m_referenceOnlyPoints;

    // If a snap point is "active", extra construction geometry is added to the helper
    // extending from the snap point to the cursor.
    OPT_VECTOR2I m_snapLineOrigin;
    OPT_VECTOR2I m_snapLineEnd;

    GFX_UPDATE_CALLBACK m_updateCallback;
};
